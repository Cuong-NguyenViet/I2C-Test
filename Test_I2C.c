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
// 1 -> Test đường I2C số 1 (Nối ra giắc IDC3 / Màn hình OLED)
// 2 -> Test đường I2C số 2 (Nối ra IC PCF8575 / Giắc IDC4)
#define TEST_CONFIG 1  

#if TEST_CONFIG == 1
    #define I2C_SDA_PIN      19
    #define I2C_SCL_PIN      22
    #define TARGET_ADDR      0x3C  // Địa chỉ OLED mặc định
    #define TEST_NAME        "Duong I2C-1 (Man hinh OLED)"
#elif TEST_CONFIG == 2
    #define I2C_SDA_PIN      2
    #define I2C_SCL_PIN      4
    #define TARGET_ADDR      0x20  // Địa chỉ PCF8575 mặc định
    #define TEST_NAME        "Duong I2C-2 (Chip PCF8575)"
#else
    #error "Vui long chon TEST_CONFIG la 1 hoac 2"
#endif

#define I2C_PORT_NUM         I2C_NUM_0

// ========================================================
// 2. MACRO ĐỊNH DẠNG LOG VÀ KHAI BÁO BIẾN TOÀN CỤC
// ========================================================
#define LOG_INFO(msg)    printf("[INFO]    %s\n", msg)
#define LOG_WARN(msg)    printf("[WARN]    %s\n", msg)
#define LOG_ERROR(msg)   printf("[ERROR]   %s\n", msg)
#define TEST_PASS(id)    printf("[TEST]    %s : PASS ==================\n\n", id)
#define TEST_FAIL(id)    printf("[TEST]    %s : FAIL ==================\n\n", id)

int found_count = 0; // Lưu số lượng thiết bị tìm thấy

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
    LOG_INFO("Dang chay I2C-003: Kiem tra toc do bus...");
    if (found_count == 0) {
        LOG_WARN("Bo qua I2C-003: Chua tim thay thiet bi de test!");
        TEST_FAIL("I2C-003");
        return;
    }

    init_i2c_master(100000);
    bool p100 = ping_device(TARGET_ADDR);
    i2c_driver_delete(I2C_PORT_NUM);

    init_i2c_master(400000);
    bool p400 = ping_device(TARGET_ADDR);
    i2c_driver_delete(I2C_PORT_NUM);

    if (p100 && p400) {
        LOG_INFO("Thiet bi phan hoi on dinh o ca hai toc do 100kHz va 400kHz.");
        TEST_PASS("I2C-003");
    } else {
        LOG_ERROR("Giao tiep gap loi khi chay o toc do cao!");
        TEST_FAIL("I2C-003");
    }
}

void test_I2C_004_missing_device(void) {
    LOG_INFO("Dang chay I2C-004: Kiem tra thiet bi thieu...");
    init_i2c_master(100000);
    
    if (!ping_device(TARGET_ADDR)) {
        char msg[50];
        sprintf(msg, "THIEU THIET BI muc tieu tai dia chi: 0x%02X", TARGET_ADDR);
        LOG_ERROR(msg);
        TEST_FAIL("I2C-004");
    } else {
        char msg[50];
        sprintf(msg, "Thiet bi muc tieu 0x%02X hien dien OK.", TARGET_ADDR);
        LOG_INFO(msg);
        TEST_PASS("I2C-004");
    }
    i2c_driver_delete(I2C_PORT_NUM);
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
    LOG_INFO("Dang chay I2C-006: Stress Test giao tiep dong thoi...");
    if (found_count == 0) {
        LOG_WARN("Bo qua I2C-006: Khong co thiet bi de stress test!");
        TEST_FAIL("I2C-006");
        return;
    }

    init_i2c_master(400000); 
    int error_count = 0;
    LOG_INFO("Dang thuc hien truy cap toc do cao 100 chu ky...");

    for (int i = 0; i < 100; i++) {
        if (!ping_device(TARGET_ADDR)) error_count++;
        esp_rom_delay_us(100);
    }
    i2c_driver_delete(I2C_PORT_NUM);

    if (error_count == 0) {
        LOG_INFO("Stress test hoan thanh my man. Ty le phan hoi 100%.");
        TEST_PASS("I2C-006");
    } else {
        char error_msg[100];
        sprintf(error_msg, "Xuat hien %d loi (NACK/Timeout) trong luc stress test!", error_count);
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
    test_I2C_001_scan_bus();       // 3. Quét số thiết bị để làm tiền đề cho các bài sau
    test_I2C_003_speed();          // 4. Test tốc độ xung nhịp
    test_I2C_004_missing_device(); // 5. Đối chiếu thiết bị chuẩn
    test_I2C_006_stress_test();    // 6. Test ép tải cường độ cao

    printf("\n======================================================\n");
    printf("     HOAN THANH TOAN BO CHUOI KIEM THU BUS I2C\n");
    printf("======================================================\n\n");
}