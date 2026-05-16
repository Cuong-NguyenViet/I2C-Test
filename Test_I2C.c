#include <stdio.h>
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ================= CẤU HÌNH PHẦN CỨNG =================
#define I2C_SDA_PIN      21    // Chân SDA của ESP32
#define I2C_SCL_PIN      22    // Chân SCL của ESP32
#define I2C_PORT_NUM     I2C_NUM_0

// !!! LƯU Ý: Thay đổi các địa chỉ dưới đây cho đúng với mạch AirSense của bạn !!!
#define DEV_1            0x20  // Địa chỉ cảm biến 1
#define DEV_2            0x3C  // Địa chỉ màn hình OLED hoặc cảm biến 2

// Danh sách các thiết bị bắt buộc phải có mặt (Dùng cho bài I2C-004)
const uint8_t REQUIRED_DEVICES[] = {DEV_1, DEV_2}; 
const int NUM_REQUIRED = sizeof(REQUIRED_DEVICES) / sizeof(REQUIRED_DEVICES[0]);

// ================= QUY CHUẨN LOG THỐNG NHẤT =================
#define LOG_INFO(msg)    printf("[INFO]    %s\n", msg)
#define LOG_WARN(msg)    printf("[WARN]    %s\n", msg)
#define LOG_ERROR(msg)   printf("[ERROR]   %s\n", msg)
#define TEST_PASS(id)    printf("[TEST]    %s : PASS ==================\n\n", id)
#define TEST_FAIL(id)    printf("[TEST]    %s : FAIL ==================\n\n", id)

// Biến toàn cục lưu trữ các thiết bị quét được thực tế
uint8_t found_devices[128];
int found_count = 0;

// ================= KHAI BÁO CÁC HÀM KIỂM THỬ =================
void test_I2C_001_scan_bus(void);
void test_I2C_002_pullup(void);
void test_I2C_003_speed(void);
void test_I2C_004_missing_device(void);
void test_I2C_005_bus_lockup(void);
void test_I2C_006_stress_test(void);

// Hàm bổ trợ khởi tạo nhanh I2C Master với tốc độ tùy chọn
static esp_err_t init_i2c_master(uint32_t speed_hz) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA_PIN,
        .sda_pullup_en = GPIO_PULLUP_DISABLE, // Kiểm tra trở kéo ngoài trên mạch thật
        .scl_io_num = I2C_SCL_PIN,
        .scl_pullup_en = GPIO_PULLUP_DISABLE,
        .master.clk_speed = speed_hz,
    };
    i2c_param_config(I2C_PORT_NUM, &conf);
    return i2c_driver_install(I2C_PORT_NUM, conf.mode, 0, 0, 0);
}

// Hàm bổ trợ kiểm tra nhanh một địa chỉ xem có ACK không
static bool ping_device(uint8_t addr) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_PORT_NUM, cmd, pdMS_TO_TICKS(20));
    i2c_cmd_link_delete(cmd);
    return (ret == ESP_OK);
}

// ================= HÀM CHÍNH (MAIN TASK) =================
void app_main(void) {
    vTaskDelay(pdMS_TO_TICKS(2000)); // Chờ hệ thống và Serial ổn định sau boot
    printf("\n--- BẮT ĐẦU CHƯƠNG TRÌNH TỔNG HỢP KIỂM THỬ BUS I2C ---\n\n");

    // Chạy tuần tự các bài kiểm tra từ 001 đến 006
    test_I2C_002_pullup();         // Test 002 chạy trước vì cấu hình chân dạng Input vật lý
    test_I2C_001_scan_bus();       // Quét tìm thiết bị hiện có
    test_I2C_003_speed();          // Thử nghiệm tốc độ
    test_I2C_004_missing_device(); // Kiểm tra thiết bị thiếu
    test_I2C_005_bus_lockup();     // Thử nghiệm khả năng tự phục hồi kẹt bus
    test_I2C_006_stress_test();     // Stress test xung đột định thời

    printf("--- HOÀN THÀNH TOÀN BỘ CHUỖI KIỂM THỬ BUS I2C ---\n");
}

// ================= CHI TIẾT LOGIC CÁC BÀI TEST =================

// I2C-001: Quét toàn bộ Bus I2C
void test_I2C_001_scan_bus(void) {
    LOG_INFO("Dang chay I2C-001: Quet bus I2C...");
    init_i2c_master(100000); // Khởi tạo ở chế độ tiêu chuẩn 100kHz
    found_count = 0;

    for (uint8_t address = 1; address < 127; address++) {
        if (ping_device(address)) {
            char msg[50];
            sprintf(msg, "Tim thay thiet bi tai dia chi 0x%02X", address);
            LOG_INFO(msg);
            found_devices[found_count++] = address;
        }
    }
    i2c_driver_delete(I2C_PORT_NUM);

    if (found_count > 0) TEST_PASS("I2C-001");
    else TEST_FAIL("I2C-001");
}

// I2C-002: Kiểm tra trở kéo (Pull-up) ngoài
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
        LOG_ERROR("Thieu tro keo ngoai hoac duong bus dang bi chap xuong GND!");
        TEST_FAIL("I2C-002");
    }
}

// I2C-003: Kiểm tra đáp ứng tốc độ Bus
void test_I2C_003_speed(void) {
    LOG_INFO("Dang chay I2C-003: Kiem tra toc do bus...");
    if (found_count == 0) {
        LOG_WARN("Khong co muc tieu thiet bi nao de kiem tra toc do. Bo qua.");
        TEST_FAIL("I2C-003");
        return;
    }

    uint8_t test_target = found_devices[0]; // Lấy thiết bị đầu tiên tìm thấy để thử nghiệm
    
    // Thử ở 100kHz
    init_i2c_master(100000);
    bool p100 = ping_device(test_target);
    i2c_driver_delete(I2C_PORT_NUM);

    // Thử ở 400kHz
    init_i2c_master(400000);
    bool p400 = ping_device(test_target);
    i2c_driver_delete(I2C_PORT_NUM);

    if (p100 && p400) {
        LOG_INFO("Thiet bi phan hoi on dinh o ca hai toc do 100kHz va 400kHz.");
        TEST_PASS("I2C-003");
    } else {
        LOG_ERROR("Giao tiep gap loi hoac thiet bi khong dap ung duoc toc do cao!");
        TEST_FAIL("I2C-003");
    }
}

// I2C-004: Kiểm tra thiếu thiết bị phần cứng bắt buộc
void test_I2C_004_missing_device(void) {
    LOG_INFO("Dang chay I2C-004: Kiem tra thiet bi thieu...");
    init_i2c_master(100000);
    bool all_found = true;

    for (int i = 0; i < NUM_REQUIRED; i++) {
        if (!ping_device(REQUIRED_DEVICES[i])) {
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

// I2C-005: Kiểm tra và xử lý lỗi treo Bus (Bus Lockup)
void test_I2C_005_bus_lockup(void) {
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
        LOG_INFO("Bus I2C khoe manh, chân SDA dang o muc CAO ranh roi.");
        TEST_PASS("I2C-005");
    }
}

// I2C-006: Stress Test giao tiếp đồng thời liên tục
void test_I2C_006_stress_test(void) {
    LOG_INFO("Dang chay I2C-006: Stress Test giao tiep dong thoi...");
    init_i2c_master(400000); // Ép chạy ở tốc độ cao nhất 400kHz để dễ lộ lỗi định thời

    int error_count = 0;
    LOG_INFO("Dang thuc hien truy cap luan phien toc do cao 100 chu ky...");

    for (int i = 0; i < 100; i++) {
        if (!ping_device(DEV_1)) error_count++;
        if (!ping_device(DEV_2)) error_count++;
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