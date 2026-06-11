#include <stdio.h>
#include "driver/i2c.h"

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
#define I2C_PORT_NUM I2C_NUM_0

// --- MACRO LOG ---
#define LOG_INFO(fmt, ...)    printf("[INFO]    " fmt "\n", ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)    printf("[WARN]    " fmt "\n", ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...)   printf("[ERROR]   " fmt "\n", ##__VA_ARGS__)
#define TEST_PASS(id)         printf("[TEST]    %s : PASS ==================\n\n", id)
#define TEST_FAIL(id)         printf("[TEST]    %s : FAIL ==================\n\n", id)

static esp_err_t init_i2c(uint32_t speed) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER, .sda_io_num = I2C_SDA_PIN, .scl_io_num = I2C_SCL_PIN,
        .sda_pullup_en = GPIO_PULLUP_DISABLE, .scl_pullup_en = GPIO_PULLUP_DISABLE,
        .master.clk_speed = speed,
    };
    i2c_param_config(I2C_PORT_NUM, &conf);
    return i2c_driver_install(I2C_PORT_NUM, conf.mode, 0, 0, 0);
}

static bool ping(uint8_t addr) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_PORT_NUM, cmd, pdMS_TO_TICKS(10));
    i2c_cmd_link_delete(cmd);
    return (ret == ESP_OK);
}

void app_main(void) {
    printf("\n======================================================\n");
    printf("   BAT DAU KIEM THU I2C-003 (TEST TOC DO CAO)\n");
    printf("   %s\n", TEST_NAME);
    printf("======================================================\n\n");

    LOG_INFO("Dang chay I2C-003: Kiem tra toc do bus tren da thiet bi...");
    uint8_t devices[127];
    int count = 0;

    // Mini-Scan o 100kHz
    init_i2c(100000);
    for (uint8_t i = 1; i < 127; i++) {
        if (ping(i)) devices[count++] = i;
    }
    
    if (count == 0) {
        LOG_WARN("Bo qua I2C-003: Khong co thiet bi nao de test!");
        TEST_FAIL("I2C-003");
        i2c_driver_delete(I2C_PORT_NUM);
        return;
    }

    // Chay test 400kHz
    i2c_driver_delete(I2C_PORT_NUM);
    init_i2c(400000);
    bool pass_400k = true;
    for (int i = 0; i < count; i++) {
        if (!ping(devices[i])) pass_400k = false;
    }
    i2c_driver_delete(I2C_PORT_NUM);

    if (pass_400k) {
        LOG_INFO("Tat ca %d thiet bi hoat dong on dinh o 100kHz va 400kHz.", count);
        TEST_PASS("I2C-003");
    } else {
        LOG_ERROR("Giao tiep that bai khi chay o toc do 400kHz!");
        TEST_FAIL("I2C-003");
    }
}