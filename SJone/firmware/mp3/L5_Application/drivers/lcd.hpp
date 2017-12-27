#pragma once

/**
 *  @module      : lcd
 *  @description : Driver for interfacing with a 20x4 SainSmart LCD screen
 */


// Enum for any function that either needs to move up or down
typedef enum 
{ 
    LCD_UP, 
    LCD_DOWN,
} lcd_move_direction_E;

// Enum for current row position
typedef enum
{
    LCD_ROW0     = 0,
    LCD_ROW1     = 1,
    LCD_ROW2     = 2,
    LCD_ROW3     = 3,
    LCD_NUM_ROWS = 4,
} lcd_row_E;

// @description     : Fills a single character on the LCD screen
// @param byte      : Byte to be sent
// @param mode      : There are two modes when sending bytes: COMMAND or DATA
void lcd_send_byte(uint8_t byte, uint8_t mode);

// @description     : Sends a string of characters to the screen
// @param str       : The string to be sent
// @param size      : The number of characters in the string
inline void lcd_send_string(uint8_t *str, uint8_t size);

// @description     : Clears all four rows of the screen
inline void lcd_clear_all_rows(void);

// @description     : Clears a single row
// @param row       : The row to be cleared (0-3)
void lcd_clear_row(lcd_row_E row);

// @description     : Moves the cursor to a (row, column) position
// @param column    : The column to move to
// @param row       : The row to move to
inline void lcd_set_cursor(uint8_t column, lcd_row_E row);

// @description     : Moves arrow symbol to a specific position
// @param pos       : The position to move to
inline void lcd_set_arrow_position(uint8_t pos);

// @description     : Moves arrow either up or down
// @param direction : The direction to move, either up or down
void lcd_move_arrow(lcd_move_direction_E direction);

// @description     : Clears the arrow at a specific position
// @param pos       : The position to move to
inline void lcd_clear_arrow(uint8_t pos);

// @description     : Clears the entire screen
inline void lcd_clear_screen(void);

// @description     : TODO : idk
inline void lcd_set_home(void);

// @description     : Initializes the screen by writing to setup registers, and clearing the screen
void lcd_init(void);