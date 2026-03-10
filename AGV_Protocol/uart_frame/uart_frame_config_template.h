/**
 * @file uart_frame_config_template.h
 * @brief UART帧协议用户配置模板
 * @details 将此文件复制到你的项目中,重命名为 uart_frame_config.h 并根据需求修改
 *
 * @warning 必须提供 uart_frame_config.h,否则编译将失败!
 * @note    参考 FreeRTOS 的 FreeRTOSConfig.h、lvgl 的 lv_conf.h 模式
 * 
 * @copyright Copyright (c) 2024-2029, JoyIot
 * @author baicha
 * @date 2025-10-06
 */

#ifndef UART_FRAME_CONFIG_TEMPLATE_H
#define UART_FRAME_CONFIG_TEMPLATE_H

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef __RL78_MCU_DEF
#include <stdint.h>
#endif

#include <stdbool.h>

    /* ========================================================================
     * 1. 帧格式魔数配置
     * ======================================================================== */

    /**
     * @brief 帧头魔数(2字节)
     * @note  用于快速识别帧起始位置,建议使用不易在正常数据中出现的值
     */
#define UART_FRAME_HEADER_MAGIC_0 0xAA
#define UART_FRAME_HEADER_MAGIC_1 0x55

    /**
     * @brief 帧尾魔数(2字节)
     * @note  用于验证帧完整性,建议与帧头魔数不同
     */
#define UART_FRAME_TAIL_MAGIC_0 0x0D
#define UART_FRAME_TAIL_MAGIC_1 0x0A

    /* ========================================================================
     * 2. 协议版本配置
     * ======================================================================== */

    /**
     * @brief 协议版本号
     * @note  用于协议兼容性检查,通信双方必须匹配
     */
#define UART_FRAME_VERSION 0x01

    /* ========================================================================
     * 3. 缓冲区大小配置
     * ======================================================================== */

    /**
     * @brief 数据区最大长度(字节)
     * @note  根据实际应用调整:
     *        - 嵌入式小型设备: 128 - 512 字节
     *        - 一般应用:       512 - 2048 字节
     *        - 大数据传输:     4096+ 字节
     */
#define UART_FRAME_DATA_MAX_SIZE 1024

    /* ========================================================================
     * 4. 数据包类型枚举定义
     * ======================================================================== */

    /**
     * @brief 数据包类型ID枚举
     * @note  这是用户必须自定义的核心配置,定义所有通信数据类型
     *
     * @warning
     *  - 枚举名称必须为 uart_frame_type_t
     *  - 必须包含 UART_FRAME_TYPE_INVALID = 0xFF
     *  - 建议保留 0x00-0x0F 作为系统预留类型
     *  - 用户自定义类型建议从 0x10 或 0x80 开始
     */
    typedef enum
    {
        /* ===== 系统预留类型 (0x00 - 0x0F) ===== */
        UART_FRAME_TYPE_HEARTBEAT = 0x00,   /**< 心跳包 */
        UART_FRAME_TYPE_ACK = 0x01,         /**< 应答确认 */
        UART_FRAME_TYPE_ERROR = 0x02,       /**< 错误信息 */

        /* ===== 用户自定义类型 (0x10 - 0xFE) ===== */
        UART_FRAME_TYPE_SENSOR_DATA = 0x10, /**< 传感器数据 */
        UART_FRAME_TYPE_CONTROL_CMD = 0x11, /**< 控制命令 */
        UART_FRAME_TYPE_STATUS_INFO = 0x12, /**< 状态信息 */
        UART_FRAME_TYPE_CONFIG_PARAM = 0x13, /**< 配置参数 */

        /* 示例: 添加更多自定义类型
        UART_FRAME_TYPE_IMAGE_DATA   = 0x20,
        UART_FRAME_TYPE_AUDIO_DATA   = 0x21,
        UART_FRAME_TYPE_LOG_MESSAGE  = 0x30,
        */

        /* ===== 必需: 无效类型标记 ===== */
        UART_FRAME_TYPE_INVALID = 0xFF /**< 无效类型(必需) */

    } uart_frame_type_t;

    /* ========================================================================
     * 5. 用户自定义数据结构示例
     * ======================================================================== */

    /**
     * @brief 数据结构设计规范
     *
     * 1. 必须使用 __attribute__((packed)) 确保跨平台内存布局一致
     * 2. 只使用固定大小类型: uint8_t, uint16_t, uint32_t, int8_t, int16_t, int32_t
     * 3. 禁止使用平台相关类型: int, long, size_t, 指针等
     * 4. 多字节数据统一使用小端序(如需大端序可在应用层转换)
     * 5. 所有字段必须明确注释其含义和单位
     */

    /* ----- 示例1: 心跳包 ----- */
    typedef struct
    {
        uint32_t counter;   /**< 心跳计数器 */
        uint32_t timestamp; /**< 时间戳(ms) */
    } __attribute__((packed)) heartbeat_packet_t;

    /* ----- 示例2: 传感器数据 ----- */
    typedef struct
    {
        uint32_t timestamp;  /**< 时间戳(ms) */
        int16_t temperature; /**< 温度(0.1℃, -327.68 ~ 327.67℃) */
        int16_t humidity;    /**< 湿度(0.1%, 0 ~ 100.0%) */
        uint16_t pressure;   /**< 气压(Pa) */
    } __attribute__((packed)) sensor_data_t;

    /* ----- 示例3: 控制命令 ----- */
    typedef struct
    {
        uint8_t motor_id;        /**< 电机ID(1-255) */
        uint8_t direction;       /**< 方向: 0=停止, 1=正转, 2=反转 */
        uint16_t speed;          /**< 速度(RPM, 0-65535) */
        int32_t target_position; /**< 目标位置(脉冲数) */
    } __attribute__((packed)) motor_control_cmd_t;

    /* ----- 示例4: 应答包 ----- */
    typedef struct
    {
        uint8_t ack_type;     /**< 应答类型: 对应原请求的类型ID */
        uint8_t result;       /**< 结果: 0=成功, 1=失败, 2=超时 */
        uint16_t error_code;  /**< 错误码(成功时为0) */
        uint32_t sequence_id; /**< 序列号(与请求匹配) */
    } __attribute__((packed)) ack_packet_t;

    /* ----- 示例5: 错误信息包 ----- */
    typedef struct
    {
        uint8_t error_level; /**< 错误级别: 0=警告, 1=错误, 2=严重 */
        uint16_t error_code; /**< 错误码 */
        uint32_t timestamp;  /**< 发生时间(ms) */
        uint8_t module_id;   /**< 模块ID */
    } __attribute__((packed)) error_info_t;

    /* ========================================================================
     * 6. 可选: 调试配置
     * ======================================================================== */

    /**
     * @brief 启用调试打印(可选)
     * @note  定义后将启用内部调试日志输出,需要提供 printf 函数
     */
// #define UART_FRAME_ENABLE_DEBUG

    /**
     * @brief 启用统计信息(可选)
     * @note  定义后将启用帧统计功能(发送/接收计数、错误统计等)
     */
// #define UART_FRAME_ENABLE_STATISTICS

#ifdef __cplusplus
}
#endif

#endif /* UART_FRAME_CONFIG_TEMPLATE_H */
