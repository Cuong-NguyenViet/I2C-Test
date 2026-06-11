#include <stdio.h>
#include "driver/i2c.h"
#include "esp_rom_sys.h"

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
    printf("   BAT DAU KIEM THU I2C-006 (STRESS TEST DA THIET BI)\n");
    printf("   %s\n", TEST_NAME);
    printf("======================================================\n\n");

    LOG_INFO("Dang chay I2C-006: Stress Test thong minh...");
    uint8_t devices[127];
    int count = 0;

    // Mini-Scan o toc do an toan
    init_i2c(100000);
    for (uint8_t i = 1; i < 127; i++) {
        if (ping(i)) devices[count++] = i;
    }
    i2c_driver_delete(I2C_PORT_NUM);

    if (count == 0) {
        LOG_WARN("Khong co thiet bi nao tren bus. Huy stress test!");
        TEST_FAIL("I2C-006");
        return;
    }

    // Chay Stress Test
    init_i2c(400000);
    int errors = 0;
    LOG_INFO("Dang ep tai luan phien %d thiet bi x 100 chu ky (400kHz)...", count);

    for (int i = 0; i < 100; i++) {
        for (int j = 0; j < count; j++) {
            if (!ping(devices[j])) errors++;
            esp_rom_delay_us(50); // Delay cuc ngan ep tai bus
        }
    }
    i2c_driver_delete(I2C_PORT_NUM);

    if (errors == 0) {
        LOG_INFO("Hoan thanh stress test 100 chu ky. Khong he rot goi tin.");
        TEST_PASS("I2C-006");
    } else {
        LOG_ERROR("Xuat hien %d loi NACK/Timeout trong luc stress test!", errors);
        TEST_FAIL("I2C-006");
    }
}