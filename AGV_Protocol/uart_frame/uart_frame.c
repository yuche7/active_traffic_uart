/**
 * @file uart_frame.c
 * @brief UART通用帧协议实现
 * @details 提供跨平台的帧打包、解析、查找等核心功能
 *
 * @copyright Copyright (c) 2024-2029, JoyIot
 * @author baicha
 * @date 2025-10-06
 */

// #include <stdio.h>
#include "uart_frame.h"
#include "uart_frame_crc.h"

/* ========== 内部辅助函数 ========== */

/**
 * @brief 小端序写入16位数据
 *
 * @param buf   缓冲区指针
 * @param value 16位数据
 */
static inline void write_uint16_le(uint8_t* buf, uint16_t value)
{
    buf[0] = (uint8_t)(value & 0xFF);
    buf[1] = (uint8_t)((value >> 8) & 0xFF);
}

/**
 * @brief 小端序读取16位数据
 *
 * @param buf   缓冲区指针
 * @return 16位数据
 */
static inline uint16_t read_uint16_le(const uint8_t* buf)
{
    return (uint16_t)(buf[0] | (buf[1] << 8));
}

/* ========== 核心API实现 ========== */

/**
 * @brief 打包数据到UART帧
 */
int32_t uart_frame_pack(uart_frame_type_t type, const void* data, uint16_t data_len,
                        uint8_t* frame_buf, size_t frame_buf_size, uint16_t* out_frame_len)
{
    /* 参数检查 */
    if (frame_buf == NULL || out_frame_len == NULL)
    {
        return UART_FRAME_ERR_NULL_POINTER;
    }

    if (data == NULL && data_len > 0)
    {
        return UART_FRAME_ERR_NULL_POINTER;
    }

    /* 检查数据长度限制 */
    if (data_len > UART_FRAME_DATA_MAX_SIZE)
    {
        return UART_FRAME_ERR_DATA_TOO_LARGE;
    }

    /* 计算完整帧长度 */
    uint16_t total_frame_len = UART_FRAME_CALC_TOTAL_LEN(data_len);

    /* 检查缓冲区大小 */
    if (frame_buf_size < total_frame_len)
    {
        return UART_FRAME_ERR_BUFFER_TOO_SMALL;
    }

    /* 偏移量指针 */
    size_t offset = 0;

    /* 1. 写入帧头魔数(2字节) */
    frame_buf[offset++] = UART_FRAME_HEADER_MAGIC_0;
    frame_buf[offset++] = UART_FRAME_HEADER_MAGIC_1;

    /* 2. 写入数据长度(2字节,小端序) */
    write_uint16_le(&frame_buf[offset], data_len);
    offset += 2;

    /* 3. 写入类型ID(1字节) */
    frame_buf[offset++] = (uint8_t)type;

    /* 4. 写入版本号(1字节) */
    frame_buf[offset++] = UART_FRAME_VERSION;

    /* 5. 写入数据区 */
    if (data_len > 0 && data != NULL)
    {
        memcpy(&frame_buf[offset], data, data_len);
        offset += data_len;
    }

    /* 6. 计算CRC16(覆盖:帧头魔数 + 长度 + 类型 + 版本 + 数据区) */
    uint16_t crc_value = uart_frame_crc16_calculate(frame_buf, offset);

    /* 7. 写入CRC16(2字节,小端序) */
    write_uint16_le(&frame_buf[offset], crc_value);
    offset += 2;

    /* 8. 写入帧尾魔数(2字节) */
    frame_buf[offset++] = UART_FRAME_TAIL_MAGIC_0;
    frame_buf[offset++] = UART_FRAME_TAIL_MAGIC_1;

    /* 输出完整帧长度 */
    *out_frame_len = (uint16_t)offset;

    return UART_FRAME_OK;
}

/**
 * @brief 解析UART帧
 */
int32_t uart_frame_parse(const uint8_t* frame_buf, size_t frame_len,
                         uart_frame_parse_result_t* result)
{
    /* 参数检查 */
    if (frame_buf == NULL || result == NULL)
    {
        return UART_FRAME_ERR_NULL_POINTER;
    }

    /* 初始化结果 */
    result->is_valid = false;
    result->type = UART_FRAME_TYPE_INVALID;
    result->version = 0;
    result->data_len = 0;
    result->data_ptr = NULL;

    /* 检查最小帧长度 */
    if (frame_len < UART_FRAME_MIN_SIZE)
    {
        return UART_FRAME_ERR_LENGTH_MISMATCH;
    }

    size_t offset = 0;

    /* 1. 验证帧头魔数 */
    if (frame_buf[offset] != UART_FRAME_HEADER_MAGIC_0
        || frame_buf[offset + 1] != UART_FRAME_HEADER_MAGIC_1)
    {
        return UART_FRAME_ERR_INVALID_HEADER;
    }
    offset += 2;

    /* 2. 读取数据长度 */
    uint16_t data_len = read_uint16_le(&frame_buf[offset]);
    offset += 2;

    /* 3. 读取类型ID */
    uint8_t type_id = frame_buf[offset++];

    /* 4. 读取版本号 */
    uint8_t version = frame_buf[offset++];

    /* 5. 验证协议版本 */
    if (version != UART_FRAME_VERSION)
    {
        return UART_FRAME_ERR_VERSION_MISMATCH;
    }

    /* 6. 验证帧长度一致性 */
    uint16_t expected_frame_len = UART_FRAME_CALC_TOTAL_LEN(data_len);
    if (frame_len != expected_frame_len)
    {
        // printf("frame_len: %d, expected_frame_len: %d\n");
        return UART_FRAME_ERR_LENGTH_MISMATCH;
    }

    /* 7. 数据区指针 */
    const uint8_t* data_ptr = (data_len > 0) ? &frame_buf[offset] : NULL;
    offset += data_len;

    /* 8. 读取CRC16 */
    uint16_t received_crc = read_uint16_le(&frame_buf[offset]);
    offset += 2;

    /* 9. 验证帧尾魔数 */
    if (frame_buf[offset] != UART_FRAME_TAIL_MAGIC_0
        || frame_buf[offset + 1] != UART_FRAME_TAIL_MAGIC_1)
    {
        return UART_FRAME_ERR_INVALID_TAIL;
    }

    /* 10. 计算并验证CRC16(覆盖:帧头到数据区结束) */
    size_t crc_calc_len = offset - 2; // 不包含CRC和帧尾
    uint16_t calculated_crc = uart_frame_crc16_calculate(frame_buf, crc_calc_len);

    if (calculated_crc != received_crc)
    {
        return UART_FRAME_ERR_CRC_MISMATCH;
    }

    /* 11. 填充解析结果 */
    result->type = (uart_frame_type_t)type_id;
    result->version = version;
    result->data_len = data_len;
    result->data_ptr = data_ptr;
    result->is_valid = true;

    return UART_FRAME_OK;
}

/**
 * @brief 在接收缓冲区中查找完整帧
 */
int32_t uart_frame_find(const uint8_t* rx_buf, size_t rx_len, size_t* frame_start,
                        size_t* frame_length)
{
    /* 参数检查 */
    if (rx_buf == NULL || frame_start == NULL || frame_length == NULL)
    {
        return UART_FRAME_ERR_NULL_POINTER;
    }

    /* 初始化输出 */
    *frame_start = 0;
    *frame_length = 0;

    /* 至少需要完整帧头才能判断 */
    if (rx_len < UART_FRAME_HEADER_SIZE)
    {
        return UART_FRAME_ERR_INVALID_HEADER;
    }

    /* 查找帧头魔数 */
    size_t i;
    for (i = 0; i <= rx_len - UART_FRAME_HEADER_SIZE; i++)
    {
        /* 检查帧头魔数 */
        if (rx_buf[i] == UART_FRAME_HEADER_MAGIC_0 && rx_buf[i + 1] == UART_FRAME_HEADER_MAGIC_1)
        {

            /* 读取数据长度字段 */
            uint16_t data_len = read_uint16_le(&rx_buf[i + 2]);

            /* 检查数据长度合理性 */
            if (data_len > UART_FRAME_DATA_MAX_SIZE)
            {
                continue; // 不合法,继续查找下一个帧头
            }

            /* 计算完整帧长度 */
            uint16_t expected_frame_len = UART_FRAME_CALC_TOTAL_LEN(data_len);

            /* 检查缓冲区是否包含完整帧 */
            if (i + expected_frame_len > rx_len)
            {
                return UART_FRAME_ERR_LENGTH_MISMATCH; // 数据不完整
            }

            /* 验证帧尾魔数 */
            size_t tail_offset = i + expected_frame_len - 2;
            if (rx_buf[tail_offset] != UART_FRAME_TAIL_MAGIC_0
                || rx_buf[tail_offset + 1] != UART_FRAME_TAIL_MAGIC_1)
            {
                continue; // 帧尾不匹配,继续查找
            }

            /* 找到完整帧 */
            *frame_start = i;
            *frame_length = expected_frame_len;
            return UART_FRAME_OK;
        }
    }

    /* 未找到有效帧头 */
    return UART_FRAME_ERR_INVALID_HEADER;
}

/**
 * @brief 验证帧格式(不含CRC校验)
 */
bool uart_frame_validate_format(const uint8_t* frame_buf, size_t frame_len)
{
    /* 参数检查 */
    if (frame_buf == NULL || frame_len < UART_FRAME_MIN_SIZE)
    {
        return false;
    }

    /* 验证帧头 */
    if (frame_buf[0] != UART_FRAME_HEADER_MAGIC_0 || frame_buf[1] != UART_FRAME_HEADER_MAGIC_1)
    {
        return false;
    }

    /* 读取数据长度 */
    uint16_t data_len = read_uint16_le(&frame_buf[2]);

    /* 验证长度一致性 */
    uint16_t expected_len = UART_FRAME_CALC_TOTAL_LEN(data_len);
    if (frame_len != expected_len)
    {
        return false;
    }

    /* 验证帧尾 */
    size_t tail_offset = frame_len - 2;
    if (frame_buf[tail_offset] != UART_FRAME_TAIL_MAGIC_0
        || frame_buf[tail_offset + 1] != UART_FRAME_TAIL_MAGIC_1)
    {
        return false;
    }

    return true;
}

/**
 * @brief 获取错误码描述字符串
 */
const char* uart_frame_get_error_string(int32_t error_code)
{
    switch (error_code)
    {
        case UART_FRAME_OK: return "Success";
        case UART_FRAME_ERR_NULL_POINTER: return "Null pointer error";
        case UART_FRAME_ERR_INVALID_PARAM: return "Invalid parameter";
        case UART_FRAME_ERR_BUFFER_TOO_SMALL: return "Buffer too small";
        case UART_FRAME_ERR_DATA_TOO_LARGE: return "Data too large";
        case UART_FRAME_ERR_INVALID_HEADER: return "Invalid frame header";
        case UART_FRAME_ERR_INVALID_TAIL: return "Invalid frame tail";
        case UART_FRAME_ERR_CRC_MISMATCH: return "CRC mismatch";
        case UART_FRAME_ERR_VERSION_MISMATCH: return "Version mismatch";
        case UART_FRAME_ERR_LENGTH_MISMATCH: return "Length mismatch";
        default: return "Unknown error";
    }
}

