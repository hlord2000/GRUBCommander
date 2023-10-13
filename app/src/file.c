#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/logging/log.h>
#include <zephyr/fs/fs.h>

#include <ff.h>

#include <settings.h>

char os_hw_switch_options[4][32] = {
    "set os_hw_switch=0\n",
    "set os_hw_switch=1\n",
    "set os_hw_switch=2\n",
    "set os_hw_switch=3\n",
};

#define DISK_MOUNT_PT "/NAND:"
#define PATH "/NAND:/boot_opt.cfg"

void boot_option_cb(lv_event_t *event) {

    struct fs_file_t file;
    fs_file_t_init(&file);

    int index = 0;
    index = lv_roller_get_selected(event->current_target);
#if 1
    fs_open(&file, PATH, FS_O_CREATE | FS_O_WRITE);
    fs_write(&file, os_hw_switch_options[index], strlen(os_hw_switch_options[index]));
    fs_close(&file);
#endif
}
