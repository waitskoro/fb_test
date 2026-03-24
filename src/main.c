#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lvgl/lvgl.h"
#include "lvgl/demos/lv_demos.h"

#include "src/lib/driver_backends.h"
#include "src/lib/simulator_util.h"
#include "src/lib/simulator_settings.h"

static void configure_simulator();

static char * selected_backend;

extern simulator_settings_t settings;

static void configure_simulator()
{
    selected_backend = NULL;
    driver_backends_register();

    const char * env_w = getenv("LV_SIM_WINDOW_WIDTH");
    const char * env_h = getenv("LV_SIM_WINDOW_HEIGHT");

    settings.window_width = atoi(env_w ? env_w : "240");
    settings.window_height = atoi(env_h ? env_h : "128");
}

void lv_example_get_started_1(void)
{
    /*Change the active screen's background color*/
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x003a57), LV_PART_MAIN);

    /*Create a white label, set its text and align it to the center*/
    lv_obj_t * label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "Hello world");
    lv_obj_set_style_text_color(lv_screen_active(), lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
}

int main()
{
    configure_simulator();

    lv_init();

    if(driver_backends_init_backend(selected_backend) == -1) {
        die("Failed to initialize display backend");
    }
    lv_example_get_started_1();
    driver_backends_run_loop();

    return 0;
}
