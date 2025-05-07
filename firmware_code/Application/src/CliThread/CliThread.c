
/**************************************************************************/ /**
 * @file      CliThread.c
 * @brief     File for the CLI Thread handler. Uses FREERTOS + CLI
 * @author    Eduardo Garcia
 * @date      2025-05-07

 ******************************************************************************/

/******************************************************************************
 * Includes
 ******************************************************************************/
#include "CliThread.h"
#include "SerialConsole.h"
#include "FreeRTOS.h"
#include "task.h"
#include "WifiHandlerThread/WifiHandler.h"
#include "ControlTask/ControlTask.h"

/******************************************************************************
 * Defines
 ******************************************************************************/
#define FIRMWARE_VERSION "0.0.1" // Firmware version that can be easily modified

/******************************************************************************
 * Forward Declarations
 ******************************************************************************/
static void FreeRTOS_read(char *character);

// Forward declarations for CLI command functions
BaseType_t CLI_GetVersion(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString);
BaseType_t CLI_GetTicks(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString);
BaseType_t CLI_FirmwareUpdate(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString);
// --- Motion CLI Command Function Declarations ---
BaseType_t CLI_Forward(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString);
BaseType_t CLI_Backward(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString);
BaseType_t CLI_LeftShift(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString);
BaseType_t CLI_RightShift(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString);
BaseType_t CLI_SayHi(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString);
BaseType_t CLI_Lie(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString);
BaseType_t CLI_Fighting(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString);
BaseType_t CLI_Pushup(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString);
BaseType_t CLI_Sleep(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString);
BaseType_t CLI_Dance1(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString);
BaseType_t CLI_Dance2(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString);
BaseType_t CLI_Dance3(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString);

/******************************************************************************
 * Variables
 ******************************************************************************/
static int8_t *const pcWelcomeMessage =
    "FreeRTOS CLI.\r\nType Help to view a list of registered commands.\r\n";

// Clear screen command
const CLI_Command_Definition_t xClearScreen =
    {
        CLI_COMMAND_CLEAR_SCREEN,
        CLI_HELP_CLEAR_SCREEN,
        CLI_CALLBACK_CLEAR_SCREEN,
        CLI_PARAMS_CLEAR_SCREEN};

// Reset command
static const CLI_Command_Definition_t xResetCommand =
    {
        "reset",
        "reset: Resets the device\r\n",
        (const pdCOMMAND_LINE_CALLBACK)CLI_ResetDevice,
        0};

// Version command
static const CLI_Command_Definition_t xVersionCommand =
    {
        "version",
        "version: Displays the firmware version\r\n",
        (const pdCOMMAND_LINE_CALLBACK)CLI_GetVersion,
        0};

// Ticks command
static const CLI_Command_Definition_t xTicksCommand =
    {
        "ticks",
        "ticks: Displays the number of ticks since scheduler start\r\n",
        (const pdCOMMAND_LINE_CALLBACK)CLI_GetTicks,
        0};

// OTAU Command
static const CLI_Command_Definition_t xOtauCommand =
    {
        "otau",
        "otau: Initiates Over-the-Air Firmware Update\r\n",
        (const pdCOMMAND_LINE_CALLBACK)CLI_OTAU,
        0};

// Firmware Update Command
static const CLI_Command_Definition_t xFirmwareUpdateCommand =
    {
        "fw",
        "fw: Downloads and installs firmware update\r\n",
        (const pdCOMMAND_LINE_CALLBACK)CLI_FirmwareUpdate,
        0};

// Golden image command
static const CLI_Command_Definition_t xGoldCommand = {
	"gold", 
	"gold:\r\n Create golden image of current firmware as g_application.bin\r\n",
	 CLI_Gold, 
	 0
};
	
static const CLI_Command_Definition_t xForwardCommand = {
	"forward",
	"forward: Move Forward\r\n",
	CLI_Forward,
	0
};

static const CLI_Command_Definition_t xBackwardCommand = {
	"backward",
	"backward: Move Backward\r\n",
	CLI_Backward,
	0
};

static const CLI_Command_Definition_t xLeftShiftCommand = {
	"left",
	"left: Shift Left\r\n",
	CLI_LeftShift,
	0
};

static const CLI_Command_Definition_t xRightShiftCommand = {
	"right",
	"right: Shift Right\r\n",
	CLI_RightShift,
	0
};

static const CLI_Command_Definition_t xSayHiCommand = {
	"hi",
	"hi: Say Hi\r\n",
	CLI_SayHi,
	0
};

static const CLI_Command_Definition_t xLieCommand = {
	"lie",
	"lie: Lie Down\r\n",
	CLI_Lie,
	0
};

static const CLI_Command_Definition_t xFightingCommand = {
	"fight",
	"fight: Fighting Mode\r\n",
	CLI_Fighting,
	0
};

static const CLI_Command_Definition_t xPushupCommand = {
	"pushup",
	"pushup: Do Push-up\r\n",
	CLI_Pushup,
	0
};

static const CLI_Command_Definition_t xSleepCommand = {
	"sleep",
	"sleep: Sleep Mode\r\n",
	CLI_Sleep,
	0
};

static const CLI_Command_Definition_t xDance1Command = {
	"dance1",
	"dance1: Perform Dance 1\r\n",
	CLI_Dance1,
	0
};

static const CLI_Command_Definition_t xDance2Command = {
	"dance2",
	"dance2: Perform Dance 2\r\n",
	CLI_Dance2,
	0
};

static const CLI_Command_Definition_t xDance3Command = {
	"dance3",
	"dance3: Perform Dance 3\r\n",
	CLI_Dance3,
	0
};


/******************************************************************************
 * Forward Declarations
 ******************************************************************************/
static void FreeRTOS_read(char *character);

/******************************************************************************
 * External Declarations
 ******************************************************************************/
extern SemaphoreHandle_t rxSemaphore; // External semaphore from SerialConsole.c
extern bool is_state_set(download_state mask); // External function to check WiFi states

/******************************************************************************
 * Callback Functions
 ******************************************************************************/

/******************************************************************************
 * CLI Thread
 ******************************************************************************/

void vCommandConsoleTask(void *pvParameters)
{
    // REGISTER COMMANDS HERE
    FreeRTOS_CLIRegisterCommand(&xClearScreen);
    FreeRTOS_CLIRegisterCommand(&xResetCommand);
    FreeRTOS_CLIRegisterCommand(&xVersionCommand);  // Register version command
    FreeRTOS_CLIRegisterCommand(&xTicksCommand);    // Register ticks command
    FreeRTOS_CLIRegisterCommand(&xOtauCommand);     // Register OTAU command
    FreeRTOS_CLIRegisterCommand(&xFirmwareUpdateCommand);  // Register fw command
	FreeRTOS_CLIRegisterCommand(&xGoldCommand);
	FreeRTOS_CLIRegisterCommand(&xForwardCommand);
    FreeRTOS_CLIRegisterCommand(&xBackwardCommand);
    FreeRTOS_CLIRegisterCommand(&xLeftShiftCommand);
    FreeRTOS_CLIRegisterCommand(&xRightShiftCommand);
    FreeRTOS_CLIRegisterCommand(&xSayHiCommand);
    FreeRTOS_CLIRegisterCommand(&xLieCommand);
    FreeRTOS_CLIRegisterCommand(&xFightingCommand);
    FreeRTOS_CLIRegisterCommand(&xPushupCommand);
    FreeRTOS_CLIRegisterCommand(&xSleepCommand);
    FreeRTOS_CLIRegisterCommand(&xDance1Command);
    FreeRTOS_CLIRegisterCommand(&xDance2Command);
    FreeRTOS_CLIRegisterCommand(&xDance3Command);

    uint8_t cRxedChar[2], cInputIndex = 0;
    BaseType_t xMoreDataToFollow;
    /* The input and output buffers are declared static to keep them off the stack. */
    static char pcOutputString[MAX_OUTPUT_LENGTH_CLI], pcInputString[MAX_INPUT_LENGTH_CLI];
    static char pcLastCommand[MAX_INPUT_LENGTH_CLI];
    static bool isEscapeCode = false;
    static char pcEscapeCodes[4];
    static uint8_t pcEscapeCodePos = 0;

    // Any semaphores/mutexes/etc you needed to be initialized, you can do them here

    /* This code assumes the peripheral being used as the console has already
    been opened and configured, and is passed into the task as the task
    parameter.  Cast the task parameter to the correct type. */

    /* Send a welcome message to the user knows they are connected. */
    SerialConsoleWriteString(pcWelcomeMessage);
    char rxChar;
    for (;;)
    {
        /* This implementation reads a single character at a time.  Wait in the
        Blocked state until a character is received. */

        FreeRTOS_read(&cRxedChar);

        if (cRxedChar[0] == '\n' || cRxedChar[0] == '\r')
        {
            /* A newline character was received, so the input command string is
            complete and can be processed.  Transmit a line separator, just to
            make the output easier to read. */
            SerialConsoleWriteString("\r\n");
            // Copy for last command
            isEscapeCode = false;
            pcEscapeCodePos = 0;
            strncpy(pcLastCommand, pcInputString, MAX_INPUT_LENGTH_CLI - 1);
            pcLastCommand[MAX_INPUT_LENGTH_CLI - 1] = 0; // Ensure null termination

            /* The command interpreter is called repeatedly until it returns
            pdFALSE.  See the "Implementing a command" documentation for an
            explanation of why this is. */
            do
            {
                /* Send the command string to the command interpreter.  Any
                output generated by the command interpreter will be placed in the
                pcOutputString buffer. */
                xMoreDataToFollow = FreeRTOS_CLIProcessCommand(
                    pcInputString,        /* The command string.*/
                    pcOutputString,       /* The output buffer. */
                    MAX_OUTPUT_LENGTH_CLI /* The size of the output buffer. */
                );

                /* Write the output generated by the command interpreter to the
                console. */
                // Ensure it is null terminated
                pcOutputString[MAX_OUTPUT_LENGTH_CLI - 1] = 0;
                SerialConsoleWriteString(pcOutputString);

            } while (xMoreDataToFollow != pdFALSE);

            /* All the strings generated by the input command have been sent.
            Processing of the command is complete.  Clear the input string ready
            to receive the next command. */
            cInputIndex = 0;
            memset(pcInputString, 0x00, MAX_INPUT_LENGTH_CLI);
        }
        else
        {
            /* The if() clause performs the processing after a newline character
    is received.  This else clause performs the processing if any other
    character is received. */

            if (true == isEscapeCode)
            {

                if (pcEscapeCodePos < CLI_PC_ESCAPE_CODE_SIZE)
                {
                    pcEscapeCodes[pcEscapeCodePos++] = cRxedChar[0];
                }
                else
                {
                    isEscapeCode = false;
                    pcEscapeCodePos = 0;
                }

                if (pcEscapeCodePos >= CLI_PC_MIN_ESCAPE_CODE_SIZE)
                {

                    // UP ARROW SHOW LAST COMMAND
                    if (strcasecmp(pcEscapeCodes, "oa"))
                    {
                        /// Delete current line and add prompt (">")
                        sprintf(pcInputString, "%c[2K\r>", 27);
                        SerialConsoleWriteString(pcInputString);
                        /// Clear input buffer
                        cInputIndex = 0;
                        memset(pcInputString, 0x00, MAX_INPUT_LENGTH_CLI);
                        /// Send last command
                        strncpy(pcInputString, pcLastCommand, MAX_INPUT_LENGTH_CLI - 1);
                        cInputIndex = (strlen(pcInputString) < MAX_INPUT_LENGTH_CLI - 1) ? strlen(pcLastCommand) : MAX_INPUT_LENGTH_CLI - 1;
                        SerialConsoleWriteString(pcInputString);
                    }

                    isEscapeCode = false;
                    pcEscapeCodePos = 0;
                }
            }
            /* The if() clause performs the processing after a newline character
            is received.  This else clause performs the processing if any other
            character is received. */

            else if (cRxedChar[0] == '\r')
            {
                /* Ignore carriage returns. */
            }
            else if (cRxedChar[0] == ASCII_BACKSPACE || cRxedChar[0] == ASCII_DELETE)
            {
                char erase[4] = {0x08, 0x20, 0x08, 0x00};
                SerialConsoleWriteString(erase);
                /* Backspace was pressed.  Erase the last character in the input
                buffer - if there are any. */
                if (cInputIndex > 0)
                {
                    cInputIndex--;
                    pcInputString[cInputIndex] = 0;
                }
            }
            // ESC
            else if (cRxedChar[0] == ASCII_ESC)
            {
                isEscapeCode = true; // Next characters will be code arguments
                pcEscapeCodePos = 0;
            }
            else
            {
                /* A character was entered.  It was not a new line, backspace
                or carriage return, so it is accepted as part of the input and
                placed into the input buffer.  When a n is entered the complete
                string will be passed to the command interpreter. */
                if (cInputIndex < MAX_INPUT_LENGTH_CLI)
                {
                    pcInputString[cInputIndex] = cRxedChar[0];
                    cInputIndex++;
                }

                // Order Echo
                cRxedChar[1] = 0;
                SerialConsoleWriteString(&cRxedChar[0]);
            }
        }
    }
}

/**************************************************************************/ /**
 * @fn			void FreeRTOS_read(char* character)
 * @brief		Blocks the CLI thread until a character is received from the UART
 * @details		This function waits on a semaphore that is given by the UART
 *              receive callback. Once the semaphore is available, it reads a
 *              character from the circular buffer and returns it.
 * @param[out]  character Pointer to store the received character
 * @note        This function uses a blocking semaphore, so the thread will be
 *              suspended until a character is available.
 *****************************************************************************/
static void FreeRTOS_read(char *character)
{
    uint8_t rxChar;
    
    // Wait until a character is available (semaphore given by usart_read_callback)
    xSemaphoreTake(rxSemaphore, portMAX_DELAY);
    
    // Once semaphore is obtained, get the character from the circular buffer
    if (SerialConsoleReadCharacter(&rxChar) != -1) {
        *character = (char)rxChar;
    }
    else {
        // No character available in buffer despite semaphore being given
        // This shouldn't happen, but handle it anyway
        *character = 0;
    }
}

/******************************************************************************
 * CLI Functions - Define here
 ******************************************************************************/

// THIS COMMAND USES vt100 TERMINAL COMMANDS TO CLEAR THE SCREEN ON A TERMINAL PROGRAM LIKE TERA TERM
// SEE http://www.csie.ntu.edu.tw/~r92094/c++/VT100.html for more info
// CLI SPECIFIC COMMANDS
static char bufCli[CLI_MSG_LEN];
BaseType_t xCliClearTerminalScreen(char *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString)
{
    char clearScreen = ASCII_ESC;
    snprintf(bufCli, CLI_MSG_LEN - 1, "%c[2J", clearScreen);
    snprintf(pcWriteBuffer, xWriteBufferLen, bufCli);
    return pdFALSE;
}

// Example CLI Command. Resets system.
BaseType_t CLI_ResetDevice(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString)
{
    system_reset();
    return pdFALSE;
}



/**************************************************************************/ /**
 * @fn			BaseType_t CLI_GetVersion(int8_t *pcWriteBuffer, size_t xWriteBufferLen, 
 *                                       const int8_t *pcCommandString)
 * @brief		Displays the firmware version
 * @details		This function is called when the user enters the "version" command
 *              in the CLI. It outputs the firmware version defined at the top of this file.
 * @param[out]  pcWriteBuffer Buffer to write the output to
 * @param[in]   xWriteBufferLen Maximum size of the output buffer
 * @param[in]   pcCommandString Command string (not used in this function)
 * @return      pdFALSE to indicate that the command is complete
 * @note        The firmware version can be changed by modifying the FIRMWARE_VERSION define.
 *****************************************************************************/
BaseType_t CLI_GetVersion(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString)
{
    snprintf((char *)pcWriteBuffer, xWriteBufferLen, "Firmware Version: %s\r\n", FIRMWARE_VERSION);
    return pdFALSE;
}

/**************************************************************************/ /**
 * @fn			BaseType_t CLI_GetTicks(int8_t *pcWriteBuffer, size_t xWriteBufferLen, 
 *                                     const int8_t *pcCommandString)
 * @brief		Displays the number of ticks since the scheduler was started
 * @details		This function is called when the user enters the "ticks" command
 *              in the CLI. It outputs the current tick count from the FreeRTOS scheduler.
 * @param[out]  pcWriteBuffer Buffer to write the output to
 * @param[in]   xWriteBufferLen Maximum size of the output buffer
 * @param[in]   pcCommandString Command string (not used in this function)
 * @return      pdFALSE to indicate that the command is complete
 * @note        Uses the FreeRTOS xTaskGetTickCount() function to get the tick count.
 *****************************************************************************/
BaseType_t CLI_GetTicks(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString)
{
    TickType_t currentTicks = xTaskGetTickCount();
    snprintf((char *)pcWriteBuffer, xWriteBufferLen, "Tick count: %lu\r\n", (unsigned long)currentTicks);
    return pdFALSE;
}

/**
 * @brief Command to initiate Over-the-Air Firmware Update
 * 
 * This function initiates the firmware download from the server,
 * saves it to the SD card, creates a flag file for the bootloader,
 * and resets the device to start the update process.
 *
 * @param pcWriteBuffer Buffer where command output will be written
 * @param xWriteBufferLen Length of the write buffer
 * @param pcCommandString Command string
 * @return pdFALSE to indicate command is complete
 */
BaseType_t CLI_OTAU(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString)
{
    // First check if SD card is ready
    if (!is_state_set(STORAGE_READY)) {
        snprintf((char *)pcWriteBuffer, xWriteBufferLen, 
                "SD card not ready. Please insert an SD card and try again.\r\n");
        return pdFALSE;
    }
    
    // Check if WiFi is connected
    if (!is_state_set(WIFI_CONNECTED)) {
        snprintf((char *)pcWriteBuffer, xWriteBufferLen, 
                "WiFi not connected. Please check WiFi connection and try again.\r\n");
        return pdFALSE;
    }
    
    // Initialize the flag to start firmware download
    snprintf((char *)pcWriteBuffer, xWriteBufferLen, 
            "Starting firmware update process...\r\n"
            "Downloading firmware from server...\r\n");
    
    // Set the WiFi state to download mode
    WifiHandlerSetState(WIFI_DOWNLOAD_INIT);
    
    // Add a flag file after download completes for the bootloader
    // This will be handled in the HTTP_DownloadFileTransaction function
    
    return pdFALSE;
}

/**
 * @brief Command to initiate Firmware Update download
 * 
 * This function initiates the firmware download from the server,
 * saves it to the SD card, creates a flag file for the bootloader,
 * and resets the device to start the update process.
 *
 * @param pcWriteBuffer Buffer where command output will be written
 * @param xWriteBufferLen Length of the write buffer
 * @param pcCommandString Command string
 * @return pdFALSE to indicate command is complete
 */
BaseType_t CLI_FirmwareUpdate(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString)
{
    // First check if SD card is ready
    if (!is_state_set(STORAGE_READY)) {
        snprintf((char *)pcWriteBuffer, xWriteBufferLen, 
                "SD card not ready. Please insert an SD card and try again.\r\n");
        return pdFALSE;
    }
    
    // Check if WiFi is connected
    if (!is_state_set(WIFI_CONNECTED)) {
        snprintf((char *)pcWriteBuffer, xWriteBufferLen, 
                "WiFi not connected. Please check WiFi connection and try again.\r\n");
        return pdFALSE;
    }
    
    // Initialize the flag to start firmware download
    snprintf((char *)pcWriteBuffer, xWriteBufferLen, 
            "Starting firmware update process...\r\n"
            "Downloading firmware from server...\r\n");
    
    // Set the WiFi state to download mode
    WifiHandlerSetState(WIFI_DOWNLOAD_INIT);
    
    return pdFALSE;
}

// This function creates a golden backup copy of "Application.bin" as "g_application.bin" on the SD card.
// It is intended to be used as a CLI command.
BaseType_t CLI_Gold(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString)
{
	const char *source_path = "0:/Application.bin";
	const char *dest_path = "0:/g_application.bin";
	
	FIL src_file, dst_file;
	FRESULT res;
	UINT bytes_read, bytes_written;
	uint8_t buffer[256];
	
	SerialConsoleWriteString("Enter gold cmd. Creating Golden copy.\r\n");
	
	// Step 1: Open source file
	res = f_open(&src_file, source_path, FA_READ);
	if (res != FR_OK) {
		snprintf((char *)pcWriteBuffer, xWriteBufferLen, "Failed to open %s (err %d)\r\n", source_path, res);
		return pdFALSE;
	}
	
	// Step 2: Create destination file
	res = f_open(&dst_file, dest_path, FA_WRITE | FA_CREATE_ALWAYS);
	if (res != FR_OK) {
		f_close(&src_file);
		snprintf((char *)pcWriteBuffer, xWriteBufferLen, "Failed to create %s (err %d)\r\n", dest_path, res);
		return pdFALSE;
	}
	
	// Step 3: Copy contents
	do {
		res = f_read(&src_file, buffer, sizeof(buffer), &bytes_read);
		if (res != FR_OK) break;

		res = f_write(&dst_file, buffer, bytes_read, &bytes_written);
		if (res != FR_OK || bytes_read != bytes_written) break;
	} while (bytes_read > 0);

	f_close(&src_file);
	f_close(&dst_file);

	if (res == FR_OK) {
		snprintf((char *)pcWriteBuffer, xWriteBufferLen, "Golden copy created: g_application.bin\r\n");
		} else {
		snprintf((char *)pcWriteBuffer, xWriteBufferLen, "Copy failed (err %d)\r\n", res);
	}

	return pdFALSE;
}

// Forward
BaseType_t CLI_Forward(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString)
{
	current_state = STATE_FORWARD;
	snprintf((char *)pcWriteBuffer, xWriteBufferLen, "Move: Forward\r\n");
	return pdFALSE;
}

// Backward
BaseType_t CLI_Backward(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString)
{
	current_state = STATE_BACKWARD;
	snprintf((char *)pcWriteBuffer, xWriteBufferLen, "Move: Backward\r\n");
	return pdFALSE;
}

// Left Shift
BaseType_t CLI_LeftShift(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString)
{
	current_state = STATE_LEFT_SHIFT;
	snprintf((char *)pcWriteBuffer, xWriteBufferLen, "Move: Left Shift\r\n");
	return pdFALSE;
}

// Right Shift
BaseType_t CLI_RightShift(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString)
{
	current_state = STATE_RIGHT_SHIFT;
	snprintf((char *)pcWriteBuffer, xWriteBufferLen, "Move: Right Shift\r\n");
	return pdFALSE;
}

// Say Hi
BaseType_t CLI_SayHi(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString)
{
	current_state = STATE_SAY_HI;
	snprintf((char *)pcWriteBuffer, xWriteBufferLen, "Move: Say Hi\r\n");
	return pdFALSE;
}

// Lie
BaseType_t CLI_Lie(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString)
{
	current_state = STATE_LIE;
	snprintf((char *)pcWriteBuffer, xWriteBufferLen, "Move: Lie Down\r\n");
	return pdFALSE;
}

// Fighting
BaseType_t CLI_Fighting(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString)
{
	current_state = STATE_FIGHTING;
	snprintf((char *)pcWriteBuffer, xWriteBufferLen, "Move: Fighting Mode\r\n");
	return pdFALSE;
}

// Pushup
BaseType_t CLI_Pushup(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString)
{
	current_state = STATE_PUSHUP;
	snprintf((char *)pcWriteBuffer, xWriteBufferLen, "Move: Push-up\r\n");
	return pdFALSE;
}

// Sleep
BaseType_t CLI_Sleep(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString)
{
	current_state = STATE_SLEEP;
	snprintf((char *)pcWriteBuffer, xWriteBufferLen, "Move: Sleep Mode\r\n");
	return pdFALSE;
}

// Dance1
BaseType_t CLI_Dance1(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString)
{
	current_state = STATE_DANCE1;
	snprintf((char *)pcWriteBuffer, xWriteBufferLen, "Move: Dance 1\r\n");
	return pdFALSE;
}

// Dance2
BaseType_t CLI_Dance2(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString)
{
	current_state = STATE_DANCE2;
	snprintf((char *)pcWriteBuffer, xWriteBufferLen, "Move: Dance 2\r\n");
	return pdFALSE;
}

// Dance3
BaseType_t CLI_Dance3(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString)
{
	current_state = STATE_DANCE3;
	snprintf((char *)pcWriteBuffer, xWriteBufferLen, "Move: Dance 3\r\n");
	return pdFALSE;
}

