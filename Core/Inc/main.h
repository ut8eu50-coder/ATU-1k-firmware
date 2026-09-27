#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"
#include "config.h"

/* Global state structure */
typedef struct {
    uint32_t freq_khz;              // Current frequency in kHz
    uint8_t  band_id;               // Current band (0-10)
    uint8_t  freq_source;           // FREQ_SOURCE_CAT, COUNTER, TCI, MANUAL
    uint8_t  usb_present;           // USB +5V detected on PA9
    uint8_t  cat_timeout;           // CAT command timeout flag
    
    float    swr_current;           // Current SWR value
    float    swr_smoothed;          // Exponentially smoothed SWR
    uint16_t power_fwd_watts;       // Forward power (calculated)
    uint16_t power_rev_watts;       // Reverse power (calculated)
    uint16_t power_net_watts;       // Net power
    
    uint8_t  relay_cap_mask;        // Current capacitor relay state
    uint8_t  relay_ind_mask;        // Current inductor relay state
    uint8_t  topology;              // TOPOLOGY_IN or TOPOLOGY_OUT
    uint8_t  bypass_enabled;        // Bypass relay state
    uint8_t  pa_output_enabled;     // PA output relay state
    
    uint8_t  tuning_in_progress;    // Auto-tune active
    uint8_t  preset_found;          // Last preset search result
    
} system_state_t;

extern system_state_t g_sys_state;
extern ADC_HandleTypeDef hadc1;
extern SPI_HandleTypeDef hspi1;      // Display
extern SPI_HandleTypeDef hspi3;      // Relays
extern I2C_HandleTypeDef hi2c1;      // FRAM
extern TIM_HandleTypeDef htim2;      // Frequency counter gate
extern TIM_HandleTypeDef htim3;      // Frequency counter + display backlight
extern TIM_HandleTypeDef htim4;      // Tuning/control timing
extern UART_HandleTypeDef huart1;    // USB VCP

/* Main application functions */
void SystemClock_Config(void);
void MX_GPIO_Init(void);
void MX_ADC1_Init(void);
void MX_SPI1_Init(void);
void MX_SPI3_Init(void);
void MX_I2C1_Init(void);
void MX_TIM2_Init(void);
void MX_TIM3_Init(void);
void MX_TIM4_Init(void);
void MX_USART1_Init(void);

void Error_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
