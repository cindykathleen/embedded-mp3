
// @description     : Opens an MP3 file in the SD card file system
// @param file_name : Struct containing name of the MP3 file to open, no need for prefixing "1:"
// @returns         : True for successful, false for unsuccessful
bool mp3_open_file(file_name_S *file_name);

// @description : Closes the currently opened file if it is open
// @returns     : True for successful, false for unsuccessful, true if no file is currently opened
bool mp3_close_file(void);

// @description : Restarts the file from the beginning
// @returns     : True for successful, false for unsuccessful
bool mp3_restart_file(void);

// @description : Get the size of the currently opened file
// @returns     : The size of the file in bytes
uint32_t mp3_get_file_size(void);

// @description : Calculates how long the song is from the bit rate and the size of the file
// @returns     : The song length in seconds
uint32_t mp3_get_song_length_in_seconds(void);

// @description                : Reads a segment from the opened file
// @param buffer               : The buffer to read data into
// @param segment_size         : The size of the segment to read
// @param current_segment_size : The size of the segment actually read, may be less than segment_size
// @returns                    : True for successful, false for unsuccessful
bool mp3_read_segment(uint8_t *buffer, uint32_t segment_size, uint32_t *current_segment_size);

bool mp3_is_file_open(void);

file_name_S mp3_get_name(void);

void mp3_get_header_info(mp3_header_S *header, uint8_t *buffer);

const char* mp3_get_artist(void);

const char* mp3_get_title(void);

const char* mp3_get_genre(void);

bool mp3_rewind_segments(uint32_t segments);

void mp3_set_direction(seek_direction_E direction);

seek_direction_E mp3_get_direction(void);

float mp3_get_percentage(void);