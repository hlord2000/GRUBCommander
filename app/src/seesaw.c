#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/input/input.h>
#include <lvgl.h>

LOG_MODULE_REGISTER(seesaw, LOG_LEVEL_DBG);

enum {
  SEESAW_STATUS_BASE = 0x00,
  SEESAW_GPIO_BASE = 0x01,
  SEESAW_SERCOM0_BASE = 0x02,

  SEESAW_TIMER_BASE = 0x08,
  SEESAW_ADC_BASE = 0x09,
  SEESAW_DAC_BASE = 0x0A,
  SEESAW_INTERRUPT_BASE = 0x0B,
  SEESAW_DAP_BASE = 0x0C,
  SEESAW_EEPROM_BASE = 0x0D,
  SEESAW_NEOPIXEL_BASE = 0x0E,
  SEESAW_TOUCH_BASE = 0x0F,
  SEESAW_KEYPAD_BASE = 0x10,
  SEESAW_ENCODER_BASE = 0x11,
  SEESAW_SPECTRUM_BASE = 0x12,
};

/** GPIO module function address registers
 */
enum {
  SEESAW_GPIO_DIRSET_BULK = 0x02,
  SEESAW_GPIO_DIRCLR_BULK = 0x03,
  SEESAW_GPIO_BULK = 0x04,
  SEESAW_GPIO_BULK_SET = 0x05,
  SEESAW_GPIO_BULK_CLR = 0x06,
  SEESAW_GPIO_BULK_TOGGLE = 0x07,
  SEESAW_GPIO_INTENSET = 0x08,
  SEESAW_GPIO_INTENCLR = 0x09,
  SEESAW_GPIO_INTFLAG = 0x0A,
  SEESAW_GPIO_PULLENSET = 0x0B,
  SEESAW_GPIO_PULLENCLR = 0x0C,
};

/** status module function address registers
 */
enum {
  SEESAW_STATUS_HW_ID = 0x01,
  SEESAW_STATUS_VERSION = 0x02,
  SEESAW_STATUS_OPTIONS = 0x03,
  SEESAW_STATUS_TEMP = 0x04,
  SEESAW_STATUS_SWRST = 0x7F,
};

/** timer module function address registers
 */
enum {
  SEESAW_TIMER_STATUS = 0x00,
  SEESAW_TIMER_PWM = 0x01,
  SEESAW_TIMER_FREQ = 0x02,
};

/** ADC module function address registers
 */
enum {
  SEESAW_ADC_STATUS = 0x00,
  SEESAW_ADC_INTEN = 0x02,
  SEESAW_ADC_INTENCLR = 0x03,
  SEESAW_ADC_WINMODE = 0x04,
  SEESAW_ADC_WINTHRESH = 0x05,
  SEESAW_ADC_CHANNEL_OFFSET = 0x07,
};

/** Sercom module function address registers
 */
enum {
  SEESAW_SERCOM_STATUS = 0x00,
  SEESAW_SERCOM_INTEN = 0x02,
  SEESAW_SERCOM_INTENCLR = 0x03,
  SEESAW_SERCOM_BAUD = 0x04,
  SEESAW_SERCOM_DATA = 0x05,
};

/** neopixel module function address registers
 */
enum {
  SEESAW_NEOPIXEL_STATUS = 0x00,
  SEESAW_NEOPIXEL_PIN = 0x01,
  SEESAW_NEOPIXEL_SPEED = 0x02,
  SEESAW_NEOPIXEL_BUF_LENGTH = 0x03,
  SEESAW_NEOPIXEL_BUF = 0x04,
  SEESAW_NEOPIXEL_SHOW = 0x05,
};

/** touch module function address registers
 */
enum {
  SEESAW_TOUCH_CHANNEL_OFFSET = 0x10,
};

/** keypad module function address registers
 */
enum {
  SEESAW_KEYPAD_STATUS = 0x00,
  SEESAW_KEYPAD_EVENT = 0x01,
  SEESAW_KEYPAD_INTENSET = 0x02,
  SEESAW_KEYPAD_INTENCLR = 0x03,
  SEESAW_KEYPAD_COUNT = 0x04,
  SEESAW_KEYPAD_FIFO = 0x10,
};

/** keypad module edge definitions
 */
enum {
  SEESAW_KEYPAD_EDGE_HIGH = 0,
  SEESAW_KEYPAD_EDGE_LOW,
  SEESAW_KEYPAD_EDGE_FALLING,
  SEESAW_KEYPAD_EDGE_RISING,
};

/** encoder module edge definitions
 */
enum {
  SEESAW_ENCODER_STATUS = 0x00,
  SEESAW_ENCODER_INTENSET = 0x10,
  SEESAW_ENCODER_INTENCLR = 0x20,
  SEESAW_ENCODER_POSITION = 0x30,
  SEESAW_ENCODER_DELTA = 0x40,
};

/** Audio spectrum module function address registers
 */
enum {
  SEESAW_SPECTRUM_RESULTS_LOWER = 0x00, // Audio spectrum bins 0-31
  SEESAW_SPECTRUM_RESULTS_UPPER = 0x01, // Audio spectrum bins 32-63
  // If some future device supports a larger spectrum, can add additional
  // "bins" working upward from here. Configurable setting registers then
  // work downward from the top to avoid collision between spectrum bins
  // and configurables.
  SEESAW_SPECTRUM_CHANNEL = 0xFD,
  SEESAW_SPECTRUM_RATE = 0xFE,
  SEESAW_SPECTRUM_STATUS = 0xFF,
};

#define SEESAW_HW_ID_CODE_SAMD09 0x55 ///< seesaw HW ID code for SAMD09
#define SEESAW_HW_ID_CODE_TINY806 0x84 ///< seesaw HW ID code for ATtiny806
#define SEESAW_HW_ID_CODE_TINY807 0x85 ///< seesaw HW ID code for ATtiny807
#define SEESAW_HW_ID_CODE_TINY816 0x86 ///< seesaw HW ID code for ATtiny816
#define SEESAW_HW_ID_CODE_TINY817 0x87 ///< seesaw HW ID code for ATtiny817
#define SEESAW_HW_ID_CODE_TINY1616 0x88 ///< seesaw HW ID code for ATtiny1616
#define SEESAW_HW_ID_CODE_TINY1617 0x89 ///< seesaw HW ID code for ATtiny1617

#define TX_BUF_SIZE 64
int seesaw_write_read_dt(const struct i2c_dt_spec *dev, uint8_t high_addr, uint8_t low_addr, uint8_t *tx_buf, size_t len, uint8_t *rx_buf, size_t rx_len) {
    int err;
    if (len > TX_BUF_SIZE - 2) {
        LOG_ERR("I2C write too long: %d", len);
        return -EINVAL;
    }

    uint8_t write_buf[TX_BUF_SIZE] = {high_addr, low_addr};

    err = i2c_write_read_dt(dev, write_buf, len + 2, rx_buf, rx_len);

    return err;
}

int seesaw_reset(const struct i2c_dt_spec *dev) {
    int err;
    uint8_t reset_buf = 0xFF; 

    err = seesaw_write_read_dt(dev, SEESAW_STATUS_BASE, SEESAW_STATUS_SWRST, &reset_buf, 1, NULL, 0);

    return err;
}

int seesaw_init(const struct i2c_dt_spec *dev) {
    int err;
    err = seesaw_reset(dev);
    if (err) {
        LOG_ERR("Seesaw reset failed: %d", err);
        return err;
    }
    k_msleep(10);

    uint8_t hardware_id;
    err = seesaw_write_read_dt(dev, SEESAW_STATUS_BASE, SEESAW_STATUS_HW_ID, NULL, 0, &hardware_id, 1);

    if (hardware_id != SEESAW_HW_ID_CODE_SAMD09) {
        LOG_ERR("Seesaw HW ID mismatch: 0x%x", hardware_id);
        return -EINVAL;
    }

    return err;
}

int seesaw_reset_encoder_position(const struct i2c_dt_spec *dev) {
    int err;
    uint8_t position[4] = {0}; 

    err = seesaw_write_read_dt(dev, SEESAW_ENCODER_BASE, SEESAW_ENCODER_POSITION, position, 4, NULL, 0);

    return err;
}

int seesaw_get_encoder_delta(const struct i2c_dt_spec *dev, int32_t *delta) {
    int err;
    uint8_t delta_buf[4] = {0};
    err = seesaw_write_read_dt(dev, SEESAW_ENCODER_BASE, SEESAW_ENCODER_DELTA, NULL, 0, delta_buf, 4);
    *delta = delta_buf[0] << 24 | delta_buf[1] << 16 | delta_buf[2] << 8 | delta_buf[3];
    return err;
}

int seesaw_get_button_pressed(const struct i2c_dt_spec *dev, uint8_t *pressed) {
    int err;
    uint8_t pressed_buf = 0;
    err = seesaw_write_read_dt(dev, SEESAW_KEYPAD_BASE, SEESAW_KEYPAD_EVENT, NULL, 0, &pressed_buf, 1);
    *pressed = pressed_buf;
    return err;
}

const struct i2c_dt_spec seesaw_dev = I2C_DT_SPEC_GET(DT_NODELABEL(seesaw)); 

static const struct device *lvgl_encoder =
	DEVICE_DT_GET(DT_COMPAT_GET_ANY_STATUS_OKAY(zephyr_lvgl_encoder_input));

int seesaw(void) {
    k_msleep(1000);
    int err;
    /*
    err = seesaw_init(&seesaw_dev);
    if (err) {
        LOG_ERR("Seesaw init failed: %d", err);
    }*/

    LOG_INF("Seesaw init done");

    int32_t encoder_delta = 0;
    while (true) {
        err = seesaw_get_encoder_delta(&seesaw_dev, &encoder_delta);
        if (err) {
            LOG_ERR("Seesaw get encoder delta failed: %d", err);
        }
        if (encoder_delta != 0) {
            err = input_report_rel(lvgl_encoder, INPUT_REL_WHEEL, encoder_delta, true, K_FOREVER);
        }
        k_msleep(50);
    }
}


K_THREAD_DEFINE(seesaw_id, 1024, seesaw, NULL, NULL, NULL, 7, 0, 0);
