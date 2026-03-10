# UART通用帧协议模块

## 📘 概述

本模块提供了一套完整的跨平台UART通信帧协议,支持RT-Thread、Linux等不同操作系统和MCU平台。通过统一的帧格式和健壮的CRC校验,确保多设备间UART通信的可靠性和一致性。

**版本**: v1.0.0
**更新日期**: 2025-10-06

---

## ✨ 核心特性

- ✅ **跨平台兼容**: 支持RT-Thread、Linux、裸机等多种平台
- ✅ **健壮的帧格式**: 帧头/帧尾魔数 + CRC16校验 + 版本控制
- ✅ **紧凑内存布局**: 使用`__attribute__((packed))`确保平台无关性
- ✅ **流式数据处理**: 提供帧查找功能,适配UART流式接收
- ✅ **完整API**: 打包、解析、查找、验证等核心功能
- ✅ **通用设计**: 类似 lvgl/FreeRTOS 的配置模式,框架与业务逻辑完全解耦

---

## 🎯 设计理念

本模块采用 **配置文件驱动** 的设计模式,类似于:
- FreeRTOS 的 `FreeRTOSConfig.h`
- lvgl 的 `lv_conf.h`

**核心原则**:
- 框架代码不包含任何业务相关的硬编码
- 用户必须提供 `uart_frame_config.h` 配置文件
- 如果配置缺失或不完整,编译将失败并给出明确错误提示

---

## 📦 文件结构

```
src/libs/uart_frame/
├── uart_frame.h                    # 主头文件(框架API)
├── uart_frame.c                    # 核心功能实现
├── uart_frame_crc.h                # CRC16校验头文件
├── uart_frame_crc.c                # CRC16实现(查表法)
├── uart_frame_config_template.h    # 配置模板(复制后修改)
├── uart_frame_config.h             # 用户配置文件(你的项目)
├── README.md                       # 本文档
└── example.md                      # 示例代码
```

**说明**:
- `uart_frame_config_template.h`: 模板文件,不要直接修改
- `uart_frame_config.h`: 复制模板后创建,根据你的项目需求配置

---

## 📐 帧格式定义

### 完整帧结构

```
┌──────────┬──────────┬──────────┬──────────┬──────────┬──────────┬──────────┐
│  帧头    │  长度    │  类型ID  │  版本号  │  数据区  │  CRC16   │  帧尾    │
│ (2字节)  │ (2字节)  │ (1字节)  │ (1字节)  │ (N字节)  │ (2字节)  │ (2字节)  │
├──────────┼──────────┼──────────┼──────────┼──────────┼──────────┼──────────┤
│ 0xAA 0x55│  小端序  │  类型    │  0x01    │  数据    │  小端序  │ 0x0D 0x0A│
└──────────┴──────────┴──────────┴──────────┴──────────┴──────────┴──────────┘
```

**⚠️ 注意**: 上述魔数、版本号等值仅为示例,实际值由 `uart_frame_config.h` 定义。

### 字段说明

| 字段     | 长度   | 说明                                |
|----------|--------|-------------------------------------|
| 帧头     | 2字节  | 魔数,用于帧同步(用户配置)          |
| 长度     | 2字节  | 数据区长度(小端序,不含帧头/帧尾)   |
| 类型ID   | 1字节  | 数据包类型标识(用户定义枚举)       |
| 版本号   | 1字节  | 协议版本(用户配置)                 |
| 数据区   | N字节  | 实际传输的结构体数据               |
| CRC16    | 2字节  | CRC16-CCITT校验值(小端序)          |
| 帧尾     | 2字节  | 魔数,标识帧结束(用户配置)          |

### CRC计算范围

CRC16覆盖范围: **帧头 + 长度 + 类型ID + 版本号 + 数据区** (不包含CRC和帧尾本身)

---

## 🔧 快速开始

### 步骤1: 创建配置文件

**第一步**: 复制模板文件

```bash
cp uart_frame_config_template.h <你的项目目录>/uart_frame_config.h
```

**第二步**: 编辑 `uart_frame_config.h`,根据你的需求修改:

```c
/* 示例配置 */

/* 1. 定义帧头/帧尾魔数 */
#define UART_FRAME_HEADER_MAGIC_0 0xAA
#define UART_FRAME_HEADER_MAGIC_1 0x55
#define UART_FRAME_TAIL_MAGIC_0   0x0D
#define UART_FRAME_TAIL_MAGIC_1   0x0A

/* 2. 定义协议版本 */
#define UART_FRAME_VERSION 0x01

/* 3. 定义数据区最大长度 */
#define UART_FRAME_DATA_MAX_SIZE 1024

/* 4. 定义数据包类型枚举 */
typedef enum {
    UART_FRAME_TYPE_HEARTBEAT = 0x00,
    UART_FRAME_TYPE_SENSOR_DATA = 0x10,
    UART_FRAME_TYPE_CONTROL_CMD = 0x11,
    // ... 添加更多类型 ...
    UART_FRAME_TYPE_INVALID = 0xFF  // 必需
} uart_frame_type_t;

/* 5. 定义你的数据结构 */
typedef struct {
    uint32_t timestamp;
    int16_t temperature;
    int16_t humidity;
} __attribute__((packed)) sensor_data_t;
```

**第三步**: 确保编译器能找到 `uart_frame_config.h`

在你的项目编译配置中添加 include 路径:

```makefile
# Makefile 示例
INCLUDES += -I<你的项目配置文件所在目录>
INCLUDES += -Isrc/libs/uart_frame
```

或者将 `uart_frame_config.h` 放在与 `uart_frame.h` 同级目录。

---

### 步骤2: 引入模块

```c
#include "uart_frame.h"  /* 会自动引入 uart_frame_config.h */
```

**⚠️ 重要**: 如果未提供 `uart_frame_config.h` 或配置不完整,编译将失败并显示:

```
error: uart_frame_config.h must define UART_FRAME_HEADER_MAGIC_0
```

### 3. 基本使用示例

#### 发送端: 打包数据

```c
#include "uart_frame.h"

// 准备要发送的数据
sensor_data_t sensor = {
    .timestamp = 12345,
    .temperature = 256,  // 25.6℃
    .humidity = 650,     // 65.0%
    .pressure = 101325,
    .accel_x = 100,
    .accel_y = -50,
    .accel_z = 980
};

// 打包缓冲区
uint8_t tx_buf[128];
uint16_t frame_len;

// 打包成帧
int32_t ret = uart_frame_pack(
    UART_FRAME_TYPE_SENSOR_DATA,
    &sensor,
    sizeof(sensor),
    tx_buf,
    sizeof(tx_buf),
    &frame_len
);

if (ret == UART_FRAME_OK) {
    // 通过UART发送
    uart_send(tx_buf, frame_len);
}
```

#### 接收端: 解析数据

```c
#include "uart_frame.h"

// 假设已接收到完整帧
uint8_t rx_buf[128];
size_t rx_len = 50; // 实际接收长度

// 查找完整帧
size_t frame_start, frame_length;
if (uart_frame_find(rx_buf, rx_len, &frame_start, &frame_length) == UART_FRAME_OK) {

    // 解析帧
    uart_frame_parse_result_t result;
    if (uart_frame_parse(&rx_buf[frame_start], frame_length, &result) == UART_FRAME_OK) {

        // 根据类型ID处理数据
        if (result.type == UART_FRAME_TYPE_SENSOR_DATA) {
            sensor_data_t* sensor = (sensor_data_t*)result.data_ptr;
            printf("Temperature: %.1f\n", sensor->temperature / 10.0);
        }
    }
}
```

---

## 🚀 API 参考

### 核心函数

#### `uart_frame_pack()`

**功能**: 打包数据到UART帧

```c
int32_t uart_frame_pack(
    uart_frame_type_t type,      // 数据包类型ID
    const void* data,             // 待打包数据指针
    uint16_t data_len,            // 数据长度
    uint8_t* frame_buf,           // 输出缓冲区
    size_t frame_buf_size,        // 缓冲区大小
    uint16_t* out_frame_len       // 输出: 实际帧长度
);
```

**返回值**:
- `UART_FRAME_OK`: 成功
- `UART_FRAME_ERR_NULL_POINTER`: 空指针错误
- `UART_FRAME_ERR_DATA_TOO_LARGE`: 数据过大
- `UART_FRAME_ERR_BUFFER_TOO_SMALL`: 缓冲区不足

---

#### `uart_frame_parse()`

**功能**: 解析UART帧

```c
int32_t uart_frame_parse(
    const uint8_t* frame_buf,            // 完整帧缓冲区
    size_t frame_len,                    // 帧长度
    uart_frame_parse_result_t* result    // 输出: 解析结果
);
```

**返回值**:
- `UART_FRAME_OK`: 成功
- `UART_FRAME_ERR_INVALID_HEADER`: 帧头错误
- `UART_FRAME_ERR_INVALID_TAIL`: 帧尾错误
- `UART_FRAME_ERR_CRC_MISMATCH`: CRC校验失败
- `UART_FRAME_ERR_VERSION_MISMATCH`: 版本不匹配

---

#### `uart_frame_find()`

**功能**: 在流式数据中查找完整帧

```c
int32_t uart_frame_find(
    const uint8_t* rx_buf,      // 接收缓冲区
    size_t rx_len,              // 有效数据长度
    size_t* frame_start,        // 输出: 帧起始位置
    size_t* frame_length        // 输出: 完整帧长度
);
```

**用途**: 适用于UART流式接收,自动查找帧头并验证帧完整性

---

#### `uart_frame_validate_format()`

**功能**: 快速验证帧格式(不含CRC校验)

```c
bool uart_frame_validate_format(
    const uint8_t* frame_buf,
    size_t frame_len
);
```

---

## 📋 数据类型定义规范

### ⚠️ 必须遵守的规则

1. **使用固定大小类型**
   ```c
   ✅ 正确: uint8_t, uint16_t, uint32_t, int8_t, int16_t, int32_t
   ❌ 错误: int, long, size_t, unsigned int
   ```

2. **必须使用packed属性**
   ```c
   typedef struct {
       uint32_t field1;
       uint16_t field2;
   } __attribute__((packed)) my_data_t;
   ```

3. **禁止使用平台相关类型**
   ```c
   ❌ 禁止: 指针、文件描述符、函数指针、枚举(作为字段)
   ```

4. **多字节数据统一小端序**
   ```c
   // 如需大端序,应用层自行转换
   uint16_t value = 0x1234;  // 在帧中表示为 0x34 0x12
   ```

### 示例: 自定义结构体

```c
/**
 * @brief 电机状态数据
 * @note 对应类型ID: UART_FRAME_TYPE_USER_DEFINED + 0x01
 */
typedef struct {
    uint8_t  motor_id;          /**< 电机ID */
    uint8_t  status;            /**< 状态: 0=停止, 1=运行, 2=故障 */
    uint16_t speed;             /**< 当前速度(RPM) */
    int32_t  position;          /**< 当前位置(脉冲数) */
    uint16_t current;           /**< 电流(mA) */
    uint16_t voltage;           /**< 电压(mV) */
} __attribute__((packed)) motor_status_t;
```

---

## 🛠️ 扩展模块

### 添加自定义类型ID

在 `uart_frame.h` 中扩展 `uart_frame_type_t` 枚举:

```c
typedef enum {
    // ... 系统预定义类型 ...

    UART_FRAME_TYPE_USER_DEFINED    = 0x80,

    // 用户自定义类型
    UART_FRAME_TYPE_MOTOR_STATUS    = 0x80,
    UART_FRAME_TYPE_GPS_DATA        = 0x81,
    UART_FRAME_TYPE_IMU_DATA        = 0x82,
    // ...

} uart_frame_type_t;
```

### 流式接收完整示例

```c
#define RX_BUF_SIZE 512
uint8_t rx_buffer[RX_BUF_SIZE];
size_t rx_pos = 0;

// UART中断或轮询接收
void uart_rx_handler(uint8_t byte) {
    if (rx_pos < RX_BUF_SIZE) {
        rx_buffer[rx_pos++] = byte;
    }

    // 尝试查找完整帧
    size_t frame_start, frame_len;
    if (uart_frame_find(rx_buffer, rx_pos, &frame_start, &frame_len) == UART_FRAME_OK) {

        // 解析帧
        uart_frame_parse_result_t result;
        if (uart_frame_parse(&rx_buffer[frame_start], frame_len, &result) == UART_FRAME_OK) {
            // 处理数据
            process_frame(&result);
        }

        // 移除已处理帧
        size_t remaining = rx_pos - (frame_start + frame_len);
        memmove(rx_buffer, &rx_buffer[frame_start + frame_len], remaining);
        rx_pos = remaining;
    }

    // 缓冲区溢出处理
    if (rx_pos >= RX_BUF_SIZE - 10) {
        rx_pos = 0;  // 丢弃数据
    }
}
```

---

## ⚠️ 注意事项

### 1. 配置文件管理

**推荐做法**:
- 将 `uart_frame_config.h` 纳入版本控制
- 不同项目使用不同的配置文件
- 通信双方必须使用相同的配置

**错误做法**:
- ❌ 直接修改 `uart_frame_config_template.h`
- ❌ 多个项目共享配置但使用不同魔数

---

### 2. 禁止在结构体中使用可变长度字段

```c
❌ 错误做法:
typedef struct {
    uint16_t data_len;
    uint8_t  data[];  // 柔性数组成员,不推荐
} __attribute__((packed)) bad_struct_t;
```

**正确做法**: 先发送固定头,后续通过多帧或分片发送可变数据。

---

### 3. 禁止跨越字节边界访问

```c
❌ 错误做法:
uint32_t* ptr = (uint32_t*)(&rx_buffer[1]);  // 未对齐访问
```

**原因**: 某些平台(如ARM Cortex-M0)不支持非对齐访问,会触发硬件异常。

---

### 4. 禁止使用浮点数直接传输

```c
❌ 错误做法:
typedef struct {
    float temperature;  // 浮点数表示在不同平台可能不同
} __attribute__((packed)) bad_float_t;

✅ 正确做法:
typedef struct {
    int16_t temperature;  // 使用整数,放大倍数(如×10)
} __attribute__((packed)) good_temp_t;
```

---

## 🐛 常见问题

### Q1: CRC校验总是失败?

**可能原因**:
1. 发送端和接收端字节序不一致
2. 结构体未使用 `__attribute__((packed))`
3. 多字节字段(如`uint16_t`)平台填充不同

**解决方法**:
- 确保双方使用相同的模块代码
- 验证结构体大小: `sizeof(your_struct_t)`
- 使用 `uart_frame_validate_format()` 先验证格式

---

### Q2: 如何处理UART接收超时?

**建议**:
```c
// 设置UART接收超时(如50ms)
if (timeout_occurred) {
    // 尝试解析已接收数据
    size_t start, len;
    if (uart_frame_find(rx_buf, rx_pos, &start, &len) != UART_FRAME_OK) {
        // 未找到完整帧,清空缓冲区
        rx_pos = 0;
    }
}
```

---

### Q3: 数据量大时如何优化?

**优化建议**:
1. 减小数据包大小,分片传输
2. 调整 `UART_FRAME_DATA_MAX_SIZE` 限制
3. 使用DMA传输减少CPU占用
4. 考虑压缩算法(如需要)

---

## 📚 参考资料

- **设计文档**: `docs/agv_commu.md`
- **示例代码**: `src/libs/uart_frame/example.md`
- **CRC算法**: CRC16-CCITT (多项式0x1021)

---

## 📝 版本历史

| 版本   | 日期       | 说明                |
|--------|------------|---------------------|
| v1.0.0 | 2025-10-06 | 初始版本,完整功能   |

---

## 📧 支持

如有问题或建议,请联系项目维护者或提交Issue。

---

**许可**: 本模块遵循项目整体许可协议。
