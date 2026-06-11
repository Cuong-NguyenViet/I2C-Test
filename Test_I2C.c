#include <stdio.h>
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ========================================================
// 1. CẤU HÌNH BÀI TEST TỰ ĐỘNG CHO MẠCH AIRSENSE
// ========================================================
// Thay đổi giá trị TEST_CONFIG:
// 1 -> Test đường I2CHW (OLED IDC3, PCF8574, DS3231)
// 2 -> Test đường I2CSW (Giắc cắm IDC4)
// 3 -> Test đường PCF8575 riêng biệt
#define TEST_CONFIG 1  

#if TEST_CONFIG == 1
    #define I2C_SDA_PIN      21
    #define I2C_SCL_PIN      22
    // Cụm này có nhiều linh kiện: OLED (0x3C), PCF8574 (0x20), DS3231 (0x68)
    const uint8_t TARGET_ADDRS[] = {0x57, 0x68}; 
    #define TEST_NAME        "Duong I2CHW (IDC3, PCF8574, DS3231)"
#elif TEST_CONFIG == 2
    #define I2C_SDA_PIN      26
    #define I2C_SCL_PIN      27
    // Điền địa chỉ cảm biến cắm ngoài vào giắc IDC4 tại đây
    const uint8_t TARGET_ADDRS[] = {0x3C}; 
    #define TEST_NAME        "Duong I2CSW"
#elif TEST_CONFIG == 3
    #define I2C_SDA_PIN      2
    #define I2C_SCL_PIN      4
    // Chỉ có IC PCF8575
    const uint8_t TARGET_ADDRS[] = {0x20}; 
    #define TEST_NAME        "Duong I2C rieng cho PCF8575"
#else
    #error "Vui long chon TEST_CONFIG la 1, 2 hoac 3"
#endif

#define I2C_PORT_NUM         I2C_NUM_0
const int NUM_TARGETS = sizeof(TARGET_ADDRS) / sizeof(TARGET_ADDRS[0]);

// ========================================================
// 2. MACRO ĐỊNH DẠNG LOG VÀ KHAI BÁO BIẾN TOÀN CỤC
// ========================================================
#define LOG_INFO(msg)    printf("[INFO]    %s\n", msg)
#define LOG_WARN(msg)    printf("[WARN]    %s\n", msg)
#define LOG_ERROR(msg)   printf("[ERROR]   %s\n", msg)
#define TEST_PASS(id)    printf("[TEST]    %s : PASS ==================\n\n", id)
#define TEST_FAIL(id)    printf("[TEST]    %s : FAIL ==================\n\n", id)

int found_count = 0; // Lưu số lượng thiết bị tìm thấy trên bus

// ========================================================
// 3. CÁC HÀM BỔ TRỢ (HELPER FUNCTIONS)
// ========================================================
static esp_err_t init_i2c_master(uint32_t speed_hz) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA_PIN,
        .sda_pullup_en = GPIO_PULLUP_DISABLE,
        .scl_io_num = I2C_SCL_PIN,
        .scl_pullup_en = GPIO_PULLUP_DISABLE,
        .master.clk_speed = speed_hz,
    };
    i2c_param_config(I2C_PORT_NUM, &conf);
    return i2c_driver_install(I2C_PORT_NUM, conf.mode, 0, 0, 0);
}

static bool ping_device(uint8_t addr) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_PORT_NUM, cmd, pdMS_TO_TICKS(20));
    i2c_cmd_link_delete(cmd);
    return (ret == ESP_OK);
}

// ========================================================
// 4. CHI TIẾT CÁC BÀI TEST 001 - 006
// ========================================================
void test_I2C_001_scan_bus(void) {
    LOG_INFO("Dang chay I2C-001: Quet bus I2C...");
    init_i2c_master(100000);
    found_count = 0;

    for (uint8_t address = 1; address < 127; address++) {
        if (ping_device(address)) {
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

void test_I2C_002_pullup(void) {
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
        LOG_ERROR("Thieu tro keo hoac duong bus bi chap xuong GND (Nho kiem tra tu dien)!");
        TEST_FAIL("I2C-002");
    }
}

void test_I2C_003_speed(void) {
    LOG_INFO("Dang chay I2C-003: Kiem tra toc do bus tren da thiet bi...");
    if (found_count == 0) {
        LOG_WARN("Bo qua I2C-003: Chua tim thay thiet bi de test!");
        TEST_FAIL("I2C-003");
        return;
    }

    bool all_pass_100k = true;
    bool all_pass_400k = true;

    // Test 100kHz
    init_i2c_master(100000);
    for (int i = 0; i < NUM_TARGETS; i++) {
        if (!ping_device(TARGET_ADDRS[i])) all_pass_100k = false;
    }
    i2c_driver_delete(I2C_PORT_NUM);

    // Test 400kHz
    init_i2c_master(400000);
    for (int i = 0; i < NUM_TARGETS; i++) {
        if (!ping_device(TARGET_ADDRS[i])) all_pass_400k = false;
    }
    i2c_driver_delete(I2C_PORT_NUM);

    if (all_pass_100k && all_pass_400k) {
        LOG_INFO("Tat ca thiet bi phan hoi on dinh o 100kHz va 400kHz.");
        TEST_PASS("I2C-003");
    } else {
        LOG_ERROR("Co thiet bi gap loi giao tiep khi chay o toc do cao!");
        TEST_FAIL("I2C-003");
    }
}

void test_I2C_004_missing_device(void) {
    LOG_INFO("Dang chay I2C-004: Kiem tra danh sach thiet bi bat buoc...");
    init_i2c_master(100000);
    bool all_found = true;
    
    for (int i = 0; i < NUM_TARGETS; i++) {
        if (!ping_device(TARGET_ADDRS[i])) {
            char msg[50];
            sprintf(msg, "THIEU THIET BI tai dia chi: 0x%02X", TARGET_ADDRS[i]);
            LOG_ERROR(msg);
            all_found = false;
        } else {
            char msg[50];
            sprintf(msg, "Thiet bi 0x%02X: OK", TARGET_ADDRS[i]);
            LOG_INFO(msg);
        }
    }
    
    i2c_driver_delete(I2C_PORT_NUM);

    if (all_found) TEST_PASS("I2C-004");
    else TEST_FAIL("I2C-004");
}

void test_I2C_005_bus_lockup(void) {
    LOG_INFO("Dang chay I2C-005: Kiem tra va xu ly Bus Lockup...");
    gpio_set_direction(I2C_SDA_PIN, GPIO_MODE_INPUT);
    
    if (gpio_get_level(I2C_SDA_PIN) == 0) {
        LOG_ERROR("Phat hien SDA bi giu chat o muc THAP (Kiem tra tu C3/C4 hoac chap han)!");
        LOG_INFO("Dang kich hoat Bus Recovery...");

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
            LOG_ERROR("Khong the phuc hoi! He thong gap loi cung nghiem trong.");
            TEST_FAIL("I2C-005");
        }
    } else {
        LOG_INFO("Bus I2C khoe manh, chan SDA dang o muc CAO ranh roi.");
        TEST_PASS("I2C-005");
    }
}

void test_I2C_006_stress_test(void) {
    LOG_INFO("Dang chay I2C-006: Stress Test da thiet bi dong thoi...");
    if (found_count == 0) {
        LOG_WARN("Bo qua I2C-006: Khong co thiet bi de stress test!");
        TEST_FAIL("I2C-006");
        return;
    }

    init_i2c_master(400000); 
    int error_count = 0;
    LOG_INFO("Dang truy cap luan phien cac thiet bi 100 chu ky o toc do cao...");

    for (int i = 0; i < 100; i++) {
        // Lặp qua tất cả thiết bị khai báo trong mảng
        for (int j = 0; j < NUM_TARGETS; j++) {
            if (!ping_device(TARGET_ADDRS[j])) error_count++;
            esp_rom_delay_us(50); // Delay cực nhỏ để ép tải Bus
        }
    }
    i2c_driver_delete(I2C_PORT_NUM);

    if (error_count == 0) {
        LOG_INFO("Stress test hoan thanh my man. Khong he rot goi tin.");
        TEST_PASS("I2C-006");
    } else {
        char error_msg[100];
        sprintf(error_msg, "Xuat hien %d loi NACK/Timeout trong luc stress test!", error_count);
        LOG_ERROR(error_msg);
        TEST_FAIL("I2C-006");
    }
}

// ========================================================
// 5. CHƯƠNG TRÌNH CHÍNH (MAIN TASK)
// ========================================================
void app_main(void) {
    vTaskDelay(pdMS_TO_TICKS(2000));
    printf("\n======================================================\n");
    printf("   BAT DAU KIEM THU TONG HOP BUS I2C - DANG TEST: \n");
    printf("   %s\n", TEST_NAME);
    printf("======================================================\n\n");

    test_I2C_002_pullup();         // 1. Đo tín hiệu điện áp tĩnh vật lý trước
    test_I2C_005_bus_lockup();     // 2. Kiểm tra xem bus có kẹt thẳng xuống đất không
    test_I2C_001_scan_bus();       // 3. Quét thiết bị thực tế
    test_I2C_004_missing_device(); // 4. Đối chiếu xem có thiếu linh kiện không
    test_I2C_003_speed();          // 5. Test tốc độ xung nhịp
    test_I2C_006_stress_test();    // 6. Test ép tải cường độ cao

    printf("\n======================================================\n");
    printf("     HOAN THANH TOAN BO CHUOI KIEM THU BUS I2C\n");
    printf("======================================================\n\n");
}