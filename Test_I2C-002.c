#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ========================================================
// CHON DUONG BUS I2C BAN MUON TEST (Mo comment 1 trong 2)
// ========================================================
// Cau hinh 1: Duong I2C ra Man hinh OLED (Giac IDC3)
#define I2C_SDA_PIN      19
#define I2C_SCL_PIN      22

// Cau hinh 2: Duong I2C ra chip PCF8575 (U2)
// #define I2C_SDA_PIN      2
// #define I2C_SCL_PIN      4
// ========================================================

#define LOG_INFO(msg)    printf("[INFO]    %s\n", msg)
#define LOG_ERROR(msg)   printf("[ERROR]   %s\n", msg)
#define TEST_PASS(id)    printf("[TEST]    %s : PASS ==================\n\n", id)
#define TEST_FAIL(id)    printf("[TEST]    %s : FAIL ==================\n\n", id)

void app_main(void) {
    vTaskDelay(pdMS_TO_TICKS(1000));
    LOG_INFO("Dang chay I2C-002: Kiem tra dien tro keo (Pull-up)...");

    gpio_config_t io_conf = {
        .pin_bit_mask = ((1ULL << I2C_SDA_PIN) | (1ULL << I2C_SCL_PIN)),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };
    gpio_config(&io_conf);
    vTaskDelay(pdMS_TO_TICKS(10)); 

    int sda_state = gpio_get_level(I2C_SDA_PIN);
    int scl_state = gpio_get_level(I2C_SCL_PIN);

    if (sda_state == 1 && scl_state == 1) {
        LOG_INFO("Dien ap SDA va SCL dat muc CAO. Tro keo OK.");
        TEST_PASS("I2C-002");
    } else {
        LOG_ERROR("Thieu tro keo ngoai hoac duong bus dang bi chap xuong GND!");
        TEST_FAIL("I2C-002");
    }
}