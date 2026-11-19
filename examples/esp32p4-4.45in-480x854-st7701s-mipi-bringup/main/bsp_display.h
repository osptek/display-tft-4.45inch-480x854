#pragma once

#include "esp_err.h"
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化 DSI 总线、ST7701S 面板、背光以及 LVGL (esp_lvgl_port)
 *
 * @return LVGL 显示对象, 失败返回 NULL
 */
lv_display_t *bsp_display_start(void);

/** @brief 打开/关闭背光 (SY7200 EN 引脚) */
void bsp_display_backlight(bool on);

#ifdef __cplusplus
}
#endif
