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
#include "stdio.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "uart_frame.h"
#ifdef UART_LOOPBACK_TEST_ENABLE
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

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
/* USER CODE BEGIN PFP */
void Send_Control_Command(void);  // Declare control command sending function
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
  /* USER CODE BEGIN 2 */
    printf("=== AGV Control System Start ===\r\n");
    HAL_Delay(100);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
    while (1)
    {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
#ifdef UART_LOOPBACK_TEST_ENABLE
      // 测试模式：执行UART回环测试
      if(uart_loopback_test_execute()) {
        // 测试通过 - 绿灯闪烁3次
        for(int i = 0; i < 3; i++) {
          led_on(led_g);
          HAL_Delay(100);
          led_off(led_g);
          HAL_Delay(100);
        }
      } else {
        // 测试失败 - 红灯长亮2秒
        led_on(led_r);
        HAL_Delay(2000);
        led_off(led_r);
      }
      HAL_Delay(UART_LOOPBACK_TEST_INTERVAL_MS);
#else
      // 正常模式：发送控制命令
      Send_Control_Command();
      HAL_Delay(2000);  // 2 秒延时，0.5Hz 发送频率
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

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

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
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
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
    
    // Set angles (straight)
    ctrl.angles.angle_left = 0.0f;
    ctrl.angles.angle_right = 0.0f;
    ctrl.angles.angle_camera = 0.0f;
    
    // Set flags
    ctrl.flags.chassis_lock = 0;     // Unlock, allow movement
    ctrl.flags.motor0_break = 0;     // Left motor brake release
    ctrl.flags.motor1_break = 0;     // Right motor brake release
    ctrl.flags.diff_locked = 0;      // Differential lock off
    
    // Set motor parameters
    ctrl.motor_rpm.left = 30;        // Left wheel PWM, 30% speed
    ctrl.motor_rpm.right = 30;       // Right wheel PWM, 30% speed
    ctrl.motor_rpm.pulses = 0;     // Target pulse count (1 revolution, approx. 0.471m)
    
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
        UART_FRAME_TYPE_CONTROL_CMD,  // Control command type
        (uint8_t*)&ctrl,              // Control command data
        sizeof(ctrl_frame_t),         // Data length
        tx_buf,                       // Output buffer
        sizeof(tx_buf),               // Buffer size
        &frame_len                    // Output frame length
    );
    
    if(rc == UART_FRAME_OK && frame_len > 0) {
        printf("Control command packed successfully, frame length: %u\r\n", frame_len);
        
        // Display the first 16 bytes in hexadecimal
        printf("Frame data (hex): ");
        for(int i = 0; i < frame_len ; i++) {
            printf("%02X ", tx_buf[i]);
						if((i+1) % 16 == 0) printf("\r\n");
        }
        printf("\r\n");
        
        // Send control command frame
        HAL_StatusTypeDef tx_status = HAL_UART_Transmit(&huart2, tx_buf, frame_len, 1000);
        
        if(tx_status == HAL_OK) {
            printf("Control command sent successfully!\r\n");
            
            // LED blink indicates successful transmission
            for(int i = 0; i < 3; i++) {
                led_on(led_g);
                HAL_Delay(100);
                led_off(led_g);
                HAL_Delay(100);
            }
        } else {
            printf("Transmission failed: %d\r\n", tx_status);
            
            // LED steady on indicates transmission failure
            led_on(led_r);
            HAL_Delay(2000);
            led_off(led_r);
        }
    } else {
        printf("Packing failed: %ld\r\n", (long)rc);
        
        // LED fast blinking indicates packing failure
        for(int i = 0; i < 5; i++) {
            led_on(led_r);
            HAL_Delay(200);
            led_off(led_r);
            HAL_Delay(200);
        }
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
