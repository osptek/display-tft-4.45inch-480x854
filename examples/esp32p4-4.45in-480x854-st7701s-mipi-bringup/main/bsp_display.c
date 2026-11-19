#include "bsp_display.h"

#include "board_config.h"
#include "st7701s_init_cmds.h"

#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_ldo_regulator.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_st7701.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"

static const char *TAG = "bsp_display";

/* LVGL 使用 RGB565, DSI 链路也使用 RGB565 (COLMOD=0x55), 帧缓冲占用 480*854*2 ≈ 800KB (PSRAM) */
#define BOARD_LCD_BITS_PER_PIXEL    16
#define BOARD_LCD_COLOR_FMT         LCD_COLOR_FMT_RGB565
#define BOARD_LVGL_COLOR_FMT        LV_COLOR_FORMAT_RGB565

void bsp_display_backlight(bool on)
{
    gpio_set_level(BOARD_LCD_BL_EN_GPIO, on ? BOARD_LCD_BL_ON_LEVEL : !BOARD_LCD_BL_ON_LEVEL);
}

static esp_err_t backlight_init(void)
{
    const gpio_config_t cfg = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << BOARD_LCD_BL_EN_GPIO,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&cfg), TAG, "backlight gpio config failed");
    bsp_display_backlight(false);   /* 先关背光, 等面板初始化完成后再开, 避免上电闪白/花屏 */
    return ESP_OK;
}

static esp_err_t dsi_phy_power_on(void)
{
#if BOARD_DSI_PHY_LDO_CHAN >= 0
    esp_ldo_channel_handle_t ldo = NULL;
    const esp_ldo_channel_config_t ldo_cfg = {
        .chan_id = BOARD_DSI_PHY_LDO_CHAN,
        .voltage_mv = BOARD_DSI_PHY_LDO_MV,
    };
    ESP_RETURN_ON_ERROR(esp_ldo_acquire_channel(&ldo_cfg, &ldo), TAG, "acquire DSI PHY LDO failed");
    ESP_LOGI(TAG, "MIPI DSI PHY powered on (LDO%d @ %dmV)", BOARD_DSI_PHY_LDO_CHAN, BOARD_DSI_PHY_LDO_MV);
#endif
    return ESP_OK;
}

static esp_err_t panel_create(esp_lcd_panel_handle_t *out_panel)
{
    /* 1. DSI 总线 (同时初始化 DSI PHY) */
    esp_lcd_dsi_bus_handle_t dsi_bus = NULL;
    const esp_lcd_dsi_bus_config_t bus_cfg = {
        .bus_id = 0,
        .num_data_lanes = BOARD_DSI_LANE_NUM,
        .phy_clk_src = MIPI_DSI_PHY_PLLREF_CLK_SRC_DEFAULT,
        .lane_bit_rate_mbps = BOARD_DSI_LANE_BITRATE_MBPS,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_dsi_bus(&bus_cfg, &dsi_bus), TAG, "new DSI bus failed");

    /* 2. DBI: 通过 DSI 发送 ST7701S 的命令/参数 */
    esp_lcd_panel_io_handle_t io = NULL;
    const esp_lcd_dbi_io_config_t dbi_cfg = {
        .virtual_channel = 0,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_dbi(dsi_bus, &dbi_cfg, &io), TAG, "new DBI io failed");

    /* 3. DPI: 视频数据通道 */
    const esp_lcd_dpi_panel_config_t dpi_cfg = {
        .virtual_channel = 0,
        .dpi_clk_src = MIPI_DSI_DPI_CLK_SRC_DEFAULT,
        .dpi_clock_freq_mhz = BOARD_LCD_DPI_CLK_MHZ,
        .in_color_format = BOARD_LCD_COLOR_FMT,
        .num_fbs = 1,
        .video_timing = {
            .h_size = BOARD_LCD_H_RES,
            .v_size = BOARD_LCD_V_RES,
            .hsync_pulse_width = BOARD_LCD_HSYNC,
            .hsync_back_porch = BOARD_LCD_HBP,
            .hsync_front_porch = BOARD_LCD_HFP,
            .vsync_pulse_width = BOARD_LCD_VSYNC,
            .vsync_back_porch = BOARD_LCD_VBP,
            .vsync_front_porch = BOARD_LCD_VFP,
        },
    };

    /* 4. ST7701S 面板 (厂家初始化序列) */
    st7701_vendor_config_t vendor_cfg = {
        .init_cmds = st7701s_init_cmds,
        .init_cmds_size = sizeof(st7701s_init_cmds) / sizeof(st7701s_init_cmds[0]),
        .mipi_config = {
            .dsi_bus = dsi_bus,
            .dpi_config = &dpi_cfg,
        },
        .flags.use_mipi_interface = 1,
    };
    const esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = BOARD_LCD_RST_GPIO,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,   /* 颜色红蓝反了就改成 BGR */
        .bits_per_pixel = BOARD_LCD_BITS_PER_PIXEL,
        .vendor_config = &vendor_cfg,
    };
    esp_lcd_panel_handle_t panel = NULL;
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_st7701(io, &panel_cfg, &panel), TAG, "new ST7701 panel failed");

    ESP_RETURN_ON_ERROR(esp_lcd_panel_reset(panel), TAG, "panel reset failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_init(panel), TAG, "panel init failed");
    /* 厂家 INI 已包含 0x29 (Display ON), 这里不再重复发送 */

    *out_panel = panel;
    return ESP_OK;
}

lv_display_t *bsp_display_start(void)
{
    esp_lcd_panel_handle_t panel = NULL;

    ESP_ERROR_CHECK(backlight_init());
    ESP_ERROR_CHECK(dsi_phy_power_on());
    ESP_ERROR_CHECK(panel_create(&panel));

    /* LVGL (esp_lvgl_port) */
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = NULL,
        .panel_handle = panel,
        .control_handle = NULL,
        .buffer_size = BOARD_LCD_H_RES * BOARD_LVGL_BUF_LINES,
        .double_buffer = true,
        .hres = BOARD_LCD_H_RES,
        .vres = BOARD_LCD_V_RES,
        .monochrome = false,
        .rotation = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        },
        .color_format = BOARD_LVGL_COLOR_FMT,
        .flags = {
            .buff_dma = true,
            .buff_spiram = false,
            .swap_bytes = false,
            .sw_rotate = false,
        },
    };
    const lvgl_port_display_dsi_cfg_t dsi_cfg = {
        .flags.avoid_tearing = false,
    };
    lv_display_t *disp = lvgl_port_add_disp_dsi(&disp_cfg, &dsi_cfg);
    if (disp == NULL) {
        ESP_LOGE(TAG, "add LVGL display failed");
        return NULL;
    }

    bsp_display_backlight(true);
    ESP_LOGI(TAG, "display ready: %dx%d", BOARD_LCD_H_RES, BOARD_LCD_V_RES);
    return disp;
}
