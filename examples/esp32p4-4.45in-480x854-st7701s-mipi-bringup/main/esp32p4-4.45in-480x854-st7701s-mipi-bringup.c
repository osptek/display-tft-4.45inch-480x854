#include <stdio.h>

#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "lvgl.h"

#include "board_config.h"
#include "bsp_display.h"

static const char *TAG = "app";

static void slider_event_cb(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target_obj(e);
    lv_obj_t *label = lv_event_get_user_data(e);
    lv_label_set_text_fmt(label, "%d%%", (int)lv_slider_get_value(slider));
}

/* 简单的测试界面: 标题 + 三原色/灰阶色条 (检查色序、坏点) + 滑条 + 按钮 */
static void create_demo_ui(lv_display_t *disp)
{
    lv_obj_t *scr = lv_display_get_screen_active(disp);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x101820), 0);
    lv_obj_set_style_pad_all(scr, 0, 0);

    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "ESP32-P4 + ST7701S\nMIPI-DSI 480x854 + LVGL");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 30);

    /* 色条: R G B W 灰阶 */
    static const uint32_t colors[] = {0xFF0000, 0x00FF00, 0x0000FF, 0xFFFFFF, 0x808080, 0x000000};
    const int bar_h = 50;
    for (int i = 0; i < (int)(sizeof(colors) / sizeof(colors[0])); i++) {
        lv_obj_t *bar = lv_obj_create(scr);
        lv_obj_remove_style_all(bar);
        lv_obj_set_size(bar, BOARD_LCD_H_RES - 40, bar_h);
        lv_obj_set_pos(bar, 20, 120 + i * (bar_h + 6));
        lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(bar, lv_color_hex(colors[i]), 0);
        lv_obj_set_style_border_width(bar, 1, 0);
        lv_obj_set_style_border_color(bar, lv_color_hex(0x606060), 0);
    }

    /* 渐变条 */
    lv_obj_t *grad = lv_obj_create(scr);
    lv_obj_remove_style_all(grad);
    lv_obj_set_size(grad, BOARD_LCD_H_RES - 40, 50);
    lv_obj_set_pos(grad, 20, 120 + 6 * (bar_h + 6));
    lv_obj_set_style_bg_opa(grad, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(grad, lv_color_black(), 0);
    lv_obj_set_style_bg_grad_color(grad, lv_color_white(), 0);
    lv_obj_set_style_bg_grad_dir(grad, LV_GRAD_DIR_HOR, 0);

    /* 滑条 */
    lv_obj_t *value = lv_label_create(scr);
    lv_label_set_text(value, "50%");
    lv_obj_set_style_text_color(value, lv_color_white(), 0);
    lv_obj_align(value, LV_ALIGN_BOTTOM_MID, 0, -190);

    lv_obj_t *slider = lv_slider_create(scr);
    lv_obj_set_width(slider, BOARD_LCD_H_RES - 100);
    lv_slider_set_value(slider, 50, LV_ANIM_OFF);
    lv_obj_align(slider, LV_ALIGN_BOTTOM_MID, 0, -150);
    lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, value);

    lv_obj_t *btn = lv_button_create(scr);
    lv_obj_set_size(btn, 200, 56);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -60);
    lv_obj_t *btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, "LVGL OK");
    lv_obj_center(btn_label);
}

void app_main(void)
{
    lv_display_t *disp = bsp_display_start();
    if (disp == NULL) {
        ESP_LOGE(TAG, "display init failed");
        return;
    }

    /* LVGL 不是线程安全的, 在 esp_lvgl_port 的任务之外操作 UI 需要加锁 */
    if (lvgl_port_lock(0)) {
        create_demo_ui(disp);
        lvgl_port_unlock();
    }
}
