/**
 * @file    oled.h
 * @brief   极简 SSD1315 / SSD1306 OLED 库（128x64, I2C）—— 课堂用
 *
 * 设计目标：不到 200 行，每 20 行对应课堂上的一个步骤，学员能一段一段照着敲。
 * 不依赖本工程的任何代码，只依赖 STM32 HAL + CubeMX 生成的 main.h。
 *
 * 课堂顺序：
 *   步骤 1  初始化        oled_init()   发 0x00 控制字 + 初始化命令序列
 *   步骤 2  显存与清屏    oled_clear()  1024 字节 = 8 页 x 128 列
 *   步骤 3  画一个点      oled_pixel()  页地址 = y/8，位掩码 = 1<<(y%8)
 *   步骤 4  刷到屏上      oled_show()   8 页 x（设页 + 128 字节显存数据）
 *   步骤 5  画矩形/横线   oled_rect() / oled_hline()
 *   步骤 6  显示文字      oled_text()   3x5 点阵字模
 *   按键课另外两个文件：keys.h / keys.c（采样 + 消抖 + 边沿）
 *
 * 硬件前提：
 *   I2C1  PB6=SCL  PB7=SDA  地址 0x3C  400 kHz（CubeMX 里选 Fast Mode）
 *   OLED  VCC=3.3V  GND  SCL  SDA
 *
 * 典型用法：
 *   oled_init(&hi2c1);
 *   oled_clear();
 *   oled_rect(10, 10, 30, 10, 1);
 *   oled_text(10, 30, "HELLO", 1);
 *   oled_show();               // 不调用它，屏幕不会有任何变化
 */

#ifndef __OLED_H
#define __OLED_H

#include <stdint.h>

#include "main.h"

#define OLED_W 128
#define OLED_H 64
#define OLED_I2C_ADDR  0x3C                                 /* 7 位地址(SA0 接 GND) */
#define OLED_I2C_ADDR8 ((OLED_I2C_ADDR << 1))               /* HAL 要 8 位地址:左移一位 */
#define OLED_TIMEOUT_MS 100

/** 初始化（内部会延时 100 ms、清屏、开显示） */
void oled_init(I2C_HandleTypeDef *hi2c);

/** 把整块显存清成黑色（只改 RAM，要 oled_show 才生效） */
void oled_clear(void);

/** 画 / 擦一个像素，越界自动丢弃 */
void oled_pixel(int x, int y, int on);

/** 画一条水平线 */
void oled_hline(int x, int y, int w, int on);

/** 画实心矩形 */
void oled_rect(int x, int y, int w, int h, int on);

/** 显示文字：支持 A-Z、0-9、空格；scale=1 时每个字符占 4x5 像素（3x5 字模 + 1 列间距） */
void oled_text(int x, int y, const char *text, int scale);

/** 把 1024 字节显存整块推到屏上（8 页，约 23 ms @400 kHz） */
void oled_show(void);

#endif /* __OLED_H */
