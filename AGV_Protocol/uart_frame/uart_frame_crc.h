/**
 * @file uart_frame_crc.h
 * @brief UART帧CRC16校验模块
 * @details 提供CRC16-CCITT校验算法实现,用于UART帧数据完整性验证
 *
 * @copyright Copyright (c) 2024-2029, JoyIot
 * @author baicha
 * @date 2025-10-06
 */

#ifndef UART_FRAME_CRC_H
#define UART_FRAME_CRC_H

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef __RL78_MCU_DEF
#include <stdint.h>
#else
#include "hal_uart.h"
#endif
#include <stddef.h>
#include <stdbool.h>

/* ========== CRC算法配置 ========== */

/** CRC16-CCITT多项式 */
#define CRC16_POLYNOMIAL 0x1021

/** CRC16初始值 */
#define CRC16_INIT_VALUE 0xFFFF

    /* ========== CRC计算函数 ========== */

    /**
 * @brief 计算CRC16-CCITT校验值
 * @details 使用查表法快速计算CRC16,适用于嵌入式系统
 *
 * @param data      数据指针
 * @param length    数据长度
 * @return CRC16校验值
 *
 * @note 采用CRC16-CCITT标准算法(多项式0x1021, 初始值0xFFFF)
 */
    uint16_t uart_frame_crc16_calculate(const uint8_t* data, size_t length);

    /**
 * @brief 验证CRC16校验值
 * @details 计算数据的CRC16并与期望值比对
 *
 * @param data          数据指针
 * @param length        数据长度
 * @param expected_crc  期望的CRC值
 * @return true: 校验通过, false: 校验失败
 */
    bool uart_frame_crc16_verify(const uint8_t* data, size_t length, uint16_t expected_crc);

#ifdef __cplusplus
}
#endif

#endif /* UART_FRAME_CRC_H */
