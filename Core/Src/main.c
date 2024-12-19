/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "songs.h"	// songs :)
#include <string.h>	// memcpy
#include "ctype.h" 	// isdigit
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
#include "ring_buffer.h"

#define TX6_BUFFER_SIZE 100

char __uart6_tx_buff[TX6_BUFFER_SIZE];
RingBuffer uart6_tx_buff;
bool UART6_TX_IsReady = true;
char uart6_tx_byte_buff[1];

bool UART6_RX_IsReady = false;
char uart6_rx_byte_buff[1];

/**
 * @brief Пытается отправить данные из кольцевого буфера
 */
void UART6_TryToTransmit_IT() {
  if (UART6_TX_IsReady && !RingBuffer_IsEmpty(&uart6_tx_buff)) {
    RingBuffer_Read(&uart6_tx_buff, uart6_tx_byte_buff, 1);
    UART6_TX_IsReady = false;
    HAL_UART_Transmit_IT(&huart6, (uint8_t*) uart6_tx_byte_buff, 1);
  }
}

/**
 * @brief Отправляет байт в режиме прерываний
 * @return true - байт записан в буфер отправки
 * @return false - буфер отправки переполнен
 */
bool UART6_TransmitByte(char byte) {
  bool result = false;
  if (!uart6_tx_buff.isFull) {
      RingBuffer_Write(&uart6_tx_buff, &byte, 1);
      result = true;
    }
  return result;
}

/**
 * @brief Получает байт по uart
 * @return true - данные получены
 * @return false - новых данных нет
 */
bool UART6_ReceiveByte(char* byte_ptr) {
  bool result = false;
  if (UART6_RX_IsReady) {
      *byte_ptr = *uart6_rx_byte_buff;
      UART6_RX_IsReady = false;
      result = true;
  } else {
    HAL_UART_Receive_IT(&huart6, (uint8_t*) uart6_rx_byte_buff, 1);
  }
  return result;
}

/**
 * @brief Получает байт по uart
 * @return true - данные записан в буфер отправки
 * @return false - буфер отправки переполнен
 */
bool UART6_TransmitString(char* str) {
  bool result = false;
  if (!uart6_tx_buff.isFull) {
    RingBuffer_Write(&uart6_tx_buff, str, strlen(str));
  }
  return result;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef* UartHandle) {
  if (UartHandle == &huart6) {
    UART6_RX_IsReady = true;
  }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef* UartHandle) {
  if (UartHandle == &huart6) {
    UART6_TX_IsReady = true;
  }
}
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
bool is_playing = false;
int i = 0; // указатель на текущую проигрываемую ноту
int note_duration = 0; // продолжительность текущей ноты

/*
 * Пользовательская мелодия
 */

int user_notes[256];
int user_durations[256];
int user_song_size = -1;

/*
 * Объявление стартовой мелодии
 */
int * song;
int * song_durations;
int song_size = sizeof(song)/sizeof (int*);


/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void SettingsMode(){
	int temp_notes[256];
	int temp_durations[256];
	int temp_song_size = 0;
	int note = 0;
	int duration = -1;
	bool is_note = true;
	is_playing = false;

	char input_sym[1];
	UART6_TransmitString("\n\r settings mode \n\r q - exit \n\r s -save \n\r example: note-duration \n\r           123-960\n\r");
	while(1){
		UART6_TryToTransmit_IT();

		if (UART6_ReceiveByte(input_sym)) {
			UART6_TransmitByte(*input_sym);
			// выход из режима создания мелодии
			if (input_sym[0] == 'q'){
				UART6_TransmitString("\n\r exit settings mode\n\r");
			  return;
			}

			// сохранение мелодии
			if (input_sym[0] == 's'){
			  memcpy(user_notes, temp_notes, 256 * sizeof(int));
			  user_song_size = temp_song_size + 1; // добавляем 1 для корректного счета длины мелодии
			  memcpy(user_durations, temp_durations, user_song_size * sizeof(int));
			  UART6_TransmitString("\n\r melody saved \n\r");
			  return;
			}

			// переключение на ввод длительности ноты
			if (input_sym[0] == '-'){
			  if (note > -1){
				  is_note = false;
			  } else {
				  UART6_TransmitString("\n\r input error \n\r");
			  }
			  continue;
			}

			if (input_sym[0] == '\r'){
				if (note > -1 && duration > -1){
					temp_notes[temp_song_size] = note;
					temp_durations[temp_song_size] = duration;
					temp_song_size++;

					is_note = true;
					note = -1;
					duration = -1;

					// переполнение
					if (temp_song_size > 256){
//						UART6_TransmitString("\n\rsize overflow error\n\r");
					}
					UART6_TransmitString("\n\r note saved \n\r");
					continue;
			  }
					UART6_TransmitString("\n\r input error \n\r");
				  continue;
			  }
			if (!isdigit(input_sym[0])){
				UART6_TransmitString("\n\r input error \n\r");
				continue;
			}
			// ввод ноты или длительности
			if (is_note){
				if (note == -1){
					note = 0;
				}
				note *= 10;
				note += input_sym[0];
			} else {
				if (duration == -1){
					duration = 0;
				}
				duration *= 10;
				duration += input_sym[0];
			}
		}
	}
}


void ProcessInput(char sym){
	if (isdigit(sym)){
		switch (sym){
			case '1': {
				start_song(elochka_notes, elochka_durations, sizeof(elochka_notes)/sizeof(int*));
				UART6_TransmitString("\n\r elochka melody is playing \n\r");
				return;
			}
			case '2': {
				start_song(pirates_notes, pirates_durations, sizeof(pirates_notes)/sizeof(int*));
				UART6_TransmitString("\n\r pirates melody is playing \n\r");
				return;
			}
			case '3': {
				start_song(doom_notes, doom_durations, sizeof(doom_notes)/sizeof(int*));
				UART6_TransmitString("\n\r doom melody is playing \n\r");
				return;
			}
			case '4': {
				start_song(godfather_notes, godfather_durations, sizeof(godfather_notes)/sizeof(int*));
				UART6_TransmitString("\n\r godfather melody is playing \n\r");
				return;
			}
			case '5': {
				if (user_song_size > 0) {
					start_song(user_notes, user_durations, sizeof(user_notes)/sizeof(int*));
					UART6_TransmitString("\n\r user melody is playing \n\r");
				} else{
					UART6_TransmitString("\n\r user melody is empty \n\r");
				}
				return;
			}
		}
	}
	if (sym == '\r'){
		SettingsMode();
		return;
	}
	UART6_TransmitString("\n\r input error \n\r");

}

void start_song(int * m, int * d, int size) {
	i = 0;
	song = m;
	song_durations = d;
	song_size = size;
	is_playing = true;
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
	RingBuffer_Init(&uart6_tx_buff, __uart6_tx_buff, TX6_BUFFER_SIZE);
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
  MX_TIM6_Init();
  MX_TIM1_Init();
  MX_USART6_UART_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  HAL_TIM_Base_Start_IT(&htim6);
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
  start_song(elochka_notes, elochka_durations, sizeof(elochka_notes)/sizeof(int*));
  char buff[1] = {0};

  while (1) {
	  UART6_TryToTransmit_IT();
	  if (UART6_ReceiveByte(buff)) {
		  UART6_TransmitByte(*buff);
		  ProcessInput(*buff);
	  }


    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 15;
  RCC_OscInitStruct.PLL.PLLN = 216;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Activate the Over-Drive mode
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
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
