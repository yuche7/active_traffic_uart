/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "uart_loopback_config.h"
#include "uart_frame.h"
#include "stdio.h"
#if UART_LOOPBACK_TEST_ENABLE
#include "uart_loopback_test.h"
#endif
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
#define RX_BUFFER_SIZE 2048
uint8_t rx_buffer[RX_BUFFER_SIZE];
uint16_t rx_index = 0;

// Status feedback data
status_feedback_t status_feedback;
volatile uint8_t status_feedback_ready = 0;

// Timing variables
uint32_t last_status_query_time = 0;
uint32_t status_query_interval = 1000; // Query every 1 seconds

// Sensor data
sensor_data_t sensor_data;
volatile uint8_t uwb_data_ready = 0;

// UWB data
int32_t uwb_x = 0;
int32_t uwb_y = 0;
uint32_t uwb_frame_count = 0;
uint8_t uwb_data_valid = 0;

// Debug counters
uint32_t interrupt_count = 0;
uint32_t byte_count = 0;
uint32_t frame_count = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
void turn_left(void);
/* USER CODE BEGIN PFP */
void Process_UWB_Data(void);
void Send_Control_Command(void); // Declare control command sending function
void Process_Status_Feedback(void);
void Query_Status_Feedback(void);
void parse_received_frame(const uint8_t* frame_data, size_t frame_len);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_USART1_UART_Init();
  MX_USART3_UART_Init();

  /* USER CODE BEGIN 2 */
  // Enable UART1 global interrupt in NVIC
  HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(USART1_IRQn);
  
  // Start USART1 receive interrupt (关键：必须开启接收中断)
  HAL_UART_Receive_IT(&huart1, &rx_buffer[rx_index], 1);
  
  printf("UART1 interrupt started. System ready.\r\n");
  printf("Will query status every %lu ms.\r\n\r\n", status_query_interval);
  
  Send_Control_Command();
  HAL_Delay(200); // 2 秒延时，0.5Hz 发送频率
    
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
   
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
#if UART_LOOPBACK_TEST_ENABLE
#if (UART_LOOPBACK_HARDWARE_ENABLE == 1)
    // 硬件回环模式：执行UART自收自发测试
    if (uart_loopback_test_execute())
    {
      // 测试通过 - 绿灯闪烁3次
      for (int i = 0; i < 3; i++)
      {
        led_on(led_g);
        HAL_Delay(100);
        led_off(led_g);
        HAL_Delay(100);
      }
    }
    else
    {
      // 测试失败 - 红灯长亮2秒
      led_on(led_r);
      HAL_Delay(2000);
      led_off(led_r);
    }
    HAL_Delay(UART_LOOPBACK_TEST_INTERVAL_MS);
#else
    // 硬件回环开关关闭：走正常AGV控制发送逻辑
    Send_Control_Command();
    HAL_Delay(2000); // 2 秒延时，0.5Hz 发送频率
#endif
#else
    // 未启用回环测试编译：走正常AGV控制发送逻辑
  
    if (status_feedback_ready) {
      status_feedback_ready = 0;
      Process_Status_Feedback();
    } 

    //Periodically query status feedback
    /*uint32_t current_time = HAL_GetTick();
    if (current_time - last_status_query_time >= status_query_interval) {
      printf("\r\n--- Querying Status Feedback ---\r\n");
      Query_Status_Feedback();
      last_status_query_time = current_time;
    }*/

     // Process UWB data
    if (uwb_data_ready) {
      uwb_data_ready = 0;
      Process_UWB_Data();
    }
    
#endif
  }
  /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Supply configuration update enable
   */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /** Configure the main internal regulator output voltage
   */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

  while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY))
  {
  }

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 5;
  RCC_OscInitStruct.PLL.PLLN = 192;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2 | RCC_CLOCKTYPE_D3PCLK1 | RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */


/**
 * @brief Send control command for AGV to move straight
 */
void Send_Control_Command(void)
{
  // Construct control command
  ctrl_frame_t ctrl = {0};
  printf("in the send control command status\n");  // Set angles (straight)
  ctrl.angles.angle_left = 0.0f;
  ctrl.angles.angle_right = 0.0f;
  ctrl.angles.angle_camera = 0.0f;

  // Set flags
  ctrl.flags.chassis_lock = 0; // Unlock, allow movement
  ctrl.flags.motor0_break = 0; // Left motor brake release
  ctrl.flags.motor1_break = 0; // Right motor brake release
  ctrl.flags.diff_locked = 0;  // Differential lock off

  // Set motor parameters
  ctrl.motor_rpm.left = 0;    // Left wheel PWM, 30% speed
  ctrl.motor_rpm.right = 80;   // Right wheel PWM, 30% speed
  ctrl.motor_rpm.pulses = 930; // Target pulse count (1 revolution, approx. 0.471m)

  // Set LED control
  ctrl.led_ctrl.red_mode = 0;      // OFF
  ctrl.led_ctrl.blue_mode = 0;     // OFF
  ctrl.led_ctrl.defenses_mode = 0; // OFF
  ctrl.led_ctrl.flow_enable = 0;   // Turn off flowing light effect
  ctrl.led_ctrl.period_ms = 0;     // Blink period 0ms
  ctrl.led_ctrl.duty_cycle = 0;    // Duty cycle 0%

  // Pack into protocol frame
  uint8_t tx_buf[256];
  uint16_t frame_len = 0;

  int32_t rc = uart_frame_pack(
      UART_FRAME_TYPE_CONTROL_CMD, // Control command type
      (uint8_t *)&ctrl,            // Control command data
      sizeof(ctrl_frame_t),        // Data length
      tx_buf,                      // Output buffer
      sizeof(tx_buf),              // Buffer size
      &frame_len                   // Output frame length
  );

  if (rc == UART_FRAME_OK && frame_len > 0)
  {
    printf("Control command packed successfully, frame length: %u\r\n", frame_len);

    // Display the first 16 bytes in hexadecimal
    printf("Frame data (hex): ");
    for (int i = 0; i < frame_len; i++)
    {
      printf("%02X ", tx_buf[i]);
      if ((i + 1) % 16 == 0)
        printf("\r\n");
    }
    printf("\r\n");

    // Send control command frame
    HAL_StatusTypeDef tx_status = HAL_UART_Transmit(&huart3, tx_buf, frame_len, 1000);

    if (tx_status == HAL_OK)
    {
      printf("Control command sent successfully!\r\n");

      // LED blink indicates successful transmission
      for (int i = 0; i < 3; i++)
      {
        led_on(led_g);
        HAL_Delay(100);
        led_off(led_g);
        HAL_Delay(100);
      }
    }
    else
    {
      printf("Transmission failed: %d\r\n", tx_status);

      // LED steady on indicates transmission failure
      led_on(led_r);
      HAL_Delay(2000);
      led_off(led_r);
    }
  }
  else
  {
    printf("Packing failed: %ld\r\n", (long)rc);

    // LED fast blinking indicates packing failure
    for (int i = 0; i < 5; i++)
    {
      led_on(led_r);
      HAL_Delay(200);
      led_off(led_r);
      HAL_Delay(200);
    }
  }
}


/**
  * @brief Query status feedback from AGV
  * @note Based on the document: "查询STATUS_FEEDBACK(请求体全零)"
  */
void Query_Status_Feedback(void)
{
  // 创建全零的请求体
  status_feedback_t req = {0};
  
  uint8_t tx_buf[256];
  uint16_t frame_len = 0;
  
  int32_t rc = uart_frame_pack(
      UART_FRAME_TYPE_STATUS_FEEDBACK,
      (uint8_t*)&req,
      sizeof(status_feedback_t),
      tx_buf,
      sizeof(tx_buf),
      &frame_len
  );
  
  if (rc == UART_FRAME_OK && frame_len > 0) {
    printf("Status query packed successfully, sending...\r\n");
    
    // Send via USART3
    HAL_StatusTypeDef tx_status = HAL_UART_Transmit(&huart3, tx_buf, frame_len, 1000);
    
    if (tx_status == HAL_OK) {
      printf("Status query sent successfully via USART3!\r\n");
    } else {
      printf("Status query transmission failed: %d\r\n", tx_status);
    }
  } else {
    printf("Status query packing failed: %ld\r\n", (long)rc);
  }
}

/**
  * @brief Process status feedback data
  * @note Print detailed status information to USART2
  */
void Process_Status_Feedback(void)
{
  printf("\r\n=== STATUS FEEDBACK RECEIVED ===\r\n");
  
  // 1. Angles
  printf("1. Current Angles:\r\n");
  printf("   Left wheel: %.1f°\r\n", status_feedback.angles.angle_left);
  printf("   Right wheel: %.1f°\r\n", status_feedback.angles.angle_right);
  printf("   Camera: %.1f°\r\n", status_feedback.angles.angle_camera);
  
  // 2. Flags
  printf("2. System Flags:\r\n");
  printf("   Chassis lock: %s\r\n", status_feedback.flags.chassis_lock ? "LOCKED" : "UNLOCKED");
  printf("   Left motor brake: %s\r\n", status_feedback.flags.motor0_break ? "ON" : "OFF");
  printf("   Right motor brake: %s\r\n", status_feedback.flags.motor1_break ? "ON" : "OFF");
  printf("   Differential lock: %s\r\n", status_feedback.flags.diff_locked ? "ON" : "OFF");
  
  // 3. Motor status
  printf("3. Motor Status:\r\n");
  printf("   Left PWM: %d (actual)\r\n", status_feedback.motor_rpm.left);
  printf("   Right PWM: %d (actual)\r\n", status_feedback.motor_rpm.right);
  printf("   Remaining pulses: %lu\r\n", status_feedback.motor_rpm.remain_pulses);
  printf("   Left wheel laps: %.2f\r\n", status_feedback.motor_rpm.left_laps);
  printf("   Right wheel laps: %.2f\r\n", status_feedback.motor_rpm.right_laps);
  
  // 4. LED control
  printf("4. LED Control:\r\n");
  printf("   Red LED mode: %u\r\n", status_feedback.led_ctrl.red_mode);
  printf("   Blue LED mode: %u\r\n", status_feedback.led_ctrl.blue_mode);
  printf("   Defenses LED mode: %u\r\n", status_feedback.led_ctrl.defenses_mode);
  printf("   Flow enable: %s\r\n", status_feedback.led_ctrl.flow_enable ? "ON" : "OFF");
  printf("   Blink period: %u ms\r\n", status_feedback.led_ctrl.period_ms);
  printf("   Duty cycle: %u%%\r\n", status_feedback.led_ctrl.duty_cycle);
  
  // 5. Execution result
  printf("5. Last Command Result: %s\r\n", status_feedback.result == 0 ? "SUCCESS" : "FAILED");
  
  printf("===================================\r\n\r\n");
}

/**
  * @brief USART1 receive interrupt callback
  * @note 这是最关键的函数！当AGV返回数据时，会触发这个中断
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
if (huart->Instance == USART1)
  {
    // 保存当前索引处的字节（HAL库已将接收到的字节存储在rx_buffer[rx_index]）
    uint8_t received_byte = rx_buffer[rx_index];
    
    // 增加计数器
    interrupt_count++;
    byte_count++;
    
    // 调试：显示接收到的字节
    if (byte_count <= 5) {
      printf("[IRQ] Byte %lu: 0x%02X at index %u\r\n", 
             byte_count, received_byte, rx_index);
    }
    
    // 增加缓冲区索引
    rx_index++;
    
    // 正确的缓冲区溢出保护
    if (rx_index >= RX_BUFFER_SIZE) {
      // 重置索引，但不清除缓冲区
      printf("[WARN] Buffer full, wrapping around...\r\n");
      rx_index = 0;
    }
    
    // 在缓冲区中查找完整的帧
    size_t frame_start = 0;
    size_t frame_length = 0;
    
    int32_t find_result = uart_frame_find(rx_buffer, rx_index, &frame_start, &frame_length);
    
    if (find_result == UART_FRAME_OK) {
      frame_count++;
      printf("[FRAME] Found #%lu! Start: %lu, Length: %lu\r\n", 
             frame_count, frame_start, frame_length);
      
      // 解析找到的帧
      parse_received_frame(&rx_buffer[frame_start], frame_length);
      
      // 从缓冲区中移除已处理的数据
      size_t bytes_to_remove = frame_start + frame_length;
      if (bytes_to_remove > 0) {
        size_t remaining = rx_index - bytes_to_remove;
        if (remaining > 0) {
          memmove(rx_buffer, &rx_buffer[bytes_to_remove], remaining);
        }
        rx_index = (uint16_t)remaining;
        printf("[BUFFER] Removed %lu bytes, now %u bytes remain\r\n", 
               bytes_to_remove, rx_index);
      }
    } else {
      // 还没有找到完整的帧
      if (byte_count % 100 == 0) {
        printf("[BUFFER] %u bytes, waiting for frame...\r\n", rx_index);
      }
    }
    
    // 继续接收下一个字节
    HAL_UART_Receive_IT(&huart1, &rx_buffer[rx_index], 1);
  }
}


/**
  * @brief Parse received UART frame
  * @note 根据文档的"最小解析"示例
  */
void parse_received_frame(const uint8_t* frame_data, size_t frame_len)
{
  uart_frame_parse_result_t r;
  int32_t ret = uart_frame_parse(frame_data, frame_len, &r);
  
  if (ret != UART_FRAME_OK)
  {
    printf("Parse failed: %s\r\n", uart_frame_get_error_string(ret));
    return;
  }

  printf("[PARSE] Type: 0x%02X, Length: %u\r\n", r.type, r.data_len);
  
  switch(r.type)
  {
    case 0x10:  // SENSOR_DATA
      printf("[SENSOR] Data frame received!\r\n");
      if (r.data_len >= sizeof(sensor_data_t)) {
        memcpy(&sensor_data, r.data_ptr, sizeof(sensor_data_t));
        
        // 打印完整有效性标志用于调试
        printf("[VALID] Flags: 0x%08lX\r\n", sensor_data.valid_flags);
        
        // 检查UWB数据有效性
        if ((sensor_data.valid_flags & DATA_INVALID_UWB) == 0) {
          uwb_data_valid = 1;
          uwb_x = sensor_data.uwb.x;
          uwb_y = sensor_data.uwb.y;
          uwb_frame_count++;
          uwb_data_ready = 1;
          printf("[UWB] Valid data ready: x=%d, y=%d\r\n", uwb_x, uwb_y);
        } else {
          uwb_data_valid = 0;
          printf("[UWB] Data marked as invalid (flag=0x%08lX)\r\n", 
                 sensor_data.valid_flags);
        }
      } else {
        printf("[ERROR] Sensor data too short: %u < %lu\r\n", 
               r.data_len, sizeof(sensor_data_t));
      }
      break;
      
    case 0x14:  // STATUS_FEEDBACK
      printf("[STATUS] Feedback frame received!\r\n");
      if (r.data_len >= sizeof(status_feedback_t)) {
        memcpy(&status_feedback, r.data_ptr, sizeof(status_feedback_t));
        status_feedback_ready = 1;
      }
      break;
      
    default: 
      printf("[UNKNOWN] Frame type: 0x%02X\r\n", r.type); 
      // 打印前20字节用于调试
      printf("Raw data: ");
      for (size_t i = 0; i < (r.data_len < 20 ? r.data_len : 20); i++) {
        printf("%02X ", r.data_ptr[i]);
      }
      printf("\r\n");
      break;
  }
}


/**
  * @brief Process UWB data
  */
void Process_UWB_Data(void)
{
  if (uwb_data_valid) {
    printf("[%lu] UWB: x=%d cm, y=%d cm\r\n", uwb_frame_count, uwb_x, uwb_y);
  }else {
    printf("[UWB] No valid data to display\r\n");
  }
}


/* USER CODE END 4 */

/* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Initializes and configures the Region and the memory to be protected
   */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x0;
  MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
  MPU_InitStruct.SubRegionDisable = 0x87;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
