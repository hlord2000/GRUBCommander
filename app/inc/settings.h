#ifndef SETTINGS_H__
#define SETTINGS_H__

#define BOOT_OPTION_BUF_SIZE 64
#define BOOT_OPTION_KEY "boot_option"

#include <lvgl.h>

typedef struct {
    char selected_boot_option[BOOT_OPTION_BUF_SIZE];
    size_t length;
} boot_option_t;

int load_boot_option(boot_option_t *boot_option);
int set_boot_option(boot_option_t *boot_option);

void boot_option_cb(lv_event_t *event);

#endif
