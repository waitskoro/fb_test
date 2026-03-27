#include "menu.h"
#include "lvgl/lvgl.h"
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>

#define DISPLAY_WIDTH  238
#define DISPLAY_HEIGHT 126
#define SCREENSAVER_TIMEOUT 60000000  // 1 минута в микросекундах

// Глобальные переменные
static lv_obj_t * main_menu = NULL;
static lv_obj_t * screensaver_obj = NULL;
static lv_timer_t * screensaver_timer = NULL;
static bool is_screensaver_active = true;
static uint64_t last_activity_time = 0;

// Структуры для хранения данных телеметрии
typedef struct {
    float voltage;
    float current;
    float temperature;
    char status[32];
} itnp_telemetry_t;

typedef struct {
    uint32_t packet_id;
    uint8_t data[64];
} paip_telemetry_t;

typedef struct {
    uint32_t packet_id;
    uint8_t data[64];
} lvs_telemetry_t;

static itnp_telemetry_t itnp_data;
static paip_telemetry_t paip_data;
static lvs_telemetry_t lvs_data;

// Функция для применения черно-белой темы к объекту
static void apply_bw_theme(lv_obj_t * obj) {
    lv_obj_set_style_bg_color(obj, lv_color_white(), 0);
    lv_obj_set_style_border_color(obj, lv_color_black(), 0);
    lv_obj_set_style_border_width(obj, 1, 0);
}

// Функция для создания стандартного контейнера
static lv_obj_t * create_standard_container(lv_obj_t * parent, int width, int height) {
    lv_obj_t * cont = lv_obj_create(parent);
    lv_obj_set_size(cont, width, height);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(cont, 5, 0);
    apply_bw_theme(cont);
    return cont;
}

// Функция для создания стандартной страницы меню
static lv_obj_t * create_menu_page(lv_obj_t * menu, const char * title) {
    lv_obj_t * page = lv_menu_page_create(menu, title);
    apply_bw_theme(page);
    
    // Отключаем прокрутку страницы
    lv_obj_remove_flag(page, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(page, LV_DIR_NONE);
    
    return page;
}

// Функция для создания кнопки меню
static lv_obj_t * create_menu_button(lv_obj_t * parent, const char * text, lv_event_cb_t event_cb, lv_obj_t * user_data) {
    lv_obj_t * btn = lv_btn_create(parent);
    lv_obj_set_width(btn, lv_pct(100));
    lv_obj_set_height(btn, 35);
    apply_bw_theme(btn);
    lv_obj_set_style_pad_all(btn, 5, 0);
    
    lv_obj_t * label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_center(label);
    lv_obj_set_style_text_color(btn, lv_color_black(), 0);
    
    if (event_cb) {
        lv_obj_add_event_cb(btn, event_cb, LV_EVENT_CLICKED, user_data);
    }
    
    return btn;
}

// Функции заполнения данных телеметрии
static void fill_itnp_data(lv_obj_t * cont) {
    char buffer[64];
    
    lv_obj_t * title_label = lv_label_create(cont);
    lv_label_set_text(title_label, "=== ТЕЛЕМЕТРИЯ ИТНП ===");
    
    lv_obj_t * status_label = lv_label_create(cont);
    snprintf(buffer, sizeof(buffer), "Статус: %s", itnp_data.status);
    lv_label_set_text(status_label, buffer);
    
    lv_obj_t * voltage_label = lv_label_create(cont);
    snprintf(buffer, sizeof(buffer), "Напряжение: %.2f V", itnp_data.voltage);
    lv_label_set_text(voltage_label, buffer);
    
    lv_obj_t * current_label = lv_label_create(cont);
    snprintf(buffer, sizeof(buffer), "Ток: %.2f A", itnp_data.current);
    lv_label_set_text(current_label, buffer);
    
    lv_obj_t * temp_label = lv_label_create(cont);
    snprintf(buffer, sizeof(buffer), "Температура: %.1f C", itnp_data.temperature);
    lv_label_set_text(temp_label, buffer);
}

static void fill_paip_data(lv_obj_t * cont) {
    char buffer[64];
    
    lv_obj_t * title_label = lv_label_create(cont);
    lv_label_set_text(title_label, "=== ТЕЛЕМЕТРИЯ ПАИП ===");
    
    lv_obj_t * packet_label = lv_label_create(cont);
    snprintf(buffer, sizeof(buffer), "Пакет: 0x3001");
    lv_label_set_text(packet_label, buffer);
    
    lv_obj_t * data_label = lv_label_create(cont);
    snprintf(buffer, sizeof(buffer), "Данные: %02X %02X %02X...", 
             paip_data.data[0], paip_data.data[1], paip_data.data[2]);
    lv_label_set_text(data_label, buffer);
}

static void fill_lvs_data(lv_obj_t * cont) {
    char buffer[64];
    
    lv_obj_t * title_label = lv_label_create(cont);
    lv_label_set_text(title_label, "=== ТЕЛЕМЕТРИЯ ЛВС ===");
    
    lv_obj_t * packet_label = lv_label_create(cont);
    snprintf(buffer, sizeof(buffer), "Пакет: 0x3002");
    lv_label_set_text(packet_label, buffer);
    
    lv_obj_t * data_label = lv_label_create(cont);
    snprintf(buffer, sizeof(buffer), "Данные: %02X %02X %02X...", 
             lvs_data.data[0], lvs_data.data[1], lvs_data.data[2]);
    lv_label_set_text(data_label, buffer);
}

// Обработчики для пунктов меню
static void menu_itnp_handler(lv_event_t * e) {
    lv_obj_t * menu = lv_event_get_user_data(e);
    lv_obj_t * page = create_menu_page(menu, "ИТНП Телеметрия");
    lv_obj_t * cont = create_standard_container(page, DISPLAY_WIDTH, DISPLAY_HEIGHT - 20);
    fill_itnp_data(cont);
    lv_menu_set_page(menu, page);
}

static void menu_paip_handler(lv_event_t * e) {
    lv_obj_t * menu = lv_event_get_user_data(e);
    lv_obj_t * page = create_menu_page(menu, "ПАИП Телеметрия");
    lv_obj_t * cont = create_standard_container(page, DISPLAY_WIDTH, DISPLAY_HEIGHT - 20);
    fill_paip_data(cont);
    lv_menu_set_page(menu, page);
}

static void menu_lvs_handler(lv_event_t * e) {
    lv_obj_t * menu = lv_event_get_user_data(e);
    lv_obj_t * page = create_menu_page(menu, "ЛВС Телеметрия");
    lv_obj_t * cont = create_standard_container(page, DISPLAY_WIDTH, DISPLAY_HEIGHT - 20);
    fill_lvs_data(cont);
    lv_menu_set_page(menu, page);
}

// Функции настройки контента для страниц настроек
static void setup_time_content(lv_obj_t * cont) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_str[32];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
    
    lv_obj_t * current_label = lv_label_create(cont);
    lv_label_set_text_fmt(current_label, "Текущее время:\n%s", time_str);
    
    lv_obj_t * set_btn = lv_btn_create(cont);
    lv_obj_set_width(set_btn, 200);
    apply_bw_theme(set_btn);
    lv_obj_t * set_label = lv_label_create(set_btn);
    lv_label_set_text(set_label, "Установить время");
    lv_obj_set_style_text_color(set_label, lv_color_black(), 0);
    
    lv_obj_t * correct_btn = lv_btn_create(cont);
    lv_obj_set_width(correct_btn, 200);
    apply_bw_theme(correct_btn);
    lv_obj_t * correct_label = lv_label_create(correct_btn);
    lv_label_set_text(correct_label, "Коррекция времени");
    lv_obj_set_style_text_color(correct_label, lv_color_black(), 0);
}

static void setup_network_content(lv_obj_t * cont) {
    lv_obj_t * ip_label = lv_label_create(cont);
    lv_label_set_text(ip_label, "IP-адрес: 192.168.1.100");
    
    lv_obj_t * edit_btn = lv_btn_create(cont);
    lv_obj_set_width(edit_btn, 200);
    apply_bw_theme(edit_btn);
    lv_obj_t * edit_label = lv_label_create(edit_btn);
    lv_label_set_text(edit_label, "Изменить IP");
    lv_obj_set_style_text_color(edit_label, lv_color_black(), 0);
}

static void setup_disk_content(lv_obj_t * cont) {
    lv_obj_t * free_label = lv_label_create(cont);
    lv_label_set_text(free_label, "Свободно: 2.3 GB");
    
    lv_obj_t * clear_btn = lv_btn_create(cont);
    lv_obj_set_width(clear_btn, 200);
    apply_bw_theme(clear_btn);
    lv_obj_t * clear_label = lv_label_create(clear_btn);
    lv_label_set_text(clear_label, "Очистить диск");
    lv_obj_set_style_text_color(clear_label, lv_color_black(), 0);
}

static void setup_calibration_content(lv_obj_t * cont) {
    lv_obj_t * cable_label = lv_label_create(cont);
    lv_label_set_text(cable_label, "Задержка кабеля: 10 ns");
    
    lv_obj_t * path_label = lv_label_create(cont);
    lv_label_set_text(path_label, "Задержка тракта: 50 ns");
    
    lv_obj_t * calibrate_btn = lv_btn_create(cont);
    lv_obj_set_width(calibrate_btn, 200);
    apply_bw_theme(calibrate_btn);
    lv_obj_t * cal_label = lv_label_create(calibrate_btn);
    lv_label_set_text(cal_label, "Выполнить калибровку");
    lv_obj_set_style_text_color(cal_label, lv_color_black(), 0);
}

static void setup_sync_content(lv_obj_t * cont) {
    lv_obj_t * sync_label = lv_label_create(cont);
    lv_label_set_text(sync_label, "Режим синхронизации:");
    
    lv_obj_t * dropdown = lv_dropdown_create(cont);
    lv_dropdown_set_options(dropdown, "Авто\nСЧ1\nСЧ2");
    lv_obj_set_width(dropdown, 200);
    apply_bw_theme(dropdown);
}

static void setup_reboot_content(lv_obj_t * cont) {
    lv_obj_t * confirm_label = lv_label_create(cont);
    lv_label_set_text(confirm_label, "Перезагрузить систему?");
    
    lv_obj_t * reboot_btn = lv_btn_create(cont);
    lv_obj_set_width(reboot_btn, 200);
    apply_bw_theme(reboot_btn);
    lv_obj_t * reboot_label = lv_label_create(reboot_btn);
    lv_label_set_text(reboot_label, "ДА, ПЕРЕЗАГРУЗИТЬ");
    lv_obj_set_style_text_color(reboot_label, lv_color_black(), 0);
}

// Функция для создания страницы настроек
static void create_settings_page(lv_obj_t * menu, const char * title, void (*setup_content)(lv_obj_t *)) {
    lv_obj_t * page = create_menu_page(menu, title);
    lv_obj_t * cont = create_standard_container(page, DISPLAY_WIDTH, DISPLAY_HEIGHT - 20);
    
    if (setup_content) {
        setup_content(cont);
    }
    
    lv_menu_set_page(menu, page);
}

// Обработчики для настроек
static void settings_time_handler(lv_event_t * e) {
    lv_obj_t * menu = lv_event_get_user_data(e);
    create_settings_page(menu, "Настройки времени", setup_time_content);
}

static void settings_network_handler(lv_event_t * e) {
    lv_obj_t * menu = lv_event_get_user_data(e);
    create_settings_page(menu, "Настройки сети", setup_network_content);
}

static void settings_disk_handler(lv_event_t * e) {
    lv_obj_t * menu = lv_event_get_user_data(e);
    create_settings_page(menu, "Настройки диска", setup_disk_content);
}

static void settings_calibration_handler(lv_event_t * e) {
    lv_obj_t * menu = lv_event_get_user_data(e);
    create_settings_page(menu, "Калибровка", setup_calibration_content);
}

static void settings_sync_handler(lv_event_t * e) {
    lv_obj_t * menu = lv_event_get_user_data(e);
    create_settings_page(menu, "Синхронизация", setup_sync_content);
}

static void settings_reboot_handler(lv_event_t * e) {
    lv_obj_t * menu = lv_event_get_user_data(e);
    create_settings_page(menu, "Перезагрузка", setup_reboot_content);
}

static void menu_settings_handler(lv_event_t * e) {
    lv_obj_t * menu = lv_event_get_user_data(e);
    lv_obj_t * settings_page = create_menu_page(menu, "Настройки");
    lv_obj_set_flex_flow(settings_page, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(settings_page, 5, 0);
    
    lv_obj_t * menu_cont = create_standard_container(settings_page, DISPLAY_WIDTH - 20, DISPLAY_HEIGHT - 30);
    
    // Создаем пункты меню
    create_menu_button(menu_cont, "4.1 Время", settings_time_handler, menu);
    create_menu_button(menu_cont, "4.2 Сеть", settings_network_handler, menu);
    create_menu_button(menu_cont, "4.3 Диск", settings_disk_handler, menu);
    create_menu_button(menu_cont, "4.4 Калибровка", settings_calibration_handler, menu);
    create_menu_button(menu_cont, "4.5 Синхронизация", settings_sync_handler, menu);
    create_menu_button(menu_cont, "4.6 Перезагрузка", settings_reboot_handler, menu);
    
    lv_menu_set_page(menu, settings_page);
}

static uint64_t get_time_us(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

static void reset_screensaver_timer(void) {
    last_activity_time = get_time_us();
}

static void show_screensaver(void) {
    if (screensaver_obj == NULL) {
        screensaver_obj = lv_obj_create(lv_scr_act());
        lv_obj_set_size(screensaver_obj, DISPLAY_WIDTH, DISPLAY_HEIGHT);
        apply_bw_theme(screensaver_obj);
        
        // Заголовок
        lv_obj_t * title = lv_label_create(screensaver_obj);
        lv_label_set_text(title, "=== СИСТЕМА МОНИТОРИНГА ===");
        lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 5);
        
        // Основные параметры
        lv_obj_t * params = lv_label_create(screensaver_obj);
        lv_label_set_text(params, "ИТНП: OK\nПАИП: СВЯЗЬ\nЛВС: АКТИВНА\nВРЕМЯ: --:--:--");
        lv_obj_align(params, LV_ALIGN_CENTER, 0, 0);
        
        // Версия
        lv_obj_t * version = lv_label_create(screensaver_obj);
        lv_label_set_text(version, "v1.0 | Ожидание...");
        lv_obj_align(version, LV_ALIGN_BOTTOM_MID, 0, -5);
    }
    
    lv_obj_clear_flag(screensaver_obj, LV_OBJ_FLAG_HIDDEN);
    if (main_menu) {
        lv_obj_add_flag(main_menu, LV_OBJ_FLAG_HIDDEN);
    }
    is_screensaver_active = true;
}

static void hide_screensaver(void) {
    if (screensaver_obj) {
        lv_obj_add_flag(screensaver_obj, LV_OBJ_FLAG_HIDDEN);
    }
    if (main_menu) {
        lv_obj_clear_flag(main_menu, LV_OBJ_FLAG_HIDDEN);
    }
    is_screensaver_active = false;
}

static void screensaver_check_cb(lv_timer_t * timer) {
    (void)timer;
    if (!is_screensaver_active && (get_time_us() - last_activity_time) > SCREENSAVER_TIMEOUT) {
        show_screensaver();
    }
}

static void handle_any_button(lv_event_t * e) {
    (void)e;
    reset_screensaver_timer();
    
    if (is_screensaver_active) {
        hide_screensaver();
    }
}

static void create_menu(void) {
    main_menu = lv_menu_create(lv_scr_act());
    lv_obj_set_size(main_menu, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    apply_bw_theme(main_menu);
    
    // Главная страница
    lv_obj_t * main_page = create_menu_page(main_menu, "Главное меню");
    lv_obj_set_style_pad_all(main_page, 5, 0);
    
    // Отключаем прокрутку у страницы
    lv_obj_remove_flag(main_page, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(main_page, LV_DIR_NONE);
    
    // Создаем контейнер для главного меню
    lv_obj_t * menu_cont = create_standard_container(main_page, DISPLAY_WIDTH - 20, DISPLAY_HEIGHT - 30);
    
    // Создаем пункты главного меню
    create_menu_button(menu_cont, "1. Телеметрия ИТНП", menu_itnp_handler, main_menu);
    create_menu_button(menu_cont, "2. Телеметрия ПАИП", menu_paip_handler, main_menu);
    create_menu_button(menu_cont, "3. Телеметрия ЛВС", menu_lvs_handler, main_menu);
    create_menu_button(menu_cont, "4. Настройки", menu_settings_handler, main_menu);
    
    lv_menu_set_page(main_menu, main_page);
    lv_obj_add_flag(main_menu, LV_OBJ_FLAG_HIDDEN);
}

void init_menu(void) {
    // Инициализация тестовых данных
    strcpy(itnp_data.status, "НОРМА");
    itnp_data.voltage = 12.5;
    itnp_data.current = 1.2;
    itnp_data.temperature = 45.3;
    
    memset(&paip_data, 0, sizeof(paip_data));
    paip_data.packet_id = 0x3001;
    paip_data.data[0] = 0xAA;
    paip_data.data[1] = 0xBB;
    
    memset(&lvs_data, 0, sizeof(lvs_data));
    lvs_data.packet_id = 0x3002;
    lvs_data.data[0] = 0xCC;
    lvs_data.data[1] = 0xDD;
    
    apply_bw_theme(lv_scr_act());
    
    create_menu();
    
    show_screensaver();
    reset_screensaver_timer();
    
    screensaver_timer = lv_timer_create(screensaver_check_cb, 1000, NULL);
    
    lv_obj_add_event_cb(lv_scr_act(), handle_any_button, LV_EVENT_ALL, NULL);
}

void update_screensaver(void) {
    if (screensaver_obj && is_screensaver_active) {
        char buffer[128];
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        char time_str[16];
        strftime(time_str, sizeof(time_str), "%H:%M:%S", tm_info);
        
        snprintf(buffer, sizeof(buffer), "ИТНП: %s\nПАИП: СВЯЗЬ\nЛВС: АКТИВНА\nВРЕМЯ: %s", 
                 itnp_data.status, time_str);
        
        lv_obj_t * params = lv_obj_get_child(screensaver_obj, 1);
        if (params) {
            lv_label_set_text(params, buffer);
        }
    }
}