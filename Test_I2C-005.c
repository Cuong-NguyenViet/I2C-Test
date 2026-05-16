#include <stdio.h>
#include "driver/gpio.h"
#include "esp_rom_sys.h" // Thư viện cho hàm delay thế hệ mới
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define I2C_SDA_PIN 21
#define I2C_SCL_PIN 22

#define LOG_INFO(msg)    printf("[INFO]    %s\n", msg)
#define LOG_ERROR(msg)   printf("[ERROR]   %s\n", msg)
#define TEST_PASS(id)    printf("[TEST]    %s : PASS ==================\n\n", id)
#define TEST_FAIL(id)    printf("[TEST]    %s : FAIL ==================\n\n", id)

void app_main(void) {
    vTaskDelay(pdMS_TO_TICKS(1000));
    LOG_INFO("Dang chay I2C-005: Kiem tra va xu ly Bus Lockup...");

    gpio_set_direction(I2C_SDA_PIN, GPIO_MODE_INPUT);
    
    if (gpio_get_level(I2C_SDA_PIN) == 0) {
        LOG_ERROR("Phat hien SDA bi giu chat o muc THAP (Bus Lockup)!");
        LOG_INFO("Dang kich hoat Bus Recovery: Tao 9 xung nhip tren SCL...");

        gpio_set_direction(I2C_SCL_PIN, GPIO_MODE_OUTPUT);
        for (int i = 0; i < 9; i++) {
            gpio_set_level(I2C_SCL_PIN, 1); esp_rom_delay_us(5);
            gpio_set_level(I2C_SCL_PIN, 0); esp_rom_delay_us(5);
        }

        gpio_set_direction(I2C_SDA_PIN, GPIO_MODE_INPUT);
        vTaskDelay(pdMS_TO_TICKS(5));

        if (gpio_get_level(I2C_SDA_PIN) == 1) {
            LOG_INFO("Giai phong Bus thanh cong! SDA da ve lai muc CAO.");
            TEST_PASS("I2C-005");
        } else {
            LOG_ERROR("Phuc hoi bang phan mem THIEU HIEU QUA. He thong can HARD RESET!");
            TEST_FAIL("I2C-005");
        }
    } else {
        LOG_INFO("Bus I2C khoe manh, chan SDA dang o muc CAO ranh roi.");
        TEST_PASS("I2C-005");
    }
}