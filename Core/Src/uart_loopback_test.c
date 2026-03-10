/**
 * @file uart_loopback_test.c
 * @brief UART回环测试模块实现
 * @details 实现UART2自发自收测试功能,验证uart_frame协议和硬件收发
 *
 * @copyright Copyright (c) 2025
 * @date 2025-03-10
 */

/* Includes ------------------------------------------------------------------*/
#include "uart_loopback_test.h"
#include "uart_loopback_config.h"
#include "uart_frame.h"
#include "usart.h"
#include "stdio.h"
#include "string.h"

/* Private defines -----------------------------------------------------------*/
#define TEST_PASS true
#define TEST_FAIL false

/* Private function prototypes -----------------------------------------------*/
static void print_test_result(bool passed, const char* stage);
static void print_frame_header_info(const uint8_t* frame_buf);

/* Public functions ----------------------------------------------------------*/

/**
 * @brief 执行UART回环测试(根据配置选择模式)
 */
bool uart_loopback_test_execute(void)
{
#if UART_LOOPBACK_CURRENT_MODE == UART_LOOPBACK_MODE_HARDWARE
    printf("\r\n========== UART Loopback Test Start ==========\r\n");
    printf("Mode: Hardware Loopback (TX-RX Shorted)\r\n");
    return uart_loopback_test_hardware();

#elif UART_LOOPBACK_CURRENT_MODE == UART_LOOPBACK_MODE_SOFTWARE
    printf("\r\n========== UART Loopback Test Start ==========\r\n");
    printf("Mode: Software Loopback (Memory Simulation)\r\n");
    return uart_loopback_test_software();

#else
#error "Invalid UART_LOOPBACK_CURRENT_MODE"
#endif
}

/**
 * @brief 软件回环测试(内存模拟接收)
 */
bool uart_loopback_test_software(void)
{
    ctrl_frame_t test_data = {0};
    uint8_t tx_buf[UART_LOOPBACK_RX_BUF_SIZE];
    uint16_t frame_len = 0;
    int32_t rc;

    /* ===== 步骤1: 构造测试数据 ===== */
    uart_loopback_build_test_data(&test_data);

#if UART_LOOPBACK_DEBUG_ENABLE
    printf("\r\n[TEST] Building test data...\r\n");
    printf("[TEST] Test data structure:\r\n");
    uart_loopback_print_ctrl_frame(&test_data);
#endif

    /* ===== 步骤2: 打包数据 ===== */
    rc = uart_frame_pack(
        UART_FRAME_TYPE_CONTROL_CMD,
        &test_data,
        sizeof(ctrl_frame_t),
        tx_buf,
        sizeof(tx_buf),
        &frame_len
    );

    if (rc != UART_FRAME_OK)
    {
        printf("[ERROR] Pack failed: %ld\r\n", (long)rc);
        print_test_result(TEST_FAIL, "Pack Function");
        return TEST_FAIL;
    }

#if UART_LOOPBACK_DEBUG_ENABLE
    printf("\r\n[SEND] Frame Length: %u bytes\r\n", frame_len);
    print_frame_header_info(tx_buf);

#if UART_LOOPBACK_HEX_DUMP_ENABLE
    printf("[SEND] Hex Dump:\r\n");
    uart_loopback_hex_dump(tx_buf, frame_len, "[SEND]");
#endif
#endif

    /* ===== 步骤3: 直接解析发送缓冲区(软件回环) ===== */
    uart_frame_parse_result_t parse_result;
    rc = uart_frame_parse(tx_buf, frame_len, &parse_result);

    if (rc != UART_FRAME_OK)
    {
        printf("[ERROR] Parse failed: %ld\r\n", (long)rc);
        print_test_result(TEST_FAIL, "Parse Function");
        return TEST_FAIL;
    }

#if UART_LOOPBACK_DEBUG_ENABLE
    printf("\r\n[PARSE] Frame Length: %u bytes\r\n", (unsigned)frame_len);
    uart_loopback_print_validation(&parse_result, tx_buf, frame_len);
#endif

    /* ===== 步骤4: 验证数据一致性 ===== */
    if (parse_result.type != UART_FRAME_TYPE_CONTROL_CMD)
    {
        printf("[ERROR] Type mismatch: expected 0x11, got 0x%02X\r\n", parse_result.type);
        print_test_result(TEST_FAIL, "Type ID Check");
        return TEST_FAIL;
    }

    if (parse_result.data_len != sizeof(ctrl_frame_t))
    {
        printf("[ERROR] Data length mismatch: expected %u, got %u\r\n",
               (unsigned)sizeof(ctrl_frame_t), parse_result.data_len);
        print_test_result(TEST_FAIL, "Data Length Check");
        return TEST_FAIL;
    }

    /* ===== 步骤5: 逐字节比较 ===== */
    ctrl_frame_t* received_data = (ctrl_frame_t*)parse_result.data_ptr;
    bool bytes_match = uart_loopback_compare_bytes(
        (const uint8_t*)&test_data,
        (const uint8_t*)received_data,
        sizeof(ctrl_frame_t)
    );

    if (!bytes_match)
    {
        print_test_result(TEST_FAIL, "Byte Comparison");
        return TEST_FAIL;
    }

    /* ===== 测试通过 ===== */
    print_test_result(TEST_PASS, "All Checks");
    return TEST_PASS;
}

/**
 * @brief 硬件回环测试(TX-RX物理短接)
 */
bool uart_loopback_test_hardware(void)
{
    ctrl_frame_t test_data = {0};
    uint8_t tx_buf[UART_LOOPBACK_RX_BUF_SIZE];
    uint8_t rx_buf[UART_LOOPBACK_RX_BUF_SIZE];
    uint16_t frame_len = 0;
    int32_t rc;

    /* ===== 步骤1: 构造测试数据 ===== */
    uart_loopback_build_test_data(&test_data);

#if UART_LOOPBACK_DEBUG_ENABLE
    printf("\r\n[TEST] Building test data...\r\n");
    printf("[TEST] Test data structure:\r\n");
    uart_loopback_print_ctrl_frame(&test_data);
#endif

    /* ===== 步骤2: 打包数据 ===== */
    rc = uart_frame_pack(
        UART_FRAME_TYPE_CONTROL_CMD,
        &test_data,
        sizeof(ctrl_frame_t),
        tx_buf,
        sizeof(tx_buf),
        &frame_len
    );

    if (rc != UART_FRAME_OK)
    {
        printf("[ERROR] Pack failed: %ld\r\n", (long)rc);
        print_test_result(TEST_FAIL, "Pack Function");
        return TEST_FAIL;
    }

#if UART_LOOPBACK_DEBUG_ENABLE
    printf("\r\n[SEND] Frame Length: %u bytes\r\n", frame_len);
    print_frame_header_info(tx_buf);

#if UART_LOOPBACK_HEX_DUMP_ENABLE
    printf("[SEND] Hex Dump:\r\n");
    uart_loopback_hex_dump(tx_buf, frame_len, "[SEND]");
#endif
#endif

    /* ===== 步骤3: 发送数据 ===== */
    HAL_StatusTypeDef tx_status = HAL_UART_Transmit(&huart2, tx_buf, frame_len, 1000);

    if (tx_status != HAL_OK)
    {
        printf("[ERROR] Transmission failed: %d\r\n", tx_status);
        print_test_result(TEST_FAIL, "UART Transmit");
        return TEST_FAIL;
    }

#if UART_LOOPBACK_DEBUG_ENABLE
    printf("[SEND] Transmission OK\r\n");
#endif

    /* ===== 步骤4: 接收数据 ===== */
#if UART_LOOPBACK_DEBUG_ENABLE
    printf("[RECV] Waiting for data... (Timeout: %d ms)\r\n", UART_LOOPBACK_RX_TIMEOUT_MS);
#endif

    HAL_StatusTypeDef rx_status = HAL_UART_Receive(&huart2, rx_buf, frame_len, UART_LOOPBACK_RX_TIMEOUT_MS);

    if (rx_status != HAL_OK)
    {
        printf("[ERROR] Receive failed: %d\r\n", rx_status);
        print_test_result(TEST_FAIL, "UART Receive");
        return TEST_FAIL;
    }

#if UART_LOOPBACK_DEBUG_ENABLE
    printf("[RECV] Received %u bytes\r\n", frame_len);
    printf("[RECV] Receive Status: OK\r\n");

#if UART_LOOPBACK_HEX_DUMP_ENABLE
    printf("[RECV] Hex Dump:\r\n");
    uart_loopback_hex_dump(rx_buf, frame_len, "[RECV]");
#endif
#endif

    /* ===== 步骤5: 解析接收数据 ===== */
    uart_frame_parse_result_t parse_result;
    rc = uart_frame_parse(rx_buf, frame_len, &parse_result);

    if (rc != UART_FRAME_OK)
    {
        printf("[ERROR] Parse failed: %ld\r\n", (long)rc);
        print_test_result(TEST_FAIL, "Parse Function");
        return TEST_FAIL;
    }

#if UART_LOOPBACK_DEBUG_ENABLE
    printf("\r\n[PARSE] Frame Length: %u bytes\r\n", frame_len);
    uart_loopback_print_validation(&parse_result, rx_buf, frame_len);
#endif

    /* ===== 步骤6: 验证数据一致性 ===== */
    if (parse_result.type != UART_FRAME_TYPE_CONTROL_CMD)
    {
        printf("[ERROR] Type mismatch: expected 0x11, got 0x%02X\r\n", parse_result.type);
        print_test_result(TEST_FAIL, "Type ID Check");
        return TEST_FAIL;
    }

    if (parse_result.data_len != sizeof(ctrl_frame_t))
    {
        printf("[ERROR] Data length mismatch: expected %u, got %u\r\n",
               (unsigned)sizeof(ctrl_frame_t), parse_result.data_len);
        print_test_result(TEST_FAIL, "Data Length Check");
        return TEST_FAIL;
    }

    /* ===== 步骤7: 逐字节比较 ===== */
    ctrl_frame_t* received_data = (ctrl_frame_t*)parse_result.data_ptr;
    bool bytes_match = uart_loopback_compare_bytes(
        (const uint8_t*)&test_data,
        (const uint8_t*)received_data,
        sizeof(ctrl_frame_t)
    );

    if (!bytes_match)
    {
        print_test_result(TEST_FAIL, "Byte Comparison");
        return TEST_FAIL;
    }

    /* ===== 步骤8: 比较发送和接收缓冲区 ===== */
    bool buffer_match = uart_loopback_compare_bytes(tx_buf, rx_buf, frame_len);

    if (!buffer_match)
    {
        printf("[ERROR] TX and RX buffers don't match!\r\n");
        print_test_result(TEST_FAIL, "Buffer Comparison");
        return TEST_FAIL;
    }

    /* ===== 测试通过 ===== */
    print_test_result(TEST_PASS, "All Checks");
    return TEST_PASS;
}

/**
 * @brief 构造测试用的控制命令数据
 */
void uart_loopback_build_test_data(void* ctrl)
{
    ctrl_frame_t* cmd = (ctrl_frame_t*)ctrl;

    /* 使用特殊值便于识别 */

    /* 角度 - 使用12.34°而非0.0° */
    cmd->angles.angle_left = UART_LOOPBACK_TEST_ANGLE;
    cmd->angles.angle_right = UART_LOOPBACK_TEST_ANGLE;
    cmd->angles.angle_camera = UART_LOOPBACK_TEST_ANGLE;

    /* 标志位 */
    cmd->flags.chassis_lock = 0;
    cmd->flags.motor0_break = 0;
    cmd->flags.motor1_break = 0;
    cmd->flags.diff_locked = 0;

    /* 电机参数 - 使用56而非30 */
    cmd->motor_rpm.left = UART_LOOPBACK_TEST_PWM;
    cmd->motor_rpm.right = UART_LOOPBACK_TEST_PWM;
    cmd->motor_rpm.pulses = UART_LOOPBACK_TEST_PULSES;

    /* LED控制 */
    cmd->led_ctrl.red_mode = 1;      /* ON */
    cmd->led_ctrl.blue_mode = 2;     /* BLINK */
    cmd->led_ctrl.defenses_mode = 0; /* OFF */
    cmd->led_ctrl.flow_enable = 1;   /* Enable */
    cmd->led_ctrl.period_ms = 500;   /* 500ms */
    cmd->led_ctrl.duty_cycle = 75;   /* 75% */
}

/**
 * @brief 逐字节比较两个缓冲区
 */
bool uart_loopback_compare_bytes(const uint8_t* expected,
                                 const uint8_t* actual,
                                 size_t len)
{
#if UART_LOOPBACK_DEBUG_ENABLE
    size_t total_passed = 0;
    size_t chunk_size = 16;
    bool all_match = true;
#endif

    for (size_t i = 0; i < len; i++)
    {
        if (expected[i] != actual[i])
        {
#if UART_LOOPBACK_DEBUG_ENABLE
            printf("[COMPARE] Mismatch at offset %u: expected 0x%02X, got 0x%02X\r\n",
                   (unsigned)i, expected[i], actual[i]);
            all_match = false;
#endif
            return TEST_FAIL;
        }
    }

#if UART_LOOPBACK_DEBUG_ENABLE
    /* 分块显示比较结果 */
    printf("\r\n[COMPARE] Byte-by-byte comparison:\r\n");

    for (size_t offset = 0; offset < len; offset += chunk_size)
    {
        size_t end = (offset + chunk_size < len) ? (offset + chunk_size) : len;
        size_t count = end - offset;

        printf("[COMPARE] [%u-%u]: PASS (%u/%u bytes match)\r\n",
               (unsigned)offset, (unsigned)(end - 1), (unsigned)count, (unsigned)count);
        total_passed += count;
    }

    printf("[COMPARE] Total: %u/%u bytes match (100%%)\r\n", (unsigned)len, (unsigned)len);
#endif

    return TEST_PASS;
}

/**
 * @brief 以十六进制格式转储数据
 */
void uart_loopback_hex_dump(const uint8_t* data, size_t len, const char* prefix)
{
    for (size_t i = 0; i < len; i++)
    {
        if (i % 16 == 0)
        {
            printf("%s ", prefix);
        }

        printf("%02X ", data[i]);

        if ((i + 1) % 16 == 0 || i == len - 1)
        {
            printf("\r\n");
        }
    }
}

/**
 * @brief 输出协议验证的详细结果
 */
void uart_loopback_print_validation(const void* result,
                                    const uint8_t* frame_buf,
                                    size_t frame_len)
{
    const uart_frame_parse_result_t* parse_result = (const uart_frame_parse_result_t*)result;

    printf("[PARSE] Frame Header: PASS (");
    printf("%02X %02X)\r\n", frame_buf[0], frame_buf[1]);

    printf("[PARSE] Frame Tail: PASS (");
    printf("%02X %02X)\r\n", frame_buf[frame_len - 2], frame_buf[frame_len - 1]);

    /* CRC16校验结果 */
    uint16_t recv_crc = (frame_buf[frame_len - 4] | (frame_buf[frame_len - 3] << 8));
    printf("[PARSE] CRC16: PASS (Recv=0x%04X)\r\n", recv_crc);

    /* 版本号 */
    printf("[PARSE] Version: PASS (0x%02X)\r\n", parse_result->version);

    /* 类型ID */
    printf("[PARSE] Type ID: PASS (0x%02X - ", parse_result->type);
    if (parse_result->type == UART_FRAME_TYPE_CONTROL_CMD)
    {
        printf("CONTROL_CMD)\r\n");
    }
    else if (parse_result->type == UART_FRAME_TYPE_SENSOR_DATA)
    {
        printf("SENSOR_DATA)\r\n");
    }
    else if (parse_result->type == UART_FRAME_TYPE_STATUS_FEEDBACK)
    {
        printf("STATUS_FEEDBACK)\r\n");
    }
    else
    {
        printf("UNKNOWN)\r\n");
    }

    /* 数据长度 */
    printf("[PARSE] Data Length: PASS (%u bytes)\r\n", parse_result->data_len);

#if UART_LOOPBACK_SHOW_PARSED_DATA
    /* 显示解析后的数据结构 */
    if (parse_result->type == UART_FRAME_TYPE_CONTROL_CMD)
    {
        printf("\r\n[PARSE] Control Frame Structure:\r\n");
        uart_loopback_print_ctrl_frame(parse_result->data_ptr);
    }
#endif
}

/**
 * @brief 输出解析后的控制命令数据结构
 */
void uart_loopback_print_ctrl_frame(const void* ctrl)
{
    const ctrl_frame_t* cmd = (const ctrl_frame_t*)ctrl;

    printf("  angles.angle_left: %.2f\r\n", cmd->angles.angle_left);
    printf("  angles.angle_right: %.2f\r\n", cmd->angles.angle_right);
    printf("  angles.angle_camera: %.2f\r\n", cmd->angles.angle_camera);
    printf("  flags.chassis_lock: %u\r\n", cmd->flags.chassis_lock);
    printf("  flags.motor0_break: %u\r\n", cmd->flags.motor0_break);
    printf("  flags.motor1_break: %u\r\n", cmd->flags.motor1_break);
    printf("  flags.diff_locked: %u\r\n", cmd->flags.diff_locked);
    printf("  motor_rpm.left: %d\r\n", cmd->motor_rpm.left);
    printf("  motor_rpm.right: %d\r\n", cmd->motor_rpm.right);
    printf("  motor_rpm.pulses: %u\r\n", cmd->motor_rpm.pulses);
    printf("  led_ctrl.red_mode: %u\r\n", cmd->led_ctrl.red_mode);
    printf("  led_ctrl.blue_mode: %u\r\n", cmd->led_ctrl.blue_mode);
    printf("  led_ctrl.defenses_mode: %u\r\n", cmd->led_ctrl.defenses_mode);
    printf("  led_ctrl.flow_enable: %u\r\n", cmd->led_ctrl.flow_enable);
    printf("  led_ctrl.period_ms: %u\r\n", cmd->led_ctrl.period_ms);
    printf("  led_ctrl.duty_cycle: %u\r\n", cmd->led_ctrl.duty_cycle);
}

/* Private functions ---------------------------------------------------------*/

/**
 * @brief 输出帧头信息
 */
static void print_frame_header_info(const uint8_t* frame_buf)
{
    printf("[SEND] Frame Header: ");
    printf("%02X %02X ", frame_buf[0], frame_buf[1]);  /* 魔数 */
    printf("%02X %02X ", frame_buf[2], frame_buf[3]);  /* 长度 */
    printf("%02X ", frame_buf[4]);                      /* 类型ID */
    printf("%02X\r\n", frame_buf[5]);                   /* 版本 */

    printf("[SEND] Type ID: 0x%02X (CONTROL_CMD)\r\n", frame_buf[4]);
    printf("[SEND] Version: 0x%02X\r\n", frame_buf[5]);

    uint16_t data_len = frame_buf[2] | (frame_buf[3] << 8);
    printf("[SEND] Data Length: %u bytes\r\n", data_len);
}

/**
 * @brief 输出测试结果
 */
static void print_test_result(bool passed, const char* stage)
{
    if (passed)
    {
        printf("\r\n========== TEST RESULT: PASS ==========\r\n");
        printf("- Pack Function: OK\r\n");
        printf("- Parse Function: OK\r\n");
        printf("- CRC16 Check: OK\r\n");
        printf("- Data Consistency: OK\r\n");
        printf("- Byte Comparison: OK\r\n");
        printf("- Test Stage: %s\r\n", stage);
        printf("=========================================\r\n");
    }
    else
    {
        printf("\r\n========== TEST RESULT: FAIL ==========\r\n");
        printf("- Error Stage: %s\r\n", stage);
        printf("- Details: See error messages above\r\n");
        printf("=========================================\r\n");
    }
}
