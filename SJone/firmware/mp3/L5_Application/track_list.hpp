
// Initializes the track list from all the files on the SD card and adds to a circular buffer
void track_list_init(void);

// Print the entire track list from the SD card
void track_list_print(void);

// Rotates the track list to switch to the next song
void track_list_next(void);

void track_list_prev(void);

uint16_t track_list_get_size(void);

void track_list_shuffle(void);

void track_list_get4(file_name_S file_names[4]);

file_name_S** track_list_get_track_list();

void track_list_convert_to_short_name(file_name_S *file_names);

char* track_list_get_short_name(uint8_t index);

mp3_header_S* track_list_get_headers();

void track_list_set_current_track(uint8_t index);

file_name_S* track_list_get_current_track();