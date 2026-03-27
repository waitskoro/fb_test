#include "lvgl/lvgl.h"

/**
 * Initialize the fbdev driver
 *
 * @return the LVGL display
 */

static lv_display_t * init_fbdev(void)
{
    lv_display_t *disp = lv_linux_fbdev_create();
    lv_linux_fbdev_set_file(disp, "/dev/fb1");

    return disp;
}

