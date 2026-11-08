/**
 * @file    snake.c
 * @brief   极简贪吃蛇:蛇身、食物各占 1 个像素,整屏 128x64 当棋盘
 *
 * 全文只有 4 件事:
 *   1. body[] 是坐标数组,body[0] 是头
 *   2. step() 走一步:算新头 → 判断撞墙/撞自己 → 整体后移
 *   3. draw() 把食物和蛇逐点画进显存,再整屏刷一次
 *   4. snake_run() 死循环:读按键 → 到时间走一步 → 重画
 *
 * 用法:main.c 的 USER CODE 2 里
 *     oled_init(&hi2c1);
 *     keys_init();
 *     snake_run();          // 不返回
 *
 * 按键:方向键转向(不能掉头);撞死后按 MID 或 RST 重开。
 * 没有分数、没有暂停、速度固定 —— 这些是留给学员的作业。
 */

#include "snake.h"

#include "keys.h"
#include "oled.h"

#define MAX_LEN 100   /* 蛇最长 100 节,再长就不长了 */
#define STEP_MS 80    /* 每走一步间隔 80 ms */

typedef struct {
    int8_t x;
    int8_t y;
} point_t;                        /* 一个点 2 字节,x 0~127,y 0~63 装得下 */

static point_t body[MAX_LEN];     /* 蛇身,body[0] 是头 */
static uint16_t len;
static point_t food;
static int8_t dx = 1;             /* 当前方向 */
static int8_t dy = 0;
static uint32_t next_step;        /* 下一步的时刻 */
static uint32_t seed = 1;         /* 伪随机,给食物选位置用 */

static uint16_t rnd(void)
{
    seed = seed * 1664525u + 1013904223u;
    return (uint16_t)(seed >> 8);
}

static int on_body(int8_t x, int8_t y)
{
    for (uint16_t i = 0; i < len; i++) {
        if (body[i].x == x && body[i].y == y) {
            return 1;
        }
    }
    return 0;
}

static void put_food(void)
{
    do {
        /* 只在外框以内的区域选点,别落到边框上 */
        food.x = (int8_t)(1 + rnd() % (OLED_W - 2));
        food.y = (int8_t)(1 + rnd() % (OLED_H - 2));
    } while (on_body(food.x, food.y));   /* 也别落在蛇身上 */
}

static void reset(void)
{
    len = 3;
    body[0].x = 32;                   /* 从屏幕中间开始,朝右 */
    body[0].y = 32;
    body[1].x = 31;
    body[1].y = 32;
    body[2].x = 30;
    body[2].y = 32;
    dx = 1;
    dy = 0;
    seed = HAL_GetTick() + 1;
    put_food();
    next_step = HAL_GetTick() + STEP_MS;
}

/* 一圈 1 像素的外边框:4 条实心矩形拼出来 */
static void draw_border(void)
{
    oled_rect(0, 0, OLED_W, 1, 1);                 /* 上 */
    oled_rect(0, OLED_H - 1, OLED_W, 1, 1);        /* 下 */
    oled_rect(0, 0, 1, OLED_H, 1);                 /* 左 */
    oled_rect(OLED_W - 1, 0, 1, OLED_H, 1);        /* 右 */
}

static void draw(void)
{
    oled_clear();
    draw_border();
    oled_pixel(food.x, food.y, 1);
    for (uint16_t i = 0; i < len; i++) {
        oled_pixel(body[i].x, body[i].y, 1);
    }
    oled_show();
}

/** 走一步;返回 1 表示撞死了 */
static int step(void)
{
    int x = body[0].x + dx;
    int y = body[0].y + dy;

    if (x < 1 || x > OLED_W - 2 || y < 1 || y > OLED_H - 2) {
        return 1;                             /* 撞边框 */
    }

    int grow = (x == food.x && y == food.y);

    /* 撞自己:不吃食物时尾巴会让开,所以最后一节不算 */
    for (uint16_t i = 0; i < len - grow; i++) {
        if (body[i].x == x && body[i].y == y) {
            return 1;
        }
    }

    /* 整体后移一位,新头放进 body[0] */
    for (uint16_t i = (len < MAX_LEN ? len : MAX_LEN - 1); i > 0; i--) {
        body[i] = body[i - 1];
    }
    body[0].x = (int8_t)x;
    body[0].y = (int8_t)y;

    if (grow) {
        if (len < MAX_LEN) {
            len++;
        }
        put_food();
    }
    return 0;
}

void snake_run(void)
{
    reset();
    draw();

    for (;;) {
        keys_scan();

        /* 转向:上下只在左右走时才算,左右只在上下走时才算 —— 自然禁掉掉头 */
        if (key_pressed(KEY_UP) && dy == 0)    { dx = 0; dy = -1; }
        if (key_pressed(KEY_DOWN) && dy == 0)  { dx = 0; dy = 1; }
        if (key_pressed(KEY_LEFT) && dx == 0)  { dx = -1; dy = 0; }
        if (key_pressed(KEY_RIGHT) && dx == 0) { dx = 1; dy = 0; }

        /* 到时间才走一步 */
        if ((int32_t)(HAL_GetTick() - next_step) >= 0) {
            next_step = HAL_GetTick() + STEP_MS;

            if (step()) {
                oled_clear();
                draw_border();
                oled_text(46, 20, "GAME OVER", 1);
                oled_text(46, 34, "PRESS MID", 1);
                oled_show();
                while (!key_pressed(KEY_MID) && !key_pressed(KEY_RST)) {
                    keys_scan();
                    HAL_Delay(1);
                }
                reset();
            }
            draw();
        }

        HAL_Delay(1);
    }
}
