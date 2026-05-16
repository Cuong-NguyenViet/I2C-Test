#include <stdio.h>
#include "driver/i2c.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define I2C_SDA_PIN      21
#define I2C_SCL_PIN      22
#define I2C_PORT_NUM     I2C_NUM_0

// !!! Điền địa chỉ 2 thiết bị thực tế trên mạch của bạn vào đây
#define DEV_1            0x20  
#define DEV_2            0x3C  

#define LOG_INFO(msg)    printf("[INFO]    %s\n", msg)
#define LOG_ERROR(msg)   printf("[ERROR]   %s\n", msg)
#define TEST_PASS(id)    printf("[TEST]    %s : PASS ==================\n\n", id)
#define TEST_FAIL(id)    printf("[TEST]    %s : FAIL ==================\n\n", id)

static esp_err_t access_device(uint8_t addr) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_PORT_NUM, cmd, pdMS_TO_TICKS(10));
    i2c_cmd_link_delete(cmd);
    return ret;
}

void app_main(void) {
    vTaskDelay(pdMS_TO_TICKS(1000));
    LOG_INFO("Dang chay I2C-006: Stress Test giao tiep dong thoi...");

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA_PIN,
        .sda_pullup_en = GPIO_PULLUP_DISABLE,
        .scl_io_num = I2C_SCL_PIN,
        .scl_pullup_en = GPIO_PULLUP_DISABLE,
        .master.clk_speed = 400000, 
    };
    i2c_param_config(I2C_PORT_NUM, &conf);
    i2c_driver_install(I2C_PORT_NUM, conf.mode, 0, 0, 0);

    int error_count = 0;
    LOG_INFO("Dang thuc hien truy cap luan phien toc do cao 100 chu ky...");

    for (int i = 0; i < 100; i++) {
        if (access_device(DEV_1) != ESP_OK) error_count++;
        if (access_device(DEV_2) != ESP_OK) error_count++;
        esp_rom_delay_us(100); 
    }

    i2c_driver_delete(I2C_PORT_NUM);

    if (error_count == 0) {
        LOG_INFO("Stress test hoan thanh my man. Khong co loi arbitration hay timing.");
        TEST_PASS("I2C-006");
    } else {
        char error_msg[100];
        sprintf(error_msg, "Phat hien xuat hien %d vi pham (NACK/Error) trong luc stress test!", error_count);
        LOG_ERROR(error_msg);
        TEST_FAIL("I2C-006");
    }
}