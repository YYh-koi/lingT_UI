#include "bt_xstack_audio.h"

int main(void)
{
    uint8_t data = 0u;
    return (int)bt_xstack_call_audio_write(&data, 65536u);
}
