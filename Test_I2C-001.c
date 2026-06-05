#include <stdio.h>
#include "driver/i2c.h"
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

#define I2C_PORT_NUM     I2C_NUM_0

#define LOG_INFO(msg)    printf("[INFO]    %s\n", msg)
#define TEST_PASS(id)    printf("[TEST]    %s : PASS ==================\n\n", id)
#define TEST_FAIL(id)    printf("[TEST]    %s : FAIL ==================\n\n", id)

void app_main(void) {
    vTaskDelay(pdMS_TO_TICKS(1000));
    LOG_INFO("Dang chay I2C-001: Quet bus I2C...");

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

    int found_count = 0;

    for (uint8_t address = 1; address < 127; address++) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);
        
        esp_err_t ret = i2c_master_cmd_begin(I2C_PORT_NUM, cmd, pdMS_TO_TICKS(20));
        i2c_cmd_link_delete(cmd);

        if (ret == ESP_OK) {
            char msg[50];
            sprintf(msg, "Tim thay thiet bi tai dia chi 0x%02X", address);
            LOG_INFO(msg);
            found_count++;
        }
    }

    i2c_driver_delete(I2C_PORT_NUM);

    if (found_count > 0) TEST_PASS("I2C-001");
    else TEST_FAIL("I2C-001");
}