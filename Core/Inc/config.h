#ifndef __CONFIG_H
#define __CONFIG_H

#include <stdint.h>

/**
 * @file config.h
 * @brief ATU-1k Configuration and Constants
 */

/* ========== Frequency Bands Definition ========== */

typedef struct {
    uint32_t freq_min_khz;
    uint32_t freq_max_khz;
    uint16_t step_khz;
    uint8_t  band_id;  // 0-10
    char     name[8];  // "160m", "80m", etc.
} band_config_t;

/* Band IDs */
#define BAND_160M    0
#define BAND_80M     1
#define BAND_60M     2
#define BAND_40M     3
#define BAND_30M     4
#define BAND_20M     5
#define BAND_17M     6
#define BAND_15M     7
#define BAND_12M     8
#define BAND_10M     9
#define BAND_6M      10
#define NUM_BANDS    11

static const band_config_t bands[NUM_BANDS] = {
    {1800, 2000, 25, BAND_160M, "160m"},
    {3500, 3800, 25, BAND_80M,  "80m"},
    {5330, 5405, 25, BAND_60M,  "60m"},
    {7000, 7300, 25, BAND_40M,  "40m"},
    {10100, 10150, 25, BAND_30M, "30m"},
    {14000, 14350, 25, BAND_20M, "20m"},
    {18068, 18168, 25, BAND_17M, "17m"},
    {21000, 21450, 25, BAND_15M, "15m"},
    {24890, 24990, 25, BAND_12M, "12m"},
    {28000, 29700, 25, BAND_10M, "10m"},
    {50000, 52000, 100, BAND_6M, "6m"}
};

/* ========== ADC Configuration ========== */

#define ADC_VREFINT_CAL_ADDR    ((uint16_t*)((uint32_t)0x1FFF7A2A))
#define ADC_MAX_CODE            4095.0f
#define ADC_VDDA_NOMINAL        3.3f
#define MIN_ADC_FWD             35      // ~28 mV noise floor

/* ========== SWR Measurement ========== */

#define SAMPLES_PER_CHANNEL     64
#define ADC_BUF_SIZE            (3 * SAMPLES_PER_CHANNEL)  // FWD, REV, VREFINT
#define SWR_ALPHA               0.15f   // Exponential smoothing
#define SWR_MEASUREMENT_PERIOD  100     // ms (10 Hz update)

/* ========== Tuning Algorithm ========== */

#define TEST_CAPACITOR_MASK     0x02    // 10 pF (bit 1)
#define TUNER_TIMEOUT_DEFAULT   30000   // ms
#define SWR_THRESHOLD_DEFAULT   1.5f    // Default threshold
#define RELAY_DELAY_MAX         100     // ms between relay changes
#define RELAY_DELAY_DEFAULT     100     // ms between relay changes

/* All clipped 0..255 LC combinations are valid for fine search */
#define FORBID_MAX_CAP_MAX_IND  0
#define MAX_CAPACITOR_MASK      0xFF    // 640 pF
#define MAX_INDUCTOR_MASK       0xFF    // 6.4 µH

/* Fine search parameters */
#define FINE_SEARCH_DELTA_CAP   3       // ±3 bits around coarse optimum
#define FINE_SEARCH_DELTA_IND   3       // ±3 bits around coarse optimum

/* ========== Topology Selection ========== */

#define TOPOLOGY_IN             0       // C_in/C_out = 0
#define TOPOLOGY_OUT            1       // C_in/C_out = 1

/* ========== GPIO Pins ========== */

/* ADC Inputs */
#define GPIO_FWD_PORT           GPIOA
#define GPIO_FWD_PIN            GPIO_PIN_0  // PA0 - ADC1_IN0
#define GPIO_REV_PORT           GPIOA
#define GPIO_REV_PIN            GPIO_PIN_1  // PA1 - ADC1_IN1

/* Frequency Counter */
#define GPIO_FREQ_COUNTER_PORT  GPIOD
#define GPIO_FREQ_COUNTER_PIN   GPIO_PIN_2  // PD2 - TIM3_ETR / TIM4_ETR
#define GPIO_USB_DETECT_PORT    GPIOA
#define GPIO_USB_DETECT_PIN     GPIO_PIN_9  // PA9 - USB +5V detect

/* Relay Control (SPI3) */
#define GPIO_SPI3_SCK_PORT      GPIOC
#define GPIO_SPI3_SCK_PIN       GPIO_PIN_10 // PC10
#define GPIO_SPI3_MOSI_PORT     GPIOC
#define GPIO_SPI3_MOSI_PIN      GPIO_PIN_12 // PC12
#define GPIO_SPI3_MISO_PORT     GPIOB
#define GPIO_SPI3_MISO_PIN      GPIO_PIN_5  // PB5 (RCLK/LATCH)

/* Direct Relay Control via GPIO/TD62783 */
#define GPIO_C_IN_OUT_PORT      GPIOB
#define GPIO_C_IN_OUT_PIN       GPIO_PIN_15 // PB15
#define GPIO_BYPASS_PORT        GPIOB
#define GPIO_BYPASS_PIN         GPIO_PIN_10 // PB10
#define GPIO_OUT_PA_PORT        GPIOB
#define GPIO_OUT_PA_PIN         GPIO_PIN_13 // PB13

/* Display Control (SPI1) */
#define GPIO_DISP_CS_PORT       GPIOB
#define GPIO_DISP_CS_PIN        GPIO_PIN_0  // PB0
#define GPIO_DISP_DC_PORT       GPIOC
#define GPIO_DISP_DC_PIN        GPIO_PIN_4  // PC4 (D/C)
#define GPIO_DISP_RST_PORT      GPIOC
#define GPIO_DISP_RST_PIN       GPIO_PIN_5  // PC5
#define GPIO_DISP_BL_PORT       GPIOB
#define GPIO_DISP_BL_PIN        GPIO_PIN_1  // PB1 (PWM TIM3)

/* Buttons (all active LOW to GND) */
#define GPIO_BTN_MENU_PORT      GPIOC
#define GPIO_BTN_MENU_PIN       GPIO_PIN_0  // PC0
#define GPIO_BTN_BYPASS_PORT    GPIOC
#define GPIO_BTN_BYPASS_PIN     GPIO_PIN_1  // PC1
#define GPIO_BTN_TUNE_PORT      GPIOC
#define GPIO_BTN_TUNE_PIN       GPIO_PIN_2  // PC2
#define GPIO_BTN_C_MINUS_PORT   GPIOC
#define GPIO_BTN_C_MINUS_PIN    GPIO_PIN_3  // PC3
#define GPIO_BTN_L_MINUS_PORT   GPIOC
#define GPIO_BTN_L_MINUS_PIN    GPIO_PIN_6  // PC6
#define GPIO_BTN_CIN_OUT_PORT   GPIOC
#define GPIO_BTN_CIN_OUT_PIN    GPIO_PIN_7  // PC7
#define GPIO_BTN_C_PLUS_PORT    GPIOC
#define GPIO_BTN_C_PLUS_PIN     GPIO_PIN_8  // PC8
#define GPIO_BTN_L_PLUS_PORT    GPIOC
#define GPIO_BTN_L_PLUS_PIN     GPIO_PIN_9  // PC9

/* ========== CAT Modes ========== */

#define CAT_MODE_YAESU          0
#define CAT_MODE_KENWOOD        1
#define CAT_MODE_ICOM_CIV       2
#define CAT_MODE_TCI_TCP        3

/* ========== Display ========== */

#define DISPLAY_WIDTH           240
#define DISPLAY_HEIGHT          320
#define DISPLAY_BL_PWM_MAX      100     // 0-100%

/* ========== Frequency Source ========== */

#define FREQ_SOURCE_CAT         0
#define FREQ_SOURCE_COUNTER     1
#define FREQ_SOURCE_TCI         2
#define FREQ_SOURCE_MANUAL      3

/* ========== FRAM Memory Layout ========== */

#define FRAM_TOTAL_SIZE         0x2000  // 8 KiB FM24CL64
#define FRAM_SERVICE_BASE       0x0000  // Header + global settings
#define FRAM_PRESETS_BASE       0x0100  // Bank 1 presets
#define FRAM_PRESET_SIZE        4       // cap, ind, flags, valid marker
#define FRAM_PRESET_SLOT_COUNT  176     // 11 amateur bands
#define FRAM_PRESET_BANK_SIZE   (FRAM_PRESET_SLOT_COUNT * FRAM_PRESET_SIZE)
#define FRAM_PRESET_BANK1_BASE  0x0100
#define FRAM_PRESET_BANK2_BASE  (FRAM_PRESET_BANK1_BASE + FRAM_PRESET_BANK_SIZE)
#define FRAM_FUTURE_BASE        (FRAM_PRESET_BANK2_BASE + FRAM_PRESET_BANK_SIZE)
#define FRAM_VALID_MARKER       0xA5

/* FRAM Service Area */
#define FRAM_HEADER_MAGIC       0x41545531u
#define FRAM_HEADER_VERSION     0x0001u
#define FRAM_SETTINGS_OFFSET    0x0008u

/* ========== Preset Structure ========== */

typedef struct __attribute__((packed)) {
    uint8_t cap_mask;           // 8-bit capacitor mask
    uint8_t ind_mask;           // 8-bit inductor mask
    uint8_t flags;              // bit0: C_in/C_out, bit1: bypass
} preset_t;

/* ========== Display Colors ========== */

#define COLOR_BLACK             0x0000
#define COLOR_WHITE             0xFFFF
#define COLOR_RED               0xF800
#define COLOR_GREEN             0x07E0
#define COLOR_BLUE              0x001F
#define COLOR_YELLOW            0xFFE0
#define COLOR_CYAN              0x07FF
#define COLOR_MAGENTA           0xF81F
#define COLOR_ORANGE            0xFD20
#define COLOR_GRAY              0x8410

/* Preset found (green), preset not found (orange) */
#define COLOR_PRESET_FOUND      COLOR_GREEN
#define COLOR_PRESET_NOT_FOUND  COLOR_ORANGE

/* ========== Firmware Identity ========== */

#define ATU_FIRMWARE_VERSION    "1.0.0"
#define ATU_FIRMWARE_AUTHOR     "UT8EU"
#define ATU_FIRMWARE_BUILD_DATE __DATE__

#endif /* __CONFIG_H */
