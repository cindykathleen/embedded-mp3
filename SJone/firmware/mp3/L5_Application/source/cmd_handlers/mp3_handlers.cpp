// Framework libraries
#include "command_handler.hpp"
// Project libraries
#include "common.hpp"
#include "decoder.hpp"


CMD_HANDLER_FUNC(mp3_handler_set_volume)
{
    bool up   = cmdParams.beginsWithIgnoreCase("up");
    bool down = cmdParams.beginsWithIgnoreCase("down");

    float percentage = 0;
    cmdParams.scanf("%*s %f", &percentage);

    if (up)
    {
        decoder_incr_volume(percentage);
        printf("[HANDLER] Incrementing volume by %f\n", percentage);
    }
    else if (down)
    {
        decoder_decr_volume(percentage);
        printf("[HANDLER] Decrementing volume by %f\n", percentage);
    }

    return true;
}

CMD_HANDLER_FUNC(mp3_handler_print_debug)
{
    decoder_print_debug();
    return true;
}

CMD_HANDLER_FUNC(mp3_handler_set_base)
{
    uint8_t amplitude  = 0;
    uint8_t freq_limit = 0;
    cmdParams.scanf("%*s %u %u", &amplitude, &freq_limit);
    printf("[HANDLER] Setting base to %u %u\n", amplitude, freq_limit);

    decoder_set_base(amplitude, freq_limit);
    return true;
}

CMD_HANDLER_FUNC(mp3_handler_set_treble)
{
    uint8_t amplitude  = 0;
    uint8_t freq_limit = 0;
    cmdParams.scanf("%*s %u %u", &amplitude, &freq_limit);
    printf("[HANDLER] Setting treble to %u %u\n", amplitude, freq_limit);

    decoder_set_treble(amplitude, freq_limit);
    return true;
}

CMD_HANDLER_FUNC(mp3_handler_set_ff)
{
    bool on  = cmdParams.beginsWithIgnoreCase("on");
    bool off = cmdParams.beginsWithIgnoreCase("off");

    if (on)       decoder_set_ff_mode(true);
    else if (off) decoder_set_ff_mode(false);
    
    return true;
}