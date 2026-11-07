/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : 实验一 主程序
  *                   STM32F407G-DISC1 / STM32F407VGT6
  *
  *  任务：
  *   1) 系统时钟 SYSCLK = 168 MHz（HSE 8 MHz -> PLL）
  *   2) 按键 B1(PA0) 控制 LED3~LED6 正转逐一点亮（200 ms 一步）
  *   3) USART2 9600-8-N-1，与计算机收发测试（回显）
  *   4) RTC 秒中断中将时间格式化为字符串，经串口发送到计算机
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
#include "rtc.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define LED_MARQUEE_PERIOD_MS   200U   /* 相邻两个 LED 的点亮间隔 */
#define KEY_DEBOUNCE_MS         20U    /* 按键消抖时间 */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* RTC 秒中断与主循环之间传递的时间数据 */
volatile uint32_t m_nRtcYMD    = 0U;
volatile uint32_t m_nRtcHMS    = 0U;
volatile uint8_t  g_rtcSecFlag = 0U;
char              g_rtcString[32] = "RTC init\r\n";

/* 跑马灯状态：s_ledIndex 0->LED3, 1->LED4, 2->LED5, 3->LED6 */
static const uint16_t s_ledPins[4] = {LD3_Pin, LD4_Pin, LD5_Pin, LD6_Pin};
static uint8_t  s_ledIndex   = 0U;
static uint8_t  s_marqueeRun = 0U;
static uint32_t s_ledTick    = 0U;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void Led_AllOff(void);
static void Led_Marquee_Step(void);
static void Key_Scan(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
#if defined(__CC_ARM)
#pragma import(__use_no_semihosting)      /* 不使用半主机模式，printf 才能正常工作 */
struct __FILE
{
  int handle;
};
FILE __stdout;

void _sys_exit(int x)
{
  (void)x;
}

void _ttywrch(int ch)
{
  (void)ch;
}
#endif

/**
  * @brief  printf 重定向：把标准库输出送到 USART2
  * @note   串口调试助手中即可看到 printf 的内容
  */
int fputc(int ch, FILE *f)
{
  (void)f;
  HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1U, HAL_MAX_DELAY);
  return ch;
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

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
  MX_RTC_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  printf("\r\n");
  printf("**********************************************\r\n");
  printf("*  STM32F407G-DISC1  Experiment 1            *\r\n");
  printf("*  Clock / GPIO / UART2 / RTC                *\r\n");
  printf("**********************************************\r\n");
  printf("SYSCLK = %lu Hz\r\n", (unsigned long)HAL_RCC_GetSysClockFreq());
  printf("HCLK   = %lu Hz\r\n", (unsigned long)HAL_RCC_GetHCLKFreq());
  printf("PCLK1  = %lu Hz (APB1, max 42 MHz)\r\n", (unsigned long)HAL_RCC_GetPCLK1Freq());
  printf("PCLK2  = %lu Hz (APB2, max 84 MHz)\r\n", (unsigned long)HAL_RCC_GetPCLK2Freq());
  printf("USART2 : 9600 bps, 8 data bits, 1 stop bit, no parity\r\n");
  printf("Press B1 (PA0) to START/STOP the LED marquee (LED3->LED6)\r\n\r\n");

  /* 使能串口接收中断：收到的字符在回调中原样回显，用于收发测试 */
  if (HAL_UART_Receive_IT(&huart2, &g_uartRxByte, 1U) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* 1) 按键扫描：切换跑马灯运行/停止 */
    Key_Scan();

    /* 2) 跑马灯：每 200 ms 正转点亮下一个 LED */
    if ((s_marqueeRun != 0U) && ((HAL_GetTick() - s_ledTick) >= LED_MARQUEE_PERIOD_MS))
    {
      s_ledTick = HAL_GetTick();
      Led_Marquee_Step();
    }

    /* 3) RTC 秒信号：时间已在中断里格式化好，这里只负责发送 */
    if (g_rtcSecFlag != 0U)
    {
      g_rtcSecFlag = 0U;
      HAL_UART_Transmit(&huart2, (uint8_t *)g_rtcString,
                        (uint16_t)strlen(g_rtcString), 100U);
    }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @note  HSE 8 MHz -> PLL(M=8, N=336, P=2) -> SYSCLK 168 MHz
  *        AHB /1 = 168 MHz, APB1 /4 = 42 MHz, APB2 /2 = 84 MHz
  *        PLLQ = 7 -> 48 MHz（USB OTG FS / SDIO 用，本实验仅作规范配置）
  *        RTC 使用 LSI（板上未焊 32.768 kHz 晶振）
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
/**
  * @brief  熄灭全部用户 LED
  */
static void Led_AllOff(void)
{
  HAL_GPIO_WritePin(GPIOD, LD3_Pin | LD4_Pin | LD5_Pin | LD6_Pin, GPIO_PIN_RESET);
}

/**
  * @brief  正转点亮下一个 LED：LED3 -> LED4 -> LED5 -> LED6 -> LED3 ...
  */
static void Led_Marquee_Step(void)
{
  Led_AllOff();
  HAL_GPIO_WritePin(GPIOD, s_ledPins[s_ledIndex], GPIO_PIN_SET);

  s_ledIndex++;
  if (s_ledIndex >= 4U)
  {
    s_ledIndex = 0U;
  }
}

/**
  * @brief  按键扫描（软件消抖）
  * @note   B1 按下时 PA0 为高电平，按下沿切换跑马灯的运行/停止
  */
static void Key_Scan(void)
{
  static uint8_t  s_lastLevel = 0U;
  static uint32_t s_keyTick   = 0U;

  uint8_t nowLevel = (HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin) == GPIO_PIN_SET) ? 1U : 0U;

  if ((nowLevel != s_lastLevel) && ((HAL_GetTick() - s_keyTick) >= KEY_DEBOUNCE_MS))
  {
    s_keyTick   = HAL_GetTick();
    s_lastLevel = nowLevel;

    if (nowLevel != 0U)          /* 只处理按下动作 */
    {
      s_marqueeRun = (s_marqueeRun != 0U) ? 0U : 1U;
      if (s_marqueeRun == 0U)
      {
        Led_AllOff();
      }
      printf("[KEY] marquee %s\r\n", (s_marqueeRun != 0U) ? "RUN" : "STOP");
    }
  }
}
/* USER CODE END 4 */

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
