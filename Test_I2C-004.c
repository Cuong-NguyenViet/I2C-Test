#include <stdio.h>
#include "driver/i2c.h"

// --- CAU HINH ---
#define TEST_CONFIG 1  
#if TEST_CONFIG == 1
    #define I2C_SDA_PIN      21
    #define I2C_SCL_PIN      22
    // DANH SÁCH BẮT BUỘC (Sửa lại cho đúng IC cần KCS trên mạch)
    const uint8_t TARGET_ADDRS[] = {0x3C, 0x20, 0x68}; 
    #define TEST_NAME        "Duong I2CHW (IDC3, PCF8574, DS3231)"
#elif TEST_CONFIG == 2
    #define I2C_SDA_PIN      26
    #define I2C_SCL_PIN      27
    const uint8_t TARGET_ADDRS[] = {0x3C}; 
    #define TEST_NAME        "Duong I2CSW (Giac IDC4)"
#elif TEST_CONFIG == 3
    #define I2C_SDA_PIN      2
    #define I2C_SCL_PIN      4
    const uint8_t TARGET_ADDRS[] = {0x20}; 
    #define TEST_NAME        "Duong I2C rieng cho PCF8575"
#endif

#define I2C_PORT_NUM I2C_NUM_0
const int NUM_TARGETS = sizeof(TARGET_ADDRS) / sizeof(TARGET_ADDRS[0]);

// --- MACRO LOG ---
#define LOG_INFO(fmt, ...)    printf("[INFO]    " fmt "\n", ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...)   printf("[ERROR]   " fmt "\n", ##__VA_ARGS__)
#define TEST_PASS(id)         printf("[TEST]    %s : PASS ==================\n\n", id)
#define TEST_FAIL(id)         printf("[TEST]    %s : FAIL ==================\n\n", id)

void app_main(void) {
    printf("\n======================================================\n");
    printf("   BAT DAU KIEM THU I2C-004 (CHECK THIEU LINH KIEN)\n");
    printf("   %s\n", TEST_NAME);
    printf("======================================================\n\n");

    LOG_INFO("Dang chay I2C-004: Kiem tra danh sach thiet bi bat buoc...");
    
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER, .sda_io_num = I2C_SDA_PIN, .scl_io_num = I2C_SCL_PIN,
        .sda_pullup_en = GPIO_PULLUP_DISABLE, .scl_pullup_en = GPIO_PULLUP_DISABLE,
        .master.clk_speed = 100000,
    };
    i2c_param_config(I2C_PORT_NUM, &conf);
    i2c_driver_install(I2C_PORT_NUM, conf.mode, 0, 0, 0);

    bool all_found = true;
    for (int i = 0; i < NUM_TARGETS; i++) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (TARGET_ADDRS[i] << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);
        if (i2c_master_cmd_begin(I2C_PORT_NUM, cmd, pdMS_TO_TICKS(20)) != ESP_OK) {
            LOG_ERROR("THIEU THIET BI tai dia chi: 0x%02X", TARGET_ADDRS[i]);
            all_found = false;
        } else {
            LOG_INFO("Thiet bi 0x%02X: OK", TARGET_ADDRS[i]);
        }
        i2c_cmd_link_delete(cmd);
    }
    i2c_driver_delete(I2C_PORT_NUM);

    if (all_found) TEST_PASS("I2C-004");
    else TEST_FAIL("I2C-004");
}