
/**
  ******************************************************************************
  * @file    app_pipeline.h
  * @author  SWTS
  * @brief   Pipeline for SELD AudioAcqu
  ******************************************************************************
  * 
  *
  ******************************************************************************
  */

#ifndef APP_PIPELINE_H
#define APP_PIPELINE_H

#include <stdint.h>
#include "ff.h"         
#include "ff_gen_drv.h" 
#include "sd_diskio_dma_standalone.h"

// Return codes
#define PIPELINE_OK    0
#define PIPELINE_ERROR -1


// variables shared with main
extern FIL SD_File_PIPELINE;
extern FATFS SD_FATFS_PIPELINE;
extern char SD_Path_PIPELINE[4];
extern uint32_t bytes_to_read; 

// Debugging functions
int32_t SD_ListRootFiles(void);
// initialise
int32_t SD_Pipeline_Init(void);
// start recording
int32_t SD_Pipeline_NewRec(const char* filename,
                uint32_t rec_number, 
                uint32_t sample_rate, 
                uint16_t channels,
                uint16_t bits_per_sample,
                uint32_t time);

//deInit?
uint32_t SD_Pipeline_StopRec();

int32_t SD_Pipeline_Write(uint8_t* pData, uint32_t size);

#endif /* APP_PIPELINE_H */