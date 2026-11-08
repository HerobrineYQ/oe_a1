/**
 * @file    oled.c
 * @brief   极简 OLED 库实现（配 oled.h）
 *
 * 代码里用 [步骤 N] 标出了课堂进度，可以直接按顺序讲 / 让学员分段敲。
 */

#include "oled.h"

#include <string.h>

/* 显存：1024 字节 = 8 页 x 128 列，每字节纵向 8 个像素（bit0 在上面） */
static uint8_t s_buf[OLED_W * OLED_H / 8];

/* 保存 I2C 句柄，下面所有函数都用它发送 */
static I2C_HandleTypeDef *s_i2c = 0;

/* 发命令：先发 1 字节控制字 0x00（表示"后面全是命令"），再跟 len 条命令。
 * 一次事务可以带多条命令，len=1 就是"发一条命令"（把那条命令放进变量，传它的地址） */
static void send_cmds(const uint8_t *cmds, uint16_t len)
{
    uint8_t buf[32];

    if (len > sizeof(buf) - 1) {
        return;
    }
    buf[0] = 0x00;
    memcpy(&buf[1], cmds, len);
    HAL_I2C_Master_Transmit(s_i2c, OLED_I2C_ADDR8, buf, (uint16_t)(len + 1), OLED_TIMEOUT_MS);
}

/* 发显存数据：控制字 0x40 表示"后面是数据" */
static void send_data(const uint8_t *data, uint16_t len)
{
    uint8_t buf[1 + OLED_W];

    if (len > OLED_W) {
        return;
    }
    buf[0] = 0x40;
    memcpy(&buf[1], data, len);
    HAL_I2C_Master_Transmit(s_i2c, OLED_I2C_ADDR8, buf, (uint16_t)(len + 1), OLED_TIMEOUT_MS);
}

/* ---------------------------------------------------------------- 步骤 1 */
void oled_init(I2C_HandleTypeDef *hi2c)
{
    /* 初始化命令表，每条命令后面的注释说明了它在做什么 */
    static const uint8_t init_cmds[] = {
        0xAE,         /* 关显示（配置期间先关掉，配好再开） */
        0xD5, 0x80,   /* 时钟分频 / 振荡频率 */
        0xA8, 0x3F,   /* 复用率 = 64 行（128x64 屏必须这一条） */
        0xD3, 0x00,   /* 显示偏移 = 0 */
        0x40,         /* 显示起始行 = 0 */
        0x8D, 0x14,   /* 电荷泵使能（模块靠它产生 OLED 需要的 ~7.5 V） */
        0x20, 0x02,   /* 页寻址模式（后面 oled_show 就用 0xB0 设页） */
        0xA1,         /* 左右方向：列 127 映射到 SEG0 */
        0xC8,         /* 上下方向：COM 反向扫描 → 屏幕坐标和图像一致 */
        0xDA, 0x12,   /* COM 引脚硬件配置（128x64 常用 0x12） */
        0x81, 0xCF,   /* 对比度（0x00~0xFF，越大越亮） */
        0xD9, 0xF1,   /* 预充电周期 */
        0xDB, 0x40,   /* VCOMH 电压 */
        0xA4,         /* 显示跟随显存内容（不是全亮） */
        0xA6,         /* 正常显示（0xA7 是反色） */
    };

    s_i2c = hi2c;
    HAL_Delay(100);              /* 模块上电后需要一点时间稳定 */

    /* 屏幕不亮时的第一件事：确认 0x3C 应答
     *   HAL_I2C_IsDeviceReady(hi2c, OLED_I2C_ADDR8, 3, 100) == HAL_OK ？
     * 不应答就查：接线、上拉电阻、SA0（0x3C/0x3D）、CubeMX 里 I2C1 是否使能 */
    send_cmds(init_cmds, sizeof(init_cmds));

    oled_clear();                /* 先清干净，避免上电花屏 */
    oled_show();

    const uint8_t display_on = 0xAF;   /* 开显示 */
    send_cmds(&display_on, 1);
}

/* ---------------------------------------------------------------- 步骤 2 */
void oled_clear(void)
{
    memset(s_buf, 0x00, sizeof(s_buf));
}

/* ---------------------------------------------------------------- 步骤 3 */
void oled_pixel(int x, int y, int on)
{
    if (x < 0 || x >= OLED_W || y < 0 || y >= OLED_H) {
        return;                 /* 越界直接丢掉，调用者不用自己判断 */
    }

    uint8_t *p = &s_buf[(y / 8) * OLED_W + x];   /* y/8 = 第几页 */
    uint8_t mask = (uint8_t)(1U << (y % 8));     /* y%8 = 这一页里的第几位 */

    if (on) {
        *p |= mask;
    } else {
        *p &= (uint8_t)~mask;
    }
}

/* ---------------------------------------------------------------- 步骤 5 */
void oled_hline(int x, int y, int w, int on)
{
    for (int i = 0; i < w; i++) {
        oled_pixel(x + i, y, on);
    }
}

void oled_rect(int x, int y, int w, int h, int on)
{
    for (int row = 0; row < h; row++) {
        oled_hline(x, y + row, w, on);
    }
}

/* ---------------------------------------------------------------- 步骤 4 */
void oled_show(void)
{
    for (int page = 0; page < 8; page++) {
        /* 0xB0+page：选中第 page 页；0x00 / 0x10：列地址低/高 4 位 = 0
         * 三条命令一次事务发完，比拆成 3 次事务省掉 2 次 START/地址/STOP */
        const uint8_t setup[3] = { (uint8_t)(0xB0 + page), 0x00, 0x10 };

        send_cmds(setup, 3);
        send_data(&s_buf[page * OLED_W], OLED_W);  /* 连续写 128 字节 */
    }
}

/* ---------------------------------------------------------------- 步骤 6 */
/* 3x5 字模：每个字符 5 行，每行 3 bit（bit2 = 最左列） */
typedef struct {
    char ch;
    uint8_t rows[5];
} glyph_t;

static const glyph_t k_font[] = {
    { ' ', { 0x0, 0x0, 0x0, 0x0, 0x0 } },
    { '0', { 0x7, 0x5, 0x5, 0x5, 0x7 } },
    { '1', { 0x2, 0x6, 0x2, 0x2, 0x7 } },
    { '2', { 0x7, 0x1, 0x7, 0x4, 0x7 } },
    { '3', { 0x7, 0x1, 0x7, 0x1, 0x7 } },
    { '4', { 0x5, 0x5, 0x7, 0x1, 0x1 } },
    { '5', { 0x7, 0x4, 0x7, 0x1, 0x7 } },
    { '6', { 0x7, 0x4, 0x7, 0x5, 0x7 } },
    { '7', { 0x7, 0x1, 0x1, 0x1, 0x1 } },
    { '8', { 0x7, 0x5, 0x7, 0x5, 0x7 } },
    { '9', { 0x7, 0x5, 0x7, 0x1, 0x7 } },
    { 'A', { 0x7, 0x5, 0x7, 0x5, 0x5 } },
    { 'B', { 0x6, 0x5, 0x6, 0x5, 0x6 } },
    { 'C', { 0x7, 0x4, 0x4, 0x4, 0x7 } },
    { 'D', { 0x6, 0x5, 0x5, 0x5, 0x6 } },
    { 'E', { 0x7, 0x4, 0x7, 0x4, 0x7 } },
    { 'F', { 0x7, 0x4, 0x7, 0x4, 0x4 } },
    { 'G', { 0x7, 0x4, 0x5, 0x5, 0x7 } },
    { 'H', { 0x5, 0x5, 0x7, 0x5, 0x5 } },
    { 'I', { 0x7, 0x2, 0x2, 0x2, 0x7 } },
    { 'J', { 0x1, 0x1, 0x1, 0x5, 0x7 } },
    { 'K', { 0x5, 0x5, 0x6, 0x5, 0x5 } },
    { 'L', { 0x4, 0x4, 0x4, 0x4, 0x7 } },
    { 'M', { 0x5, 0x7, 0x7, 0x5, 0x5 } },
    { 'N', { 0x5, 0x7, 0x7, 0x7, 0x5 } },
    { 'O', { 0x7, 0x5, 0x5, 0x5, 0x7 } },
    { 'P', { 0x7, 0x5, 0x7, 0x4, 0x4 } },
    { 'Q', { 0x7, 0x5, 0x5, 0x6, 0x3 } },
    { 'R', { 0x7, 0x5, 0x7, 0x6, 0x5 } },
    { 'S', { 0x7, 0x4, 0x7, 0x1, 0x7 } },
    { 'T', { 0x7, 0x2, 0x2, 0x2, 0x2 } },
    { 'U', { 0x5, 0x5, 0x5, 0x5, 0x7 } },
    { 'V', { 0x5, 0x5, 0x5, 0x5, 0x2 } },
    { 'W', { 0x5, 0x5, 0x7, 0x7, 0x5 } },
    { 'X', { 0x5, 0x5, 0x2, 0x5, 0x5 } },
    { 'Y', { 0x5, 0x5, 0x2, 0x2, 0x2 } },
    { 'Z', { 0x7, 0x1, 0x2, 0x4, 0x7 } },
};

static const uint8_t *find_glyph(char ch)
{
    for (unsigned i = 0; i < sizeof(k_font) / sizeof(k_font[0]); i++) {
        if (k_font[i].ch == ch) {
            return k_font[i].rows;
        }
    }

    return 0;                    /* 没收录的字符就当空格，只占位不画 */
}

void oled_text(int x, int y, const char *text, int scale)
{
    if (text == 0 || scale < 1) {
        return;
    }

    for (const char *p = text; *p != '\0'; p++) {
        const uint8_t *rows = find_glyph(*p);

        if (rows != 0) {
            for (int row = 0; row < 5; row++) {
                for (int col = 0; col < 3; col++) {
                    if ((rows[row] & (uint8_t)(0x4U >> col)) != 0U) {
                        /* 字模里的 1 个点放大成 scale x scale 的方块 */
                        oled_rect(x + col * scale, y + row * scale, scale, scale, 1);
                    }
                }
            }
        }

        x += 4 * scale;          /* 3 列字模 + 1 列字间距 */
    }
}
