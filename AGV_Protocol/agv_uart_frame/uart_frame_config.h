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

#include <stdint.h>
#include <stdbool.h>
#include <float.h>

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
        /* ===== 用户自定义类型 (0x10 - 0xFE) ===== */
        UART_FRAME_TYPE_SENSOR_DATA = 0x10,     /**< 传感器数据 */
        UART_FRAME_TYPE_CONTROL_CMD = 0x11,     /**< 控制命令 */
	UART_FRAME_TYPE_COMMON_REPLY = 0x12,    /**< 通用回复 - 新增 */
        UART_FRAME_TYPE_STATUS_FEEDBACK = 0x14, /**< 状态反馈 */

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

// ========== 数据无效性标志位定义（无效=1，有效=0）==========
#define DATA_INVALID_UWB (1 << 0)  // 0x01 - UWB数据无效
#define DATA_INVALID_IMU (1 << 1)  // 0x02 - IMU数据无效
#define DATA_INVALID_UDS1 (1 << 2) // 0x04 - UDS1数据无效
#define DATA_INVALID_UDS2 (1 << 3) // 0x08 - UDS2数据无效
#define DATA_ALL_INVALID 0x0F      // 全部传感器无效

#define IMU_ARRAY_NUM 5

    // UART_FRAME_TYPE_SENSOR_DATA
    typedef struct // size 8
    {
        uint32_t frame_id;    // 传感器数据包序号（独立计数，不依赖UWB）
        uint32_t valid_flags; // 数据有效性位标志

        // ===== UWB 定位数据 =====
        struct // size 20
        {
            uint32_t packid; // 包ID
            int32_t mode;    // 模式
            int32_t x;       // X坐标
            int32_t y;       // Y坐标
            int32_t rate;    // 速率
        } uwb;               // 包ID、模式、X、Y、速率

        // ===== IMU 姿态数据 =====
        struct // size (20 + 4 + 36) * 5 =
        {
            // ===== 原始数据 =====
            int16_t gyro_raw[3];  // 陀螺仪原始值 [X, Y, Z]
            int16_t accel_raw[3]; // 加速度计原始值 [X, Y, Z]
            int16_t mag_raw[3];   // 磁力计原始值 [X, Y, Z]
            int16_t reserved;     // 保留位，填充字节
            float temperature;    // 温度 (°C, 来自陀螺仪)

            float accel_x, accel_y, accel_z;       // 加速度 (m/s²)
            float roll_6dof, pitch_6dof, yaw_6dof; // 6轴姿态（陀螺仪+加速度计）
            float roll_9dof, pitch_9dof, yaw_9dof; // 9轴姿态（加入磁力计修正）
        } imu[IMU_ARRAY_NUM];

        // ===== 超声波雷达数据 =====
        struct // size 4
        {
            uint16_t distance1_mm; // 雷达1距离（毫米）
            uint16_t distance2_mm; // 雷达2距离（毫米）
        } uds;
    } __attribute__((packed)) sensor_data_t;

    // UART_FRAME_TYPE_CONTROL_CMD
    typedef int8_t pwm_duty_t;

    typedef struct
    {
        struct
        {
            // 负角度为顺时针，正角度为逆时针，0°为车头朝向正前方
            float angle_left;
            float angle_right;
            float angle_camera;
        } angles; // 舵轮角度

        struct
        {
            uint32_t chassis_lock : 1; // 底盘锁，0为解锁，1为锁定
            uint32_t motor0_break : 1; // 电机刹车，0为解锁，1为锁定
            uint32_t motor1_break : 1; // 电机刹车，0为解锁，1为锁定
            uint32_t diff_locked : 1;  // 差速锁，0为解锁，1为锁定
#ifdef UART_PID_DEBUG
            uint32_t pid_valid : 1; // pid 有效性，0 为无效，1 为有效
#endif                              // UART_PID_DEBUG
        } flags;

        struct
        {
            pwm_duty_t left;
            pwm_duty_t right;
            uint16_t reserved; // 四字节对齐
            uint32_t pulses;   // 脉冲数
#ifdef UART_PID_DEBUG
            float kp;
            float ki;
            float kd;
#endif               // UART_PID_DEBUG
        } motor_rpm; // 电机转速

        struct
        {
            uint32_t red_mode : 2;      // RED LED模式: 00=OFF, 01=ON, 10=BLINK
            uint32_t blue_mode : 2;     // BLUE LED模式: 00=OFF, 01=ON, 10=BLINK
            uint32_t defenses_mode : 2; // DEFENSES LED模式: 00=OFF, 01=ON, 10=BLINK
            uint32_t flow_enable : 1;   // 流水模式: 0=禁用, 1=启用
            uint32_t period_ms : 12;    // 闪烁周期(ms), 0-4095, 0表示不更新
            uint32_t duty_cycle : 7;    // 占空比(0-100)
            uint32_t reserved : 6;      // 保留位
        } led_ctrl;                     // LED控制

        // uint8_t pad[64]; // 填充
    } __attribute__((packed)) ctrl_frame_t;

    // UART_FRAME_TYPE_STATUS_FEEDBACK
    typedef struct
    {
        struct
        {
            // 负角度为顺时针，正角度为逆时针，0°为车头朝向正前方
            float angle_left;
            float angle_right;
            float angle_camera;
        } angles; // 舵轮角度

        struct
        {
            uint32_t chassis_lock : 1; // 底盘锁，0为解锁，1为锁定
            uint32_t motor0_break : 1; // 电机刹车，0为解锁，1为锁定
            uint32_t motor1_break : 1; // 电机刹车，0为解锁，1为锁定
            uint32_t diff_locked : 1;  // 差速锁，0为解锁，1为锁定
#ifdef UART_PID_DEBUG
            uint32_t pid_valid : 1; // pid 有效性，0 为无效，1 为有效
#endif                              // UART_PID_DEBUG
        } flags;

        struct
        {
            pwm_duty_t left;
            pwm_duty_t right;
            uint16_t reserved;      // 四字节对齐
            uint32_t remain_pulses; // 剩余脉冲数
#ifdef UART_PID_DEBUG
            float kp;
            float ki;
            float kd;
#endif                        // UART_PID_DEBUG
            float left_laps;  // 左轮圈数
            float right_laps; // 右轮圈数
        } motor_rpm;          // 电机转速

        struct
        {
            uint32_t red_mode : 2;      // RED LED模式: 00=OFF, 01=ON, 10=BLINK
            uint32_t blue_mode : 2;     // BLUE LED模式: 00=OFF, 01=ON, 10=BLINK
            uint32_t defenses_mode : 2; // DEFENSES LED模式: 00=OFF, 01=ON, 10=BLINK
            uint32_t flow_enable : 1;   // 流水模式: 0=禁用, 1=启用
            uint32_t period_ms : 12;    // 闪烁周期(ms), 0-4095, 0表示不更新
            uint32_t duty_cycle : 7;    // 占空比(0-100)
            uint32_t reserved : 6;      // 保留位
        } led_ctrl;                     // LED控制

        uint8_t result; // 执行结果: 0=成功, 1=失败
    } __attribute__((packed)) status_feedback_t;

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
