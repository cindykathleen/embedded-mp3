#include "mp3_tasks.hpp"
// Standard libraries
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// Framework libraries
#include "i2c2.hpp"
// Project libraries
#include "lcd.hpp"


// Macro for acquiring I2C singleton instance
#define I2C                (I2C2::getInstance())

// Static I2C address for LCD peripheral
#define LCD_ADDRESS        (0x4E)

// LCD Commands
#define LCD_CLEARDISPLAY   (0x01)
#define LCD_RETURNHOME     (0x02)
#define LCD_ENTRYMODESET   (0x04)
#define LCD_DISPLAYCONTROL (0x08)
#define LCD_SETDDRAMADDR   (0x80)

#define LCD_MODE_COMMAND   (0x00)
#define LCD_MODE_DATA      (0x01)

#define BLOCK_CHAR         (0xFF)
#define ARROW_CHAR         (0x7E)

// Static variables
static const uint8_t row_offset[4] = { 0x00, 0x40, 0x14, 0x54 };
// static const char *songStartLine   = "------------------|";
static uint8_t  currentArrowPos    = 0;
// static uint32_t currentSongOffset  = 0;
// static uint32_t currentSongIndex   = 0;
// static uint32_t currentScreenIndex = 0;
// static uint32_t songTimeElapsed    = 0;
// static bool     songInterrupt      = false;

static inline void lcd_transfer_byte(uint8_t *bytes, uint8_t size)
{
    I2C.writeRegisters(LCD_ADDRESS, bytes, size); 
}

void lcd_send_byte(uint8_t byte, uint8_t mode) 
{
    uint8_t upper = 0;
    uint8_t lower = 0;
    uint8_t bytes[4] = { 0 };
    
    upper     = byte & 0xF0;                 // get upper 4 bits of char
    upper    |= LCD_DISPLAYCONTROL | mode;   // backlight 0x8 | mode 0x1
    bytes[0]  = upper |  (LCD_ENTRYMODESET); // byte0 (enable 0x04)
    bytes[1]  = upper & ~(LCD_ENTRYMODESET); // byte1 inverse

    lower     = (byte << 4);                 // get lower 4 bits of char
    lower    |= LCD_DISPLAYCONTROL | mode;   // backlight 0x8 | mode 0x1
    bytes[2]  = lower |  (LCD_ENTRYMODESET); // byte2 (enable 0x04)
    bytes[3]  = lower & ~(LCD_ENTRYMODESET); // byte3 inverse
    
    lcd_transfer_byte(bytes, 4);
}

inline void lcd_clear_all_rows(void) 
{
    for (int i=0; i<4; i++)
    {
        lcd_clear_row(i);
    }
}

void lcd_clear_row(uint8_t row) 
{
    // Don't do anything if incorrect row
    // TODO : change to enum?
    if (row >= 3) return;

    // Move cursor to row to be cleared
    lcd_set_cursor(1, row);

    // To clear each character, a space needs to be written to it
    const char *empty_row = "                   ";
    lcd_send_string((uint8_t *)empty_row, strlen(empty_row));

    DELAY_MS(50);
}

inline void lcd_set_cursor(uint8_t column, uint8_t row) 
{
    lcd_send_byte(LCD_SETDDRAMADDR | (column + row_offset[row]), LCD_MODE_COMMAND);
}

inline void lcd_set_arrow_position(uint8_t pos) 
{
    lcd_send_byte(LCD_SETDDRAMADDR | row_offset[pos], LCD_MODE_COMMAND);
}

void lcd_move_arrow(lcd_move_direction_E direction)
{
    switch (direction)
    {
        case LCD_UP:
            if (currentArrowPos < 3) 
            {
                lcd_clear_arrow(currentArrowPos);
                lcd_set_arrow_position(currentArrowPos + 1);
                lcd_send_byte(ARROW_CHAR, LCD_MODE_DATA);
                currentArrowPos++;
            }
            break;

        case LCD_DOWN:
            if (currentArrowPos > 0)
            {
                lcd_clear_arrow(currentArrowPos);
                lcd_set_arrow_position(currentArrowPos - 1);
                lcd_send_byte(ARROW_CHAR, LCD_MODE_DATA);
                currentArrowPos--;
            }
            break;
    }
}

inline void lcd_clear_arrow(uint8_t pos) 
{
    lcd_set_arrow_position(pos);
    lcd_send_byte(' ', LCD_MODE_DATA);
}

inline void lcd_clear_screen(void) 
{
    lcd_send_byte(LCD_CLEARDISPLAY, LCD_MODE_COMMAND); // 0x01 | mode2
}

inline void lcd_set_home(void) 
{
    lcd_send_byte(LCD_RETURNHOME, LCD_MODE_COMMAND); // 0x02 | mode2
}

inline void lcd_send_string(uint8_t *str, uint8_t size) 
{
    for (uint8_t i=0; i<size; i++)
    {
        lcd_send_byte(str[i], LCD_MODE_DATA);
    }
}

void lcd_init(void) 
{
    // Random value to clear backback
    uint8_t rd = 0x60;
    I2C.readRegisters(LCD_ADDRESS, &rd, 1); 
    DELAY_MS(100);

    uint8_t bytes[2] = { 0x34, 0x30 };
    lcd_transfer_byte(bytes, 2);
    DELAY_MS(5);

    lcd_transfer_byte(bytes, 2);
    DELAY_MS(5);

    lcd_transfer_byte(bytes, 2); 
    DELAY_MS(5);

    uint8_t init[14] = { 0x24, 0x20,
                         0x24, 0x20,
                         0x84, 0x80,
                         0x04, 0x00,
                         0xC4, 0xC0,
                         0x04, 0x00,
                         0x14, 0x10, };
    lcd_transfer_byte(init, 14); 
    DELAY_MS(2);

    // Turn on backlight (last 0x00)
    // TODO : double check this
    uint8_t setup[6] = { 0x04, 0x00,
                         0x64, 60,
                         00,   0x08 };
    lcd_transfer_byte(setup, 6); 
    DELAY_MS(5);

    lcd_clear_screen();
}