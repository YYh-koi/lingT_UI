#include "at/btstack_at.h"
#include "btstack_at_internal.h"

_Static_assert(BTSTACK_AT_COMMAND_MAX == 512u, "command bound");
_Static_assert(BTSTACK_AT_OUTPUT_MAX == 600u, "output bound");
_Static_assert(BTSTACK_AT_INPUT_QUEUE_LEN == 8u, "input depth");
_Static_assert(BTSTACK_AT_EVENT_QUEUE_LEN == 16u, "event depth");

int main(void)
{
    btstack_at_command_t command;

    command.id = BTSTACK_AT_COMMAND_AT;
    return (command.id == BTSTACK_AT_COMMAND_AT) ? 0 : 1;
}
