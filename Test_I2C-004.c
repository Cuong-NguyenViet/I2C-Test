#include <stdio.h>
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define I2C_SDA_PIN      21
#define I2C_SCL_PIN      22
#define I2C_PORT_NUM     I2C_NUM_0

// !!! Điền các địa chỉ thiết bị bắt buộc phải có trên mạch AirSense của bạn vào đây
const uint8_t REQUIRED_DEVICES[] = {0x20, 0x3C}; 
const int NUM_REQUIRED = sizeof(REQUIRED_DEVICES) / sizeof(REQUIRED_DEVICES[0]);

#define LOG_INFO(msg)    printf("[INFO]    %s\n", msg)
#define LOG_ERROR(msg)   printf("[ERROR]   %s\n", msg)
#define TEST_PASS(id)    printf("[TEST]    %s : PASS ==================\n\n", id)
#define TEST_FAIL(id)    printf("[TEST]    %s : FAIL ==================\n\n", id)

void app_main(void) {
    vTaskDelay(pdMS_TO_TICKS(1000));
    LOG_INFO("Dang chay I2C-004: Kiem tra thiet bi thieu...");

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA_PIN,
        .sda_pullup_en = GPIO_PULLUP_DISABLE,
        .scl_io_num = I2C_SCL_PIN,
        .scl_pullup_en = GPIO_PULLUP_DISABLE,
        .master.clk_speed = 100000,
    };
    i2c_param_config(I2C_PORT_NUM, &conf);
    i2c_driver_install(I2C_PORT_NUM, conf.mode, 0, 0, 0);

    bool all_found = true;

    for (int i = 0; i < NUM_REQUIRED; i++) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (REQUIRED_DEVICES[i] << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);
        
        esp_err_t ret = i2c_master_cmd_begin(I2C_PORT_NUM, cmd, pdMS_TO_TICKS(20));
        i2c_cmd_link_delete(cmd);

        if (ret != ESP_OK) {
            char msg[50];
            sprintf(msg, "THIEU THIET BI tai dia chi: 0x%02X", REQUIRED_DEVICES[i]);
            LOG_ERROR(msg);
            all_found = false;
        } else {
            char msg[50];
            sprintf(msg, "Thiet bi 0x%02X: OK", REQUIRED_DEVICES[i]);
            LOG_INFO(msg);
        }
    }

    i2c_driver_delete(I2C_PORT_NUM);

    if (all_found) TEST_PASS("I2C-004");
    else TEST_FAIL("I2C-004");
}