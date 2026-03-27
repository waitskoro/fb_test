#include <stdlib.h>

#include "lvgl/lvgl.h"
#include "display/fbdev.c"
#include "ui/mainmenu/menu.c"
#include "lvgl/src/core/lv_global.h"

#define DISPLAY_WIDTH  240
#define DISPLAY_HEIGHT 128

#include "src/fonts/ubuntu_mono_14.c"
#define SELECTED_FONT &ubuntu_mono_14

static void init_theme_with_font(void)
{
    lv_theme_t * theme = lv_theme_default_init(
        NULL,                           
        lv_palette_main(LV_PALETTE_BLUE),  
        lv_palette_main(LV_PALETTE_RED),   
        LV_THEME_DEFAULT_DARK,          
        SELECTED_FONT                   
    );
    
    lv_disp_set_theme(NULL, theme);
}

static lv_display_t * init_display()
{
    lv_display_t * display = init_fbdev();
    lv_display_set_resolution(display, DISPLAY_WIDTH, DISPLAY_HEIGHT);  

    return display;
}

static void indev_deleted_cb(lv_event_t * e)
{
    if(LV_GLOBAL_DEFAULT()->deinit_in_progress) return;
    lv_obj_t * cursor_obj = lv_event_get_user_data(e);
    lv_obj_delete(cursor_obj);
}

static void set_mouse_cursor_icon(lv_indev_t * indev, lv_display_t * display)
{
    LV_IMAGE_DECLARE(mouse_cursor_icon);
    lv_obj_t * cursor_obj = lv_image_create(lv_display_get_screen_active(display));
    lv_image_set_src(cursor_obj, &mouse_cursor_icon);
    lv_indev_set_cursor(indev, cursor_obj);

    lv_indev_add_event_cb(indev, indev_deleted_cb, LV_EVENT_DELETE, cursor_obj);
}

static void discovery_cb(lv_indev_t * indev, lv_evdev_type_t type, void * user_data)
{
    (void)type; 

    lv_display_t * display = (lv_display_t *)user_data;
    lv_indev_set_display(indev, display);
    set_mouse_cursor_icon(indev, display);
}

static lv_indev_t * init_pointer_evdev(lv_display_t * display)
{
    const char * input_device = getenv("LV_LINUX_EVDEV_POINTER_DEVICE");

    if(input_device == NULL) {
        LV_LOG_USER("Using evdev automatic discovery.");
        lv_evdev_discovery_start(discovery_cb, display);
        return NULL;
    }

    lv_indev_t * indev = lv_evdev_create(LV_INDEV_TYPE_POINTER, input_device);

    if(indev == NULL) {
        return NULL;
    }

    lv_indev_set_display(indev, display);

    set_mouse_cursor_icon(indev, display);
    return indev;
}

int main()
{
    lv_init();
    lv_display_t * display = init_display();
    init_theme_with_font();
    init_pointer_evdev(display);

    init_menu();

    while(1) {
        lv_timer_handler();
        update_screensaver();
        usleep(5000);
    }

    return 0;
}