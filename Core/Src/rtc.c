/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    rtc.c
  * @brief   This file provides code for the configuration
  *          of the RTC instances.
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
#include "rtc.h"

/* USER CODE BEGIN 0 */
#include <stdio.h>
/* USER CODE END 0 */

RTC_HandleTypeDef hrtc;

/* RTC init function */
void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 249;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */
  {
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};

    /* 上电后先给一个确定的初始时间：2025-11-07 09:00:00 */
    sTime.Hours          = 9U;
    sTime.Minutes        = 0U;
    sTime.Seconds        = 0U;
    sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    sTime.StoreOperation = RTC_STOREOPERATION_RESET;
    if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
    {
      Error_Handler();
    }

    sDate.WeekDay = RTC_WEEKDAY_FRIDAY;
    sDate.Month   = RTC_MONTH_NOVEMBER;
    sDate.Date    = 7U;
    sDate.Year    = 25U;
    if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
    {
      Error_Handler();
    }

    /* 秒脉冲中断：计数值 0 + ck_spre(1 Hz) -> 每 1 s 中断一次 */
    if (HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 0U, RTC_WAKEUPCLOCK_CK_SPRE_16BITS) != HAL_OK)
    {
      Error_Handler();
    }
  }
  /* USER CODE END RTC_Init 2 */

}

void HAL_RTC_MspInit(RTC_HandleTypeDef* rtcHandle)
{

  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
  if(rtcHandle->Instance==RTC)
  {
  /* USER CODE BEGIN RTC_MspInit 0 */

  /* USER CODE END RTC_MspInit 0 */

  /** Initializes the peripherals clock
  */
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
    PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
    {
      Error_Handler();
    }

    /* RTC clock enable */
    __HAL_RCC_RTC_ENABLE();
  /* USER CODE BEGIN RTC_MspInit 1 */
    /* 秒中断：优先级最高，保证 1 Hz 计时稳定 */
    HAL_NVIC_SetPriority(RTC_WKUP_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(RTC_WKUP_IRQn);
  /* USER CODE END RTC_MspInit 1 */
  }
}

void HAL_RTC_MspDeInit(RTC_HandleTypeDef* rtcHandle)
{

  if(rtcHandle->Instance==RTC)
  {
  /* USER CODE BEGIN RTC_MspDeInit 0 */

  /* USER CODE END RTC_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_RTC_DISABLE();
  /* USER CODE BEGIN RTC_MspDeInit 1 */

  /* USER CODE END RTC_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */
/**
  * @brief  RTC WakeUp（秒信号）中断回调
  * @note   读出时间/日期 -> 换算 YYMMDD、HHMMSS -> 格式化成字符串；
  *         置位标志后由主循环发送，避免在中断里长时间阻塞串口
  */
void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *rtcHandle)
{
  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef sDate = {0};

  /* 必须先读时间再读日期：读日期会解锁影子寄存器 */
  if (HAL_RTC_GetTime(rtcHandle, &sTime, RTC_FORMAT_BIN) != HAL_OK)
  {
    return;
  }
  if (HAL_RTC_GetDate(rtcHandle, &sDate, RTC_FORMAT_BIN) != HAL_OK)
  {
    return;
  }

  m_nRtcYMD = (uint32_t)sDate.Year * 10000U + (uint32_t)sDate.Month * 100U + sDate.Date;
  m_nRtcHMS = (uint32_t)sTime.Hours * 10000U + (uint32_t)sTime.Minutes * 100U + sTime.Seconds;

  (void)snprintf(g_rtcString, sizeof(g_rtcString),
                 "20%02lu-%02lu-%02lu %02lu:%02lu:%02lu\r\n",
                 (unsigned long)sDate.Year,
                 (unsigned long)sDate.Month,
                 (unsigned long)sDate.Date,
                 (unsigned long)sTime.Hours,
                 (unsigned long)sTime.Minutes,
                 (unsigned long)sTime.Seconds);

  g_rtcSecFlag = 1U;
}
/* USER CODE END 1 */
