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
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>
#include <math.h>
#include "app_pipeline.h"
#include "ff.h"
#include "sd_diskio_dma_standalone.h"
#include "stm32u5xx.h"
#include "stm32u5xx_hal.h"
#include "stm32u5xx_hal_conf.h"
#include "stm32u5xx_hal_def.h"
#include "stm32u5xx_hal_gpio.h"
#include "stm32u5xx_hal_mdf.h"

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

MDF_HandleTypeDef MdfHandle0;
MDF_FilterConfigTypeDef MdfFilterConfig0;
MDF_HandleTypeDef MdfHandle1;
MDF_FilterConfigTypeDef MdfFilterConfig1;
MDF_HandleTypeDef MdfHandle2;
MDF_FilterConfigTypeDef MdfFilterConfig2;
MDF_HandleTypeDef MdfHandle3;
MDF_FilterConfigTypeDef MdfFilterConfig3;
MDF_HandleTypeDef MdfHandle4;
MDF_FilterConfigTypeDef MdfFilterConfig4;
MDF_HandleTypeDef MdfHandle5;
MDF_FilterConfigTypeDef MdfFilterConfig5;
DMA_NodeTypeDef Node_GPDMA1_Channel0;
DMA_QListTypeDef List_GPDMA1_Channel0;
DMA_HandleTypeDef handle_GPDMA1_Channel0;

SD_HandleTypeDef hsd1;

/* USER CODE BEGIN PV */
MDF_DmaConfigTypeDef pDmaConfig;



/*------definition of buffer constants ------*/
#define SAMPLES_COUNT 4096
#define SAMPLES_BYTES SAMPLES_COUNT*2
#define HALFSAMPLES SAMPLES_COUNT/2
#define HALFSAMPLESBYTES SAMPLES_BYTES/2


/*-------definition of buffer for DMAs--------*/

// called INTlVD as it DMA interleaved samples from filter0 and transfers it to the SD card
volatile int16_t INTLVD[SAMPLES_COUNT] __attribute__((aligned(32)));
int8_t  INTLVD_ready = 0;
int32_t bytes_recorded = 0;
// debugging
uint8_t MDF_DMA_ERROR_FLAG = 0;


/* ------ buffer split in two for double buffering -------*/
volatile uint16_t* SDBUFFER1 = (uint16_t*)INTLVD;
volatile uint16_t* SDBUFFER2 = (uint16_t*)INTLVD + HALFSAMPLES;


/* --------DMA FLAGS-------------*/
volatile uint8_t CPLT, HALFCPLT = 0;
uint8_t MDFTESTFLAG = 0;

/* ---------Audio hardware parameters -------*/
uint32_t sample_rate = 24000;
uint32_t channels = 6;
uint32_t bitrate = 16;

// this tells the pipeline what recording number we are on
uint32_t recording_number = 1;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_GPDMA1_Init(void);
static void MX_ICACHE_Init(void);
static void MX_MDF1_Init(void);
static void MX_SDMMC1_SD_Init(void);
/* USER CODE BEGIN PFP */

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
  MX_GPDMA1_Init();
  MX_ICACHE_Init();
  MX_MDF1_Init();
  MX_SDMMC1_SD_Init();
  /* USER CODE BEGIN 2 */


  /*--------- initialise SD card -----------*/
  if(SD_Pipeline_Init() != PIPELINE_OK){
    // printf("init of sd failed\n");
    Error_Handler();
  }


  /* ----------start the pipeline------------ */

  // this function creates the header for a new WAV file
  if(SD_Pipeline_NewRec(recording_name, recording_number, sample_rate, channels, bitrate, 
    recording_length)!=PIPELINE_OK){
    // printf("init of pipeline failed\n");
    Error_Handler();
  }

  /*---------link DMA to MDF-----------*/

  pDmaConfig.MsbOnly = ENABLE;
  pDmaConfig.Address = (uint32_t) INTLVD;
  // data length is samples count * 2, because 16 bit sample consists of 2 bytes
  pDmaConfig.DataLength = SAMPLES_COUNT * 2; 



    
  /*------start all the MDF instances -------*/

  if(HAL_MDF_AcqStart(&MdfHandle1, &MdfFilterConfig1) != HAL_OK){
    Error_Handler();
  }
  if(HAL_MDF_AcqStart(&MdfHandle2, &MdfFilterConfig2) != HAL_OK){
    Error_Handler();
  }
  if(HAL_MDF_AcqStart(&MdfHandle3, &MdfFilterConfig3) != HAL_OK){
    Error_Handler();
  }
  if(HAL_MDF_AcqStart(&MdfHandle4, &MdfFilterConfig4) != HAL_OK){
    Error_Handler();
  }
  if(HAL_MDF_AcqStart(&MdfHandle5, &MdfFilterConfig5) != HAL_OK){
    Error_Handler();
  }
  // configure filter0 as the interleaved communicator with the SD card
  if(HAL_MDF_AcqStart_DMA(&MdfHandle0, &MdfFilterConfig0, &pDmaConfig) != HAL_OK){
    f_close(&SD_File_PIPELINE);
    Error_Handler();
  }


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
     if (INTLVD_ready)
      {
        HAL_MDF_AcqStop_DMA(&MdfHandle0);
        SD_Pipeline_StopRec(); 
        // printf("SUCCESS! File creation complete\n");
        INTLVD_ready = 0;
        while(1){
          HAL_GPIO_TogglePin(led_yellow_GPIO_Port, led_yellow_Pin);
          HAL_Delay(200);
        }
      }

      if(HALFCPLT){
        //unset flag
        HALFCPLT = 0;

        //calculate how many bytes are left in audio recording
        int32_t temp = (bytes_to_read<HALFSAMPLESBYTES) ? bytes_to_read : HALFSAMPLESBYTES;

        //if more than zero, continue - else set flag for completed transfer
        if(temp > 0){
        if(SD_Pipeline_Write((uint8_t*) SDBUFFER1, temp) != PIPELINE_OK) MDF_DMA_ERROR_FLAG = 1;
          
        // decrement bytes to read
        bytes_to_read = bytes_to_read - temp;
        }
        else{
          INTLVD_ready = 1;
        }
      }


      //duplicate above logic
      if(CPLT){
        // int32_t write_start = HAL_GetTick();
        //unset flag
        CPLT = 0;
        //calculate how many bytes are left in audio recording
        int32_t temp = (bytes_to_read<HALFSAMPLESBYTES) ? bytes_to_read : HALFSAMPLESBYTES;

        //if more than zero, continue - else set flag for completed transfer
        if(temp > 0){
        if(SD_Pipeline_Write((uint8_t*) SDBUFFER2, temp) != PIPELINE_OK) MDF_DMA_ERROR_FLAG = 1;

        //decrement bytes to read
        bytes_to_read = bytes_to_read - temp;
        // int32_t write_end = HAL_GetTick();
        // printf("wrote %li bytes in %li ms\n",temp, write_end-write_start);
        }
        else{
          INTLVD_ready = 1;
        }
        
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
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE3) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_0;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLMBOOST = RCC_PLLMBOOST_DIV4;
  RCC_OscInitStruct.PLL.PLLM = 3;
  RCC_OscInitStruct.PLL.PLLN = 8;
  RCC_OscInitStruct.PLL.PLLP = 11;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLLVCIRANGE_1;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_PCLK3;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GPDMA1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPDMA1_Init(void)
{

  /* USER CODE BEGIN GPDMA1_Init 0 */

  /* USER CODE END GPDMA1_Init 0 */

  /* Peripheral clock enable */
  __HAL_RCC_GPDMA1_CLK_ENABLE();

  /* GPDMA1 interrupt Init */
    HAL_NVIC_SetPriority(GPDMA1_Channel0_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(GPDMA1_Channel0_IRQn);

  /* USER CODE BEGIN GPDMA1_Init 1 */

  /* USER CODE END GPDMA1_Init 1 */
  /* USER CODE BEGIN GPDMA1_Init 2 */

  /* USER CODE END GPDMA1_Init 2 */

}

/**
  * @brief ICACHE Initialization Function
  * @param None
  * @retval None
  */
static void MX_ICACHE_Init(void)
{

  /* USER CODE BEGIN ICACHE_Init 0 */

  /* USER CODE END ICACHE_Init 0 */

  /* USER CODE BEGIN ICACHE_Init 1 */

  /* USER CODE END ICACHE_Init 1 */

  /** Enable instruction cache in 1-way (direct mapped cache)
  */
  if (HAL_ICACHE_ConfigAssociativityMode(ICACHE_1WAY) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_ICACHE_Enable() != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ICACHE_Init 2 */

  /* USER CODE END ICACHE_Init 2 */

}

/**
  * @brief MDF1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_MDF1_Init(void)
{

  /* USER CODE BEGIN MDF1_Init 0 */
  
  MdfHandle3.Init.FilterBistream = MDF_BITSTREAM1_FALLING;

  MdfHandle4.Init.FilterBistream = MDF_BITSTREAM2_RISING;

  MdfHandle5.Init.FilterBistream = MDF_BITSTREAM2_FALLING;


  /* USER CODE END MDF1_Init 0 */

  /* USER CODE BEGIN MDF1_Init 1 */

  /* USER CODE END MDF1_Init 1 */

  /**
    MdfHandle0 structure initialization and HAL_MDF_Init function call
  */
  MdfHandle0.Instance = MDF1_Filter0;
  MdfHandle0.Init.CommonParam.InterleavedFilters = 5;
  MdfHandle0.Init.CommonParam.ProcClockDivider = 2;
  MdfHandle0.Init.CommonParam.OutputClock.Activation = ENABLE;
  MdfHandle0.Init.CommonParam.OutputClock.Pins = MDF_OUTPUT_CLOCK_0;
  MdfHandle0.Init.CommonParam.OutputClock.Divider = 2;
  MdfHandle0.Init.CommonParam.OutputClock.Trigger.Activation = DISABLE;
  MdfHandle0.Init.SerialInterface.Activation = ENABLE;
  MdfHandle0.Init.SerialInterface.Mode = MDF_SITF_LF_MASTER_SPI_MODE;
  MdfHandle0.Init.SerialInterface.ClockSource = MDF_SITF_CCK0_SOURCE;
  MdfHandle0.Init.SerialInterface.Threshold = 4;
  MdfHandle0.Init.FilterBistream = MDF_BITSTREAM0_RISING;
  if (HAL_MDF_Init(&MdfHandle0) != HAL_OK)
  {
    Error_Handler();
  }

  /**
    MdfFilterConfig0, MdfOldConfig0 and/or MdfScdConfig0 structures initialization

    WARNING : only structures are filled, no specific init function call for filter
  */
  MdfFilterConfig0.DataSource = MDF_DATA_SOURCE_BSMX;
  MdfFilterConfig0.Delay = 0;
  MdfFilterConfig0.CicMode = MDF_ONE_FILTER_SINC5;
  MdfFilterConfig0.DecimationRatio = 32;
  MdfFilterConfig0.Offset = 0;
  MdfFilterConfig0.Gain = 0;
  MdfFilterConfig0.ReshapeFilter.Activation = ENABLE;
  MdfFilterConfig0.ReshapeFilter.DecimationRatio = MDF_RSF_DECIMATION_RATIO_4;
  MdfFilterConfig0.HighPassFilter.Activation = ENABLE;
  MdfFilterConfig0.HighPassFilter.CutOffFrequency = MDF_HPF_CUTOFF_0_00125FPCM;
  MdfFilterConfig0.Integrator.Activation = DISABLE;
  MdfFilterConfig0.SoundActivity.Activation = DISABLE;
  MdfFilterConfig0.AcquisitionMode = MDF_MODE_ASYNC_CONT;
  MdfFilterConfig0.FifoThreshold = MDF_FIFO_THRESHOLD_NOT_EMPTY;
  MdfFilterConfig0.DiscardSamples = 0;

  /**
    MdfHandle1 structure initialization and HAL_MDF_Init function call
  */
  MdfHandle1.Instance = MDF1_Filter1;
  MdfHandle1.Init.CommonParam.InterleavedFilters = 5;
  MdfHandle1.Init.CommonParam.ProcClockDivider = 2;
  MdfHandle1.Init.CommonParam.OutputClock.Activation = ENABLE;
  MdfHandle1.Init.CommonParam.OutputClock.Pins = MDF_OUTPUT_CLOCK_0;
  MdfHandle1.Init.CommonParam.OutputClock.Divider = 2;
  MdfHandle1.Init.CommonParam.OutputClock.Trigger.Activation = DISABLE;
  MdfHandle1.Init.SerialInterface.Activation = ENABLE;
  MdfHandle1.Init.SerialInterface.Mode = MDF_SITF_LF_MASTER_SPI_MODE;
  MdfHandle1.Init.SerialInterface.ClockSource = MDF_SITF_CCK0_SOURCE;
  MdfHandle1.Init.SerialInterface.Threshold = 4;
  MdfHandle1.Init.FilterBistream = MDF_BITSTREAM0_FALLING;
  if (HAL_MDF_Init(&MdfHandle1) != HAL_OK)
  {
    Error_Handler();
  }

  /**
    MdfFilterConfig1, MdfOldConfig1 and/or MdfScdConfig1 structures initialization

    WARNING : only structures are filled, no specific init function call for filter
  */
  MdfFilterConfig1.DataSource = MDF_DATA_SOURCE_BSMX;
  MdfFilterConfig1.Delay = 0;
  MdfFilterConfig1.CicMode = MDF_ONE_FILTER_SINC5;
  MdfFilterConfig1.DecimationRatio = 32;
  MdfFilterConfig1.Offset = 0;
  MdfFilterConfig1.Gain = 0;
  MdfFilterConfig1.ReshapeFilter.Activation = ENABLE;
  MdfFilterConfig1.ReshapeFilter.DecimationRatio = MDF_RSF_DECIMATION_RATIO_4;
  MdfFilterConfig1.HighPassFilter.Activation = ENABLE;
  MdfFilterConfig1.HighPassFilter.CutOffFrequency = MDF_HPF_CUTOFF_0_00125FPCM;
  MdfFilterConfig1.Integrator.Activation = DISABLE;
  MdfFilterConfig1.AcquisitionMode = MDF_MODE_ASYNC_CONT;
  MdfFilterConfig1.FifoThreshold = MDF_FIFO_THRESHOLD_NOT_EMPTY;
  MdfFilterConfig1.DiscardSamples = 0;

  /**
    MdfHandle2 structure initialization and HAL_MDF_Init function call
  */
  MdfHandle2.Instance = MDF1_Filter2;
  MdfHandle2.Init.CommonParam.InterleavedFilters = 5;
  MdfHandle2.Init.CommonParam.ProcClockDivider = 2;
  MdfHandle2.Init.CommonParam.OutputClock.Activation = ENABLE;
  MdfHandle2.Init.CommonParam.OutputClock.Pins = MDF_OUTPUT_CLOCK_0;
  MdfHandle2.Init.CommonParam.OutputClock.Divider = 2;
  MdfHandle2.Init.CommonParam.OutputClock.Trigger.Activation = DISABLE;
  MdfHandle2.Init.SerialInterface.Activation = ENABLE;
  MdfHandle2.Init.SerialInterface.Mode = MDF_SITF_LF_MASTER_SPI_MODE;
  MdfHandle2.Init.SerialInterface.ClockSource = MDF_SITF_CCK0_SOURCE;
  MdfHandle2.Init.SerialInterface.Threshold = 4;
  MdfHandle2.Init.FilterBistream = MDF_BITSTREAM1_RISING;
  if (HAL_MDF_Init(&MdfHandle2) != HAL_OK)
  {
    Error_Handler();
  }

  /**
    MdfFilterConfig2, MdfOldConfig2 and/or MdfScdConfig2 structures initialization

    WARNING : only structures are filled, no specific init function call for filter
  */
  MdfFilterConfig2.DataSource = MDF_DATA_SOURCE_BSMX;
  MdfFilterConfig2.Delay = 0;
  MdfFilterConfig2.CicMode = MDF_ONE_FILTER_SINC5;
  MdfFilterConfig2.DecimationRatio = 32;
  MdfFilterConfig2.Offset = 0;
  MdfFilterConfig2.Gain = 0;
  MdfFilterConfig2.ReshapeFilter.Activation = ENABLE;
  MdfFilterConfig2.ReshapeFilter.DecimationRatio = MDF_RSF_DECIMATION_RATIO_4;
  MdfFilterConfig2.HighPassFilter.Activation = ENABLE;
  MdfFilterConfig2.HighPassFilter.CutOffFrequency = MDF_HPF_CUTOFF_0_00125FPCM;
  MdfFilterConfig2.Integrator.Activation = DISABLE;
  MdfFilterConfig2.SoundActivity.Activation = DISABLE;
  MdfFilterConfig2.AcquisitionMode = MDF_MODE_ASYNC_CONT;
  MdfFilterConfig2.FifoThreshold = MDF_FIFO_THRESHOLD_NOT_EMPTY;
  MdfFilterConfig2.DiscardSamples = 0;

  /**
    MdfHandle3 structure initialization and HAL_MDF_Init function call
  */
  MdfHandle3.Instance = MDF1_Filter3;
  MdfHandle3.Init.CommonParam.InterleavedFilters = 5;
  MdfHandle3.Init.CommonParam.ProcClockDivider = 2;
  MdfHandle3.Init.CommonParam.OutputClock.Activation = ENABLE;
  MdfHandle3.Init.CommonParam.OutputClock.Pins = MDF_OUTPUT_CLOCK_0;
  MdfHandle3.Init.CommonParam.OutputClock.Divider = 2;
  MdfHandle3.Init.CommonParam.OutputClock.Trigger.Activation = DISABLE;
  MdfHandle3.Init.SerialInterface.Activation = DISABLE;
  if (HAL_MDF_Init(&MdfHandle3) != HAL_OK)
  {
    Error_Handler();
  }

  /**
    MdfFilterConfig3, MdfOldConfig3 and/or MdfScdConfig3 structures initialization

    WARNING : only structures are filled, no specific init function call for filter
  */
  MdfFilterConfig3.DataSource = MDF_DATA_SOURCE_BSMX;
  MdfFilterConfig3.Delay = 0;
  MdfFilterConfig3.CicMode = MDF_ONE_FILTER_SINC5;
  MdfFilterConfig3.DecimationRatio = 32;
  MdfFilterConfig3.Offset = 0;
  MdfFilterConfig3.Gain = 0;
  MdfFilterConfig3.ReshapeFilter.Activation = ENABLE;
  MdfFilterConfig3.ReshapeFilter.DecimationRatio = MDF_RSF_DECIMATION_RATIO_4;
  MdfFilterConfig3.HighPassFilter.Activation = ENABLE;
  MdfFilterConfig3.HighPassFilter.CutOffFrequency = MDF_HPF_CUTOFF_0_00125FPCM;
  MdfFilterConfig3.Integrator.Activation = DISABLE;
  MdfFilterConfig3.SoundActivity.Activation = DISABLE;
  MdfFilterConfig3.AcquisitionMode = MDF_MODE_ASYNC_CONT;
  MdfFilterConfig3.FifoThreshold = MDF_FIFO_THRESHOLD_NOT_EMPTY;
  MdfFilterConfig3.DiscardSamples = 0;

  /**
    MdfHandle4 structure initialization and HAL_MDF_Init function call
  */
  MdfHandle4.Instance = MDF1_Filter4;
  MdfHandle4.Init.CommonParam.InterleavedFilters = 5;
  MdfHandle4.Init.CommonParam.ProcClockDivider = 2;
  MdfHandle4.Init.CommonParam.OutputClock.Activation = ENABLE;
  MdfHandle4.Init.CommonParam.OutputClock.Pins = MDF_OUTPUT_CLOCK_0;
  MdfHandle4.Init.CommonParam.OutputClock.Divider = 2;
  MdfHandle4.Init.CommonParam.OutputClock.Trigger.Activation = DISABLE;
  MdfHandle4.Init.SerialInterface.Activation = DISABLE;
  if (HAL_MDF_Init(&MdfHandle4) != HAL_OK)
  {
    Error_Handler();
  }

  /**
    MdfFilterConfig4, MdfOldConfig4 and/or MdfScdConfig4 structures initialization

    WARNING : only structures are filled, no specific init function call for filter
  */
  MdfFilterConfig4.DataSource = MDF_DATA_SOURCE_BSMX;
  MdfFilterConfig4.Delay = 0;
  MdfFilterConfig4.CicMode = MDF_ONE_FILTER_SINC5;
  MdfFilterConfig4.DecimationRatio = 32;
  MdfFilterConfig4.Offset = 0;
  MdfFilterConfig4.Gain = 0;
  MdfFilterConfig4.ReshapeFilter.Activation = ENABLE;
  MdfFilterConfig4.ReshapeFilter.DecimationRatio = MDF_RSF_DECIMATION_RATIO_4;
  MdfFilterConfig4.HighPassFilter.Activation = ENABLE;
  MdfFilterConfig4.HighPassFilter.CutOffFrequency = MDF_HPF_CUTOFF_0_00125FPCM;
  MdfFilterConfig4.Integrator.Activation = DISABLE;
  MdfFilterConfig4.SoundActivity.Activation = DISABLE;
  MdfFilterConfig4.AcquisitionMode = MDF_MODE_ASYNC_CONT;
  MdfFilterConfig4.FifoThreshold = MDF_FIFO_THRESHOLD_NOT_EMPTY;
  MdfFilterConfig4.DiscardSamples = 0;

  /**
    MdfHandle5 structure initialization and HAL_MDF_Init function call
  */
  MdfHandle5.Instance = MDF1_Filter5;
  MdfHandle5.Init.CommonParam.InterleavedFilters = 5;
  MdfHandle5.Init.CommonParam.ProcClockDivider = 2;
  MdfHandle5.Init.CommonParam.OutputClock.Activation = ENABLE;
  MdfHandle5.Init.CommonParam.OutputClock.Pins = MDF_OUTPUT_CLOCK_0;
  MdfHandle5.Init.CommonParam.OutputClock.Divider = 2;
  MdfHandle5.Init.CommonParam.OutputClock.Trigger.Activation = DISABLE;
  MdfHandle5.Init.SerialInterface.Activation = DISABLE;
  if (HAL_MDF_Init(&MdfHandle5) != HAL_OK)
  {
    Error_Handler();
  }

  /**
    MdfFilterConfig5, MdfOldConfig5 and/or MdfScdConfig5 structures initialization

    WARNING : only structures are filled, no specific init function call for filter
  */
  MdfFilterConfig5.DataSource = MDF_DATA_SOURCE_BSMX;
  MdfFilterConfig5.Delay = 0;
  MdfFilterConfig5.CicMode = MDF_ONE_FILTER_SINC5;
  MdfFilterConfig5.DecimationRatio = 32;
  MdfFilterConfig5.Offset = 0;
  MdfFilterConfig5.Gain = 0;
  MdfFilterConfig5.ReshapeFilter.Activation = ENABLE;
  MdfFilterConfig5.ReshapeFilter.DecimationRatio = MDF_RSF_DECIMATION_RATIO_4;
  MdfFilterConfig5.HighPassFilter.Activation = ENABLE;
  MdfFilterConfig5.HighPassFilter.CutOffFrequency = MDF_HPF_CUTOFF_0_00125FPCM;
  MdfFilterConfig5.Integrator.Activation = DISABLE;
  MdfFilterConfig5.SoundActivity.Activation = DISABLE;
  MdfFilterConfig5.AcquisitionMode = MDF_MODE_ASYNC_CONT;
  MdfFilterConfig5.FifoThreshold = MDF_FIFO_THRESHOLD_NOT_EMPTY;
  MdfFilterConfig5.DiscardSamples = 0;
  /* USER CODE BEGIN MDF1_Init 2 */

  /* USER CODE END MDF1_Init 2 */

}

/**
  * @brief SDMMC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SDMMC1_SD_Init(void)
{

  /* USER CODE BEGIN SDMMC1_Init 0 */

  /* USER CODE END SDMMC1_Init 0 */

  /* USER CODE BEGIN SDMMC1_Init 1 */

  /* USER CODE END SDMMC1_Init 1 */
  hsd1.Instance = SDMMC1;
  hsd1.Init.ClockEdge = SDMMC_CLOCK_EDGE_FALLING;
  hsd1.Init.ClockPowerSave = SDMMC_CLOCK_POWER_SAVE_DISABLE;
  hsd1.Init.BusWide = SDMMC_BUS_WIDE_1B;
  hsd1.Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_DISABLE;
  hsd1.Init.ClockDiv = 0;
  if (HAL_SD_Init(&hsd1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SDMMC1_Init 2 */

  /* USER CODE END SDMMC1_Init 2 */

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
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(led_yellow_GPIO_Port, led_yellow_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : led_yellow_Pin */
  GPIO_InitStruct.Pin = led_yellow_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(led_yellow_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PB4 */
  GPIO_InitStruct.Pin = GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* ------Interrupt funcitons -------*/
void HAL_MDF_AcqCpltCallback(MDF_HandleTypeDef *hmdf){
  // set complete flag to 1
  CPLT = 1;
}
void HAL_MDF_AcqHalfCpltCallback(MDF_HandleTypeDef *hmdf){
  // set half complete flag to 1
  HALFCPLT = 1;
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
