#include "btstack_at_internal.h"

typedef struct {
    char *data;
    uint16_t capacity;
    uint16_t length;
    int failed;
} btstack_at_writer_t;

static void btstack_at_writer_init(
    btstack_at_writer_t *writer,
    char *output,
    uint16_t capacity)
{
    writer->data = output;
    writer->capacity = capacity;
    writer->length = 0u;
    writer->failed = ((output == NULL) || (capacity == 0u)) ? 1 : 0;
}

static void btstack_at_writer_byte(
    btstack_at_writer_t *writer,
    char value)
{
    if (writer->failed != 0) {
        return;
    }
    if (writer->length >= writer->capacity) {
        writer->failed = 1;
        return;
    }
    writer->data[writer->length++] = value;
}

static void btstack_at_writer_text(
    btstack_at_writer_t *writer,
    const char *text)
{
    uint16_t index = 0u;

    if (text == NULL) {
        writer->failed = 1;
        return;
    }
    while (text[index] != '\0') {
        btstack_at_writer_byte(writer, text[index]);
        index++;
    }
}

static void btstack_at_writer_decimal(
    btstack_at_writer_t *writer,
    uint32_t value)
{
    char digits[10];
    uint8_t count = 0u;

    do {
        digits[count++] = (char)('0' + (value % 10u));
        value /= 10u;
    } while ((value != 0u) && (count < sizeof(digits)));

    while (count != 0u) {
        btstack_at_writer_byte(writer, digits[--count]);
    }
}

static void btstack_at_writer_signed(
    btstack_at_writer_t *writer,
    int32_t value)
{
    uint32_t magnitude;
    if (value < 0) {
        btstack_at_writer_byte(writer, '-');
        magnitude = (uint32_t)(-(value + 1)) + 1u;
    } else {
        magnitude = (uint32_t)value;
    }
    btstack_at_writer_decimal(writer, magnitude);
}

static void btstack_at_writer_decimal_width(
    btstack_at_writer_t *writer,
    uint32_t value,
    uint8_t width)
{
    char digits[10];
    uint8_t count = 0u;

    do {
        digits[count++] = (char)('0' + (value % 10u));
        value /= 10u;
    } while ((value != 0u) && (count < sizeof(digits)));
    while (width > count) {
        btstack_at_writer_byte(writer, '0');
        width--;
    }
    while (count != 0u) {
        btstack_at_writer_byte(writer, digits[--count]);
    }
}

static void btstack_at_writer_address(
    btstack_at_writer_t *writer,
    const uint8_t address[BTSTACK_AT_ADDRESS_LEN])
{
    static const char hex[] = "0123456789ABCDEF";
    uint8_t index;

    if (address == NULL) {
        writer->failed = 1;
        return;
    }
    for (index = 0u; index < BTSTACK_AT_ADDRESS_LEN; index++) {
        btstack_at_writer_byte(writer, hex[address[index] >> 4]);
        btstack_at_writer_byte(writer, hex[address[index] & 0x0fu]);
    }
}

static void btstack_at_writer_hex(
    btstack_at_writer_t *writer,
    uint32_t value,
    uint8_t digits)
{
    static const char hex[] = "0123456789ABCDEF";
    uint8_t shift;

    if ((digits == 0u) || (digits > 8u)) {
        writer->failed = 1;
        return;
    }
    shift = (uint8_t)((digits - 1u) * 4u);
    while (digits != 0u) {
        btstack_at_writer_byte(
            writer,
            hex[(value >> shift) & 0x0fu]);
        digits--;
        if (shift >= 4u) {
            shift = (uint8_t)(shift - 4u);
        }
    }
}

static void btstack_at_writer_hex_bytes(btstack_at_writer_t *writer,
    const uint8_t *data, uint16_t length)
{
    uint16_t index;

    if ((data == NULL) && (length != 0u)) {
        writer->failed = 1;
        return;
    }
    for (index = 0u; index < length; index++) {
        btstack_at_writer_hex(writer, data[index], 2u);
    }
}

static void btstack_at_writer_uuid(btstack_at_writer_t *writer,
    const bt_xstack_uuid_t *uuid)
{
    if (uuid == NULL) {
        writer->failed = 1;
        return;
    }
    if (uuid->type == BT_XSTACK_UUID_TYPE_16) {
        btstack_at_writer_hex(writer, uuid->value.uuid16, 4u);
    } else {
        btstack_at_writer_hex_bytes(writer, uuid->value.uuid128, 16u);
    }
}

static int btstack_at_writer_finish(
    btstack_at_writer_t *writer,
    uint16_t *output_length)
{
    if ((output_length == NULL) || (writer->failed != 0)) {
        if (output_length != NULL) {
            *output_length = 0u;
        }
        if ((writer->data != NULL) && (writer->capacity != 0u)) {
            writer->data[0] = '\0';
        }
        return -1;
    }
    *output_length = writer->length;
    if (writer->length < writer->capacity) {
        writer->data[writer->length] = '\0';
    }
    return 0;
}

static void btstack_at_writer_prefix(
    btstack_at_writer_t *writer,
    const char *name)
{
    btstack_at_writer_byte(writer, '+');
    btstack_at_writer_text(writer, name);
    btstack_at_writer_byte(writer, '=');
}

static void btstack_at_writer_line_end(btstack_at_writer_t *writer)
{
    btstack_at_writer_byte(writer, '\r');
    btstack_at_writer_byte(writer, '\n');
}

static void btstack_at_writer_form_feed(btstack_at_writer_t *writer)
{
    btstack_at_writer_byte(writer, 0x0c);
}

int btstack_at_format_simple(
    int success,
    char *output,
    uint16_t capacity,
    uint16_t *output_length)
{
    btstack_at_writer_t writer;

    btstack_at_writer_init(&writer, output, capacity);
    btstack_at_writer_text(&writer, (success != 0) ? "OK" : "ERROR");
    btstack_at_writer_line_end(&writer);
    return btstack_at_writer_finish(&writer, output_length);
}

int btstack_at_format_address_response(
    const char *name,
    const uint8_t address[BTSTACK_AT_ADDRESS_LEN],
    char *output,
    uint16_t capacity,
    uint16_t *output_length)
{
    btstack_at_writer_t writer;

    btstack_at_writer_init(&writer, output, capacity);
    btstack_at_writer_prefix(&writer, name);
    btstack_at_writer_address(&writer, address);
    btstack_at_writer_line_end(&writer);
    btstack_at_writer_text(&writer, "OK");
    btstack_at_writer_line_end(&writer);
    return btstack_at_writer_finish(&writer, output_length);
}

int btstack_at_format_uint_line(
    const char *name,
    uint32_t value,
    int append_ok,
    char *output,
    uint16_t capacity,
    uint16_t *output_length)
{
    btstack_at_writer_t writer;

    btstack_at_writer_init(&writer, output, capacity);
    btstack_at_writer_prefix(&writer, name);
    btstack_at_writer_decimal(&writer, value);
    btstack_at_writer_line_end(&writer);
    if (append_ok != 0) {
        btstack_at_writer_text(&writer, "OK");
        btstack_at_writer_line_end(&writer);
    }
    return btstack_at_writer_finish(&writer, output_length);
}

int btstack_at_format_text_response(
    const char *name,
    const char *value,
    char *output,
    uint16_t capacity,
    uint16_t *output_length)
{
    btstack_at_writer_t writer;

    btstack_at_writer_init(&writer, output, capacity);
    btstack_at_writer_prefix(&writer, name);
    btstack_at_writer_text(&writer, value);
    btstack_at_writer_line_end(&writer);
    btstack_at_writer_text(&writer, "OK");
    btstack_at_writer_line_end(&writer);
    return btstack_at_writer_finish(&writer, output_length);
}

int btstack_at_format_hex_response(
    const char *name,
    uint32_t value,
    uint8_t digits,
    char *output,
    uint16_t capacity,
    uint16_t *output_length)
{
    btstack_at_writer_t writer;

    btstack_at_writer_init(&writer, output, capacity);
    btstack_at_writer_prefix(&writer, name);
    btstack_at_writer_hex(&writer, value, digits);
    btstack_at_writer_line_end(&writer);
    btstack_at_writer_text(&writer, "OK");
    btstack_at_writer_line_end(&writer);
    return btstack_at_writer_finish(&writer, output_length);
}

int btstack_at_format_plist_record(
    uint8_t index,
    uint32_t profile_mask,
    const uint8_t address[BTSTACK_AT_ADDRESS_LEN],
    const char *name,
    char *output,
    uint16_t capacity,
    uint16_t *output_length)
{
    btstack_at_writer_t writer;

    btstack_at_writer_init(&writer, output, capacity);
    btstack_at_writer_prefix(&writer, "PLIST");
    btstack_at_writer_decimal(&writer, index);
    btstack_at_writer_byte(&writer, 0x0c);
    btstack_at_writer_decimal(&writer, profile_mask);
    btstack_at_writer_byte(&writer, 0x0c);
    btstack_at_writer_address(&writer, address);
    if ((name != NULL) && (name[0] != '\0')) {
        btstack_at_writer_byte(&writer, 0x0c);
        btstack_at_writer_text(&writer, name);
    }
    btstack_at_writer_line_end(&writer);
    return btstack_at_writer_finish(&writer, output_length);
}

int btstack_at_format_plist_end(
    char *output,
    uint16_t capacity,
    uint16_t *output_length)
{
    btstack_at_writer_t writer;

    btstack_at_writer_init(&writer, output, capacity);
    btstack_at_writer_text(&writer, "+PLIST=E");
    btstack_at_writer_line_end(&writer);
    return btstack_at_writer_finish(&writer, output_length);
}

int btstack_at_format_track_info(
    const char *title,
    const char *artist,
    const char *album,
    char *output,
    uint16_t capacity,
    uint16_t *output_length)
{
    btstack_at_writer_t writer;

    btstack_at_writer_init(&writer, output, capacity);
    btstack_at_writer_prefix(&writer, "TRACKINFO");
    btstack_at_writer_text(&writer, title);
    btstack_at_writer_byte(&writer, 0x0c);
    btstack_at_writer_text(&writer, artist);
    btstack_at_writer_byte(&writer, 0x0c);
    btstack_at_writer_text(&writer, album);
    btstack_at_writer_line_end(&writer);
    return btstack_at_writer_finish(&writer, output_length);
}

static void btstack_at_writer_uint_fields(
    btstack_at_writer_t *writer,
    const uint32_t *values,
    uint8_t value_count)
{
    uint8_t index;
    for (index = 0u; index < value_count; index++) {
        if (index != 0u) {
            btstack_at_writer_byte(writer, ',');
        }
        btstack_at_writer_decimal(writer, values[index]);
    }
}

typedef struct {
    btstack_at_event_id_t id;
    const char *name;
} btstack_at_numeric_event_name_t;

static const btstack_at_numeric_event_name_t
    btstack_at_numeric_event_names[] = {
    { BTSTACK_AT_EVENT_DEVSTAT, "DEVSTAT" },
    { BTSTACK_AT_EVENT_PWRSTAT, "PWRSTAT" },
    { BTSTACK_AT_EVENT_MICMUTED, "MICMUTED" },
    { BTSTACK_AT_EVENT_HFPAUDIO, "HFPAUDIO" },
    { BTSTACK_AT_EVENT_HFPVR, "HFPVR" },
    { BTSTACK_AT_EVENT_HFPSIG, "HFPSIG" },
    { BTSTACK_AT_EVENT_HFPROAM, "HFPROAM" },
    { BTSTACK_AT_EVENT_HFPBATT, "HFPBATT" },
    { BTSTACK_AT_EVENT_HFPIBR, "HFPIBR" },
    { BTSTACK_AT_EVENT_HFPRING, "HFPRING" },
    { BTSTACK_AT_EVENT_A2DPSTAT, "A2DPSTAT" },
    { BTSTACK_AT_EVENT_A2DPPCMCONFIG, "A2DPPCMCONFIG" },
    { BTSTACK_AT_EVENT_AVRCPSTAT, "AVRCPSTAT" },
    { BTSTACK_AT_EVENT_PLAYSTAT, "PLAYSTAT" },
    { BTSTACK_AT_EVENT_PLAYMODE, "PLAYMODE" },
    { BTSTACK_AT_EVENT_TRACKSTAT, "TRACKSTAT" },
    { BTSTACK_AT_EVENT_ABSVOL, "ABSVOL" }
};

static const char *btstack_at_numeric_event_name(
    btstack_at_event_id_t id)
{
    uint16_t index;

    for (index = 0u;
         index < (sizeof(btstack_at_numeric_event_names) /
                  sizeof(btstack_at_numeric_event_names[0]));
         index++) {
        if (btstack_at_numeric_event_names[index].id == id) {
            return btstack_at_numeric_event_names[index].name;
        }
    }
    return NULL;
}

int btstack_at_format_event(
    const btstack_at_event_t *event,
    char *output,
    uint16_t capacity,
    uint16_t *output_length)
{
    btstack_at_writer_t writer;
    const char *name;

    if (event == NULL) {
        return -1;
    }
    btstack_at_writer_init(&writer, output, capacity);

    name = btstack_at_numeric_event_name(event->id);
    if (name != NULL) {
        btstack_at_writer_prefix(&writer, name);
        btstack_at_writer_uint_fields(&writer,
            event->data.numeric.values,
            event->data.numeric.value_count);
        btstack_at_writer_line_end(&writer);
        return btstack_at_writer_finish(&writer, output_length);
    }

    switch (event->id) {
    case BTSTACK_AT_EVENT_LE: {
        const bt_xstack_le_event_t *le = &event->data.le;
        if ((le->type == BT_XSTACK_LE_EVENT_ADV_STARTED) ||
            (le->type == BT_XSTACK_LE_EVENT_ADV_STOPPED)) {
            btstack_at_writer_prefix(&writer, "LEADV");
        } else if ((le->type == BT_XSTACK_LE_EVENT_SCAN_RESULT) ||
                   (le->type == BT_XSTACK_LE_EVENT_SCAN_STOPPED)) {
            btstack_at_writer_prefix(&writer, "LESCAN");
        } else if ((le->type >= BT_XSTACK_LE_EVENT_CONNECTING) &&
                   (le->type <= BT_XSTACK_LE_EVENT_CONNECTION_UPDATED)) {
            btstack_at_writer_prefix(&writer, "LECONN");
        } else {
            btstack_at_writer_prefix(&writer, "LEPAIR");
        }
        btstack_at_writer_decimal(&writer, (uint32_t)le->type);
        btstack_at_writer_byte(&writer, ',');
        btstack_at_writer_signed(&writer, le->status);
        if (le->type == BT_XSTACK_LE_EVENT_SCAN_RESULT) {
            btstack_at_writer_byte(&writer, ',');
            btstack_at_writer_decimal(&writer, le->data.scan.address_type);
            btstack_at_writer_byte(&writer, ',');
            btstack_at_writer_address(&writer, le->data.scan.address.bytes);
            btstack_at_writer_byte(&writer, ',');
            btstack_at_writer_signed(&writer, le->data.scan.rssi);
            btstack_at_writer_byte(&writer, ',');
            btstack_at_writer_decimal(&writer,
                le->data.scan.advertising_event_type);
            btstack_at_writer_byte(&writer, ',');
            btstack_at_writer_hex_bytes(&writer, le->data.scan.data,
                le->data.scan.data_len);
        } else if ((le->type >= BT_XSTACK_LE_EVENT_CONNECTING) &&
                   (le->type <= BT_XSTACK_LE_EVENT_CONNECTION_UPDATED)) {
            btstack_at_writer_byte(&writer, ',');
            btstack_at_writer_decimal(&writer,
                le->data.link.connection_handle);
            btstack_at_writer_byte(&writer, ',');
            btstack_at_writer_decimal(&writer, le->data.link.address_type);
            btstack_at_writer_byte(&writer, ',');
            btstack_at_writer_address(&writer, le->data.link.address.bytes);
            btstack_at_writer_byte(&writer, ',');
            btstack_at_writer_decimal(&writer, le->data.link.role);
            btstack_at_writer_byte(&writer, ',');
            btstack_at_writer_decimal(&writer, le->data.link.reason);
            btstack_at_writer_byte(&writer, ',');
            btstack_at_writer_decimal(&writer, le->data.link.interval);
            btstack_at_writer_byte(&writer, ',');
            btstack_at_writer_decimal(&writer, le->data.link.latency);
            btstack_at_writer_byte(&writer, ',');
            btstack_at_writer_decimal(&writer,
                le->data.link.supervision_timeout);
        } else if (le->type >= BT_XSTACK_LE_EVENT_PAIRING_REQUEST) {
            btstack_at_writer_byte(&writer, ',');
            btstack_at_writer_decimal_width(&writer,
                le->data.security.passkey, 6u);
            btstack_at_writer_byte(&writer, ',');
            btstack_at_writer_decimal(&writer,
                le->data.security.bond_index);
            btstack_at_writer_byte(&writer, ',');
            btstack_at_writer_decimal(&writer,
                le->data.security.encrypted ? 1u : 0u);
            btstack_at_writer_byte(&writer, ',');
            btstack_at_writer_decimal(&writer,
                le->data.security.bonded ? 1u : 0u);
        }
        break;
    }

    case BTSTACK_AT_EVENT_GATT: {
        const bt_xstack_gatt_event_t *gatt = &event->data.gatt;
        const bt_xstack_gatt_value_event_t *value = &gatt->data.value;
        const char *gatt_name =
            (gatt->type >= BT_XSTACK_GATT_EVENT_SERVER_COMPLETE) ?
                "GATTS" : "GATTC";
        btstack_at_writer_prefix(&writer, gatt_name);
        btstack_at_writer_decimal(&writer, (uint32_t)gatt->type);
        btstack_at_writer_byte(&writer, ',');
        btstack_at_writer_signed(&writer, gatt->status);
        switch (gatt->type) {
        case BT_XSTACK_GATT_EVENT_SERVICE:
            btstack_at_writer_byte(&writer, ',');
            btstack_at_writer_decimal(&writer,
                gatt->data.service.start_handle);
            btstack_at_writer_byte(&writer, ',');
            btstack_at_writer_decimal(&writer,
                gatt->data.service.end_handle);
            btstack_at_writer_byte(&writer, ',');
            btstack_at_writer_uuid(&writer, &gatt->data.service.uuid);
            break;
        case BT_XSTACK_GATT_EVENT_CHARACTERISTIC:
            btstack_at_writer_byte(&writer, ',');
            btstack_at_writer_decimal(&writer,
                gatt->data.characteristic.start_handle);
            btstack_at_writer_byte(&writer, ',');
            btstack_at_writer_decimal(&writer,
                gatt->data.characteristic.value_handle);
            btstack_at_writer_byte(&writer, ',');
            btstack_at_writer_decimal(&writer,
                gatt->data.characteristic.end_handle);
            btstack_at_writer_byte(&writer, ',');
            btstack_at_writer_decimal(&writer,
                gatt->data.characteristic.properties);
            btstack_at_writer_byte(&writer, ',');
            btstack_at_writer_uuid(&writer,
                &gatt->data.characteristic.uuid);
            break;
        case BT_XSTACK_GATT_EVENT_DESCRIPTOR:
            btstack_at_writer_byte(&writer, ',');
            btstack_at_writer_decimal(&writer,
                gatt->data.descriptor.handle);
            btstack_at_writer_byte(&writer, ',');
            btstack_at_writer_uuid(&writer, &gatt->data.descriptor.uuid);
            break;
        case BT_XSTACK_GATT_EVENT_MTU:
            btstack_at_writer_byte(&writer, ',');
            btstack_at_writer_decimal(&writer, value->mtu);
            break;
        case BT_XSTACK_GATT_EVENT_CLIENT_COMPLETE:
            btstack_at_writer_byte(&writer, ',');
            btstack_at_writer_decimal(&writer, value->operation);
            break;
        case BT_XSTACK_GATT_EVENT_SERVER_COMPLETE:
            btstack_at_writer_byte(&writer, ',');
            btstack_at_writer_decimal(&writer, value->operation);
            btstack_at_writer_byte(&writer, ',');
            btstack_at_writer_decimal(&writer, value->value_handle);
            break;
        default:
            btstack_at_writer_byte(&writer, ',');
            btstack_at_writer_decimal(&writer, value->value_handle);
            btstack_at_writer_byte(&writer, ',');
            btstack_at_writer_decimal(&writer, value->value_len);
            btstack_at_writer_byte(&writer, ',');
            btstack_at_writer_hex_bytes(&writer, value->value,
                value->value_len);
            btstack_at_writer_byte(&writer, ',');
            btstack_at_writer_decimal(&writer, value->truncated);
            break;
        }
        break;
    }

    case BTSTACK_AT_EVENT_PBAPSTART:
        btstack_at_writer_prefix(&writer, "PBAPSTART");
        btstack_at_writer_address(&writer,
            event->data.phonebook.address.bytes);
        break;

    case BTSTACK_AT_EVENT_PBAPCONTACT: {
        uint8_t index;
        btstack_at_writer_prefix(&writer, "PBAPCONTACT");
        btstack_at_writer_decimal(&writer, event->data.phonebook.category);
        btstack_at_writer_form_feed(&writer);
        btstack_at_writer_text(&writer,
            event->data.phonebook.data.contact.name);
        for (index = 0u;
             index < event->data.phonebook.data.contact.number_count;
             index++) {
            btstack_at_writer_form_feed(&writer);
            btstack_at_writer_text(&writer,
                event->data.phonebook.data.contact.numbers[index].value);
            btstack_at_writer_form_feed(&writer);
            btstack_at_writer_decimal(&writer,
                event->data.phonebook.data.contact.numbers[index].type_flags);
        }
        break;
    }

    case BTSTACK_AT_EVENT_PBAPCALL:
        btstack_at_writer_prefix(&writer, "PBAPCALL");
        btstack_at_writer_decimal(&writer, event->data.phonebook.category);
        btstack_at_writer_form_feed(&writer);
        btstack_at_writer_text(&writer,
            event->data.phonebook.data.call.name);
        btstack_at_writer_form_feed(&writer);
        btstack_at_writer_text(&writer,
            event->data.phonebook.data.call.number);
        btstack_at_writer_form_feed(&writer);
        btstack_at_writer_text(&writer,
            event->data.phonebook.data.call.timestamp);
        btstack_at_writer_form_feed(&writer);
        btstack_at_writer_decimal(&writer,
            event->data.phonebook.data.call.direction);
        break;

    case BTSTACK_AT_EVENT_PBAPCATEND:
        btstack_at_writer_prefix(&writer, "PBAPCATEND");
        btstack_at_writer_decimal(&writer, event->data.phonebook.category);
        btstack_at_writer_form_feed(&writer);
        btstack_at_writer_decimal(&writer, event->data.phonebook.status);
        break;

    case BTSTACK_AT_EVENT_PBAPDONE:
        btstack_at_writer_prefix(&writer, "PBAPDONE");
        btstack_at_writer_decimal(&writer, event->data.phonebook.status);
        btstack_at_writer_form_feed(&writer);
        btstack_at_writer_decimal(&writer,
            event->data.phonebook.parser_error_count);
        break;

    case BTSTACK_AT_EVENT_PBAPAUTHREQ:
        btstack_at_writer_prefix(&writer, "PBAPAUTHREQ");
        btstack_at_writer_address(&writer,
            event->data.phonebook.address.bytes);
        break;

    case BTSTACK_AT_EVENT_PBAPERROR:
        btstack_at_writer_prefix(&writer, "PBAPERROR");
        btstack_at_writer_decimal(&writer, event->data.phonebook.status);
        btstack_at_writer_form_feed(&writer);
        btstack_at_writer_decimal(&writer, event->data.phonebook.category);
        btstack_at_writer_form_feed(&writer);
        btstack_at_writer_decimal(&writer,
            event->data.phonebook.parser_error_count);
        break;

    case BTSTACK_AT_EVENT_SCAN_RESULT:
        btstack_at_writer_prefix(&writer, "SCAN");
        btstack_at_writer_decimal(&writer, event->data.scan.profile_mask);
        btstack_at_writer_byte(&writer, 0x0c);
        btstack_at_writer_signed(&writer, event->data.scan.rssi);
        btstack_at_writer_byte(&writer, 0x0c);
        btstack_at_writer_decimal(&writer, event->data.scan.address_type);
        btstack_at_writer_byte(&writer, 0x0c);
        btstack_at_writer_address(&writer, event->data.scan.address);
        btstack_at_writer_byte(&writer, 0x0c);
        btstack_at_writer_text(&writer, event->data.scan.name);
        btstack_at_writer_byte(&writer, 0x0c);
        btstack_at_writer_hex(&writer,
            event->data.scan.class_of_device, 6u);
        break;

    case BTSTACK_AT_EVENT_SCAN_END:
        btstack_at_writer_text(&writer, "+SCAN=E");
        break;

    case BTSTACK_AT_EVENT_PAIR_REQUEST:
        btstack_at_writer_prefix(&writer, "PAIRREQ");
        btstack_at_writer_decimal_width(
            &writer,
            event->data.peer.values[0],
            6u);
        btstack_at_writer_byte(&writer, ',');
        btstack_at_writer_address(&writer, event->data.peer.address);
        if (event->data.peer.text[0] != '\0') {
            btstack_at_writer_byte(&writer, ',');
            btstack_at_writer_text(&writer, event->data.peer.text);
        }
        break;

    case BTSTACK_AT_EVENT_PAIRED:
        btstack_at_writer_prefix(&writer, "PAIRED");
        btstack_at_writer_decimal(&writer, event->data.peer.values[0]);
        btstack_at_writer_byte(&writer, ',');
        btstack_at_writer_address(&writer, event->data.peer.address);
        break;

    case BTSTACK_AT_EVENT_HFPSTAT:
        btstack_at_writer_prefix(&writer, "HFPSTAT");
        btstack_at_writer_decimal(&writer, event->data.peer.values[0]);
        if (event->data.peer.text[0] != '\0') {
            btstack_at_writer_byte(&writer, ',');
            btstack_at_writer_text(&writer, event->data.peer.text);
        }
        break;

    case BTSTACK_AT_EVENT_HFPDEV:
    case BTSTACK_AT_EVENT_A2DPDEV:
        btstack_at_writer_prefix(
            &writer,
            (event->id == BTSTACK_AT_EVENT_HFPDEV) ?
                "HFPDEV" : "A2DPDEV");
        btstack_at_writer_address(&writer, event->data.peer.address);
        if (event->data.peer.text[0] != '\0') {
            btstack_at_writer_byte(&writer, 0x0c);
            btstack_at_writer_text(&writer, event->data.peer.text);
        }
        break;

    case BTSTACK_AT_EVENT_HFPNET:
    case BTSTACK_AT_EVENT_HFPMANU:
    case BTSTACK_AT_EVENT_HFPNUM:
        name = (event->id == BTSTACK_AT_EVENT_HFPNET) ? "HFPNET" :
               (event->id == BTSTACK_AT_EVENT_HFPMANU) ? "HFPMANU" :
               "HFPNUM";
        btstack_at_writer_prefix(&writer, name);
        btstack_at_writer_text(&writer, event->data.text.value);
        break;

    case BTSTACK_AT_EVENT_HFPLOST:
    case BTSTACK_AT_EVENT_A2DPLOST:
        btstack_at_writer_prefix(
            &writer,
            (event->id == BTSTACK_AT_EVENT_HFPLOST) ?
                "HFPLOST" : "A2DPLOST");
        btstack_at_writer_address(&writer, event->data.peer.address);
        break;

    case BTSTACK_AT_EVENT_HFPMTPX:
        btstack_at_writer_prefix(&writer, "HFPMTPX");
        btstack_at_writer_uint_fields(&writer,
            event->data.peer.values,
            event->data.peer.value_count);
        if (event->data.peer.text[0] != '\0') {
            btstack_at_writer_byte(&writer, ',');
            btstack_at_writer_text(&writer, event->data.peer.text);
        }
        break;

    case BTSTACK_AT_EVENT_TRACKINFO:
        btstack_at_writer_prefix(&writer, "TRACKINFO");
        btstack_at_writer_text(&writer, event->data.metadata.title);
        btstack_at_writer_byte(&writer, 0x0c);
        btstack_at_writer_text(&writer, event->data.metadata.artist);
        btstack_at_writer_byte(&writer, 0x0c);
        btstack_at_writer_text(&writer, event->data.metadata.album);
        break;

    default:
        writer.failed = 1;
        break;
    }

    btstack_at_writer_line_end(&writer);
    return btstack_at_writer_finish(&writer, output_length);
}
