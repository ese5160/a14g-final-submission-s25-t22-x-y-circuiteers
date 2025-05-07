/**
 * @file      BootMain.c
 * @brief     Main file for the ESE5160 bootloader
 * @author    Eduardo Garcia
 * @date      2024-03-03
 * @version   2.0
 * @copyright Copyright University of Pennsylvania
 ******************************************************************************/

#include "conf_example.h"
#include "sd_mmc_spi.h"
#include <asf.h>
#include <string.h>

#include "ASF/sam0/drivers/dsu/crc32/crc32.h"
#include "SD Card/SdCard.h"
#include "SerialConsole/SerialConsole.h"
#include "Systick/Systick.h"

/* Defines */
#define APP_START_ADDRESS           ((uint32_t) 0x12000)
#define APP_START_RESET_VEC_ADDRESS (APP_START_ADDRESS + (uint32_t) 0x04)

/* Function Declarations */
static void jumpToApplication(void);
static bool StartFilesystemAndTest(void);
static void configure_nvm(void);
static bool flash_firmware(const char* filename);

/* Global Variables */
Ctrl_status status;
FRESULT res;
FATFS fs;
FIL file_object;


int main(void) {

	/*1.) INIT SYSTEM PERIPHERALS INITIALIZATION*/
	system_init();
	delay_init();
	InitializeSerialConsole();
	system_interrupt_enable_global();
	
	/* Initialize SD MMC stack */
	sd_mmc_init();

	// Initialize the NVM driver
	configure_nvm();

	irq_initialize_vectors();
	cpu_irq_enable();

	// Configure CRC32
	dsu_crc32_init();

	SerialConsoleWriteString("ESE5160 - ENTER BOOTLOADER");   // Order to add string to TX Buffer

	/*END SYSTEM PERIPHERALS INITIALIZATION*/

	/*2.) STARTS SIMPLE SD CARD MOUNTING AND TEST!*/

	// EXAMPLE CODE ON MOUNTING THE SD CARD AND WRITING TO A FILE
	// See function inside to see how to open a file
	SerialConsoleWriteString("\x0C\n\r-- SD/MMC Card Example on FatFs --\n\r");

	if (StartFilesystemAndTest() == false) {
		SerialConsoleWriteString("SD CARD failed! Check your connections. System will restart in 5 seconds...");
		delay_cycles_ms(5000);
		NVIC_SystemReset(); 
		} else {
		SerialConsoleWriteString("SD CARD mount success! Filesystem also mounted. \r\n");
	}

	/*END SIMPLE SD CARD MOUNTING AND TEST!*/

	/*3.) STARTS BOOTLOADER HERE!*/
	delay_ms(1000);
	// Students - this is your mission!
	int firmware_update_state = 0;
	
	SerialConsoleWriteString("Checking for firmware update flags...\n\r");
    
    /* Check if update flag exists */
    FIL flagFile;
    if (f_open(&flagFile, "0:FlagA.txt", FA_READ) == FR_OK) {
        f_close(&flagFile);
        SerialConsoleWriteString("Update flag found. Proceeding with update...\r\n");
        
        /* Try to flash the main firmware */
        if (flash_firmware("0:Application.bin")) {
            SerialConsoleWriteString("Main firmware updated successfully.\r\n");
            f_unlink("0:FlagA.txt");
        } 
        else {
            /* Try to flash the golden image instead */
            SerialConsoleWriteString("Main firmware update failed. Trying golden image...\r\n");
            if (flash_firmware("0:g_application.bin")) {
                SerialConsoleWriteString("Golden image flashed successfully.\r\n");
                f_unlink("0:FlagA.txt");
            }
            else {
                SerialConsoleWriteString("Both firmware updates failed. Continuing with existing app.\r\n");
            }
        }
    }
    else {
        SerialConsoleWriteString("No update flag found. Continuing with existing app.\r\n");
    }
    
    SerialConsoleWriteString("ESE5160 - EXIT BOOTLOADER");
    delay_cycles_ms(100);
    DeinitializeSerialConsole();
    sd_mmc_deinit();
    jumpToApplication();
   
}


/**
 * @brief Flash firmware from file into application flash region
 * @param filename Name of the binary file to flash
 * @return true if flashing was successful
 */

static bool flash_firmware(const char* filename) {
	FIL binFile;
	uint8_t buffer[NVMCTRL_PAGE_SIZE];
	UINT bytes_read;
	uint32_t total_read = 0;
	char msg[64];
	
	/* Open firmware file */
	SerialConsoleWriteString("Opening file: ");
	SerialConsoleWriteString(filename);
	SerialConsoleWriteString("\r\n");
	
	if (f_open(&binFile, filename, FA_READ) != FR_OK) {
		SerialConsoleWriteString("Failed to open file\r\n");
		return false;
	}
	
	/* Get file size */
	uint32_t file_size = f_size(&binFile);
	snprintf(msg, sizeof(msg), "File size: %lu bytes\r\n", file_size);
	SerialConsoleWriteString(msg);
	
	if (file_size < 1024) {  // Basic size sanity check
		SerialConsoleWriteString("File too small to be valid firmware\r\n");
		f_close(&binFile);
		return false;
	}
	
	/* Calculate how many rows to erase */
	uint32_t rows_to_erase = (file_size + NVMCTRL_ROW_SIZE - 1) / NVMCTRL_ROW_SIZE;
	
	/* Erase flash area */
	SerialConsoleWriteString("Erasing application area...\r\n");
	for (uint32_t i = 0; i < rows_to_erase; i++) {
		enum status_code erase_status = nvm_erase_row(APP_START_ADDRESS + i * NVMCTRL_ROW_SIZE);
		if (erase_status != STATUS_OK) {
			snprintf(msg, sizeof(msg), "Erase failed at row %lu\r\n", i);
			SerialConsoleWriteString(msg);
			f_close(&binFile);
			return false;
		}
	}
	
	/* Flash the firmware - read and write page by page */
	SerialConsoleWriteString("Flashing firmware...\r\n");
	
	uint32_t current_addr = APP_START_ADDRESS;
	
	while (total_read < file_size) {
		/* Clear buffer and read data */
		memset(buffer, 0xFF, NVMCTRL_PAGE_SIZE);
		
		uint32_t bytes_to_read = (file_size - total_read >= NVMCTRL_PAGE_SIZE) ?
		NVMCTRL_PAGE_SIZE : (file_size - total_read);
		
		FRESULT read_res = f_read(&binFile, buffer, bytes_to_read, &bytes_read);
		
		if (read_res != FR_OK) {
			snprintf(msg, sizeof(msg), "File read error: %d\r\n", read_res);
			SerialConsoleWriteString(msg);
			f_close(&binFile);
			return false;
		}
		
		if (bytes_read == 0) break; 
		
		/* Write full page to flash */
		enum status_code write_status = nvm_write_buffer(current_addr, buffer, NVMCTRL_PAGE_SIZE);
		
		if (write_status != STATUS_OK) {
			snprintf(msg, sizeof(msg), "Flash write error at 0x%08lx: %d\r\n",
			current_addr, write_status);
			SerialConsoleWriteString(msg);
			f_close(&binFile);
			return false;
		}
		
		/* Update tracking */
		current_addr += NVMCTRL_PAGE_SIZE;
		total_read += bytes_read;
		
		/* Progress indicator every 4K */
		if ((total_read % 4096) == 0) {
			SerialConsoleWriteString(".");
		}
	}
	
	f_close(&binFile);
	
	snprintf(msg, sizeof(msg), "\r\nFlashed %lu bytes to address 0x%08lx\r\n",
	total_read, APP_START_ADDRESS);
	SerialConsoleWriteString(msg);
	return true;
}


/**
 * @brief Mounts the SD card filesystem
 * @return true if mount is successful
 */
static bool StartFilesystemAndTest(void) {
    Ctrl_status sdStatus = SdCard_Initiate();
    if (sdStatus == CTRL_GOOD) {
        SerialConsoleWriteString("SD Card initiated correctly!\r\n");
        memset(&fs, 0, sizeof(FATFS));
        res = f_mount(LUN_ID_SD_MMC_0_MEM, &fs);
        if (FR_INVALID_DRIVE == res) {
            SerialConsoleWriteString("Mount failed!\r\n");
            return false;
        }
        SerialConsoleWriteString("SD card mounted successfully.\r\n");
        return true;
    } else {
        SerialConsoleWriteString("SD Card initiation failed!\r\n");
        return false;
    }
}


/**
 * @brief Transfers execution from bootloader to main application
 */
static void jumpToApplication(void) {
    void (*applicationCodeEntry)(void);
    __set_MSP(*(uint32_t *) APP_START_ADDRESS);
    SCB->VTOR = ((uint32_t) APP_START_ADDRESS & SCB_VTOR_TBLOFF_Msk);
    applicationCodeEntry = (void (*)(void))(unsigned *) (*(unsigned *) (APP_START_RESET_VEC_ADDRESS));
    applicationCodeEntry();
}

/**
 * @brief Configures NVM controller for page-based writes
 */
static void configure_nvm(void) {
    struct nvm_config config_nvm;
    nvm_get_config_defaults(&config_nvm);
    config_nvm.manual_page_write = false;
    nvm_set_config(&config_nvm);
}