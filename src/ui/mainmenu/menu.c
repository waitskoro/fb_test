#include "menu.h"
#include "lvgl/lvgl.h"

#define ITEM_MENU_1 "Телеметрия ИТНП"
#define ITEM_MENU_2 "Телеметрия ПАИП"
#define ITEM_MENU_3 "Телеметрия ЛВС"
#define ITEM_MENU_4 "Настройки"

// Выберите нужный шрифт, раскомментировав одну из строк:
// #define USE_12PT_FONT
#define USE_14PT_FONT

#ifdef USE_12PT_FONT
    #include "src/fonts/ubuntu_mono_12.c"
    #define SELECTED_FONT &ubuntu_mono_12
#elif defined(USE_14PT_FONT)
    #include "src/fonts/ubuntu_mono_14.c"
    #define SELECTED_FONT &ubuntu_mono_14
#else
    // Шрифт по умолчанию, если ничего не выбрано
    #include "src/fonts/ubuntu_mono_14.c"
    #define SELECTED_FONT &ubuntu_mono_14
#endif

static void back_event_handler(lv_event_t * e)
{
    lv_obj_t * obj = lv_event_get_target(e);
    lv_obj_t * menu = lv_event_get_user_data(e);
    (void)obj;
    (void)menu;
}

static void create_new_label(lv_obj_t * main_page, const char * text)
{
    lv_obj_t * cont = lv_menu_cont_create(main_page);
    lv_obj_t * label = lv_label_create(cont);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, SELECTED_FONT, 0);
}

void lv_example_menu_1(void)
{
    lv_obj_t * menu = lv_menu_create(lv_scr_act());
    lv_obj_set_size(menu, lv_disp_get_hor_res(NULL), lv_disp_get_ver_res(NULL));
    lv_obj_add_event_cb(menu, back_event_handler, LV_EVENT_CLICKED, menu);
    lv_obj_center(menu);

    lv_obj_t * sub_page = lv_menu_page_create(menu, NULL);
    lv_obj_t * cont = lv_menu_cont_create(sub_page);
    lv_obj_t * label = lv_label_create(cont);
    lv_label_set_text(label, "Hello, I am hiding here");

    lv_obj_t * main_page = lv_menu_page_create(menu, NULL);

    create_new_label(main_page, ITEM_MENU_1);
    create_new_label(main_page, ITEM_MENU_2);
    create_new_label(main_page, ITEM_MENU_3);
    create_new_label(main_page, ITEM_MENU_4);

    lv_menu_set_page(menu, main_page);
}

void init_menu(void)
{
    lv_example_menu_1();
}