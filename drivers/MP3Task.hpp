#include <scheduler_task>
#include "vs1053b.hpp"
#include "buttons.hpp"

#define MP3_SEGMENT_SIZE (1024)

typedef enum
{
    IDLE,
    PLAY,
    STOP,
    FORWARD,
    BACKWARD
} mp3_player_next_state;

class MP3Task : public scheduler_task
{
public:

    MP3Task(uint8_t priority) : scheduler_task(priority, "MP3Task", 8196), MP3Player(gpio_init)
    {
        // Initialize the driver with all the gpio pins
        const vs1053b_gpio_init_t gpio_init = {
            .port_reset = GPIO_PORT0,
            .port_dreq  = GPIO_PORT0,
            .port_xcs   = GPIO_PORT0,
            .port_xdcs  = GPIO_PORT0,
            .pin_reset  = 0,
            .pin_dreq   = 0,
            .pin_xcs    = 0,
            .pin_xdcs   = 0,
        };
    }

    bool run(void *p)
    {
        CheckButtons();
        HandleStateLogic();
        return true;
    }

    void HandleStateLogic()
    {
        static int byte = 0;
        static bool last_segment = false;
        static uint8_t current_segment_size = 0;

        switch (next_state)
        {
            case IDLE:
                break;
            case PLAY:
                // Not currently playing so open the file
                if (!MP3Player.IsPlaying())
                {
                    // Reset values to default
                    byte = 0;
                    last_segment = false;
                    current_segment_size = 0;
                    mp3_file = fopen("1:", "r");
                }

                // Read a segment at a time before sending to device
                for (int i=0; i<MP3_SEGMENT_SIZE; i++)
                {
                    byte = fgetc(mp3_file);
                    // Make sure to exit early if reached EOF
                    if (byte != EOF)
                    {
                        mp3[i] = (uint8_t)byte;
                        current_segment_size++;
                    }
                    else
                    {
                        last_segment = true;
                        break;
                    }
                }

                // Send segment to device
                MP3Player.PlaySegment(mp3, current_segment_size, last_segment);

                if (last_segment) 
                {
                    fclose(mp3_file);
                    next_state = IDLE;
                }
                break;
            case STOP:
                if (MP3Player.IsPlaying())
                {
                    // Close file if previously playing
                    if (mp3_file)
                    {
                        fclose(mp3_file);
                    }
                    // Stop playback
                    MP3Player.CancelDecoding();
                    // Go back to playing to finish the last segment(s)
                    next_state = PLAY;
                }
                // No need to cancel if not currently playing
                else
                {
                    next_state = IDLE;
                }
                break;
            case FORWARD:
                next_state = IDLE;
                break;
            case BACKWARD:
                next_state = IDLE;
                break;
        }
    }

    void CheckButtons()
    {
        // Only one button can be registered at a time
        if (Button0::getInstance().IsPressed())
        {
            next_state = PLAY;
        }
        else if (Button1::getInstance().IsPressed())
        {
            next_state = STOP;
        }
        else if (Button2::getInstance().IsPressed())
        {
            next_state = FORWARD;
        }
        else if (Button3::getInstance().IsPressed())
        {
            next_state = BACKWARD;
        }
    }

private:

    FILE *mp3_file;
    VS1053b MP3Player;
    uint8_t mp3[MP3_SEGMENT_SIZE];
    mp3_player_next_state next_state;
};