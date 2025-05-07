/**
 * @file    ST7735.c
 * @brief   Driver for ST7735-based LCD screen using SPI.
 *
 * Provides SPI initialization, LCD setup, pixel/region drawing,
 * text rendering, and screen control for a 128x160 RGB display.
 * Designed for use with FreeRTOS and Atmel SAMW25 platform.
 */

#include "ST7735.h"

struct spi_module spi_master_instance;  // SPI master instance
struct spi_slave_inst slave;           // SPI slave device instance

/**
 * @fn      void configure_port_pins_LCD(void)
 * @brief   Configures GPIO pins for LCD data and slave select.
 * @details Sets DAT_PIN and SLAVE_SELECT_PIN as outputs. Initializes SS pin high.
 */
void configure_port_pins_LCD(void)
{
    struct port_config config_port_pin;
    port_get_config_defaults(&config_port_pin);
    config_port_pin.direction = PORT_PIN_DIR_OUTPUT;

    // Configure LCD data pin as output
    port_pin_set_config(DAT_PIN, &config_port_pin);

    // Configure slave select (SS) pin as output
    struct port_config config_port_pins;
    port_get_config_defaults(&config_port_pins);
    config_port_pins.direction = PORT_PIN_DIR_OUTPUT;
    port_pin_set_config(SLAVE_SELECT_PIN, &config_port_pins);

    // Set slave select pin HIGH (inactive)
    port_pin_set_output_level(SLAVE_SELECT_PIN, 1);
}

/**
 * @fn      void configure_spi_master(void)
 * @brief   Initializes the SPI master interface for LCD communication.
 * @details Sets SPI pinmux, mode, baudrate, and attaches the slave device.
 */
void configure_spi_master(void)
{
    struct spi_config config_spi_master;
    struct spi_slave_inst_config slave_dev_config;

    // Default configuration for slave
    spi_slave_inst_get_config_defaults(&slave_dev_config);
    slave_dev_config.ss_pin = SLAVE_SELECT_PIN;
    spi_attach_slave(&slave, &slave_dev_config);

    // Load default SPI master config
    spi_get_config_defaults(&config_spi_master);
    config_spi_master.mux_setting = CONF_MASTER_MUX_SETTING;
    config_spi_master.pinmux_pad0 = CONF_MASTER_PINMUX_PAD0;
    config_spi_master.pinmux_pad1 = CONF_MASTER_PINMUX_PAD1;
    config_spi_master.pinmux_pad2 = CONF_MASTER_PINMUX_PAD2;
    config_spi_master.pinmux_pad3 = CONF_MASTER_PINMUX_PAD3;
    config_spi_master.mode_specific.master.baudrate = 12000000;  // 12 MHz

    // Initialize and enable SPI
    spi_init(&spi_master_instance, CONF_MASTER_SPI_MODULE, &config_spi_master);
    spi_enable(&spi_master_instance);
}

/**
 * @fn      void spi_io(unsigned char o)
 * @brief   Sends one byte over SPI to the LCD.
 * @param   o - Byte to transmit
 */
void spi_io(unsigned char o) {
    spi_write(&spi_master_instance, o);
}

/**
 * @fn      void drawString(short x, short y, char* str, short fg, short bg)
 * @brief   Draws a string on the LCD at a given position.
 * @details Characters are spaced horizontally. Uses drawChar() for rendering.
 * 
 * @param   x, y   - Starting coordinate
 * @param   str    - Null-terminated string
 * @param   fg, bg - Foreground and background colors
 */
void drawString(short x, short y, char* str, short fg, short bg){
    int i = 0;
    int spacing = 6;  // Horizontal pixel spacing between characters
    while (str[i]) {
        drawChar(x + spacing * i, y, str[i], fg, bg);
        i++;
    }
}

/**
 * @fn      void drawChar(short x, short y, unsigned char c, short fg, short bg)
 * @brief   Draws a single ASCII character on the LCD using pixel font.
 * @details Uses a 5x8 bitmap from ASCII[] and writes pixels via LCD_drawPixel.
 * 
 * @param   x, y   - Top-left position to draw character
 * @param   c      - Character to render (ASCII range)
 * @param   fg, bg - Foreground and background colors
 */
void drawChar(short x, short y, unsigned char c, short fg, short bg){
    char row = c - 0x20;
    int i;
    if ((MAX_X - x > 7) && (MAX_Y - y > 7)) {
        for (i = 0; i < 5; i++) {
            char pixels = ASCII[row][i];  // Get bitmap for character
            for (int j = 0; j < 8; j++) {
                if (((pixels >> j) & 1) == 1) {
                    LCD_drawPixel(x + i, y + j, fg);  // Foreground pixel
                } else {
                    LCD_drawPixel(x + i, y + j, bg);  // Background pixel
                }
            }
        }
    }
}

/**
 * @fn      void drawRectangle(short x1, short y1, short x2, short y2, short c)
 * @brief   Fills a rectangular area on the LCD with a specific color.
 * @param   x1, y1 - Top-left corner
 * @param   x2, y2 - Bottom-right corner
 * @param   c      - Fill color
 */
void drawRectangle(short x1, short y1, short x2, short y2, short c){
    for (int i = x1; i <= x2; i++) {
        for (int j = y1; j <= y2; j++) {
            LCD_drawPixel(i, j, c);
        }
    }
}

/**
 * @fn      void LCD_command(unsigned char com)
 * @brief   Sends a command byte to the LCD over SPI.
 * @details Sets DAT_PIN low (command mode), selects the SPI slave, 
 *          sends the command byte, and deselects the slave.
 * 
 * @param   com - Command byte to send
 */
void LCD_command(unsigned char com) {
    port_pin_set_output_level(DAT_PIN, false);  // Command mode (D/C = 0)
    spi_select_slave(&spi_master_instance, &slave, true);   // Enable SPI slave
    spi_io(com);                                            // Send command byte
    spi_select_slave(&spi_master_instance, &slave, false);  // Disable SPI slave
}

/**
 * @fn      void LCD_data(unsigned char dat)
 * @brief   Sends a single data byte to the LCD over SPI.
 * @details Sets DAT_PIN high (data mode), selects the SPI slave,
 *          sends the data byte, and deselects the slave.
 * 
 * @param   dat - Data byte to send
 */
void LCD_data(unsigned char dat) {
    port_pin_set_output_level(DAT_PIN, true);  // Data mode (D/C = 1)
    spi_select_slave(&spi_master_instance, &slave, true);   // Enable SPI slave
    spi_io(dat);                                             // Send data byte
    spi_select_slave(&spi_master_instance, &slave, false);   // Disable SPI slave
}

/**
 * @fn      void LCD_data16(unsigned short dat)
 * @brief   Sends a 16-bit data word to the LCD over SPI.
 * @details Sends high byte first (big-endian). Used for color or position data.
 * 
 * @param   dat - 16-bit data to send
 */
void LCD_data16(unsigned short dat) {
    port_pin_set_output_level(DAT_PIN, true);  // Data mode (D/C = 1)
    spi_select_slave(&spi_master_instance, &slave, true);   // Enable SPI slave
    spi_io(dat >> 8);    // Send high byte first
    spi_io(dat);         // Send low byte
    spi_select_slave(&spi_master_instance, &slave, false);  // Disable SPI slave
}

/**
 * @fn      void LCD_command(unsigned char com)
 * @brief   Sends a command byte to the LCD controller.
 * @details Sets the D/C pin (DAT_PIN) low to indicate a command,
 *          selects the SPI slave, sends the command byte, and then deselects the slave.
 *
 * @param   com - The command byte to send to the LCD
 */
void LCD_command(unsigned char com) {
    port_pin_set_output_level(DAT_PIN, false);  // Set D/C = 0 for command
    spi_select_slave(&spi_master_instance, &slave, true);   // Activate slave (CS low)
    spi_io(com);                                            // Send command byte
    spi_select_slave(&spi_master_instance, &slave, false);  // Deactivate slave (CS high)
}

/**
 * @fn      void LCD_data(unsigned char dat)
 * @brief   Sends a single 8-bit data byte to the LCD controller.
 * @details Sets the D/C pin (DAT_PIN) high to indicate data,
 *          selects the SPI slave, sends the byte, and then deselects the slave.
 *
 * @param   dat - The data byte to send
 */
void LCD_data(unsigned char dat) {
    port_pin_set_output_level(DAT_PIN, true);  // Set D/C = 1 for data
    spi_select_slave(&spi_master_instance, &slave, true);   // Activate slave
    spi_io(dat);                                             // Send data byte
    spi_select_slave(&spi_master_instance, &slave, false);  // Deactivate slave
}

/**
 * @fn      void LCD_data16(unsigned short dat)
 * @brief   Sends a 16-bit data word (e.g. color in RGB565) to the LCD controller.
 * @details Sends the high byte first, followed by the low byte.
 *
 * @param   dat - The 16-bit data to send
 */
void LCD_data16(unsigned short dat) {
    port_pin_set_output_level(DAT_PIN, true);  // Set D/C = 1 for data
    spi_select_slave(&spi_master_instance, &slave, true);   // Activate slave
    spi_io(dat >> 8);  // Send high byte (MSB first)
    spi_io(dat);       // Send low byte (LSB)
    spi_select_slave(&spi_master_instance, &slave, false);  // Deactivate slave
}

/**
 * @fn      void LCD_init(void)
 * @brief   Initializes the LCD display and sets up SPI communication.
 * @details Configures port pins and SPI interface, then sends the standard
 *          ST7735 initialization commands (software reset and sleep out).
 * 
 * @note    This function assumes ST7735-compatible command set.
 */
void LCD_init() {
	configure_port_pins_LCD();
	configure_spi_master();
	spi_select_slave(&spi_master_instance, &slave, false);
	vTaskDelay(1000);
	LCD_command(ST7735_SWRESET);
	vTaskDelay(50);
	LCD_command(ST7735_SLPOUT);
	vTaskDelay(5);
	LCD_command(ST7735_FRMCTR1);
	LCD_data(0x01);
	LCD_data(0x2C);
	LCD_data(0x2D);
	vTaskDelay(1);
	LCD_command(ST7735_FRMCTR2);
	LCD_data(0x01);
	LCD_data(0x2C);
	LCD_data(0x2D);
	vTaskDelay(1);
	LCD_command(ST7735_FRMCTR3);
	LCD_data(0x01);
	LCD_data(0x2C);
	LCD_data(0x2D);
	LCD_data(0x01);
	LCD_data(0x2C);
	LCD_data(0x2D);
	vTaskDelay(1);
	LCD_command(ST7735_INVCTR);
	LCD_data(0x07);
	vTaskDelay(1);
	LCD_command(ST7735_PWCTR1);
	LCD_data(0x0A);
	LCD_data(0x02);
	LCD_data(0x84);
	vTaskDelay(1);
	LCD_command(ST7735_PWCTR2);
	LCD_data(0xC5);
	vTaskDelay(1);
	LCD_command( ST7735_PWCTR3);
	LCD_data(0x0A);
	LCD_data(0x00);
	vTaskDelay(1);
	LCD_command( ST7735_PWCTR4);
	LCD_data(0x8A);
	LCD_data(0x2A);
	vTaskDelay(1);
	LCD_command( ST7735_PWCTR5);
	LCD_data(0x8A);
	LCD_data(0xEE);
	vTaskDelay(1);
	LCD_command(ST7735_VMCTR1);
	LCD_data(0x0E);
	vTaskDelay(1);
	LCD_command(ST7735_INVOFF);
	LCD_command(ST7735_MADCTL);
	LCD_data(0xC8);
	vTaskDelay(1);
	LCD_command(ST7735_COLMOD);
	LCD_data(0x05);
	//delay_ms(1);
	vTaskDelay(1);
	LCD_command(ST7735_CASET);
	LCD_data(0x00);
	LCD_data(0x00);
	LCD_data(0x00);
	LCD_data(0x7F);
	vTaskDelay(1);
	LCD_command(ST7735_RASET);
	LCD_data(0x00);
	LCD_data(0x00);
	LCD_data(0x00);
	LCD_data(0x9F);
	vTaskDelay(1);
	LCD_command(ST7735_GMCTRP1);
	LCD_data(0x02);
	LCD_data(0x1C);
	LCD_data(0x07);
	LCD_data(0x12);
	LCD_data(0x37);
	LCD_data(0x32);
	LCD_data(0x29);
	LCD_data(0x2D);
	LCD_data(0x29);
	LCD_data(0x25);
	LCD_data(0x2B);
	LCD_data(0x39);
	LCD_data(0x00);
	LCD_data(0x01);
	LCD_data(0x03);
	LCD_data(0x10);
	vTaskDelay(1);
	LCD_command(ST7735_GMCTRN1);
	LCD_data(0x03);
	LCD_data(0x1D);
	LCD_data(0x07);
	LCD_data(0x06);
	LCD_data(0x2E);
	LCD_data(0x2C);
	LCD_data(0x29);
	LCD_data(0x2D);
	LCD_data(0x2E);
	LCD_data(0x2E);
	LCD_data(0x37);
	LCD_data(0x3F);
	LCD_data(0x00);
	LCD_data(0x00);
	LCD_data(0x02);
	LCD_data(0x10);
	vTaskDelay(1);
	LCD_command(ST7735_NORON);
	vTaskDelay(10);
	LCD_command(ST7735_DISPON);
	vTaskDelay(100);
}

/**
 * @fn      void LCD_drawPixel(unsigned short x, unsigned short y, unsigned short color)
 * @brief   Draws a single pixel on the LCD at (x, y) with the specified color.
 * @details Sets the address window to (x, y) and writes one 16-bit color value.
 *
 * @param   x     - X coordinate (column)
 * @param   y     - Y coordinate (row)
 * @param   color - 16-bit RGB565 color
 */
void LCD_drawPixel(unsigned short x, unsigned short y, unsigned short color) {
    LCD_setAddr(x, y, x + 1, y + 1);  // Set address window to single pixel
    LCD_data16(color);               // Write pixel color
}

/**
 * @fn      void LCD_setAddr(unsigned short x0, unsigned short y0, unsigned short x1, unsigned short y1)
 * @brief   Sets the active drawing window (region) on the LCD.
 * @details This defines the rectangular area where subsequent pixel data will be written.
 *
 * @param   x0, y0 - Top-left corner of the region
 * @param   x1, y1 - Bottom-right corner of the region
 */
void LCD_setAddr(unsigned short x0, unsigned short y0, unsigned short x1, unsigned short y1) {
    LCD_command(ST7735_CASET);   // Column address set
    LCD_data16(x0);
    LCD_data16(x1);

    LCD_command(ST7735_RASET);   // Row address set
    LCD_data16(y0);
    LCD_data16(y1);

    LCD_command(ST7735_RAMWR);   // Write to RAM (ready for pixel data)
}

/**
 * @fn      void LCD_clearScreen(unsigned short color)
 * @brief   Fills the entire screen with a single color.
 * @details Sets address region to full screen and writes color to every pixel.
 *
 * @param   color - 16-bit RGB565 color to fill the screen with
 */
void LCD_clearScreen(unsigned short color) {
    int i;

    // Set address to full screen area
    LCD_setAddr(0, 0, _GRAMWIDTH, _GRAMHEIGH);

    // Write color to all pixels
    for (i = 0; i < _GRAMSIZE; i++) {
        LCD_data16(color);
    }
}

