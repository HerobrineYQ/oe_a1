/**
 * @file    keys.c
 * @brief   极简按键库实现（配 keys.h）
 */

#include "keys.h"

#define SCAN_PERIOD_MS   10U   /* 采样间隔 */
#define STABLE_SAMPLES    2U   /* 连续几次一致才认 → 20 ms 消抖 */

typedef struct {
    GPIO_TypeDef *port;
    uint16_t pin;
} key_pin_t;

/* 引脚表：想换脚只改这里（CubeMX 里也要配成 Input + Pull-up） */
static const key_pin_t k_pins[KEY_COUNT] = {
    { GPIOA, GPIO_PIN_7 },   /* KEY_UP    */
    { GPIOA, GPIO_PIN_6 },   /* KEY_DOWN  */
    { GPIOA, GPIO_PIN_5 },   /* KEY_LEFT  */
    { GPIOA, GPIO_PIN_4 },   /* KEY_RIGHT */
    { GPIOA, GPIO_PIN_3 },   /* KEY_MID   */
    { GPIOA, GPIO_PIN_2 },   /* KEY_SET   */
    { GPIOA, GPIO_PIN_1 },   /* KEY_RST   */
};

static uint8_t s_stable[KEY_COUNT];   /* 消抖后的状态：1 = 按下 */
static uint8_t s_count[KEY_COUNT];    /* 连续几次采样与 stable 不一致 */
static uint8_t s_pressed[KEY_COUNT];  /* 一次性"按下"事件 */
static uint32_t s_last_scan_ms;

void keys_init(void)
{
    for (int i = 0; i < KEY_COUNT; i++) {
        s_stable[i] = 0;
        s_count[i] = 0;
        s_pressed[i] = 0;
    }
    s_last_scan_ms = 0;
}

/* ---------------------------------------------------------------- 步骤 2/3 */
void keys_scan(void)
{
    const uint32_t now = HAL_GetTick();

    if ((uint32_t)(now - s_last_scan_ms) < SCAN_PERIOD_MS) {
        return;               /* 还没到 10 ms，本次不采样 */
    }
    s_last_scan_ms = now;

    for (int i = 0; i < KEY_COUNT; i++) {
        /* 步骤 1：上拉输入，按下接地 → 读到 RESET 就是按下 */
        const uint8_t raw =
            (HAL_GPIO_ReadPin(k_pins[i].port, k_pins[i].pin) == GPIO_PIN_RESET) ? 1U : 0U;

        if (raw == s_stable[i]) {
            s_count[i] = 0;   /* 和当前状态一致，抖动计数清零 */
            continue;
        }

        s_count[i]++;
        if (s_count[i] < STABLE_SAMPLES) {
            continue;         /* 连续几次都变了，才认为是真的变了 */
        }

        s_count[i] = 0;
        s_stable[i] = raw;

        if (raw != 0U) {
            s_pressed[i] = 1U;   /* 步骤 4：松开 → 按下，产生一次边沿事件 */
        }
    }
}

/* ---------------------------------------------------------------- 步骤 5 */
int key_pressed(int key)
{
    if (key < 0 || key >= KEY_COUNT) {
        return 0;
    }

    if (s_pressed[key]) {
        s_pressed[key] = 0;   /* 读走即清除，保证"按一次只算一次" */
        return 1;
    }

    return 0;
}

int key_is_down(int key)
{
    if (key < 0 || key >= KEY_COUNT) {
        return 0;
    }

    return s_stable[key] != 0U;
}
