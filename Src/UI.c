#include "stdio.h"

#include "ui.h"
#include "st7789v.h"
#include "colors.h"
#include "ui_fonts.h"
#include "ui_icons.h"
#include "ui_draw.h"

#include "st7789v.h"

/* ===== ВНУТРЕННИЕ ФУНКЦИИ ===== */

/* ---------- Макет экрана 240x320 (книжная) ---------- */
/* Верхняя строка: температура/напряжение */
#define X_VOLT    20
#define X_TEMP   170
#define Y_TOP    10

/* Часы крупные */
#define X_TIME    10
#define Y_TIME    83

/* Дата */
#define X_DATE   80
#define Y_DATE   165

/* Основной пробег (сверху, как ты сказал) */
#define X_ODO_MAIN 120
#define Y_ODO_MAIN 10  /* подгони: у тебя ранее было (120,10) */

/* Рамка блоков счетчиков */
#define BOX_X      5
#define BOX_Y    200
#define BOX_W    230
#define BOX_H    115
#define BOX_VX   120
#define BOX_HY   (BOX_Y + BOX_H/2)

/* Позиции 3 счетчиков (пример: 2 слева + 1 справа сверху) */
#define X_CNT_L    115
#define X_CNT_R    230
#define Y_CNT_0    215
#define Y_CNT_1    272

/* ---------- Внутренние helpers ---------- */

//static void ui_draw_time(void);
//static void ui_draw_top_icons(void);
//static void ui_draw_voltage_temp(void);
//static void ui_draw_time_placeholder(void);
//static void ui_draw_date_placeholder(void);
//static void ui_draw_main_odometer_placeholder(void);
//static void ui_draw_odometer_frame(void);
//static void ui_draw_odometer_icons(void);
//static void ui_draw_odometer_placeholders(void);

/* ===== ПУБЛИЧНАЯ ===== */


static void draw_mono(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t bpr,
                      const uint8_t *bmp, uint16_t fg, uint16_t bg)
{
    lcd_draw_mono_1bpp(x, y, w, h, bmp, bpr, fg, bg);
}

/* font12: поиск индекса символа по карте */
static int font12_find(char ch)
{
    for (int i = 0; font12_map[i] != 0; i++) {
        if (font12_map[i] == ch) return i;
    }
    return -1;
}

static void ui_draw_char12(uint16_t x, uint16_t y, char ch, uint16_t fg, uint16_t bg)
{
    int idx = font12_find(ch);
    if (idx < 0) return;

    lcd_draw_mono_1bpp(
        x, y,
        FONT12_W, FONT12_H,
        &font12_glyphs[idx][0],
        FONT12_BPR,
        fg, bg
    );
}

static void ui_draw_string12(uint16_t x, uint16_t y, const char *s, uint16_t fg, uint16_t bg)
{
    uint16_t cx = x;
    while (*s) {
        ui_draw_char12(cx, y, *s, fg, bg);
        cx += (FONT12_W + 1); // шаг между символами
        s++;
    }
}


static void ui_draw_digit75(uint16_t x, uint16_t y, uint8_t d, uint16_t fg, uint16_t bg)
{
    if (d > 9) return;
    draw_mono(x, y, FONT75_DIGITS_W, FONT75_DIGITS_H, FONT75_DIGITS_BPR,
              &font75_digits[d][0], fg, bg);
}

static void ui_draw_colon75(uint16_t x, uint16_t y, uint16_t fg, uint16_t bg)
{
    /* В твоей генерации punct есть map; если у тебя отдельный массив colon — замени */
    /* Предположим, что ':' — первый символ в font75_punct_map */
    int idx = -1;
    for (int i = 0; font75_punct_map[i] != 0; i++) {
        if (font75_punct_map[i] == ':') { idx = i; break; }
    }
    if (idx < 0) return;

    draw_mono(x, y, FONT75_PUNCT_W, FONT75_PUNCT_H, FONT75_PUNCT_BPR,
              &font75_punct[idx][0], fg, bg);
}

/* --------- font36: 6 digits --------- */

static void ui_draw_digit36(uint16_t x, uint16_t y, uint8_t d, uint16_t fg, uint16_t bg)
{
    if (d > 9) return;
    draw_mono(x, y, FONT36_DIGITS_W, FONT36_DIGITS_H, FONT36_DIGITS_BPR,
              &font36_digits[d][0], fg, bg);
}

static void ui_draw_number6_font36_center(uint16_t cx, uint16_t y, uint32_t val)
{
    char s[7];
    s[6] = 0;
    for (int i = 5; i >= 0; i--) { s[i] = (val % 10) + '0'; val /= 10; }

    uint16_t dx = FONT36_DIGITS_W + 1;
    uint16_t total_w = 6 * dx - 1;
    uint16_t x = (cx > total_w/2) ? (cx - total_w/2) : 0;

    for (int i = 0; i < 6; i++) {
        ui_draw_digit36(x + i*dx, y, (uint8_t)(s[i] - '0'), WHITE, BLACK);
    }
}

/* --------- font27: 5 digits --------- */

static void ui_draw_digit27(uint16_t x, uint16_t y, uint8_t d, uint16_t fg, uint16_t bg)
{
    if (d > 9) return;
    draw_mono(x, y, FONT27_DIGITS_W, FONT27_DIGITS_H, FONT27_DIGITS_BPR,
              &font27_digits[d][0], fg, bg);
}

static void ui_draw_number5_font27_right(uint16_t rx, uint16_t y, uint32_t val)
{
    char s[6];
    s[5] = 0;
    for (int i = 4; i >= 0; i--) { s[i] = (val % 10) + '0'; val /= 10; }

    uint16_t dx = FONT27_DIGITS_W + 1;
    uint16_t total_w = 5 * dx - 1;
    uint16_t x = (rx > total_w) ? (rx - total_w) : 0;

    for (int i = 0; i < 5; i++) {
        ui_draw_digit27(x + i*dx, y, (uint8_t)(s[i] - '0'), WHITE, BLACK);
    }
}

/* --------- дата/темп/вольты --------- */

static void ui_draw_date(uint8_t dd, uint8_t mo, uint16_t yyyy)
{
    char buf[16];
    /* 22.03.2026 */
    snprintf(buf, sizeof(buf), "%02u.%02u.%04u", dd, mo, (unsigned)yyyy);

    /* Небольшой трюк: очистим зону под дату, чтобы не было хвостов */
    lcd_fill_rect(60, Y_DATE-2, 170, FONT12_H+4, BLACK);

    /* Вывод по центру около X_DATE */
    ui_draw_string12(X_DATE - 8*4, Y_DATE, buf, WHITE, BLACK); // подгони центр
}

static void ui_draw_voltage(uint16_t mv)
{
    /* 14700 -> "14.7V" */
    uint16_t v = mv / 1000;
    uint16_t d1 = (mv % 1000) / 100; // десятые

    char buf[8];
    snprintf(buf, sizeof(buf), "%u.%uV", v, d1);

    lcd_fill_rect(40, 45, 90, FONT12_H+4, BLACK);
    ui_draw_string12(X_VOLT, 50, buf, WHITE, BLACK);
}

static void ui_draw_temp(int16_t t)
{
    /* -12°C */
    char buf[8];
    int neg = (t < 0);
    if (neg) t = -t;

    /* degree: используем '\xB0' нельзя — лучше прямо '°' если файл UTF-8,
       или вставь через \u00B0 в генератор, а тут просто '°' */
    if (neg) snprintf(buf, sizeof(buf), "-%dC", (int)t);
    else     snprintf(buf, sizeof(buf), "%dC", (int)t);

    /* очистка */
    lcd_fill_rect(140, 45, 90, FONT12_H+4, BLACK);

    /* печатаем число */
    ui_draw_string12(X_TEMP, 50, buf, WHITE, BLACK);

    /* дорисуем градус отдельным символом между числом и C:
       найдём позицию перед 'C' */
    /* очень просто: рисуем '°' на фиксированной позиции (подгони) */
    ui_draw_char12(X_TEMP + (FONT12_W + 1) * (neg ? 3 : 2), 50, '°', WHITE, BLACK);
}

void ui_draw_static(void)
{
	lcd_fill_screen(BLACK);


	/* Рамка блока счётчиков */
    /* Если у тебя нет line/rect примитивов — рисуем прямоугольник fill_rect */
    /* Верх/низ */
    lcd_fill_rect(BOX_X, BOX_Y, BOX_W, 1, WHITE);
    lcd_fill_rect(BOX_X, BOX_Y + BOX_H - 1, BOX_W, 1, WHITE);
    /* Лево/право */
    lcd_fill_rect(BOX_X, BOX_Y, 1, BOX_H, WHITE);
    lcd_fill_rect(BOX_X + BOX_W - 1, BOX_Y, 1, BOX_H, WHITE);

    /* Разделители */
    lcd_fill_rect(BOX_VX, BOX_Y, 1, BOX_H, WHITE);
    lcd_fill_rect(BOX_X, BOX_HY, BOX_W, 1, WHITE);

    /* Можно тут же нарисовать иконки 24x24 позже */
	ui_draw_rect(5, 5, 230, 310, WHITE);
}

void ui_draw_all(const ui_data_t *d)
{
    /* Время */
    ui_draw_time(d->hh, d->mm);

    /* Дата */
    ui_draw_date(d->dd, d->mo, d->yyyy);

    /* Вольты/темп */
    ui_draw_voltage(d->volt_mv);
    ui_draw_temp(d->temp_c);

    /* Основной пробег (6 цифр, сверху) */
    /* Чистим область под основной пробег (подгони под свой макет) */
    lcd_fill_rect(20, Y_ODO_MAIN, 200, FONT36_DIGITS_H + 2, BLACK);
    ui_draw_number6_font36_center(X_ODO_MAIN, Y_ODO_MAIN, d->odo_main);

    /* Счётчики в рамках: 3 штуки (пример по твоей раскладке) */
    /* Лево верх */
    lcd_fill_rect(40, Y_CNT_0, 90, FONT27_DIGITS_H + 2, BLACK);
    ui_draw_number5_font27_right(X_CNT_L, Y_CNT_0, d->odo0);

    /* Лево низ */
    lcd_fill_rect(40, Y_CNT_1, 90, FONT27_DIGITS_H + 2, BLACK);
    ui_draw_number5_font27_right(X_CNT_L, Y_CNT_1, d->odo1);

    /* Право верх */
    lcd_fill_rect(150, Y_CNT_0, 85, FONT27_DIGITS_H + 2, BLACK);
    ui_draw_number5_font27_right(X_CNT_R, Y_CNT_0, d->odo2);
}

void ui_draw_time(uint8_t hh, uint8_t mm)
{
    uint16_t x = X_TIME;
    uint16_t y = Y_TIME;

    uint16_t dx = FONT75_DIGITS_W + 2;   // подгони (у тебя уже норм)
    ui_draw_digit75(x + dx*0, y, hh/10, WHITE, BLACK);
    ui_draw_digit75(x + dx*1, y, hh%10, WHITE, BLACK);
    ui_draw_colon75 (x + dx*2, y, WHITE, BLACK);
    ui_draw_digit75(x + dx*3, y, mm/10, WHITE, BLACK);
    ui_draw_digit75(x + dx*4, y, mm%10, WHITE, BLACK);
}


/* Рисует один глиф 1bpp */
static void ui_draw_glyph_1bpp(uint16_t x, uint16_t y,
                               uint16_t w, uint16_t h, uint16_t bpr,
                               const uint8_t *bmp,
                               uint16_t fg, uint16_t bg)
{
    lcd_draw_mono_1bpp(x, y, w, h, bmp, bpr, fg, bg);
}

void ui_draw_static_test(void)
{
    /* Очистка экрана */
    lcd_fill_screen(BLACK);

    /* Рамки счётчиков (если уже есть ui_draw_static — лучше вызвать её) */
    ui_draw_static();

    /* ===== Верх: напряжение / температура ===== */
    lcd_fill_rect(0, 40, 240, 30, BLACK);     // чистим строку
    ui_draw_string12(20,  50, "14.7V", WHITE, BLACK);
    ui_draw_string12(160, 50, "-12",   WHITE, BLACK);
    ui_draw_char12  (160 + (FONT12_W + 1) * 3, 45, '\xB0', WHITE, BLACK);
    ui_draw_char12  (160 + (FONT12_W + 1) * 4, 50, 'C', WHITE, BLACK);

    /* ===== Часы крупные ===== */
    ui_draw_time(88, 88);

    /* ===== Дата ===== */
    lcd_fill_rect(0, 160, 240, 25, BLACK);
    ui_draw_string12(55, 165, "22.03.2026", WHITE, BLACK);

    /* ===== Основной пробег (6 цифр, font36) ===== */
    lcd_fill_rect(0, 0, 240, 40, BLACK);
    ui_draw_number6_font36_center(120, 10, 123456);

    /* ===== Счётчики (5 цифр, font27) ===== */
    /* Лево-верх */
    lcd_fill_rect(5, 205, 115, 50, BLACK);
    ui_draw_number5_font27_right(115, 215, 88);

    /* Лево-низ */
    lcd_fill_rect(5, 262, 115, 50, BLACK);
    ui_draw_number5_font27_right(115, 272, 88);

    /* Право-верх */
    lcd_fill_rect(120, 205, 115, 50, BLACK);
    ui_draw_number5_font27_right(230, 215, 5488);

    /* Право-низ (если хочешь четвертый) */
    lcd_fill_rect(120, 262, 115, 50, BLACK);
    ui_draw_number5_font27_right(230, 272, 11118);
}
