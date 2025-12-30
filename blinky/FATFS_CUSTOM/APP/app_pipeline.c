/**
  ******************************************************************************
  * @file    app_pipeline.c
  * @author  SWTS
  * @brief   Pipeline for SELD AudioAcqu
  ******************************************************************************
  * 
  *
  ******************************************************************************
  */



#include "app_pipeline.h"
#include "ff.h"
#include "stm32u5xx_hal_mdf.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>

/*----GLOBAL VARIABLES FATFS ----- */

FIL SD_File_PIPELINE;
FATFS SD_FATFS_PIPELINE;
char SD_Path_PIPELINE[4];
uint32_t bytes_to_read; 


/*-----EXTERNAL DRIVER------*/
// defined in sd_diskio_dma_standalone.c

extern const Diskio_drvTypeDef SD_DMA_Driver;


// functions to be used later in this file
static FRESULT Write_WAV_Header(uint32_t sample_rate, 
                                uint16_t channels,
                                uint16_t bits_per_sample,
                                uint32_t time /* amount of audio recorded in seconds*/);





// main function
int32_t SD_Pipeline_NewRec(const char* filename,
        uint32_t rec_number, 
        uint32_t sample_rate, 
        uint16_t channels,
        uint16_t bits_per_sample,
        uint32_t time){
        
    
    // filename calc
    char fname [64];
    snprintf(fname, 64, "%s_%lu.WAV", filename, (unsigned long)rec_number);
    //create file
    if(f_open(&SD_File_PIPELINE, fname, FA_CREATE_ALWAYS | FA_WRITE  ) != FR_OK){
        return PIPELINE_ERROR;
    }

    // write WAV header
    if(Write_WAV_Header(sample_rate,channels,bits_per_sample, time) != FR_OK){
        return PIPELINE_ERROR;
    }

    
    return PIPELINE_OK;
}

//record
int32_t SD_Pipeline_Write(uint8_t* pData, uint32_t size) {
    UINT bytesWritten;

    if ((uint32_t)pData % 32 != 0) {
        // printf("FAILED ALIGNMENT! Buffer: 0x%lX (Remainder: %lu)\n", (uint32_t)pData, (uint32_t)pData % 32);
    }

    FRESULT res = f_write(&SD_File_PIPELINE, pData, size, &bytesWritten);
    if (res != FR_OK) {
        // printf("[SD] Attempted f write. Size: %lu bytes.\n", size);

        // printf("F_WRITE FAILED! Error Code: %d\n", res);
        return PIPELINE_ERROR;
    }
    if(bytesWritten != size) {
        // printf("WRITE SIZE MISMATCH! Requested %lu, Wrote %u\n", size, bytesWritten);
    }

    return PIPELINE_OK;
}

// stop recording 
uint32_t SD_Pipeline_StopRec(){
    f_sync(&SD_File_PIPELINE);
    return f_close(&SD_File_PIPELINE);
}


/**
  * @brief  Write header for WAV file
  * @param time amount of audio recorded in seconds
  */
static FRESULT Write_WAV_Header(uint32_t sample_rate, 
                                uint16_t channels,
                                uint16_t bits_per_sample,
                                uint32_t time /* amount of audio recorded in seconds*/)
{
    
    uint8_t header[44];
    UINT bytesWritten;
    uint32_t byteRate = sample_rate * channels * (bits_per_sample / 8);
    bytes_to_read = byteRate*time;
    uint32_t subChunk2Size = bytes_to_read;
    uint32_t chunkSize = 36 + subChunk2Size;

    /* RIFF Chunk */
    memcpy(header, "RIFF", 4);
    memcpy(&header[4], &chunkSize, 4);
    memcpy(&header[8], "WAVE", 4);

    /* fmt Chunk */
    memcpy(&header[12], "fmt ", 4);
    uint32_t subChunk1Size = 16;
    memcpy(&header[16], &subChunk1Size, 4);
    uint16_t audioFormat = 1; // PCM
    memcpy(&header[20], &audioFormat, 2);
    memcpy(&header[22], &channels, 2);
    memcpy(&header[24], &sample_rate, 4);
    memcpy(&header[28], &byteRate, 4);
    uint16_t blockAlign = channels * (bits_per_sample / 8);
    memcpy(&header[32], &blockAlign, 2);
    memcpy(&header[34], &bits_per_sample, 2);

    /* data Chunk */
    memcpy(&header[36], "data", 4);
    memcpy(&header[40], &subChunk2Size, 4);

    /* Move pointer to beginning of file and allocate whole memory */
    f_lseek(&SD_File_PIPELINE, bytes_to_read + 44);
    f_lseek(&SD_File_PIPELINE, 0);
    
    /* Overwrite placeholder with real header */
    return f_write(&SD_File_PIPELINE, header, 44, &bytesWritten);
    
}
   

/**
* @brief  Initialises and mounts a FAT SD card
*/
int32_t SD_Pipeline_Init(void){
    /* 1. Link the SD Driver */
    if (FATFS_LinkDriver(&SD_DMA_Driver, SD_Path_PIPELINE) != 0){
        return PIPELINE_ERROR;
    }
    /* 2. Mount the SD Card */
    /* Force mount (1) to check if card is inserted immediately */
    if (f_mount(&SD_FATFS_PIPELINE, (TCHAR const*)SD_Path_PIPELINE, 1) != FR_OK){
        return PIPELINE_ERROR;
    }

    return PIPELINE_OK;
}