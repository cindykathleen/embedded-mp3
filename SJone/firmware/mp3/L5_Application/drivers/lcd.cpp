#include "mp3_tasks.hpp"
// Standard libraries
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// Framework libraries
#include "i2c2.hpp"
#include "utilities.h"
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
static lcd_row_E  current_row      = LCD_ROW0;
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

void lcd_clear_row(lcd_row_E row) 
{
    // Move cursor to row to be cleared
    lcd_set_cursor(1, row);

    // To clear each character, a space needs to be written to it
    const char *empty_row = "                   ";
    lcd_send_string((uint8_t *)empty_row, strlen(empty_row));

    DELAY_MS(50);
}

inline void lcd_clear_all_rows(void) 
{
    for (int i=0; i<LCD_NUM_ROWS; i++)
    {
        lcd_clear_row((lcd_row_E)i);
    }
}

inline void lcd_set_cursor(uint8_t column, lcd_row_E row) 
{
    lcd_send_byte(LCD_SETDDRAMADDR | (column + row_offset[(uint8_t)row]), LCD_MODE_COMMAND);
}

inline void lcd_set_arrow_position(uint8_t pos) 
{
    lcd_send_byte(LCD_SETDDRAMADDR | row_offset[pos], LCD_MODE_COMMAND);
}

static inline void lcd_increment_row(void)
{
    switch (current_row)
    {
        case LCD_ROW0: current_row = LCD_ROW1; break;
        case LCD_ROW1: current_row = LCD_ROW2; break;
        case LCD_ROW2: current_row = LCD_ROW3; break;
        case LCD_ROW3: current_row = LCD_ROW0; break;
        default:                               break;
    }
}

static inline void lcd_decrement_row(void)
{
    switch (current_row)
    {
        case LCD_ROW0: current_row = LCD_ROW3; break;
        case LCD_ROW1: current_row = LCD_ROW0; break;
        case LCD_ROW2: current_row = LCD_ROW1; break;
        case LCD_ROW3: current_row = LCD_ROW2; break;
        default:                               break;
    }
}

void lcd_move_arrow(lcd_move_direction_E direction)
{
    switch (direction)
    {
        case LCD_UP:
            // Can only go up if not already at the top
            if (current_row != LCD_ROW3) 
            {
                lcd_clear_arrow(current_row);
                lcd_set_arrow_position((uint8_t)current_row + 1);
                lcd_send_byte(ARROW_CHAR, LCD_MODE_DATA);
                lcd_increment_row();
            }
            break;

        case LCD_DOWN:
            // Can only go down if not already at the bottom
            if (current_row != LCD_ROW0)
            {
                lcd_clear_arrow(current_row);
                lcd_set_arrow_position((uint8_t)current_row - 1);
                lcd_send_byte(ARROW_CHAR, LCD_MODE_DATA);
                lcd_decrement_row();
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
    // Assert size is less than the width of the screen (20 - 1 for arrow)
    size = MIN(19, size);

    for (uint8_t i=0; i<size; i++)
    {
        lcd_send_byte(str[i], LCD_MODE_DATA);
    }
}

void lcd_init(void) 
{
    // Random value to clear backback
    const uint8_t rd = 0x60;
    I2C.readRegisters(LCD_ADDRESS, (uint8_t*)&rd, 1); 
    delay_ms(100);

    const uint8_t bytes[2] = { 0x34, 0x30 };
    lcd_transfer_byte((uint8_t*)bytes, 2);
    delay_ms(5);

    lcd_transfer_byte((uint8_t*)bytes, 2);
    delay_ms(5);

    lcd_transfer_byte((uint8_t*)bytes, 2); 
    delay_ms(5);

    const uint8_t init[14] = { 0x24, 0x20,
                               0x24, 0x20,
                               0x84, 0x80,
                               0x04, 0x00,
                               0xC4, 0xC0,
                               0x04, 0x00,
                               0x14, 0x10 };
    lcd_transfer_byte((uint8_t*)init, 14); 
    delay_ms(2);

    // Turn on backlight (last 0x00)
    // TODO : double check this
    const uint8_t setup[6] = { 0x04, 0x00,
                               0x64, 0x60,
                               0x00, 0x08 };
    lcd_transfer_byte((uint8_t*)setup, 6); 
    delay_ms(5);

    lcd_clear_screen();

    delay_ms(100);
    // Reset cursor, and initialize arrow
    lcd_set_cursor(0, LCD_ROW0);
    lcd_send_byte(ARROW_CHAR, LCD_MODE_DATA);
}