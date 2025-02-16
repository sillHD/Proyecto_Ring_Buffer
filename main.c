/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include <stdio.h>
#include  <string.h>
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "keypad.h" 
#include "ring_buffer.h"
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
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint32_t key_pressed_tick = 0;
uint16_t column_pressed = 0;

uint32_t debounce_tick = 0;
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if ((debounce_tick + 200) > HAL_GetTick()) {
    return;
  }
  debounce_tick = HAL_GetTick();
  key_pressed_tick = HAL_GetTick();
  column_pressed = GPIO_Pin;
}



/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
  int main(void) {
    /* USER CODE BEGIN 1 */
    void process_command(ring_buffer_t *rb, uint8_t *buffer, char *state) {
      if (ring_buffer_size(rb) == 5) {  // Verifica si el comando es de longitud 5
          // Lee el comando completo del buffer
          for (int i = 0; i < 5; i++) {
              ring_buffer_read(rb, &buffer[i]);
          }
          buffer[5] = '\0';  // Asegura el término del string
  
          // Procesa el comando basado en su contenido
          if (strcmp((char *)buffer, "#*A*#") == 0) {
              if (strcmp(state, "Op") == 0) {
                  HAL_UART_Transmit(&huart2, (uint8_t *)"Puerta ya esta abierta\r\n", 26, 100);
              } else {
                  HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);  // Cambia el estado del LED
                  HAL_UART_Transmit(&huart2, (uint8_t *)"Puerta Abierta\r\n", 17, 100);
                  strcpy(state, "Op");
              }
          } else if (strcmp((char *)buffer, "#*C*#") == 0) {
              if (strcmp(state, "Cl") == 0) {
                  HAL_UART_Transmit(&huart2, (uint8_t *)"Puerta ya esta cerrada\r\n", 27, 100);
              } else {
                  HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);  // Cambia el estado del LED
                  HAL_UART_Transmit(&huart2, (uint8_t *)"Puerta Cerrada\r\n", 17, 100);
                  strcpy(state, "Cl");
              }
          } else if (strcmp((char *)buffer, "#*1*#") == 0) {
              if (strcmp(state, "Cl") == 0) {
                  HAL_UART_Transmit(&huart2, (uint8_t *)"Estado de la puerta:Cerrada\r\n", 36, 100);
              } else {
                  HAL_UART_Transmit(&huart2, (uint8_t *)"Estado de la puerta:Abierta\r\n", 36, 100);
              }
          } else if (strcmp((char *)buffer, "#*0*#") == 0) {
              HAL_UART_Transmit(&huart2, (uint8_t *)"Buffer limpiado y puerta cerrada\r\n", 38, 100);
              ring_buffer_reset(rb);
              strcpy(state, "Cl");
              HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
              HAL_Delay(500);
              HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
              HAL_Delay(500);
              HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
              HAL_Delay(500);
              HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
          } else {
              HAL_UART_Transmit(&huart2, (uint8_t *)"Comando no reconocido\r\n", 24, 100);
          }
  
          // Reinicia el buffer de comando
          for (int i = 0; i < 5; i++) {
              buffer[i] = '_';
          }
      }
    }
    /* USER CODE END 1 */

    /* MCU Configuration--------------------------------------------------------*/

    HAL_Init();  // Inicializa el HAL

    SystemClock_Config();  // Configura el sistema de reloj

    /* Initialize all configured peripherals */
    MX_GPIO_Init();  // Inicializa los pines GPIO
    MX_USART2_UART_Init();  // Inicializa UART2
    initialise_monitor_handles();  // Inicializa el monitor de depuración
    setvbuf(stdout, NULL, _IONBF, 0);  // Desactiva el buffer de stdout

    /* USER CODE BEGIN Init */
    
    /* USER CODE END Init */

    /* Infinite loop */
    ring_buffer_t rb_matrix;
    ring_buffer_t rb_pc;
    uint8_t buffer_matrix[5];  // Buffer de tamaño 10 para almacenar el comando completo
    uint8_t buffer_pc[5];
    for (int i = 0; i < sizeof(buffer_matrix); i++) {
      buffer_matrix[i] = '_';
      buffer_pc[i] = '_';
    }  
    keypad_init();  // Inicializa el teclado matricial
    ring_buffer_init(&rb_matrix, buffer_matrix, 5);  // Inicializa el ring buffer
    ring_buffer_init(&rb_pc, buffer_pc, 5);
    HAL_UART_Transmit(&huart2, (uint8_t *)"Hello World\r\n\0", 20, 100);  // Envía el mensaje "Hello World"

    uint8_t key;  // Variable para almacenar la tecla presionada desde el teclado matricial
    uint8_t pc_key;  // Variable para recibir teclas desde la PC
    char state[3] = "Cl";  // Estado inicial de la puerta: cerrada

    while (1) {
      // Procesa teclas del teclado matricial
      if (column_pressed != 0 && (key_pressed_tick + 5) < HAL_GetTick()) {
          key = keypad_scan(column_pressed);
          column_pressed = 0;
          if (key != 'E') {
              ring_buffer_write(&rb_matrix, key);
              uint8_t size = ring_buffer_size(&rb_matrix);
              char msg[45];
              snprintf(msg, sizeof(msg), "Key: %c, Buffer: %s, Size: %d\r\n", key, buffer_matrix, size);
              HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), 100);
          }
      }

      // Procesa teclas recibidas desde la PC
      if (HAL_UART_Receive(&huart2, &pc_key, 1, 10) == HAL_OK) {
          ring_buffer_write(&rb_pc, pc_key);
          uint8_t size = ring_buffer_size(&rb_pc);
          char msg[45];
          snprintf(msg, sizeof(msg), "PC Key: %c, Buffer: %s, Size: %d\r\n", pc_key, buffer_pc, size);
          HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), 100);
      }

      // Procesa comandos para ambos buffers
      process_command(&rb_matrix, buffer_matrix, state);
      process_command(&rb_pc, buffer_pc, state);

      HAL_Delay(100);
  }
}


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
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 10;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
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

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, LD2_Pin|ROW_1_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, ROW_2_Pin|ROW_4_Pin|ROW_3_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : COLUMN__1_Pin */
  GPIO_InitStruct.Pin = COLUMN_1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(COLUMN_1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : COLUMN_4_Pin */
  GPIO_InitStruct.Pin = COLUMN_4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(COLUMN_4_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : COLUMN_2_Pin COLUMN_3_Pin */
  GPIO_InitStruct.Pin = COLUMN_2_Pin|COLUMN_3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : ROW_1_Pin */
  GPIO_InitStruct.Pin = ROW_1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(ROW_1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : ROW_2_Pin ROW_4_Pin ROW_3_Pin */
  GPIO_InitStruct.Pin = ROW_2_Pin|ROW_4_Pin|ROW_3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

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

#ifdef  USE_FULL_ASSERT
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
