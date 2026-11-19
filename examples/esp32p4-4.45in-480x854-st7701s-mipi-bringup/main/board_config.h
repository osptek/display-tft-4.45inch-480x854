/*
 * 板级配置: ESP32-P4 + 4.45" 480x854 ST7701S MIPI-DSI 屏 (YDP445B003-V1 / YDP400BT001-V4 转接)
 *
 * 引脚来自原理图 (J3, 30P 0.5mm FPC):
 *   MIPI DSI  : ESP32-P4 专用 DSI 引脚 (D0/D1/CLK, 不可更改)
 *   LCD_RST   : GPIO5
 *   LCD_TE    : GPIO4   (DSI video 模式下不使用, 仅预留)
 *   背光 EN   : GPIO20  (SY7200 升压 EN, 高电平使能背光)
 *   触摸      : TP_INT=GPIO21, TP_SDA=GPIO7, TP_SCL=GPIO8, TP_RST=GPIO6 (本工程暂未接入)
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------- GPIO ---------------- */
#define BOARD_LCD_RST_GPIO          5
#define BOARD_LCD_TE_GPIO           4
#define BOARD_LCD_BL_EN_GPIO        20
#define BOARD_LCD_BL_ON_LEVEL       1

#define BOARD_TP_INT_GPIO           21
#define BOARD_TP_RST_GPIO           6
#define BOARD_TP_I2C_SDA_GPIO       7
#define BOARD_TP_I2C_SCL_GPIO       8

/* ---------------- MIPI DSI PHY 供电 ---------------- */
/* ESP32-P4 的 VDD_MIPI_DPHY 需要 2.5V, 这里使用片内 LDO_VO3 供电.
 * 如果你的板子使用外部 LDO 给 DPHY 供电, 将 CHAN 改为 -1 */
#define BOARD_DSI_PHY_LDO_CHAN      3
#define BOARD_DSI_PHY_LDO_MV        2500

/* ---------------- 面板参数 ---------------- */
#define BOARD_LCD_H_RES             480
#define BOARD_LCD_V_RES             854

#define BOARD_DSI_LANE_NUM          2
#define BOARD_DSI_LANE_BITRATE_MBPS 500     /* 每 lane 速率, 需 >= pclk*bpp/lane_num (30MHz*24/2=360Mbps) */

/* DPI 视频时序, 帧率 ≈ 30MHz / (480+10+40+40) / (854+4+20+20) ≈ 55~60Hz
 * 若出现花屏/抖动/偏移, 优先微调下面几个porch值和像素时钟 */
#define BOARD_LCD_DPI_CLK_MHZ       30
#define BOARD_LCD_HSYNC             10
#define BOARD_LCD_HBP               40
#define BOARD_LCD_HFP               40
#define BOARD_LCD_VSYNC             4
#define BOARD_LCD_VBP               20
#define BOARD_LCD_VFP               20

/* ---------------- LVGL ---------------- */
/* LVGL 绘制缓冲区行数 (RGB565: 480*行数*2 字节, 双缓冲, 位于 PSRAM/内部 DMA 内存) */
#define BOARD_LVGL_BUF_LINES        100

#ifdef __cplusplus
}
#endif
