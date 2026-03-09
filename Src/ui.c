#include <stdio.h>
#include <string.h>
#include <stm32f3xx_hal.h>

#include "ui.h"
#include "gfx.h"
#include "ui_fonts.h"
#include "buttons.h"
#include "ui_icons.h"
#include "nv.h"
#include "app.h"
#include "app_timeset.h"
#include "ds3231.h"
#include "speed.h"
#include "logo.h"

// ------------------------------------------
// цвета
// ------------------------------------------
#define UI_FG          0xFFFF  // белый
#define UI_BG          0x0000  // черный
#define UI_CLOCK_COLOR  0xFFFF  // приглушённый голубой
#define UI_YELLOW      0xFFE0  // бледно-желтый 0xFFE0 - оригинал
#define UI_ICON_COLOR  0xFFE0  // бледно-желтый , оригинел - 0x7BEF серый
#define UI_WARN_RED    0xF800  // красный для варнингов

#define SVC_LIMIT_KM   100u    // меньше 100 км - красным

// ------------------------------------------
// координаты экрана (240x320)
// ------------------------------------------

// верхняя строка: напруга/температура
#define X_VOLT    20
#define X_TEMP   170
#define Y_TOPTXT  55

// часы крупные
#define X_TIME    10
#define Y_TIME    83
#define COLON_X   99

// дата
#define X_DATE    110
#define Y_DATE   165

// основной пробег (самый верх)
#define X_ODO_MAIN_CENTER 120
#define Y_ODO_MAIN         10

// рамка нижнего блока (там trip или svc)
#define BOX_X      5
#define BOX_Y    190
#define BOX_W    230
#define BOX_H    125

// иконка варнинга
#define WARN_X     10
#define WARN_Y     10

// стрелка выбора строки
#define ARROW_W     6
#define ARROW_H    18
#define ARROW_X     (BOX_X + 55)

// таймаут возврата с сервисной страницы
#define UI_SVC_TIMEOUT_MS  10000u

// пресеты для сервиса (на всякий)
#define SVC_PRESET_OIL    7000u
#define SVC_PRЕSET_SPARK  20000u
#define SVC_PRESET_GRM    60000u

// ------------------------------------------
// слои и кэши
// ------------------------------------------
static ui_layer_t g_layer = UI_LAYER_FORWARD;

// кэш для счетчиков - чтоб лишний раз не дергать дисплей
typedef struct {
    uint32_t trip_fuel;
    uint32_t trip_day;
    uint32_t trip_ab;
    uint8_t  valid;
} ui_cache_t;
static ui_cache_t g_cache = {0};

// кэш для верхней строки и времени/даты
typedef struct {
    char volt[8];      // "14.7V"
    char temp[10];     // "-12°C"
    char date[16];     // "22.03.2026"
    uint8_t hh, mm;    // текущее время
    uint8_t valid_top;
    uint8_t valid_time;
    uint8_t valid_date;
} ui_cache_top_t;
static ui_cache_top_t g_top = {0};

// кэш одометра
static uint8_t  g_odo_valid = 0;
static uint32_t g_odo_last  = 0;

// ------------------------------------------
// режимы и состояния
// ------------------------------------------
static ui_page_t g_page = UI_PAGE_MAIN;          // текущая страница
static uint32_t  g_page_deadline_ms = 0;         // когда сваливать с SVC
static uint8_t   g_warn_on = 0;                   // горит ли варнинг
static uint8_t   g_svc_drawn = 0;                  // рисовали ли svc страницу

static ui_mode_t g_mode = UI_MODE_BROWSE;
static uint8_t   g_sel_visible = 0;                // видна ли стрелка
static uint8_t   g_sel_row = 0;                    // выбранная строка (0..2)
static uint8_t   g_dirty = 0;                       // флаг "надо сохранить в eeprom"

static uint32_t  g_reset_exit_deadline_ms = 0;      // таймер для выхода из сброса

// для timeset режима
static app_timeset_field_t g_ts_field = APP_TS_HH;

// ------------------------------------------
// хелперы
// ------------------------------------------

// цвет строки сервиса (красный если мало осталось)
static inline uint16_t svc_color(uint32_t remaining_km)
{
    return (remaining_km < SVC_LIMIT_KM) ? UI_WARN_RED : UI_FG;
}

// надо ли включать варнинг?
static inline uint8_t svc_warn_needed(const ui_data_t *d)
{
    return (d->svc_oil   < SVC_LIMIT_KM) ||
           (d->svc_grm   < SVC_LIMIT_KM) ||
           (d->svc_spark < SVC_LIMIT_KM);
}

// следующая страница
static ui_page_t page_next(ui_page_t p)
{
    if (p >= UI_PAGE_SVC) return UI_PAGE_MAIN;
    return (ui_page_t)(p + 1);
}

// предыдущая страница
static ui_page_t page_prev(ui_page_t p)
{
    if (p == UI_PAGE_MAIN) return UI_PAGE_SVC;
    return (ui_page_t)(p - 1);
}

// ------------------------------------------
// отрисовка стрелки
// ------------------------------------------
static void draw_arrow_row(uint8_t row, uint16_t color)
{
    int row_h = BOX_H / 3;                           // высота строки
    int y = BOX_Y + row * row_h + (row_h - ARROW_H)/2;
    GFX_FillRect(ARROW_X, y, ARROW_W, ARROW_H, color);
}

// ------------------------------------------
// умные функции перерисовки (чтоб не мигало)
// ------------------------------------------

// перерисовать строку шрифтом 12 (если изменилась)
static void redraw_string12(int x, int y, const char *old_s, const char *new_s)
{
    if (old_s && old_s[0]) {
        UIF_DrawString12(x, y, old_s, UI_BG, UI_BG);   // стереть старую
    }
    if (new_s && new_s[0]) {
        UIF_DrawString12(x, y, new_s, UI_FG, UI_BG);   // нарисовать новую
    }
}

// перерисовать цифру 75 (если изменилась)
static void redraw_digit75(int x, int y, uint8_t old_d, uint8_t new_d)
{
    if (old_d == new_d) return;
    UIF_DrawDigit75(x, y, old_d, UI_BG, UI_BG);   // стереть
    UIF_DrawDigit75(x, y, new_d, UI_FG, UI_BG);   // нарисовать
}

// перерисовать 6-значное число справа (для счетчиков)
static void redraw_uint27_right6(int right_x, int y, uint32_t oldv, uint32_t newv)
{
    if (oldv == newv) return;
    UIF_UpdateNumber6_Right27(right_x, y, oldv, newv, UI_YELLOW, UI_BG);
}

// ------------------------------------------
// отрисовка конкретных элементов
// ------------------------------------------

// напряжение
static void draw_voltage(uint16_t mv)
{
    uint16_t v  = mv / 1000;
    uint16_t d1 = (mv % 1000) / 100;
    char buf[8];
    snprintf(buf, sizeof(buf), "%u.%uV", (unsigned)v, (unsigned)d1);

    if (!g_top.valid_top || strcmp(g_top.volt, buf) != 0) {
        redraw_string12(X_VOLT, Y_TOPTXT, g_top.valid_top ? g_top.volt : "", buf);
        strncpy(g_top.volt, buf, sizeof(g_top.volt));
        g_top.volt[sizeof(g_top.volt) - 1] = 0;
    }
    g_top.valid_top = 1;
}

// температура (с градусами)
static void draw_temp(int16_t t)
{
    char buf[10];
    int neg = (t < 0);
    if (neg) t = -t;

    if (neg) snprintf(buf, sizeof(buf), "-%d%cC", (int)t, (char)0xB0);
    else     snprintf(buf, sizeof(buf),  "%d%cC", (int)t, (char)0xB0);

    if (!g_top.valid_top || strcmp(g_top.temp, buf) != 0) {
        redraw_string12(X_TEMP, Y_TOPTXT, g_top.valid_top ? g_top.temp : "", buf);
        strncpy(g_top.temp, buf, sizeof(g_top.temp));
        g_top.temp[sizeof(g_top.temp) - 1] = 0;
    }
    g_top.valid_top = 1;
}

// время (крупно)
static void draw_time(uint8_t hh, uint8_t mm)
{
    int a75 = UIF_Adv75();
    int x = X_TIME;
    int y = Y_TIME;

    // первый запуск - рисуем всё
    if (!g_top.valid_time) {
        UIF_DrawDigit75(x + 0*a75, y, hh/10, UI_CLOCK_COLOR, UI_BG);
        UIF_DrawDigit75(x + 1*a75, y, hh%10, UI_CLOCK_COLOR, UI_BG);
        UIF_DrawPunct75(COLON_X, y, ':', UI_CLOCK_COLOR, UI_BG);
        UIF_DrawDigit75(x + 3*a75, y, mm/10, UI_CLOCK_COLOR, UI_BG);
        UIF_DrawDigit75(x + 4*a75, y, mm%10, UI_CLOCK_COLOR, UI_BG);

        g_top.hh = hh;
        g_top.mm = mm;
        g_top.valid_time = 1;
        return;
    }

    // обновляем только изменившиеся цифры
    redraw_digit75(x + 0*a75, y, g_top.hh/10, hh/10);
    redraw_digit75(x + 1*a75, y, g_top.hh%10, hh%10);
    redraw_digit75(x + 3*a75, y, g_top.mm/10, mm/10);
    redraw_digit75(x + 4*a75, y, g_top.mm%10, mm%10);

    g_top.hh = hh;
    g_top.mm = mm;
}

// дата
static void draw_date(uint8_t dd, uint8_t mo, uint16_t yyyy)
{
    char buf[16];
    snprintf(buf, sizeof(buf), "%02u.%02u.%04u", dd, mo, (unsigned)yyyy);

    int a12 = UIF_Adv12();
    int x = X_DATE - 4 * a12;    // сдвиг влево чтоб по центру было

    if (!g_top.valid_date || strcmp(g_top.date, buf) != 0) {
        redraw_string12(x, Y_DATE, g_top.valid_date ? g_top.date : "", buf);
        strncpy(g_top.date, buf, sizeof(g_top.date));
        g_top.date[sizeof(g_top.date) - 1] = 0;
        g_top.valid_date = 1;
    }
}

// одометр
static void draw_odo_main(uint32_t val)
{
    if (val > 999999) val %= 1000000;

    if (!g_odo_valid) {
        UIF_DrawNumber6_Center36_Fixed(X_ODO_MAIN_CENTER, Y_ODO_MAIN, val, UI_YELLOW, UI_BG);
        g_odo_last = val;
        g_odo_valid = 1;
        return;
    }

    if (g_odo_last == val) return;

    // стереть старое, нарисовать новое
    UIF_DrawNumber6_Center36_Fixed(X_ODO_MAIN_CENTER, Y_ODO_MAIN, g_odo_last, UI_BG, UI_BG);
    UIF_DrawNumber6_Center36_Fixed(X_ODO_MAIN_CENTER, Y_ODO_MAIN, val, UI_YELLOW, UI_BG);
    g_odo_last = val;
}

// иконки для главной страницы (бенз, суточные, АБ)
static void draw_page_main_static_icons(void)
{
    int x = BOX_X + 10;
    int y0 = 192;
    int y1 = 234;
    int y2 = 276;

    UI_DrawIcon36(x, y0, ICON_BENZ, UI_ICON_COLOR, UI_BG);
    UI_DrawIcon36(x, y1, ICON_SUT,  UI_ICON_COLOR, UI_BG);
    UI_DrawIcon36(x, y2, ICON_AB,   UI_ICON_COLOR, UI_BG);

    // разделители строк
    int row_h = BOX_H / 3;
    GFX_DrawHLine(BOX_X, BOX_Y + row_h,     BOX_W, UI_FG);
    GFX_DrawHLine(BOX_X, BOX_Y + 2*row_h,   BOX_W, UI_FG);
}

// иконки для сервисной страницы (масло, грм, свечи)
static void draw_page_svc_static_icons(void)
{
    int x = BOX_X + 10;
    int y0 = 192;
    int y1 = 234;
    int y2 = 276;

    UI_DrawIcon36(x, y0, ICON_OIL,   UI_ICON_COLOR, UI_BG);
    UI_DrawIcon36(x, y1, ICON_GRM,   UI_ICON_COLOR, UI_BG);
    UI_DrawIcon36(x, y2, ICON_SPARK, UI_ICON_COLOR, UI_BG);

    // разделители строк
    int row_h = BOX_H / 3;
    GFX_DrawHLine(BOX_X, BOX_Y + row_h,     BOX_W, UI_FG);
    GFX_DrawHLine(BOX_X, BOX_Y + 2*row_h,   BOX_W, UI_FG);
}

// значения на главной странице (трипы)
static void draw_page_main_values(const ui_data_t *d)
{
    int row_h = BOX_H / 3;
    int right_x = BOX_X + BOX_W - 8;

    redraw_uint27_right6(right_x, BOX_Y + 0*row_h + 8, g_cache.trip_fuel, d->trip_fuel);
    redraw_uint27_right6(right_x, BOX_Y + 1*row_h + 8, g_cache.trip_day,  d->trip_day);
    redraw_uint27_right6(right_x, BOX_Y + 2*row_h + 8, g_cache.trip_ab,   d->trip_ab);

    g_cache.trip_fuel = d->trip_fuel;
    g_cache.trip_day  = d->trip_day;
    g_cache.trip_ab   = d->trip_ab;
}

// значения на сервисной странице (остатки)
static void draw_page_svc_values_once(const ui_data_t *d)
{
    int row_h = BOX_H / 3;
    int right_x = BOX_X + BOX_W - 8;

    // разделители (на всякий)
    GFX_DrawHLine(BOX_X, BOX_Y + row_h,     BOX_W, UI_FG);
    GFX_DrawHLine(BOX_X, BOX_Y + 2*row_h,   BOX_W, UI_FG);

    // рисуем цифры с правильным цветом
    UIF_DrawNumber6_Right27_Fixed(right_x, BOX_Y + 0*row_h + 8, d->svc_oil, UI_BG, UI_BG);
    UIF_DrawUInt_Right27(right_x, BOX_Y + 0*row_h + 8, d->svc_oil,   svc_color(d->svc_oil),   UI_BG);

    UIF_DrawNumber6_Right27_Fixed(right_x, BOX_Y + 1*row_h + 8, d->svc_grm, UI_BG, UI_BG);
    UIF_DrawUInt_Right27(right_x, BOX_Y + 1*row_h + 8, d->svc_grm,   svc_color(d->svc_grm),   UI_BG);

    UIF_DrawNumber6_Right27_Fixed(right_x, BOX_Y + 2*row_h + 8, d->svc_spark, UI_BG, UI_BG);
    UIF_DrawUInt_Right27(right_x, BOX_Y + 2*row_h + 8, d->svc_spark, svc_color(d->svc_spark), UI_BG);

    g_svc_drawn = 1;
}

// обновление варнинга
static void ui_warn_update(const ui_data_t *d)
{
    uint8_t need = svc_warn_needed(d);
    if (need == g_warn_on) return;

    if (need) {
        UI_DrawIcon36(WARN_X, WARN_Y, ICON_ERROR, UI_WARN_RED, UI_BG);
    } else {
        GFX_FillRect(WARN_X, WARN_Y, 36, 36, UI_BG);   // размер иконки
    }
    g_warn_on = need;
}

// рамка для timeset режима
static void timeset_draw_focus(app_timeset_field_t f, uint16_t color)
{
    const int pad = 2;
    const int a75 = UIF_Adv75();
    const int h75 = UIF_H75();
    const int a12 = UIF_Adv12();
    const int h12 = UIF_H12();

    const int hh_x = X_TIME + 0 * a75;
    const int mm_x = X_TIME + 3 * a75;
    const int date_x0 = X_DATE - 4 * a12;

    switch (f) {
    case APP_TS_HH:
        GFX_DrawRect(hh_x - pad, Y_TIME - pad, 2*a75 + 2*pad - 1, h75 + 2*pad, color);
        break;
    case APP_TS_MM:
        GFX_DrawRect(mm_x - pad, Y_TIME - pad, 2*a75 + 2*pad - 1, h75 + 2*pad, color);
        break;
    case APP_TS_DD:
        GFX_DrawRect(date_x0 - pad, Y_DATE - pad, 2*a12 + 2*pad - 1, h12 + 2*pad, color);
        break;
    case APP_TS_MO:
        GFX_DrawRect(date_x0 + 3*a12 - pad, Y_DATE - pad, 2*a12 + 2*pad - 1, h12 + 2*pad, color);
        break;
    case APP_TS_YYYY:
        GFX_DrawRect(date_x0 + 6*a12 - pad, Y_DATE - pad, 4*a12 + 2*pad - 1, h12 + 2*pad, color);
        break;
    default:
        break;
    }
}

// ------------------------------------------
// публичные функции
// ------------------------------------------

void UI_Init(void)
{
    g_layer = UI_LAYER_FORWARD;
}



void UI_DrawStatic(void)
{
    GFX_FillScreen(UI_BG);

    // рамка нижнего блока
    GFX_DrawRect(BOX_X, BOX_Y, BOX_W, BOX_H, UI_FG);

    if (g_page == UI_PAGE_MAIN) {
        draw_page_main_static_icons();
    } else {
        draw_page_svc_static_icons();
    }

    // сброс кэшей
    memset(&g_cache, 0, sizeof(g_cache));
    memset(&g_top, 0, sizeof(g_top));
    g_odo_valid = 0;
}

void UI_DrawTopText(const ui_data_t *d)
{
    draw_voltage(d->volt_mv);
    draw_temp(d->temp_c);
}

void UI_DrawTime(const ui_data_t *d)
{
    draw_time(d->hh, d->mm);
}

void UI_DrawDate(const ui_data_t *d)
{
    draw_date(d->dd, d->mo, d->yyyy);
}

void UI_DrawOdoMain(const ui_data_t *d)
{
    draw_odo_main(d->odo_main);
}

void UI_DrawCounters(const ui_data_t *d)
{
    if (!g_cache.valid) {
        g_cache.valid = 1;
        g_cache.trip_fuel = ~d->trip_fuel;
        g_cache.trip_day  = ~d->trip_day;
        g_cache.trip_ab   = ~d->trip_ab;
    }

    if (g_page == UI_PAGE_MAIN) {
        draw_page_main_values(d);
    } else {
        if (!g_svc_drawn) {
            draw_page_svc_values_once(d);
        }
    }
}

void UI_DrawAll(const ui_data_t *d)
{
    UI_DrawTopText(d);
    UI_DrawTime(d);
    UI_DrawDate(d);
    UI_DrawOdoMain(d);
    UI_DrawCounters(d);
    ui_warn_update(d);
}

void UI_SetPage(ui_page_t page)
{
    if (page > UI_PAGE_SVC) page = UI_PAGE_MAIN;
    if (page == g_page) return;

    g_page = page;

    // чистим нижний блок
    GFX_FillRect(BOX_X+1, BOX_Y+1, BOX_W-2, BOX_H-2, UI_BG);
    GFX_DrawRect(BOX_X, BOX_Y, BOX_W, BOX_H, UI_FG);

    if (g_page == UI_PAGE_MAIN) {
        draw_page_main_static_icons();
    } else {
        draw_page_svc_static_icons();
        g_svc_drawn = 0;
    }

    g_cache.valid = 0;   // перерисовать цифры
}

ui_page_t UI_GetPage(void)
{
    return g_page;
}

void UI_OnUserActivity(uint32_t now_ms)
{
    if (g_page == UI_PAGE_SVC) {
        g_page_deadline_ms = now_ms + UI_SVC_TIMEOUT_MS;
    }
}

void UI_Tick(uint32_t now_ms, const ui_data_t *d)
{
    // таймаут на сервисной странице
    if (g_page == UI_PAGE_SVC) {
        if (g_page_deadline_ms == 0) {
            g_page_deadline_ms = now_ms + UI_SVC_TIMEOUT_MS;
        } else if ((int32_t)(now_ms - g_page_deadline_ms) >= 0) {
            UI_SetPage(UI_PAGE_MAIN);
            g_page_deadline_ms = 0;
            UI_DrawCounters(d);
        }
    }

    // таймаут после сброса
    if (g_reset_exit_deadline_ms) {
        if ((int32_t)(now_ms - g_reset_exit_deadline_ms) >= 0) {
            g_reset_exit_deadline_ms = 0;
            UI_SetSelVisible(0);
            g_mode = UI_MODE_BROWSE;
            UI_SetPage(UI_PAGE_MAIN);
            g_dirty = 1;
            UI_DrawCounters(d);
        }
    }
}

void UI_SetSelVisible(uint8_t on)
{
    if (on == g_sel_visible) return;
    g_sel_visible = on;
    draw_arrow_row(g_sel_row, on ? UI_FG : UI_BG);
}

void UI_SetSelRow(uint8_t row)
{
    if (row > 2) row = 2;
    if (row == g_sel_row) return;

    if (g_sel_visible) {
        draw_arrow_row(g_sel_row, UI_BG);
        draw_arrow_row(row, UI_FG);
    }
    g_sel_row = row;
}

uint8_t UI_GetSelRow(void)
{
    return g_sel_row;
}

void UI_ResetSelectedCounter(ui_data_t *d)
{
    if (g_page == UI_PAGE_MAIN) {
        APP_ResetTrip(d, g_sel_row);
        g_cache.valid = 0;
    } else {
        APP_ResetSvc(d, g_sel_row);
        g_svc_drawn = 0;
    }
    UI_DrawCounters(d);
    g_dirty = 1;
    ui_warn_update(d);
}

void UI_UpdateWarn(const ui_data_t *d)
{
    ui_warn_update(d);
}

uint8_t UI_IsDirty(void)
{
    return g_dirty;
}

void UI_ClearDirty(void)
{
    g_dirty = 0;
}

void UI_SetDirty(void)
{
    g_dirty = 1;
}

void UI_HandleButtonEvent(uint32_t now_ms, btn_id_t id, btn_evt_t evt, ui_data_t *d)
{
    (void)now_ms;   // пока не юзаем, но пригодится

    if (evt == BTN_EVT_NONE) return;

    // ================== TIMESET MODE ==================
    if (g_mode == UI_MODE_TIMESET) {
        if (evt == BTN_EVT_HOLD_2S && id == BTN_SEL) {
            timeset_draw_focus(g_ts_field, UI_BG);
            DS3231_WriteTimeDate(d);
            g_mode = UI_MODE_BROWSE;
            return;
        }

        if (evt == BTN_EVT_PRESS) {
            timeset_draw_focus(g_ts_field, UI_BG);
            if (id == BTN_SEL) {
                g_ts_field = (app_timeset_field_t)((g_ts_field + 1) % APP_TS_MAX);
            } else if (id == BTN_UP) {
                APP_TimeSet_Change(d, g_ts_field, +1);
            } else if (id == BTN_DN) {
                APP_TimeSet_Change(d, g_ts_field, -1);
            }
            UI_DrawTime(d);
            UI_DrawDate(d);
            timeset_draw_focus(g_ts_field, UI_YELLOW);
        }
        return;
    }

    // ================== BROWSE MODE ==================
    if (g_mode == UI_MODE_BROWSE) {
        if (evt == BTN_EVT_HOLD_2S && id == BTN_SEL && g_page == UI_PAGE_MAIN) {
            // вход в timeset
            g_mode = UI_MODE_TIMESET;
            g_ts_field = APP_TS_HH;
            APP_TimeSet_Enter(d);
            UI_DrawTime(d);
            UI_DrawDate(d);
            timeset_draw_focus(g_ts_field, UI_YELLOW);
            return;
        }

        if (evt == BTN_EVT_PRESS) {
            if (id == BTN_UP) {
                UI_SetPage(page_prev(g_page));
                UI_DrawCounters(d);
            } else if (id == BTN_DN) {
                UI_SetPage(page_next(g_page));
                UI_DrawCounters(d);
            } else if (id == BTN_SEL) {
                g_mode = UI_MODE_EDIT;
                g_sel_row = 0;
                UI_SetSelRow(0);
                UI_SetSelVisible(1);
                g_reset_exit_deadline_ms = HAL_GetTick() + 10000u;
            }
        }
        return;
    }

    // ================== EDIT MODE ==================
    if (g_mode == UI_MODE_EDIT) {
        if (evt == BTN_EVT_HOLD_2S && id == BTN_SEL) {
            UI_ResetSelectedCounter(d);
            g_reset_exit_deadline_ms = HAL_GetTick() + 10000u;
            return;
        }

        if (evt == BTN_EVT_PRESS) {
            if (id == BTN_UP) {
                if (g_sel_row > 0) UI_SetSelRow(g_sel_row - 1);
            } else if (id == BTN_DN) {
                if (g_sel_row < 2) UI_SetSelRow(g_sel_row + 1);
            } else if (id == BTN_SEL) {
                UI_SetSelVisible(0);
                g_mode = UI_MODE_BROWSE;
                if (g_dirty) {
                    NV_Save(d, SPEED_GetPulseRem());
                    g_dirty = 0;
                }
            }
        }
        return;
    }
}
