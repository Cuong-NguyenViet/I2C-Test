#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// --- CAU HINH ---
#define TEST_CONFIG 1  
#if TEST_CONFIG == 1
    #define I2C_SDA_PIN      21
    #define I2C_SCL_PIN      22
    #define TEST_NAME        "Duong I2CHW (IDC3, PCF8574, DS3231)"
#elif TEST_CONFIG == 2
    #define I2C_SDA_PIN      26
    #define I2C_SCL_PIN      27
    #define TEST_NAME        "Duong I2CSW (Giac IDC4)"
#elif TEST_CONFIG == 3
    #define I2C_SDA_PIN      2
    #define I2C_SCL_PIN      4
    #define TEST_NAME        "Duong I2C rieng cho PCF8575"
#endif

// --- MACRO LOG ---
#define LOG_INFO(fmt, ...)    printf("[INFO]    " fmt "\n", ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...)   printf("[ERROR]   " fmt "\n", ##__VA_ARGS__)
#define TEST_PASS(id)         printf("[TEST]    %s : PASS ==================\n\n", id)
#define TEST_FAIL(id)         printf("[TEST]    %s : FAIL ==================\n\n", id)

void app_main(void) {
    printf("\n======================================================\n");
    printf("   BAT DAU KIEM THU I2C-002 (DO DIEN TRO KEO)\n");
    printf("   %s\n", TEST_NAME);
    printf("======================================================\n\n");

    LOG_INFO("Dang chay I2C-002: Kiem tra dien tro keo (Pull-up)...");
    
    gpio_config_t io_conf = {
        .pin_bit_mask = ((1ULL << I2C_SDA_PIN) | (1ULL << I2C_SCL_PIN)),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE, 
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };
    gpio_config(&io_conf);
    vTaskDelay(pdMS_TO_TICKS(10));

    int sda = gpio_get_level(I2C_SDA_PIN);
    int scl = gpio_get_level(I2C_SCL_PIN);

    if (sda == 1 && scl == 1) {
        LOG_INFO("Dien ap SDA va SCL dat muc CAO (3.3V). Tro keo OK.");
        TEST_PASS("I2C-002");
    } else {
        LOG_ERROR("Thieu tro keo hoac chap GND! SDA=%d, SCL=%d", sda, scl);
        TEST_FAIL("I2C-002");
    }
}