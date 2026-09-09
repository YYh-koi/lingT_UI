#include <stdint.h>

#include "btstack_at_internal.h"

static uint16_t text_length(const char *text)
{
    uint16_t length = 0u;
    while (text[length] != '\0') {
        length++;
    }
    return length;
}

static int bytes_equal(
    const char *actual,
    uint16_t actual_length,
    const char *expected,
    uint16_t expected_length)
{
    uint16_t index;
    if (actual_length != expected_length) {
        return 0;
    }
    for (index = 0u; index < actual_length; index++) {
        if ((uint8_t)actual[index] != (uint8_t)expected[index]) {
            return 0;
        }
    }
    return 1;
}

int btstack_at_protocol_tests(void)
{
    static const uint8_t address[6] = {
        0x1c, 0x5c, 0xf2, 0x26, 0xd7, 0x73
    };
    static const char plist_expected[] = {
        '+','P','L','I','S','T','=','1',0x0c,
        '4','2','4',0x0c,
        '1','C','5','C','F','2','2','6','D','7','7','3',0x0c,
        'P','h','o','n','e','\r','\n'
    };
    static const char track_expected[] = {
        '+','T','R','A','C','K','I','N','F','O','=',
        'T','i','t','l','e',0x0c,
        'A','r','t','i','s','t',0x0c,
        'A','l','b','u','m','\r','\n'
    };
    char output[BTSTACK_AT_OUTPUT_MAX];
    uint16_t output_length = 0u;
    int failures = 0;
    btstack_at_event_t event;

    failures += (sizeof(btstack_at_event_t) <= 268u) ? 0 : 1;
    event.data.numeric.values[0] = 0u;

    failures += btstack_at_format_simple(
        1,
        output,
        sizeof(output),
        &output_length);
    failures += bytes_equal(
        output,
        output_length,
        "OK\r\n",
        text_length("OK\r\n")) ? 0 : 1;

    failures += btstack_at_format_simple(
        0,
        output,
        sizeof(output),
        &output_length);
    failures += bytes_equal(
        output,
        output_length,
        "ERROR\r\n",
        text_length("ERROR\r\n")) ? 0 : 1;

    failures += btstack_at_format_address_response(
        "ADDR",
        address,
        output,
        sizeof(output),
        &output_length);
    failures += bytes_equal(
        output,
        output_length,
        "+ADDR=1C5CF226D773\r\nOK\r\n",
        text_length("+ADDR=1C5CF226D773\r\nOK\r\n")) ? 0 : 1;

    failures += btstack_at_format_plist_record(
        1u,
        424u,
        address,
        "Phone",
        output,
        sizeof(output),
        &output_length);
    failures += bytes_equal(
        output,
        output_length,
        plist_expected,
        sizeof(plist_expected)) ? 0 : 1;

    failures += btstack_at_format_plist_end(
        output,
        sizeof(output),
        &output_length);
    failures += bytes_equal(
        output,
        output_length,
        "+PLIST=E\r\n",
        text_length("+PLIST=E\r\n")) ? 0 : 1;

    failures += btstack_at_format_uint_line(
        "HFPVR",
        1u,
        0,
        output,
        sizeof(output),
        &output_length);
    failures += bytes_equal(
        output,
        output_length,
        "+HFPVR=1\r\n",
        text_length("+HFPVR=1\r\n")) ? 0 : 1;

    failures += btstack_at_format_text_response(
        "NAME",
        "bluetooth_auto",
        output,
        sizeof(output),
        &output_length);
    failures += bytes_equal(
        output,
        output_length,
        "+NAME=bluetooth_auto\r\nOK\r\n",
        text_length("+NAME=bluetooth_auto\r\nOK\r\n")) ? 0 : 1;

    failures += btstack_at_format_hex_response(
        "COD",
        0x240404u,
        6u,
        output,
        sizeof(output),
        &output_length);
    failures += bytes_equal(
        output,
        output_length,
        "+COD=240404\r\nOK\r\n",
        text_length("+COD=240404\r\nOK\r\n")) ? 0 : 1;

    failures += btstack_at_format_track_info(
        "Title",
        "Artist",
        "Album",
        output,
        sizeof(output),
        &output_length);
    failures += bytes_equal(
        output,
        output_length,
        track_expected,
        sizeof(track_expected)) ? 0 : 1;

    {
        uint16_t index;
        uint8_t *bytes = (uint8_t *)&event;
        for (index = 0u; index < sizeof(event); index++) {
            bytes[index] = 0u;
        }
    }
    event.id = BTSTACK_AT_EVENT_HFPSTAT;
    event.data.peer.values[0] = 5u;
    event.data.peer.value_count = 1u;
    event.data.peer.text[0] = '1';
    event.data.peer.text[1] = '0';
    event.data.peer.text[2] = '0';
    event.data.peer.text[3] = '8';
    failures += btstack_at_format_event(
        &event,
        output,
        sizeof(output),
        &output_length);
    failures += bytes_equal(
        output,
        output_length,
        "+HFPSTAT=5,1008\r\n",
        text_length("+HFPSTAT=5,1008\r\n")) ? 0 : 1;

    event.id = BTSTACK_AT_EVENT_HFPDEV;
    event.data.peer.text[0] = '\0';
    {
        uint8_t index;
        for (index = 0u; index < 6u; index++) {
            event.data.peer.address[index] = address[index];
        }
    }
    failures += btstack_at_format_event(
        &event,
        output,
        sizeof(output),
        &output_length);
    failures += bytes_equal(
        output,
        output_length,
        "+HFPDEV=1C5CF226D773\r\n",
        text_length("+HFPDEV=1C5CF226D773\r\n")) ? 0 : 1;

    event.id = BTSTACK_AT_EVENT_A2DPPCMCONFIG;
    event.data.numeric.values[0] = 44100u;
    event.data.numeric.values[1] = 2u;
    event.data.numeric.values[2] = 16u;
    event.data.numeric.value_count = 3u;
    failures += btstack_at_format_event(
        &event,
        output,
        sizeof(output),
        &output_length);
    failures += bytes_equal(
        output,
        output_length,
        "+A2DPPCMCONFIG=44100,2,16\r\n",
        text_length("+A2DPPCMCONFIG=44100,2,16\r\n")) ? 0 : 1;

    event.data.numeric.value_count = 1u;
    event.data.numeric.values[0] = 1u;
    event.id = BTSTACK_AT_EVENT_PWRSTAT;
    failures += btstack_at_format_event(
        &event, output, sizeof(output), &output_length);
    failures += bytes_equal(
        output, output_length, "+PWRSTAT=1\r\n",
        text_length("+PWRSTAT=1\r\n")) ? 0 : 1;

    event.id = BTSTACK_AT_EVENT_HFPAUDIO;
    failures += btstack_at_format_event(
        &event, output, sizeof(output), &output_length);
    failures += bytes_equal(
        output, output_length, "+HFPAUDIO=1\r\n",
        text_length("+HFPAUDIO=1\r\n")) ? 0 : 1;

    event.id = BTSTACK_AT_EVENT_A2DPSTAT;
    failures += btstack_at_format_event(
        &event, output, sizeof(output), &output_length);
    failures += bytes_equal(
        output, output_length, "+A2DPSTAT=1\r\n",
        text_length("+A2DPSTAT=1\r\n")) ? 0 : 1;

    event.id = BTSTACK_AT_EVENT_PLAYSTAT;
    failures += btstack_at_format_event(
        &event, output, sizeof(output), &output_length);
    failures += bytes_equal(
        output, output_length, "+PLAYSTAT=1\r\n",
        text_length("+PLAYSTAT=1\r\n")) ? 0 : 1;

    event.id = BTSTACK_AT_EVENT_ABSVOL;
    event.data.numeric.values[0] = 100u;
    failures += btstack_at_format_event(
        &event, output, sizeof(output), &output_length);
    failures += bytes_equal(
        output, output_length, "+ABSVOL=100\r\n",
        text_length("+ABSVOL=100\r\n")) ? 0 : 1;

    event.id = BTSTACK_AT_EVENT_PBAPCONTACT;
    event.data.phonebook.category = BT_XSTACK_PHONEBOOK_CATEGORY_CONTACTS;
    event.data.phonebook.data.contact.number_count = 1u;
    event.data.phonebook.data.contact.numbers[0].type_flags =
        BT_XSTACK_PHONEBOOK_TEL_CELL;
    event.data.phonebook.data.contact.name[0] = 'A';
    event.data.phonebook.data.contact.name[1] = 'l';
    event.data.phonebook.data.contact.name[2] = 'i';
    event.data.phonebook.data.contact.name[3] = 'c';
    event.data.phonebook.data.contact.name[4] = 'e';
    event.data.phonebook.data.contact.name[5] = '\0';
    event.data.phonebook.data.contact.numbers[0].value[0] = '1';
    event.data.phonebook.data.contact.numbers[0].value[1] = '3';
    event.data.phonebook.data.contact.numbers[0].value[2] = '8';
    event.data.phonebook.data.contact.numbers[0].value[3] = '\0';
    failures += btstack_at_format_event(
        &event, output, sizeof(output), &output_length);
    failures += bytes_equal(
        output, output_length,
        "+PBAPCONTACT=0\x0c" "Alice\x0c" "138\x0c" "4\r\n",
        text_length("+PBAPCONTACT=0\x0c" "Alice\x0c" "138\x0c" "4\r\n")) ? 0 : 1;

    event.id = BTSTACK_AT_EVENT_PBAPSTART;
    {
        uint8_t index;
        for (index = 0u; index < 6u; index++) {
            event.data.phonebook.address.bytes[index] = address[index];
        }
    }
    failures += btstack_at_format_event(
        &event, output, sizeof(output), &output_length);
    failures += bytes_equal(output, output_length,
        "+PBAPSTART=1C5CF226D773\r\n",
        text_length("+PBAPSTART=1C5CF226D773\r\n")) ? 0 : 1;

    event.id = BTSTACK_AT_EVENT_PBAPCALL;
    event.data.phonebook.category = BT_XSTACK_PHONEBOOK_CATEGORY_INCOMING;
    event.data.phonebook.data.call.name[0] = 'B';
    event.data.phonebook.data.call.name[1] = '\0';
    event.data.phonebook.data.call.number[0] = '1';
    event.data.phonebook.data.call.number[1] = '0';
    event.data.phonebook.data.call.number[2] = '0';
    event.data.phonebook.data.call.number[3] = '8';
    event.data.phonebook.data.call.number[4] = '6';
    event.data.phonebook.data.call.number[5] = '\0';
    event.data.phonebook.data.call.timestamp[0] = '2';
    event.data.phonebook.data.call.timestamp[1] = '0';
    event.data.phonebook.data.call.timestamp[2] = '2';
    event.data.phonebook.data.call.timestamp[3] = '6';
    event.data.phonebook.data.call.timestamp[4] = '\0';
    event.data.phonebook.data.call.direction =
        BT_XSTACK_PHONEBOOK_DIRECTION_INCOMING;
    failures += btstack_at_format_event(
        &event, output, sizeof(output), &output_length);
    failures += bytes_equal(output, output_length,
        "+PBAPCALL=1\x0c" "B\x0c" "10086\x0c" "2026\x0c" "1\r\n",
        text_length("+PBAPCALL=1\x0c" "B\x0c" "10086\x0c" "2026\x0c" "1\r\n")) ? 0 : 1;

    event.data.phonebook.category = BT_XSTACK_PHONEBOOK_CATEGORY_MISSED;
    event.data.phonebook.status = BT_XSTACK_PHONEBOOK_STATUS_PARTIAL;
    event.id = BTSTACK_AT_EVENT_PBAPCATEND;
    failures += btstack_at_format_event(
        &event, output, sizeof(output), &output_length);
    failures += bytes_equal(output, output_length,
        "+PBAPCATEND=3\x0c" "1\r\n",
        text_length("+PBAPCATEND=3\x0c" "1\r\n")) ? 0 : 1;

    event.data.phonebook.parser_error_count = 2u;
    event.id = BTSTACK_AT_EVENT_PBAPDONE;
    failures += btstack_at_format_event(
        &event, output, sizeof(output), &output_length);
    failures += bytes_equal(output, output_length,
        "+PBAPDONE=1\x0c" "2\r\n",
        text_length("+PBAPDONE=1\x0c" "2\r\n")) ? 0 : 1;

    event.id = BTSTACK_AT_EVENT_PBAPAUTHREQ;
    failures += btstack_at_format_event(
        &event, output, sizeof(output), &output_length);
    failures += bytes_equal(output, output_length,
        "+PBAPAUTHREQ=1C5CF226D773\r\n",
        text_length("+PBAPAUTHREQ=1C5CF226D773\r\n")) ? 0 : 1;

    event.id = BTSTACK_AT_EVENT_PBAPERROR;
    failures += btstack_at_format_event(
        &event, output, sizeof(output), &output_length);
    failures += bytes_equal(output, output_length,
        "+PBAPERROR=1\x0c" "3\x0c" "2\r\n",
        text_length("+PBAPERROR=1\x0c" "3\x0c" "2\r\n")) ? 0 : 1;

    failures += (btstack_at_format_simple(
        0,
        output,
        4u,
        &output_length) != 0) ? 0 : 1;

    return failures;
}

int main(void)
{
    return btstack_at_protocol_tests();
}
