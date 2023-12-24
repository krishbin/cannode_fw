/*

The MIT License (MIT)

Copyright (c) 2022 Ryan Edwards

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/

/* Includes ------------------------------------------------------------------*/
#include "board.h"
#include "usbd_gs_can.h"
#include "can.h"
#include "led.h"

FDCAN_HandleTypeDef hfdcan2;
LED_HandleTypeDef hled1;
LED_HandleTypeDef hled2;
LED_HandleTypeDef hled3;

extern USBD_GS_CAN_HandleTypeDef hGS_CAN;

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV1;
  RCC_OscInitStruct.PLL.PLLN = 18;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV6;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/** Configure pins
*/
void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, FDCAN2_TX_LED_Pin|BUS_STATE_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(FDCAN2_RX_LED_GPIO_Port, FDCAN2_RX_LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : FDCAN2_TX_LED_Pin BUS_STATE_Pin */
  GPIO_InitStruct.Pin = FDCAN2_TX_LED_Pin|BUS_STATE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : FDCAN2_RX_LED_Pin */
  GPIO_InitStruct.Pin = FDCAN2_RX_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(FDCAN2_RX_LED_GPIO_Port, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

void main_init_cb(void)
{
	/* carry over the LED blink from original firmware */
	for (uint8_t i=0; i<10; i++)
	{
		HAL_GPIO_TogglePin(FDCAN2_RX_LED_GPIO_Port, FDCAN2_RX_LED_Pin);
		HAL_Delay(50);
		HAL_GPIO_TogglePin(FDCAN2_TX_LED_GPIO_Port, FDCAN2_TX_LED_Pin);
	}

	hGS_CAN.channels[0] = &hfdcan2;
	can_init(hGS_CAN.channels[0], FDCAN2);
	led_init(&hled1, FDCAN2_RX_LED_GPIO_Port, FDCAN2_RX_LED_Pin, LED_MODE_ACTIVE,	  LED_ACTIVE_HIGH);
	led_init(&hled2, FDCAN2_TX_LED_GPIO_Port, FDCAN2_TX_LED_Pin, LED_MODE_INACTIVE, LED_ACTIVE_HIGH);
	led_init(&hled3, BUS_STATE_GPIO_Port, BUS_STATE_Pin, LED_MODE_INACTIVE, LED_ACTIVE_HIGH);
}

void main_task_cb(void)
{
	/* update all the LEDs */
	led_update(&hled1);
	led_update(&hled2);
	led_update(&hled3);
}

void can_on_tx_cb(uint8_t channel, struct gs_host_frame *frame)
{
	UNUSED(channel);
	UNUSED(frame);
	led_indicate_rxtx(&hled1);
}

void can_on_rx_cb(uint8_t channel, struct gs_host_frame *frame)
{
	UNUSED(channel);
	UNUSED(frame);
	led_indicate_rxtx(&hled3);
}

void can_identify_cb(uint32_t do_identify)
{
	if (do_identify) {
		led_blink_start(&hled3, 250);
	}
	else {
		led_blink_stop(&hled3);
	}
}