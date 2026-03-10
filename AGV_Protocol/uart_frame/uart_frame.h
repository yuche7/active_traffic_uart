/**
 * @file uart_frame.h
 * @brief UART通用帧协议模块 - 跨平台头文件
 * @details 提供跨平台的UART通信帧封装和解析功能,支持RT-Thread和Linux系统
 *
 * @note 本模块设计为平台无关,可在不同MCU和操作系统间共享使用
 * @warning 使用前必须提供 uart_frame_config.h 配置文件,参考 uart_frame_config_template.h
 * 
 * @copyright Copyright (c) 2024-2029, JoyIot
 * @author baicha
 * @date 2025-10-06
 */

#ifndef UART_FRAME_H
#define UART_FRAME_H

#ifdef __cplusplus
extern "C"
{
#endif

/* ========== 平台相关性处理 ========== */
#if defined(__RTTHREAD__) || defined(RT_THREAD)
/* RT-Thread平台 */
// #include <rtthread.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#elif defined(__linux__) || defined(__unix__)
/* Linux/Unix平台 */
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#else
/* 裸机或其他平台 */
#ifndef __RL78_MCU_DEF
#include <stdint.h>
#endif
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#endif

    /* ========== 引入用户配置文件 ========== */

    /**
     * @brief 用户必须提供配置文件
     * @note  将 uart_frame_config_template.h 复制到你的项目中,
     *        重命名为 uart_frame_config.h 并根据需求修改
     */
#include "uart_frame_config.h"

    /* ========== 编译期配置检查 ========== */

#ifndef UART_FRAME_HEADER_MAGIC_0
#error "uart_frame_config.h must define UART_FRAME_HEADER_MAGIC_0"
#endif

#ifndef UART_FRAME_HEADER_MAGIC_1
#error "uart_frame_config.h must define UART_FRAME_HEADER_MAGIC_1"
#endif

#ifndef UART_FRAME_TAIL_MAGIC_0
#error "uart_frame_config.h must define UART_FRAME_TAIL_MAGIC_0"
#endif

#ifndef UART_FRAME_TAIL_MAGIC_1
#error "uart_frame_config.h must define UART_FRAME_TAIL_MAGIC_1"
#endif

#ifndef UART_FRAME_VERSION
#error "uart_frame_config.h must define UART_FRAME_VERSION"
#endif

#ifndef UART_FRAME_DATA_MAX_SIZE
#error "uart_frame_config.h must define UART_FRAME_DATA_MAX_SIZE"
#endif

    /* 检查类型枚举定义 */
    typedef uart_frame_type_t _uart_frame_type_check_t;

    /* ========== 帧格式固定参数(由配置计算) ========== */

    /** 帧头固定部分长度(不含数据区) */
#define UART_FRAME_HEADER_SIZE 6 // 帧头(2) + 长度(2) + 类型ID(1) + 版本号(1)

    /** 帧尾固定部分长度 */
#define UART_FRAME_TAIL_SIZE 4 // CRC(2) + 帧尾(2)

    /** 完整帧最小长度 */
#define UART_FRAME_MIN_SIZE (UART_FRAME_HEADER_SIZE + UART_FRAME_TAIL_SIZE)

    /** 完整帧最大长度 */
#define UART_FRAME_MAX_SIZE (UART_FRAME_HEADER_SIZE + UART_FRAME_DATA_MAX_SIZE + UART_FRAME_TAIL_SIZE)

    /* ========== 错误码定义 ========== */

    /**
 * @brief 帧处理错误码
 */
    typedef enum
    {
        UART_FRAME_OK = 0,                    /**< 成功 */
        UART_FRAME_ERR_NULL_POINTER = -1,     /**< 空指针错误 */
        UART_FRAME_ERR_INVALID_PARAM = -2,    /**< 无效参数 */
        UART_FRAME_ERR_BUFFER_TOO_SMALL = -3, /**< 缓冲区太小 */
        UART_FRAME_ERR_DATA_TOO_LARGE = -4,   /**< 数据过大 */
        UART_FRAME_ERR_INVALID_HEADER = -5,   /**< 无效帧头 */
        UART_FRAME_ERR_INVALID_TAIL = -6,     /**< 无效帧尾 */
        UART_FRAME_ERR_CRC_MISMATCH = -7,     /**< CRC校验失败 */
        UART_FRAME_ERR_VERSION_MISMATCH = -8, /**< 版本不匹配 */
        UART_FRAME_ERR_LENGTH_MISMATCH = -9,  /**< 长度不匹配 */
        UART_FRAME_ERR_UNKNOWN = -100         /**< 未知错误 */
    } uart_frame_error_t;

    /* ========== 数据结构定义 ========== */

    /**
 * @brief UART帧头结构
 * @note 使用packed属性确保跨平台内存布局一致性
 */
    typedef struct
    {
        uint8_t magic[2]; /**< 帧头魔数: 0xAA 0x55 */
        uint16_t length;  /**< 数据区长度(小端序) */
        uint8_t type_id;  /**< 数据包类型ID */
        uint8_t version;  /**< 协议版本号 */
    } __attribute__((packed)) uart_frame_header_t;

    /**
 * @brief UART帧尾结构
 */
    typedef struct
    {
        uint16_t crc16;   /**< CRC16校验值(小端序) */
        uint8_t magic[2]; /**< 帧尾魔数: 0x0D 0x0A */
    } __attribute__((packed)) uart_frame_tail_t;

    /**
 * @brief 完整UART帧结构(用于解析)
 * @note 不直接用于打包,仅用于描述帧格式
 */
    typedef struct
    {
        uart_frame_header_t header;             /**< 帧头 */
        uint8_t data[UART_FRAME_DATA_MAX_SIZE]; /**< 数据区 */
        uart_frame_tail_t tail;                 /**< 帧尾 */
    } __attribute__((packed)) uart_frame_t;

    /**
 * @brief 帧解析结果结构
 */
    typedef struct
    {
        uart_frame_type_t type;  /**< 数据包类型 */
        uint8_t version;         /**< 协议版本 */
        uint16_t data_len;       /**< 数据长度 */
        const uint8_t* data_ptr; /**< 数据指针(指向原始缓冲区) */
        bool is_valid;           /**< 是否有效 */
    } uart_frame_parse_result_t;

    /* ========== 核心API函数 ========== */

    /**
 * @brief 打包数据到UART帧
 * @details 将用户数据封装成完整的UART帧格式,自动计算长度和CRC
 *
 * @param type          数据包类型ID
 * @param data          待打包的数据指针
 * @param data_len      数据长度
 * @param frame_buf     输出缓冲区(用于存放完整帧)
 * @param frame_buf_size 输出缓冲区大小
 * @param out_frame_len 输出参数,返回实际打包后的帧长度
 *
 * @return 错误码
 *         - UART_FRAME_OK: 成功
 *         - UART_FRAME_ERR_NULL_POINTER: 空指针
 *         - UART_FRAME_ERR_DATA_TOO_LARGE: 数据过大
 *         - UART_FRAME_ERR_BUFFER_TOO_SMALL: 缓冲区不足
 *
 * @note frame_buf大小至少为 data_len + UART_FRAME_MIN_SIZE
 */
    int32_t uart_frame_pack(uart_frame_type_t type, const void* data, uint16_t data_len,
                            uint8_t* frame_buf, size_t frame_buf_size, uint16_t* out_frame_len);

    /**
 * @brief 解析UART帧
 * @details 解析接收到的完整帧,验证帧头、帧尾、CRC,提取数据
 *
 * @param frame_buf     接收到的完整帧缓冲区
 * @param frame_len     帧缓冲区长度
 * @param result        输出参数,解析结果
 *
 * @return 错误码
 *         - UART_FRAME_OK: 成功
 *         - UART_FRAME_ERR_NULL_POINTER: 空指针
 *         - UART_FRAME_ERR_INVALID_HEADER: 帧头错误
 *         - UART_FRAME_ERR_INVALID_TAIL: 帧尾错误
 *         - UART_FRAME_ERR_CRC_MISMATCH: CRC校验失败
 *         - UART_FRAME_ERR_VERSION_MISMATCH: 版本不匹配
 *         - UART_FRAME_ERR_LENGTH_MISMATCH: 长度不匹配
 *
 * @note result->data_ptr 指向 frame_buf 中的数据区,调用者需确保 frame_buf 生命周期
 */
    int32_t uart_frame_parse(const uint8_t* frame_buf, size_t frame_len,
                             uart_frame_parse_result_t* result);

    /**
 * @brief 在接收缓冲区中查找完整帧
 * @details 在流式接收的数据中查找帧头,并验证帧的完整性
 *
 * @param rx_buf        接收缓冲区
 * @param rx_len        缓冲区中有效数据长度
 * @param frame_start   输出参数,找到的帧起始位置
 * @param frame_length  输出参数,完整帧长度
 *
 * @return 错误码
 *         - UART_FRAME_OK: 找到完整帧
 *         - UART_FRAME_ERR_NULL_POINTER: 空指针
 *         - UART_FRAME_ERR_INVALID_HEADER: 未找到有效帧头
 *         - UART_FRAME_ERR_LENGTH_MISMATCH: 数据不完整
 *
 * @note 此函数仅查找和验证帧结构,不进行CRC校验
 */
    int32_t uart_frame_find(const uint8_t* rx_buf, size_t rx_len, size_t* frame_start,
                            size_t* frame_length);

    /**
 * @brief 验证帧格式(不含CRC校验)
 * @details 快速验证帧头、帧尾、长度字段的正确性
 *
 * @param frame_buf     帧缓冲区
 * @param frame_len     帧长度
 *
 * @return true: 帧格式有效, false: 帧格式无效
 */
    bool uart_frame_validate_format(const uint8_t* frame_buf, size_t frame_len);

    /**
 * @brief 获取错误码描述字符串
 *
 * @param error_code    错误码
 * @return 错误描述字符串
 */
    const char* uart_frame_get_error_string(int32_t error_code);

/* ========== 辅助宏定义 ========== */

/**
 * @brief 计算打包后完整帧的长度
 * @param data_len 数据区长度
 */
#define UART_FRAME_CALC_TOTAL_LEN(data_len) ((data_len) + UART_FRAME_MIN_SIZE)

/**
 * @brief 判断类型ID是否为用户自定义类型
 */
#define UART_FRAME_IS_USER_TYPE(type_id)                                                           \
    ((type_id) >= UART_FRAME_TYPE_USER_DEFINED && (type_id) < UART_FRAME_TYPE_INVALID)

#ifdef __cplusplus
}
#endif

#endif /* UART_FRAME_H */
