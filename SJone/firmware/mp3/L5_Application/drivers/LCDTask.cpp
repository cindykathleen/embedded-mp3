#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "i2c2.hpp"
#include "mp3_tasks.hpp"
#include "buttons.hpp"
#include "common.hpp"

#define LCD_ADDRESS             0x4E
// LCD Commands
// ---------------------------------------------------------------------------
#define LCD_CLEARDISPLAY        0x01
#define LCD_RETURNHOME          0x02
#define LCD_ENTRYMODESET        0x04
#define LCD_DISPLAYCONTROL      0x08
#define LCD_CURSORSHIFT         0x10
#define LCD_FUNCTIONSET         0x20
#define LCD_SETCGRAMADDR        0x40
#define LCD_SETDDRAMADDR        0x80

// flags for display entry mode
// ---------------------------------------------------------------------------
#define LCD_ENTRYRIGHT          0x00
#define LCD_ENTRYLEFT           0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

// flags for display on/off and cursor control
// ---------------------------------------------------------------------------
#define LCD_DISPLAYON           0x04
#define LCD_DISPLAYOFF          0x00
#define LCD_CURSORON            0x02
#define LCD_CURSOROFF           0x00
#define LCD_BLINKON             0x01
#define LCD_BLINKOFF            0x00

// flags for display/cursor shift
// ---------------------------------------------------------------------------
#define LCD_DISPLAYMOVE         0x08
#define LCD_CURSORMOVE          0x00
#define LCD_MOVERIGHT           0x04
#define LCD_MOVELEFT            0x00

// flags for function set
// ---------------------------------------------------------------------------
#define LCD_8BITMODE            0x10
#define LCD_4BITMODE            0x00
#define LCD_2LINE               0x08
#define LCD_1LINE               0x00
#define LCD_5x10DOTS            0x04
#define LCD_5x8DOTS             0x00


// Define COMMAND and DATA LCD Rs (used by send method).
// ---------------------------------------------------------------------------
#define COMMAND                 0x00
#define DATA                    0x01
#define FOUR_BITS               0x02

#define MAX_SONG_LIST           10
#define MAX_COL_LENGTH          20

#define BLOCK_CHAR              0xFF
#define ARROW_CHAR              0x7E

const uint8_t row_offset[4] = {0x00, 0x40, 0x14, 0x54};
const char *songStartLine = "------------------|";


static file_name_S **track_list;
static mp3_header_S* headers;
static uint8_t track_list_size = 0;

static uint8_t currentArrowPos = 0;
static uint32_t currentSongOffset = 0;
static uint32_t currentSongIndex = 0;
static uint32_t currentScreenIndex = 0;
static uint32_t songTimeElapsed = 0;
static bool songInterrupt = false;

void updateSongTimer();
void selectRow(uint32_t rowSelected);
void moveLineUp();
void moveLineDown();
void printSongs(uint8_t startIndex);
void clearAllLines();
void clearLine(uint8_t row);
void setCursor(uint8_t column, uint8_t row);
void moveArrowDown();
void moveArrowUp();
void clearArrow(uint8_t pos);
void setArrowPosition(uint8_t pos);
void clearDisplay();
void setHome();
void sendData(char c, uint8_t mode = DATA);
void sendString(char *str, uint32_t size);
void write(uint8_t * wdata, uint32_t wlength);
void I2C_THIS_IS_TOTALLY_NOT_HARDCODED_MAGIC();
void playSongScreenSetup(uint8_t rowSelected);
void initSwitches();
void display_screen();


void selectRow(uint32_t rowSelected) 
{
    clearDisplay();
    setHome();

    if (currentScreenIndex == 0) {
        playSongScreenSetup(rowSelected);
        currentScreenIndex++;
    }
}

void moveLineUp() 
{
    if (currentArrowPos == 0) {
        if (currentSongOffset > 0) {
            currentSongOffset--;
            clearAllLines();
            printSongs(currentSongOffset);
        }
    } else {
        moveArrowUp();
    }

    if (currentSongIndex > 0) {
        currentSongIndex--;
    }
}

void moveLineDown() 
{
    if (currentArrowPos == 3) {
        if ((currentSongOffset + 1) < (track_list_size-3)) {
            currentSongOffset++;
            clearAllLines();
            printSongs(currentSongOffset);
        }
    } 
    else if ((currentSongOffset + 1) < (track_list_size-3)) {
        moveArrowDown();
    }

    if (currentSongIndex < track_list_size-1) {
        currentSongIndex++;
    }

    printf("%d %d %d\n", track_list_size, currentSongOffset, currentSongIndex);
}

void printSongs(uint8_t startIndex) 
{
    uint8_t line = 0;
    for (int i = startIndex; i < (startIndex+4); ++i) {
        setCursor(1, line);
        uint32_t len = MIN(strlen(track_list[i]->short_name), 20);

        char *short_name = track_list_get_short_name(i);
        if (short_name)
        {
            printf("Short: %s\n", short_name);
            sendString(short_name, len);
            line++;            
        }
    }
}

void clearAllLines() 
{
    for (uint8_t i = 0; i < 4; ++i) {
        clearLine(i);
    }
}

void clearLine(uint8_t row) 
{
    setCursor(1, row);
    char *filler = "                   ";
    uint32_t strLength = strlen(filler);
    sendString(filler, strLength);
    DELAY_MS(50);
}

void setCursor(uint8_t column, uint8_t row) 
{ // 1 column over from the arrow
    sendData(LCD_SETDDRAMADDR | (column + row_offset[row]), COMMAND);
}

void moveArrowDown() 
{
    if (currentArrowPos < 3) {
        clearArrow(currentArrowPos);
        setArrowPosition(currentArrowPos + 1);
        sendData(ARROW_CHAR);
        currentArrowPos++;
    }
}

void moveArrowUp() 
{
    if (currentArrowPos > 0) {
        clearArrow(currentArrowPos);
        setArrowPosition(currentArrowPos - 1);
        sendData(ARROW_CHAR);
        currentArrowPos--;
    }
}

void clearArrow(uint8_t pos) 
{
    setArrowPosition(pos);
    sendData(' ');
}

void setArrowPosition(uint8_t pos) 
{
    sendData(LCD_SETDDRAMADDR | row_offset[pos], COMMAND);
}

void clearDisplay() 
{
    sendData(LCD_CLEARDISPLAY, COMMAND); // 0x01 | mode2
}

void setHome() 
{
    sendData(LCD_RETURNHOME, COMMAND); // 0x02 | mode2
}

void sendData(char c, uint8_t mode) 
{
    uint8_t upper, lower, byte0, byte1, byte2, byte3;
    
    upper  = c & 0xF0;                    // get upper 4 bits of char
    upper |= LCD_DISPLAYCONTROL | mode;   // backlight 0x8 | mode 0x1
    byte0  = upper |  (LCD_ENTRYMODESET); // byte0 (enable 0x04)
    byte1  = upper & ~(LCD_ENTRYMODESET); // byte1 inverse

    lower  = (c << 4);                    // get lower 4 bits of char
    lower |= LCD_DISPLAYCONTROL | mode;   // backlight 0x8 | mode 0x1
    byte2  = lower |  (LCD_ENTRYMODESET); // byte2 (enable 0x04)
    byte3  = lower & ~(LCD_ENTRYMODESET); // byte3 inverse
    
    uint8_t send[4] = {byte0,byte1,byte2,byte3};
    write(send,4);
}

void sendString(char *str, uint32_t size) 
{
    for (uint32_t i = 0; i < size; i++) {
        sendData(str[i]);
    }
}

void write(uint8_t * wdata, uint32_t wlength)
{
    I2C2::getInstance().writeRegisters(LCD_ADDRESS, wdata, wlength); 
}

void I2C_THIS_IS_TOTALLY_NOT_HARDCODED_MAGIC() 
{
    uint8_t rd = 0x60; // random value - clear backback
    I2C2::getInstance().readRegisters(LCD_ADDRESS, &rd, 1); 
    DELAY_MS(100);
    // wait 100ms
    uint8_t send[2] = {0x34,0x30};
    write(send,2);
    DELAY_MS(5);
    // wait 4.5ms
    write(send,2);
    DELAY_MS(5);
    // wait 4.5ms
    write(send,2); 
    DELAY_MS(5);
    // wait 200ns
    uint8_t init[14] = {0x24,0x20,0x24,0x20,0x84,0x80,0x04,0x00,0xC4,0xC0,0x04,0x00,0x14,0x10};
    write(init,14); 
    DELAY_MS(2);
    // wait 2ms
    // turn on backlight (last 0x00)
    uint8_t setup[6] = {0x04,0x00,0x64,60,00,0x08};
    write(setup,6); 
    DELAY_MS(5);

    clearDisplay();
}

void playSongScreenSetup(uint8_t rowSelected) 
{
    char *name = headers[currentSongIndex].file_name.short_name;
    uint8_t length = strlen(name);
    sendString(name, length);

    // Artist Name
    setCursor(0,1);
    char *artist = headers[currentSongIndex].artist;
    length = strlen(artist);
    sendString(artist, length);

    // Genre
    setCursor(0,2);
    char *genre = headers[currentSongIndex].genre;
    length = strlen(genre);
    sendString(genre, length);

    // Set Song Timeline - bottom row
    setCursor(0,3);
    sendData(BLOCK_CHAR);
    uint32_t lineLen = strlen(songStartLine);
    sendString((char *)songStartLine,lineLen);
}

void initSwitches() 
{
    // // Initialize onboard Switch (SW0)
    // LPC_PINCON->PINSEL2 &= ~(0x3 << 9); 
    // LPC_GPIO1->FIODIR   &= ~(0x1 << 9);

    // // SW1
    // LPC_PINCON->PINSEL2 &= ~(0x3 << 10); 
    // LPC_GPIO1->FIODIR   &= ~(0x1 << 10);

    // // SW2 & 3
    // LPC_GPIO1->FIODIR   &= ~(0x1 << 14);
    // LPC_GPIO1->FIODIR   &= ~(0x1 << 15);

    // // External Switches P1.23, 1.28, 1.29, 1.30
    // LPC_PINCON->PINSEL3 &= ~(0x3 << 14); // P1.23
    // LPC_PINCON->PINSEL3 &= ~(0x3 << 24); // P1.28
    // LPC_PINCON->PINSEL3 &= ~(0x3 << 26); // P1.29
    // LPC_PINCON->PINSEL3 &= ~(0x3 << 28); // P1.30

    // LPC_GPIO1->FIODIR   &= ~(0x1 << 23);
    // LPC_GPIO1->FIODIR   &= ~(0x1 << 28);
    // LPC_GPIO1->FIODIR   &= ~(0x1 << 29);
    // LPC_GPIO1->FIODIR   &= ~(0x1 << 30);
}

file_name_S file_names[4] = { 0 };

void display_screen()
{
    track_list_get4(file_names);

    sendString(file_names[0].short_name, strlen(file_names[0].short_name));
    moveLineDown();
    printf("Current Index:     %lu\n"
           "Current Arrow:     %d\n"
           "CurrentSongOffset: %lu\n", currentSongIndex, currentArrowPos, currentSongOffset);

    sendString(file_names[1].short_name, strlen(file_names[1].short_name));
    moveLineDown();
    printf("Current Index:     %lu\n"
           "Current Arrow:     %d\n"
           "CurrentSongOffset: %lu\n", currentSongIndex, currentArrowPos, currentSongOffset);

    sendString(file_names[2].short_name, strlen(file_names[2].short_name));
    moveLineDown();
    printf("Current Index:     %lu\n"
           "Current Arrow:     %d\n"
           "CurrentSongOffset: %lu\n", currentSongIndex, currentArrowPos, currentSongOffset);

    sendString(file_names[3].short_name, strlen(file_names[3].short_name));
    moveLineDown();
    printf("Current Index:     %lu\n"
           "Current Arrow:     %d\n"
           "CurrentSongOffset: %lu\n", currentSongIndex, currentArrowPos, currentSongOffset);
}

void LCDTask(void *p)
{
    printf("LCD set up.\n");

    LPC_GPIO1->FIODIR   &= ~(0x1 << 29);
    LPC_GPIO1->FIODIR   &= ~(0x1 << 28);
    LPC_GPIO1->FIODIR   &= ~(0x1 << 23);
    LPC_GPIO1->FIODIR   &= ~(0x1 << 22);
    LPC_GPIO1->FIODIR   &= ~(0x1 << 20);
    LPC_GPIO1->FIODIR   &= ~(0x1 << 19);

    track_list_init();
    track_list = track_list_get_track_list();
    track_list_size = track_list_get_size();
    headers = track_list_get_headers();

    I2C_THIS_IS_TOTALLY_NOT_HARDCODED_MAGIC();
    DELAY_MS(100);
    setCursor(0, 0);
    sendData(ARROW_CHAR);

    printSongs(currentSongOffset);
    
    printf("LCD set up.\n");

    while (1)
    {
        if (LPC_GPIO1->FIOPIN & (1 << 22))
        {
            DELAY_MS(100);
            while (LPC_GPIO1->FIOPIN & (1 << 22));
            if (currentScreenIndex == 0) moveLineDown();
        }
        else if (LPC_GPIO1->FIOPIN & (1 << 23))
        {
            DELAY_MS(100);
            while (LPC_GPIO1->FIOPIN & (1 << 23));
            if (currentScreenIndex == 0) moveLineUp();
        }
        else if (LPC_GPIO1->FIOPIN & (1 << 28))
        {
            DELAY_MS(100);
            while (LPC_GPIO1->FIOPIN & (1 << 28));
            if (currentScreenIndex == 0) selectRow(currentSongIndex);
            track_list_set_current_track(currentSongIndex);
            // Unblock DecoderTask
            printf("Unblocking decodertask...\n");
            xSemaphoreGive(PlaySem);
        }

        // {
        //     MP3Player.IncrementVolume();
        // }
        // else if (sw4.IsHigh())
        // {
        //     MP3Player.DecrementVolume();
        // }
    }
}