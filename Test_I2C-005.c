#include <stdio.h>
#include "driver/gpio.h"
#include "esp_rom_sys.h"
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
    printf("   BAT DAU KIEM THU I2C-005 (XU LY KET BUS)\n");
    printf("   %s\n", TEST_NAME);
    printf("======================================================\n\n");

    LOG_INFO("Dang chay I2C-005: Kiem tra va xu ly Bus Lockup...");
    
    gpio_set_direction(I2C_SDA_PIN, GPIO_MODE_INPUT);
    
    if (gpio_get_level(I2C_SDA_PIN) == 0) {
        LOG_ERROR("SDA bi ket muc THAP! Tien hanh phuc hoi (9 Clocks)...");
        gpio_set_direction(I2C_SCL_PIN, GPIO_MODE_OUTPUT);
        
        for (int i = 0; i < 9; i++) {
            gpio_set_level(I2C_SCL_PIN, 1); esp_rom_delay_us(5);
            gpio_set_level(I2C_SCL_PIN, 0); esp_rom_delay_us(5);
        }

        gpio_set_direction(I2C_SDA_PIN, GPIO_MODE_INPUT);
        vTaskDelay(pdMS_TO_TICKS(5));

        if (gpio_get_level(I2C_SDA_PIN) == 1) {
            LOG_INFO("Giai phong Bus thanh cong!");
            TEST_PASS("I2C-005");
        } else {
            LOG_ERROR("Phat hien chap cung tren board (Loi vat ly)!");
            TEST_FAIL("I2C-005");
        }
    } else {
        LOG_INFO("Bus khoe manh, SDA dang tha noi muc CAO ranh roi.");
        TEST_PASS("I2C-005");
    }
}