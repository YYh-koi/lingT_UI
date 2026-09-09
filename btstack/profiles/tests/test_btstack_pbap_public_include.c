#include "profiles/btstack_pbap_client.h"

int main(void)
{
    btstack_pbap_client_event_t event;
    btstack_vcard_parser_t parser;
    return (sizeof(event) != 0u && sizeof(parser) != 0u) ? 0 : 1;
}
