
/******************************************************************************
 * Includes
 ******************************************************************************/
#include "SerialConsole.h"
#include <stdio.h>         // For vsnprintf
#include <stdarg.h>        // For va_list, va_start, va_end
#include <string.h>
#include "FreeRTOS.h"
#include "semphr.h"

/******************************************************************************
 * Defines
 ******************************************************************************/
#define RX_BUFFER_SIZE 512 ///< Size of character buffer for RX, in bytes
#define TX_BUFFER_SIZE 512 ///< Size of character buffers for TX, in bytes

/******************************************************************************
 * Structures and Enumerations
 ******************************************************************************/
cbuf_handle_t cbufRx; ///< Circular buffer handler for receiving characters
cbuf_handle_t cbufTx; ///< Circular buffer handler for transmitting characters

char latestRx; ///< Holds the latest character received
char latestTx; ///< Holds the latest character to be transmitted

/******************************************************************************
 * Callback Declarations
 ******************************************************************************/
void usart_write_callback(struct usart_module *const usart_module); // Callback for when we finish writing characters to UART
void usart_read_callback(struct usart_module *const usart_module); // Callback for when we finis reading characters from UART

/******************************************************************************
 * Local Function Declarations
 ******************************************************************************/
static void configure_usart(void);
static void configure_usart_callbacks(void);

/******************************************************************************
 * Global Variables
 ******************************************************************************/
struct usart_module usart_instance;
char rxCharacterBuffer[RX_BUFFER_SIZE]; 			   ///< Buffer to store received characters
char txCharacterBuffer[TX_BUFFER_SIZE]; 			   ///< Buffer to store characters to be sent
enum eDebugLogLevels currentDebugLevel = LOG_INFO_LVL; ///< Default debug level
SemaphoreHandle_t rxSemaphore = NULL; ///< Semaphore to signal when new character is available

/******************************************************************************
 * Global Functions
 ******************************************************************************/

/**
 * @brief Initializes the UART and registers callbacks.
 */
void InitializeSerialConsole(void)
{
    // Initialize circular buffers for RX and TX
	cbufRx = circular_buf_init((uint8_t *)rxCharacterBuffer, RX_BUFFER_SIZE);
    cbufTx = circular_buf_init((uint8_t *)txCharacterBuffer, TX_BUFFER_SIZE);

    // Create binary semaphore for signaling character reception
    rxSemaphore = xSemaphoreCreateBinary();
    
    // Configure USART and Callbacks
	configure_usart();
    configure_usart_callbacks();
    NVIC_SetPriority(SERCOM4_IRQn, 10);

    usart_read_buffer_job(&usart_instance, (uint8_t *)&latestRx, 1); // Kicks off constant reading of characters

	// Add any other calls you need to do to initialize your Serial Console
}

/**
 * @brief Deinitializes the UART.
 */
void DeinitializeSerialConsole(void)
{
    usart_disable(&usart_instance);
}

/**
 * @brief Writes a string to the UART.
 * @param string Pointer to the string to send.
 */
void SerialConsoleWriteString(char *string)
{
    if (string != NULL)
    {
        for (size_t iter = 0; iter < strlen(string); iter++)
        {
            circular_buf_put(cbufTx, string[iter]);
        }

        if (usart_get_job_status(&usart_instance, USART_TRANSCEIVER_TX) == STATUS_OK)
        {
            circular_buf_get(cbufTx, (uint8_t *)&latestTx); // Perform only if the SERCOM TX is free (not busy)
            usart_write_buffer_job(&usart_instance, (uint8_t *)&latestTx, 1);
        }
    }
}

/**
 * @brief Reads a character from the RX buffer.
 * @param rxChar Pointer to store the received character.
 * @return -1 if buffer is empty, otherwise the character read.
 */
int SerialConsoleReadCharacter(uint8_t *rxChar)
{
    vTaskSuspendAll();
    int a = circular_buf_get(cbufRx, (uint8_t *)rxChar);
    xTaskResumeAll();
    return a;
}

/**
 * @brief Gets the current debug log level.
 * @return The current debug level.
 */
enum eDebugLogLevels getLogLevel(void)
{
    return currentDebugLevel;
}

/**
 * @brief Sets the debug log level.
 * @param debugLevel The debug level to set.
 */
void setLogLevel(enum eDebugLogLevels debugLevel)
{
    currentDebugLevel = debugLevel;
}

/**
 * @brief Logs a message at the specified debug level.
 */
/**
 * @brief Logs a message if its level is >= the current global log level.
 *
 * @param level   The log level of this message (INFO, DEBUG, WARNING, etc.)
 * @param format  The printf-style format string.
 * @param ...     Additional arguments for the format string.
 *
 * This function uses vsnprintf() to format the message into a buffer
 * and then writes it to the TX circular buffer via SerialConsoleWriteString().
 * Messages with a level lower than the current log level will not be printed.
 */
void LogMessage(enum eDebugLogLevels level, const char *format, ...)

    // Todo: Implement Debug Logger
	// More detailed descriptions are in header file

{
	// Only log if this message's level >= current debug level
	if (level < getLogLevel())
	{
		return;
	}

	char buffer[256]; // Temporary buffer to hold formatted string

	// Handle variable arguments
	va_list args;
	va_start(args, format);
	vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);

	// Send the formatted string to the console
	SerialConsoleWriteString(buffer);
}

/**
 * @fn			LogMessage Debug
 * @brief
 * @note
 */
void LogMessageDebug(const char *format, ...) { LogMessage(LOG_DEBUG_LVL, format); };

/*
COMMAND LINE INTERFACE COMMANDS
*/

/******************************************************************************
 * Local Functions
 ******************************************************************************/

/**************************************************************************/ 
/**
 * @fn			static void configure_usart(void)
 * @brief		Code to configure the SERCOM "EDBG_CDC_MODULE" to be a UART channel running at 115200 8N1
 * @note
 *****************************************************************************/
static void configure_usart(void)
{
	struct usart_config config_usart;
	usart_get_config_defaults(&config_usart);

	config_usart.baudrate = 115200;
	config_usart.mux_setting = EDBG_CDC_SERCOM_MUX_SETTING;
	config_usart.pinmux_pad0 = EDBG_CDC_SERCOM_PINMUX_PAD0;
	config_usart.pinmux_pad1 = EDBG_CDC_SERCOM_PINMUX_PAD1;
	config_usart.pinmux_pad2 = EDBG_CDC_SERCOM_PINMUX_PAD2;
	config_usart.pinmux_pad3 = EDBG_CDC_SERCOM_PINMUX_PAD3;
	while (usart_init(&usart_instance,
					  EDBG_CDC_MODULE,
					  &config_usart) != STATUS_OK)
	{
	}

	usart_enable(&usart_instance);
}

/**************************************************************************/ 
/**
 * @fn			static void configure_usart_callbacks(void)
 * @brief		Code to register callbacks
 * @note
 *****************************************************************************/
static void configure_usart_callbacks(void)
{
	usart_register_callback(&usart_instance,
							usart_write_callback,
							USART_CALLBACK_BUFFER_TRANSMITTED);
	usart_register_callback(&usart_instance,
							usart_read_callback,
							USART_CALLBACK_BUFFER_RECEIVED);
	usart_enable_callback(&usart_instance, USART_CALLBACK_BUFFER_TRANSMITTED);
	usart_enable_callback(&usart_instance, USART_CALLBACK_BUFFER_RECEIVED);
}

/******************************************************************************
 * Callback Functions
 ******************************************************************************/

/**************************************************************************/ 
/**
 * @fn			void usart_read_callback(struct usart_module *const usart_module)
 * @brief		Callback called when the system finishes receiving a byte from UART
 * @details     This function stores the received character in the circular buffer
 *              and signals any waiting threads that a character is available.
 * @param[in]   usart_module Pointer to the USART module
 * @note        This is called from an interrupt context, so it should be kept
 *              as short as possible.
 *****************************************************************************/
void usart_read_callback(struct usart_module *const usart_module)
{
    // Store the received character in the circular buffer
    circular_buf_put(cbufRx, latestRx);
    
    // Notify waiting task that a character is available
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (rxSemaphore != NULL) {
        xSemaphoreGiveFromISR(rxSemaphore, &xHigherPriorityTaskWoken);
    }
    
    // Start another read job for the next character
    usart_read_buffer_job(&usart_instance, (uint8_t *)&latestRx, 1);
    
    // If a higher priority task was woken, request a context switch
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/**************************************************************************/ 
/**
 * @fn			void usart_write_callback(struct usart_module *const usart_module)
 * @brief		Callback called when the system finishes sending all the bytes requested from a UART read job
 * @note
 *****************************************************************************/
void usart_write_callback(struct usart_module *const usart_module)
{
	if (circular_buf_get(cbufTx, (uint8_t *)&latestTx) != -1) // Only continue if there are more characters to send
	{
		usart_write_buffer_job(&usart_instance, (uint8_t *)&latestTx, 1);
	}
}

/**
 * @brief Prints a formatted log message with non-printable characters filtered
 * @param[in] logLevel Log level of the message
 * @param[in] format Format string
 * @param[in] ... Variable arguments for format string
 */
void LogMessageFormatted(enum eDebugLogLevels logLevel, const char *format, ...)
{
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    // Clean the buffer of non-printable characters
    for (int i = 0; buffer[i] != 0; i++) {
        if (buffer[i] < 32 || buffer[i] > 126) {
            buffer[i] = '.'; // Replace with a dot or space
        }
    }
    
    // Now use the cleaned buffer
    LogMessage(logLevel, "%s", buffer);
}

