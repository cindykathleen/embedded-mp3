#include "mp3_tasks.hpp"
// Standard libraries
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// Framework libraries
#include "i2c2.hpp"
// Project libraries
#include "lcd.hpp"

// #define I2C                     (I2C2::getInstance())

// #define LCD_ADDRESS             (0x4E)

// // LCD Commands
// #define LCD_CLEARDISPLAY        (0x01)
// #define LCD_RETURNHOME          (0x02)
// #define LCD_ENTRYMODESET        (0x04)
// #define LCD_DISPLAYCONTROL      (0x08)
// #define LCD_SETDDRAMADDR        (0x80)

// #define COMMAND                 (0x00)
// #define DATA                    (0x01)

// #define BLOCK_CHAR              (0xFF)
// #define ARROW_CHAR              (0x7E)

// // #define LCD_CURSORSHIFT         0x10
// // #define LCD_FUNCTIONSET         0x20
// // #define LCD_SETCGRAMADDR        0x40

// // Flags for display entry mode
// // #define LCD_ENTRYRIGHT          0x00
// // #define LCD_ENTRYLEFT           0x02
// // #define LCD_ENTRYSHIFTINCREMENT 0x01
// // #define LCD_ENTRYSHIFTDECREMENT 0x00

// // Flags for display on/off and cursor control
// // #define LCD_DISPLAYON           0x04
// // #define LCD_DISPLAYOFF          0x00
// // #define LCD_CURSORON            0x02
// // #define LCD_CURSOROFF           0x00
// // #define LCD_BLINKON             0x01
// // #define LCD_BLINKOFF            0x00

// // Flags for display/cursor shift
// // #define LCD_DISPLAYMOVE         0x08
// // #define LCD_CURSORMOVE          0x00
// // #define LCD_MOVERIGHT           0x04
// // #define LCD_MOVELEFT            0x00

// // Flags for function set
// // #define LCD_8BITMODE            0x10
// // #define LCD_4BITMODE            0x00
// // #define LCD_2LINE               0x08
// // #define LCD_1LINE               0x00
// // #define LCD_5x10DOTS            0x04
// // #define LCD_5x8DOTS             0x00

// // #define FOUR_BITS               0x02

// // #define MAX_SONG_LIST           10
// // #define MAX_COL_LENGTH          20

// typedef enum { LCD_UP, LCD_DOWN } lcd_move_direction_E;

// static const uint8_t row_offset[4] = { 0x00, 0x40, 0x14, 0x54 };
// static const char *songStartLine = "------------------|";


// // static file_name_S **track_list;
// // static mp3_header_S* headers;
// // static uint8_t track_list_size = 0;

// // static uint8_t currentArrowPos = 0;
// // static uint32_t currentSongOffset = 0;
// // static uint32_t currentSongIndex = 0;
// // static uint32_t currentScreenIndex = 0;
// // static uint32_t songTimeElapsed = 0;
// // static bool songInterrupt = false;

// static void lcd_select_row(uint32_t row);
// static void lcd_move_line_up(void);
// static void lcd_move_line_down(void);
// static void printSongs(uint8_t startIndex);
// static void lcd_clear_all_rows();
// static void lcd_clear_row(uint8_t row);
// static void lcd_set_cursor(uint8_t column, uint8_t row);
// static void moveArrowDown();
// static void moveArrowUp();
// static void lcd_clear_arrow(uint8_t pos);
// static void lcd_set_arrow_position(uint8_t pos);
// static void lcd_clear_display();
// static void setHome();
// static void lcd_send_byte(char c, uint8_t mode = DATA);
// static void sendString(char *str, uint32_t size);
// static void lcd_transfer_byte(uint8_t * wdata, uint32_t wlength);
// static void I2C_THIS_IS_TOTALLY_NOT_HARDCODED_MAGIC();
// static void playSongScreenSetup(uint8_t rowSelected);
// static void initSwitches();
// static void display_screen();

// #if 0
// void lcd_select_row(uint32_t row) 
// {
//     lcd_clear_display();
//     setHome();

//     if (currentScreenIndex == 0) 
//     {
//         playSongScreenSetup(row);
//         currentScreenIndex++;
//     }
// }

// void lcd_move_line_up(void) 
// {
//     if (currentArrowPos == 0)
//     {
//         if (currentSongOffset > 0)
//         {
//             currentSongOffset--;
//             lcd_clear_all_rows();
//             printSongs(currentSongOffset);
//         }
//     } 
//     else 
//     {
//         moveArrowUp();
//     }

//     if (currentSongIndex > 0)
//     {
//         currentSongIndex--;
//     }
// }

// void lcd_move_line_down(void) 
// {
//     if (currentArrowPos == 3) {
//         if ((currentSongOffset + 1) < (track_list_size-3)) {
//             currentSongOffset++;
//             lcd_clear_all_rows();
//             printSongs(currentSongOffset);
//         }
//     } 
//     else if ((currentSongOffset + 1) < (track_list_size-3)) {
//         moveArrowDown();
//     }

//     if (currentSongIndex < track_list_size-1) {
//         currentSongIndex++;
//     }

//     printf("%d %d %d\n", track_list_size, currentSongOffset, currentSongIndex);
// }

// void printSongs(uint8_t startIndex) 
// {
//     uint8_t line = 0;
//     for (int i = startIndex; i < (startIndex+4); ++i) {
//         lcd_set_cursor(1, line);
//         uint32_t len = MIN(strlen(track_list[i]->short_name), 20);

//         char *short_name = track_list_get_short_name(i);
//         if (short_name)
//         {
//             printf("Short: %s\n", short_name);
//             sendString(short_name, len);
//             line++;            
//         }
//     }
// }
// #endif

// static void lcd_transfer_byte(uint8_t *bytes, uint32_t size)
// {
//     I2C.writeRegisters(LCD_ADDRESS, bytes, size); 
// }

// static void lcd_send_byte(char c, uint8_t mode) 
// {
//     uint8_t upper = 0;
//     uint8_t lower = 0;
//     uint8_t bytes[4] = { 0 };
    
//     upper     = c & 0xF0;                    // get upper 4 bits of char
//     upper    |= LCD_DISPLAYCONTROL | mode;   // backlight 0x8 | mode 0x1
//     bytes[0]  = upper |  (LCD_ENTRYMODESET); // byte0 (enable 0x04)
//     bytes[1]  = upper & ~(LCD_ENTRYMODESET); // byte1 inverse

//     lower     = (c << 4);                    // get lower 4 bits of char
//     lower    |= LCD_DISPLAYCONTROL | mode;   // backlight 0x8 | mode 0x1
//     bytes[2]  = lower |  (LCD_ENTRYMODESET); // byte2 (enable 0x04)
//     bytes[3]  = lower & ~(LCD_ENTRYMODESET); // byte3 inverse
    
//     lcd_transfer_byte(bytes, 4);
// }

// static void lcd_clear_all_rows() 
// {
//     for (int i=0; i<4; i++)
//     {
//         lcd_clear_row(i);
//     }
// }

// static void lcd_clear_row(uint8_t row) 
// {
//     // Move cursor to row to be cleared
//     lcd_set_cursor(1, row);

//     // To clear each character, a space needs to be written to it
//     const char *empty_row = "                   ";
//     sendString(empty_row, strlen(empty_row));

//     DELAY_MS(50);
// }

// static void lcd_set_cursor(uint8_t column, uint8_t row) 
// {
//     lcd_send_byte(LCD_SETDDRAMADDR | (column + row_offset[row]), COMMAND);
// }

// static void lcd_set_arrow_position(uint8_t pos) 
// {
//     lcd_send_byte(LCD_SETDDRAMADDR | row_offset[pos], COMMAND);
// }

// static void lcd_move_arrow(lcd_move_direction_E direction)
// {
//     switch (direction)
//     {
//         case LCD_UP:
//             if (currentArrowPos < 3) 
//             {
//                 lcd_clear_arrow(currentArrowPos);
//                 lcd_set_arrow_position(currentArrowPos + 1);
//                 lcd_send_byte(ARROW_CHAR);
//                 currentArrowPos++;
//             }
//             break;

//         case LCD_DOWN:
//             if (currentArrowPos > 0)
//             {
//                 lcd_clear_arrow(currentArrowPos);
//                 lcd_set_arrow_position(currentArrowPos - 1);
//                 lcd_send_byte(ARROW_CHAR);
//                 currentArrowPos--;
//             }
//             break;
//     }
// }

// static inline void lcd_clear_arrow(uint8_t pos) 
// {
//     lcd_set_arrow_position(pos);
//     lcd_send_byte(' ');
// }

// static inline void lcd_clear_display() 
// {
//     lcd_send_byte(LCD_CLEARDISPLAY, COMMAND); // 0x01 | mode2
// }

// // void setHome() 
// // {
// //     lcd_send_byte(LCD_RETURNHOME, COMMAND); // 0x02 | mode2
// // }

// // void sendString(char *str, uint32_t size) 
// // {
// //     for (uint32_t i = 0; i < size; i++) {
// //         lcd_send_byte(str[i]);
// //     }
// // }



// // void playSongScreenSetup(uint8_t rowSelected) 
// // {
// //     char *name = headers[currentSongIndex].file_name.short_name;
// //     uint8_t length = strlen(name);
// //     sendString(name, length);

// //     // Artist Name
// //     lcd_set_cursor(0,1);
// //     char *artist = headers[currentSongIndex].artist;
// //     length = strlen(artist);
// //     sendString(artist, length);

// //     // Genre
// //     lcd_set_cursor(0,2);
// //     char *genre = headers[currentSongIndex].genre;
// //     length = strlen(genre);
// //     sendString(genre, length);

// //     // Set Song Timeline - bottom row
// //     lcd_set_cursor(0,3);
// //     lcd_send_byte(BLOCK_CHAR);
// //     uint32_t lineLen = strlen(songStartLine);
// //     sendString((char *)songStartLine,lineLen);
// // }

void Init_LCDTask(void) 
{
    lcd_init();
}

// // void LCDTask(void *p)
// // {
// //     printf("LCD set up.\n");

// //     LPC_GPIO1->FIODIR   &= ~(0x1 << 29);
// //     LPC_GPIO1->FIODIR   &= ~(0x1 << 28);
// //     LPC_GPIO1->FIODIR   &= ~(0x1 << 23);
// //     LPC_GPIO1->FIODIR   &= ~(0x1 << 22);
// //     LPC_GPIO1->FIODIR   &= ~(0x1 << 20);
// //     LPC_GPIO1->FIODIR   &= ~(0x1 << 19);

// //     track_list_init();
// //     track_list = track_list_get_track_list();
// //     track_list_size = track_list_get_size();
// //     headers = track_list_get_headers();

// //     DELAY_MS(100);
// //     lcd_set_cursor(0, 0);
// //     lcd_send_byte(ARROW_CHAR);

// //     printSongs(currentSongOffset);
    
// //     printf("LCD set up.\n");

// //     while (1)
// //     {
// //         if (LPC_GPIO1->FIOPIN & (1 << 22))
// //         {
// //             DELAY_MS(100);
// //             while (LPC_GPIO1->FIOPIN & (1 << 22));
// //             if (currentScreenIndex == 0) lcd_move_line_down();
// //         }
// //         else if (LPC_GPIO1->FIOPIN & (1 << 23))
// //         {
// //             DELAY_MS(100);
// //             while (LPC_GPIO1->FIOPIN & (1 << 23));
// //             if (currentScreenIndex == 0) lcd_move_line_up();
// //         }
// //         else if (LPC_GPIO1->FIOPIN & (1 << 28))
// //         {
// //             DELAY_MS(100);
// //             while (LPC_GPIO1->FIOPIN & (1 << 28));
// //             if (currentScreenIndex == 0) lcd_select_row(currentSongIndex);
// //             track_list_set_current_track(currentSongIndex);
// //             // Unblock DecoderTask
// //             printf("Unblocking decodertask...\n");
// //             xSemaphoreGive(PlaySem);
// //         }
// //     }
// // }