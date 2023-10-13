#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/input/input.h>

#include <lvgl.h>
#include <lvgl_input_device.h>

#include <settings.h>

LOG_MODULE_REGISTER(display, LOG_LEVEL_DBG);

static const struct device *lvgl_encoder =
	DEVICE_DT_GET(DT_COMPAT_GET_ANY_STATUS_OKAY(zephyr_lvgl_encoder_input));

int display(void) {
	const struct device *display_dev;
    
	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display_dev)) {
		LOG_ERR("Device not ready, aborting test");
		return 0;
	}

    lv_theme_t * dark_theme;
    dark_theme = lv_theme_mono_init(lv_disp_get_default(), true, &lv_font_unscii_8);
    lv_disp_set_theme(lv_disp_get_default(), dark_theme);

    /* Create a roller */
    lv_obj_t * roller = lv_roller_create(lv_scr_act());
    k_sleep(K_MSEC(100));
    lv_obj_add_event_cb(roller, boot_option_cb, LV_EVENT_KEY, NULL);
    lv_roller_set_options(roller, "Linux\nWindows\nUEFI\nGRUB", LV_ROLLER_MODE_INFINITE);

    lv_obj_set_size(roller, LV_HOR_RES-8, LV_VER_RES-8);
    lv_obj_align(roller, LV_ALIGN_CENTER, 0, 0);
    lv_roller_set_visible_row_count(roller, 3);

	lv_group_t *roller_group;
	roller_group = lv_group_create();
	lv_group_add_obj(roller_group, roller);
    lv_indev_set_group(lvgl_input_get_indev(lvgl_encoder), roller_group);
    lv_group_set_editing(roller_group, true);

	lv_task_handler();
	display_blanking_off(display_dev);

	while (1) {
		lv_task_handler();
		k_sleep(K_MSEC(10));
	}
}

K_THREAD_DEFINE(display_thread, 8192, display, NULL, NULL, NULL, 5, 0, 0);
