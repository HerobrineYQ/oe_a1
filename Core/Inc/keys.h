/**
 * @file    keys.h
 * @brief   极简按键库（7 个按键：五向摇杆 + SET + RESET）—— 课堂用
 *
 * 课堂顺序：
 *   步骤 1  读原始电平      HAL_GPIO_ReadPin()，上拉输入：松开=1，按下=0
 *   步骤 2  定时采样        keys_scan() 里用 HAL_GetTick() 卡 10 ms 采样一次
 *   步骤 3  消抖            连续 2 次采样一致才认为电平变了（= 20 ms 消抖）
 *   步骤 4  边沿检测        从"松开"变"按下"的那一次，置一个一次性标志
 *   步骤 5  对外三个接口    key_pressed()（边沿）/ key_is_down()（按住）
 *
 * 硬件前提（CubeMX 里把 PA1~PA7 配成 GPIO_Input + Pull-up）：
 *   PA7=UP  PA6=DOWN  PA5=LEFT  PA4=RIGHT  PA3=MID  PA2=SET  PA1=RESET
 *   7 个开关的公共端 COM 接 GND
 *
 * 典型用法：
 *   keys_init();
 *   while (1) {
 *       keys_scan();                       // 每次循环都调，内部自己节流
 *       if (key_pressed(KEY_MID)) { ... }   // 按下的那一下
 *       if (key_is_down(KEY_LEFT)) { ... }  // 一直按住时（连续移动）
 *       HAL_Delay(1);
 *   }
 */

#ifndef __KEYS_H
#define __KEYS_H

#include "main.h"

#define KEY_UP     0
#define KEY_DOWN   1
#define KEY_LEFT   2
#define KEY_RIGHT  3
#define KEY_MID    4
#define KEY_SET    5
#define KEY_RST    6
#define KEY_COUNT  7

/** 清空内部状态（GPIO 由 CubeMX 生成的 MX_GPIO_Init 配置） */
void keys_init(void);

/** 采样一次，内部每 10 ms 才真正读一次引脚；主循环里反复调用即可 */
void keys_scan(void);

/** 是否刚刚按下（边沿，读一次就清掉） */
int key_pressed(int key);

/** 当前是否按住（消抖后） */
int key_is_down(int key);

#endif /* __KEYS_H */
