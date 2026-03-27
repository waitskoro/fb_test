
#include "lvgl/lvgl.h"
#include "display/fbdev.c"
#include "ui/mainmenu/menu.c"

#define DISPLAY_WIDTH  240
#define DISPLAY_HEIGHT 128

static void init_display()
{
    lv_display_t * display = init_fbdev();
    lv_display_set_resolution(display, DISPLAY_WIDTH, DISPLAY_HEIGHT);  
}

int main()
{
    LV_FONT_DECLARE(ubuntu_mono_12)

    lv_init();
    init_display();

    init_menu();

    while(1) {
        lv_timer_handler();
        usleep(5000);
    }

    return 0;
}
