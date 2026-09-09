#include "btstack_at_internal.h"
#include "btstack_at_command_table.h"

typedef struct {
    const char *data;
    uint16_t length;
} btstack_at_slice_t;


static void btstack_at_clear_command(btstack_at_command_t *command)
{
    uint16_t index;
    uint8_t *bytes = (uint8_t *)command;

    for (index = 0u; index < sizeof(*command); index++) {
        bytes[index] = 0u;
    }
    command->id = BTSTACK_AT_COMMAND_UNSUPPORTED;
    command->form = BTSTACK_AT_FORM_EXECUTE;
}

static int btstack_at_is_command_character(char value)
{
    return ((value >= 'A') && (value <= 'Z')) ||
           ((value >= '0') && (value <= '9'));
}

static int btstack_at_is_printable(char value)
{
    return (value >= 0x20) && (value <= 0x7e);
}

static int btstack_at_hex_value(char value)
{
    if ((value >= '0') && (value <= '9')) {
        return value - '0';
    }
    if ((value >= 'A') && (value <= 'F')) {
        return value - 'A' + 10;
    }
    if ((value >= 'a') && (value <= 'f')) {
        return value - 'a' + 10;
    }
    return -1;
}

static int btstack_at_parse_unsigned(
    btstack_at_slice_t slice,
    uint32_t maximum,
    uint32_t *value)
{
    uint16_t index;
    uint32_t parsed = 0u;

    if ((slice.length == 0u) || (value == NULL)) {
        return -1;
    }
    for (index = 0u; index < slice.length; index++) {
        uint32_t digit;
        if ((slice.data[index] < '0') || (slice.data[index] > '9')) {
            return -1;
        }
        digit = (uint32_t)(slice.data[index] - '0');
        if ((digit > maximum) ||
            (parsed > ((maximum - digit) / 10u))) {
            return -1;
        }
        parsed = (parsed * 10u) + digit;
    }
    *value = parsed;
    return 0;
}

static int btstack_at_parse_hex_unsigned(
    btstack_at_slice_t slice,
    uint16_t required_length,
    uint32_t *value)
{
    uint16_t index;
    uint32_t parsed = 0u;

    if ((slice.length != required_length) || (value == NULL)) {
        return -1;
    }
    for (index = 0u; index < slice.length; index++) {
        int digit = btstack_at_hex_value(slice.data[index]);
        if (digit < 0) {
            return -1;
        }
        parsed = (parsed << 4) | (uint32_t)digit;
    }
    *value = parsed;
    return 0;
}

static int btstack_at_parse_hex_bytes(btstack_at_slice_t slice,
    uint8_t *destination, uint16_t capacity, uint16_t *length)
{
    uint16_t index;

    if ((destination == NULL) || (length == NULL) ||
        ((slice.length & 1u) != 0u) || ((slice.length / 2u) > capacity)) {
        return -1;
    }
    for (index = 0u; index < slice.length; index += 2u) {
        int high = btstack_at_hex_value(slice.data[index]);
        int low = btstack_at_hex_value(slice.data[index + 1u]);
        if ((high < 0) || (low < 0)) {
            return -1;
        }
        destination[index / 2u] = (uint8_t)((high << 4) | low);
    }
    *length = slice.length / 2u;
    return 0;
}

static int btstack_at_parse_uuid(btstack_at_slice_t slice,
    bt_xstack_uuid_t *uuid)
{
    uint32_t uuid16;
    uint16_t length;

    if (uuid == NULL) {
        return -1;
    }
    if (slice.length == 4u) {
        if (btstack_at_parse_hex_unsigned(slice, 4u, &uuid16) != 0) {
            return -1;
        }
        uuid->type = BT_XSTACK_UUID_TYPE_16;
        uuid->value.uuid16 = (uint16_t)uuid16;
        return 0;
    }
    if (slice.length != 32u) {
        return -1;
    }
    uuid->type = BT_XSTACK_UUID_TYPE_128;
    return btstack_at_parse_hex_bytes(slice, uuid->value.uuid128,
        sizeof(uuid->value.uuid128), &length);
}

static int btstack_at_parse_address(
    btstack_at_slice_t slice,
    uint8_t address[BTSTACK_AT_ADDRESS_LEN])
{
    uint8_t index;

    if (slice.length != 12u) {
        return -1;
    }
    for (index = 0u; index < BTSTACK_AT_ADDRESS_LEN; index++) {
        int high = btstack_at_hex_value(slice.data[index * 2u]);
        int low = btstack_at_hex_value(slice.data[(index * 2u) + 1u]);
        if ((high < 0) || (low < 0)) {
            return -1;
        }
        address[index] = (uint8_t)((high << 4) | low);
    }
    return 0;
}

static int btstack_at_copy_text(
    btstack_at_slice_t slice,
    uint16_t minimum,
    uint16_t maximum,
    char destination[BTSTACK_AT_TEXT_MAX],
    uint8_t *destination_length)
{
    uint16_t index;

    if ((slice.length < minimum) ||
        (slice.length > maximum) ||
        (slice.length >= BTSTACK_AT_TEXT_MAX)) {
        return -1;
    }
    for (index = 0u; index < slice.length; index++) {
        if (!btstack_at_is_printable(slice.data[index])) {
            return -1;
        }
        destination[index] = slice.data[index];
    }
    destination[slice.length] = '\0';
    *destination_length = (uint8_t)slice.length;
    return 0;
}

static int btstack_at_split_parameters(
    const char *data,
    uint16_t length,
    btstack_at_slice_t slices[6],
    uint8_t *count)
{
    uint16_t start = 0u;
    uint16_t index;
    uint8_t used = 0u;

    if ((data == NULL) || (length == 0u) || (count == NULL)) {
        return -1;
    }
    for (index = 0u; index <= length; index++) {
        if ((index == length) || (data[index] == ',')) {
            if ((used >= 6u) || (index == start)) {
                return -1;
            }
            slices[used].data = &data[start];
            slices[used].length = index - start;
            used++;
            start = index + 1u;
        }
    }
    *count = used;
    return 0;
}

static int btstack_at_validate_single_unsigned(
    btstack_at_slice_t slice,
    uint32_t maximum,
    btstack_at_command_t *command)
{
    if (btstack_at_parse_unsigned(slice, maximum, &command->values[0]) != 0) {
        return -1;
    }
    command->value_count = 1u;
    return 0;
}

static int btstack_at_slice_equal(btstack_at_slice_t slice, const char *text)
{
    uint16_t index = 0u;

    while (text[index] != '\0') index++;
    if (slice.length != index) return 0;
    for (index = 0u; index < slice.length; index++) {
        if (slice.data[index] != text[index]) return 0;
    }
    return 1;
}

static int btstack_at_parse_cover_encoding(btstack_at_slice_t slice,
    bt_xstack_cover_art_encoding_t *encoding)
{
    if (btstack_at_slice_equal(slice, "JPEG")) *encoding = BT_XSTACK_COVER_ART_ENCODING_JPEG;
    else if (btstack_at_slice_equal(slice, "PNG")) *encoding = BT_XSTACK_COVER_ART_ENCODING_PNG;
    else if (btstack_at_slice_equal(slice, "GIF")) *encoding = BT_XSTACK_COVER_ART_ENCODING_GIF;
    else if (btstack_at_slice_equal(slice, "BMP")) *encoding = BT_XSTACK_COVER_ART_ENCODING_BMP;
    else if (btstack_at_slice_equal(slice, "WBMP")) *encoding = BT_XSTACK_COVER_ART_ENCODING_WBMP;
    else return -1;
    return 0;
}

static int btstack_at_parse_cover_transformation(btstack_at_slice_t slice,
    uint8_t *transformation)
{
    if (btstack_at_slice_equal(slice, "NONE")) *transformation = BT_XSTACK_COVER_ART_TRANSFORMATION_NONE;
    else if (btstack_at_slice_equal(slice, "STRETCH")) *transformation = BT_XSTACK_COVER_ART_TRANSFORMATION_STRETCH;
    else if (btstack_at_slice_equal(slice, "FILL")) *transformation = BT_XSTACK_COVER_ART_TRANSFORMATION_FILL;
    else if (btstack_at_slice_equal(slice, "CROP")) *transformation = BT_XSTACK_COVER_ART_TRANSFORMATION_CROP;
    else return -1;
    return 0;
}

static int btstack_at_validate_set(
    btstack_at_command_t *command,
    const char *parameters,
    uint16_t parameters_length)
{
    btstack_at_slice_t slices[6];
    uint8_t count = 0u;
    uint32_t value;

    if (btstack_at_split_parameters(
            parameters,
            parameters_length,
            slices,
            &count) != 0) {
        return -1;
    }

    switch (command->id) {
    case BTSTACK_AT_COMMAND_PROFILE:
        if ((count != 1u) ||
            (btstack_at_validate_single_unsigned(
                slices[0],
                BTSTACK_AT_PROFILE_MASK,
                command) != 0) ||
            (command->values[0] != BTSTACK_AT_PROFILE_MASK)) {
            return -1;
        }
        return 0;

    case BTSTACK_AT_COMMAND_BTEN:
    case BTSTACK_AT_COMMAND_MICMUTE:
    case BTSTACK_AT_COMMAND_A2DPMUTE:
    case BTSTACK_AT_COMMAND_HFPVR:
    case BTSTACK_AT_COMMAND_REPEAT:
    case BTSTACK_AT_COMMAND_SHUFFLE:
    case BTSTACK_AT_COMMAND_COVERMODE:
        return (count == 1u) ?
            btstack_at_validate_single_unsigned(slices[0], 1u, command) : -1;

    case BTSTACK_AT_COMMAND_NAME:
        if ((count < 1u) || (count > 2u) ||
            (btstack_at_copy_text(
                slices[0],
                1u,
                31u,
                command->text,
                &command->text_length) != 0)) {
            return -1;
        }
        if (count == 2u) {
            if (btstack_at_validate_single_unsigned(
                    slices[1],
                    1u,
                    command) != 0) {
                return -1;
            }
        } else {
            command->values[0] = 0u;
            command->value_count = 1u;
        }
        return 0;

    case BTSTACK_AT_COMMAND_PIN:
        return (count == 1u) ?
            btstack_at_copy_text(
                slices[0],
                4u,
                15u,
                command->text,
                &command->text_length) : -1;

    case BTSTACK_AT_COMMAND_SSP:
        return (count == 1u) ?
            btstack_at_validate_single_unsigned(slices[0], 3u, command) : -1;

    case BTSTACK_AT_COMMAND_CFM:
        if ((count != 2u) ||
            (btstack_at_parse_address(slices[0], command->address) != 0) ||
            (btstack_at_validate_single_unsigned(
                slices[1],
                1u,
                command) != 0)) {
            return -1;
        }
        command->has_address = true;
        return 0;

    case BTSTACK_AT_COMMAND_PAIR:
    case BTSTACK_AT_COMMAND_PAGE:
        return (count == 1u) ?
            btstack_at_validate_single_unsigned(slices[0], 1u, command) : -1;

    case BTSTACK_AT_COMMAND_COD:
        if ((count != 1u) ||
            (btstack_at_parse_hex_unsigned(slices[0], 6u, &value) != 0)) {
            return -1;
        }
        command->values[0] = value;
        command->value_count = 1u;
        return 0;

    case BTSTACK_AT_COMMAND_PLIST:
        if (count != 1u) {
            return -1;
        }
        if (slices[0].length == 12u) {
            if (btstack_at_parse_address(slices[0], command->address) != 0) {
                return -1;
            }
            command->has_address = true;
            return 0;
        }
        return btstack_at_validate_single_unsigned(slices[0], 8u, command);

    case BTSTACK_AT_COMMAND_TDL:
        return (count == 1u) ?
            btstack_at_validate_single_unsigned(slices[0], 8u, command) : -1;

    case BTSTACK_AT_COMMAND_SCAN:
        if ((count < 1u) || (count > 3u) ||
            (btstack_at_parse_unsigned(slices[0], 1u, &value) != 0)) {
            return -1;
        }
        command->values[0] = value;
        command->values[1] = 10u;
        command->value_count = 2u;
        if (value == 0u) {
            return (count == 1u) ? 0 : -1;
        }
        if ((count >= 2u) &&
            (btstack_at_parse_unsigned(
                slices[1],
                48u,
                &command->values[1]) != 0 ||
             command->values[1] == 0u)) {
            return -1;
        }
        if ((count == 3u) &&
            (btstack_at_copy_text(
                slices[2],
                1u,
                25u,
                command->text,
                &command->text_length) != 0)) {
            return -1;
        }
        return 0;

    case BTSTACK_AT_COMMAND_SPKVOL:
        return (count == 1u) ?
            btstack_at_validate_single_unsigned(slices[0], 15u, command) : -1;

    case BTSTACK_AT_COMMAND_HFPCFG:
        return (count == 1u) ?
            btstack_at_validate_single_unsigned(slices[0], 7u, command) : -1;

    case BTSTACK_AT_COMMAND_HFPCONN:
    case BTSTACK_AT_COMMAND_A2DPCONN:
        if ((count != 1u) ||
            (btstack_at_parse_address(slices[0], command->address) != 0)) {
            return -1;
        }
        command->has_address = true;
        return 0;

    case BTSTACK_AT_COMMAND_HFPDIAL:
        return (count == 1u) ?
            btstack_at_copy_text(
                slices[0],
                1u,
                25u,
                command->text,
                &command->text_length) : -1;

    case BTSTACK_AT_COMMAND_HFPDTMF:
        if ((count != 1u) || (slices[0].length != 1u)) {
            return -1;
        }
        if (!(((slices[0].data[0] >= '0') &&
               (slices[0].data[0] <= '9')) ||
              (slices[0].data[0] == '#') ||
              (slices[0].data[0] == '*'))) {
            return -1;
        }
        command->text[0] = slices[0].data[0];
        command->text[1] = '\0';
        command->text_length = 1u;
        return 0;

    case BTSTACK_AT_COMMAND_HFPMCAL:
        return (count == 1u) ?
            btstack_at_validate_single_unsigned(slices[0], 2u, command) : -1;

    case BTSTACK_AT_COMMAND_HFPADTS:
        if ((count != 1u) ||
            (btstack_at_parse_unsigned(slices[0], 2u, &value) != 0) ||
            (value == 0u)) {
            return -1;
        }
        command->values[0] = value;
        command->value_count = 1u;
        return 0;

    case BTSTACK_AT_COMMAND_PBAPSYNC:
        if ((count != 2u) ||
            (btstack_at_parse_address(slices[0], command->address) != 0) ||
            (btstack_at_parse_unsigned(slices[1], 15u, &value) != 0) ||
            (value == 0u)) {
            return -1;
        }
        command->has_address = true;
        command->values[0] = value;
        command->value_count = 1u;
        return 0;

    case BTSTACK_AT_COMMAND_COVERIMAGE:
        if ((count != 4u) ||
            (btstack_at_parse_cover_encoding(slices[0],
                &command->cover_art_descriptor.encoding) != 0) ||
            (btstack_at_parse_unsigned(slices[1], 0xffffu, &value) != 0) ||
            (value == 0u)) {
            return -1;
        }
        command->cover_art_descriptor.width = (uint16_t)value;
        if ((btstack_at_parse_unsigned(slices[2], 0xffffu, &value) != 0) ||
            (value == 0u) ||
            (btstack_at_parse_cover_transformation(slices[3],
                &command->cover_art_descriptor.transformation) != 0)) {
            return -1;
        }
        command->cover_art_descriptor.height = (uint16_t)value;
        return 0;

    case BTSTACK_AT_COMMAND_PBAPAUTH:
        return (count == 1u) ?
            btstack_at_copy_text(slices[0], 0u,
                BT_XSTACK_PHONEBOOK_PASSWORD_MAX,
                command->text, &command->text_length) : -1;

    case BTSTACK_AT_COMMAND_LEADV: {
        uint16_t binary_length;
        if ((count != 1u) ||
            (btstack_at_parse_unsigned(slices[0], 1u, &value) != 0)) {
            if (count != 6u) {
                return -1;
            }
            if ((btstack_at_parse_unsigned(slices[0], 1u, &value) != 0) ||
                (value == 0u)) {
                return -1;
            }
        }
        command->values[0] = value;
        command->value_count = 1u;
        if (value == 0u) {
            return (count == 1u) ? 0 : -1;
        }
        if ((count != 6u) ||
            (btstack_at_parse_unsigned(slices[1], 0xffffu, &value) != 0)) {
            return -1;
        }
        command->le_adv.interval_min = (uint16_t)value;
        if (btstack_at_parse_unsigned(slices[2], 0xffffu, &value) != 0) {
            return -1;
        }
        command->le_adv.interval_max = (uint16_t)value;
        if ((command->le_adv.interval_min > command->le_adv.interval_max) ||
            (btstack_at_parse_unsigned(slices[3], 4u, &value) != 0)) {
            return -1;
        }
        command->le_adv.advertising_type = (uint8_t)value;
        command->le_adv.channel_map = 7u;
        if (btstack_at_parse_hex_bytes(slices[4], command->le_adv.data,
                sizeof(command->le_adv.data), &binary_length) != 0) {
            return -1;
        }
        command->le_adv.data_len = (uint8_t)binary_length;
        if (btstack_at_parse_hex_bytes(slices[5],
                command->le_adv.scan_response,
                sizeof(command->le_adv.scan_response), &binary_length) != 0) {
            return -1;
        }
        command->le_adv.scan_response_len = (uint8_t)binary_length;
        return 0;
    }

    case BTSTACK_AT_COMMAND_LESCAN:
        if ((count != 5u) ||
            (btstack_at_parse_unsigned(slices[0], 1u, &value) != 0)) {
            return -1;
        }
        command->values[0] = value;
        command->value_count = 1u;
        if ((btstack_at_parse_unsigned(slices[1], 1u, &value) != 0)) {
            return -1;
        }
        command->le_scan.active = (value != 0u);
        if (btstack_at_parse_unsigned(slices[2], 0xffffu, &value) != 0) {
            return -1;
        }
        command->le_scan.interval = (uint16_t)value;
        if (btstack_at_parse_unsigned(slices[3], 0xffffu, &value) != 0) {
            return -1;
        }
        command->le_scan.window = (uint16_t)value;
        if ((command->le_scan.window == 0u) ||
            (command->le_scan.window > command->le_scan.interval) ||
            (btstack_at_parse_unsigned(slices[4], 1u, &value) != 0)) {
            return -1;
        }
        command->le_scan.duplicate_filter = (value != 0u);
        return 0;

    case BTSTACK_AT_COMMAND_LECONN:
        if ((count != 2u) ||
            (btstack_at_parse_unsigned(slices[0], 1u, &value) != 0) ||
            (btstack_at_parse_address(slices[1], command->address) != 0)) {
            return -1;
        }
        command->values[0] = value;
        command->value_count = 1u;
        command->has_address = true;
        return 0;

    case BTSTACK_AT_COMMAND_LECFM:
        return (count == 1u) ?
            btstack_at_validate_single_unsigned(slices[0], 1u, command) : -1;

    case BTSTACK_AT_COMMAND_LEPASS:
        return (count == 1u) ?
            btstack_at_validate_single_unsigned(slices[0], 999999u, command) : -1;

    case BTSTACK_AT_COMMAND_GATTSADD:
        return ((count == 1u) &&
                (btstack_at_parse_uuid(slices[0], &command->uuid) == 0)) ? 0 : -1;

    case BTSTACK_AT_COMMAND_GATTSC:
        if ((count != 5u) ||
            (btstack_at_parse_unsigned(slices[0], 0xffffu,
                &command->values[0]) != 0) ||
            (btstack_at_parse_uuid(slices[1], &command->uuid) != 0) ||
            (btstack_at_parse_unsigned(slices[2], 0xffffu,
                &command->values[1]) != 0) ||
            (btstack_at_parse_unsigned(slices[3],
                BT_XSTACK_GATT_VALUE_MAX, &command->values[2]) != 0) ||
            (command->values[2] == 0u) ||
            (btstack_at_parse_unsigned(slices[4],
                BT_XSTACK_GATT_SECURITY_AUTHENTICATED, &value) != 0)) {
            return -1;
        }
        command->security = (uint8_t)value;
        command->value_count = 3u;
        return 0;

    case BTSTACK_AT_COMMAND_GATTSVAL:
    case BTSTACK_AT_COMMAND_GATTSNTF:
    case BTSTACK_AT_COMMAND_GATTSIND:
        if ((count != 2u) ||
            (btstack_at_parse_unsigned(slices[0], 0xffffu,
                &command->values[0]) != 0) ||
            (btstack_at_parse_hex_bytes(slices[1], command->binary,
                sizeof(command->binary), &command->binary_length) != 0)) {
            return -1;
        }
        command->value_count = 1u;
        return 0;

    case BTSTACK_AT_COMMAND_GATTCDISC:
        if (count != 1u) {
            return -1;
        }
        if ((slices[0].length == 1u) && (slices[0].data[0] == '0')) {
            command->value_count = 0u;
            return 0;
        }
        command->value_count = 1u;
        return btstack_at_parse_uuid(slices[0], &command->uuid);

    case BTSTACK_AT_COMMAND_GATTCC:
        if ((count != 3u) ||
            (btstack_at_parse_unsigned(slices[0], 0xffffu,
                &command->values[0]) != 0) ||
            (btstack_at_parse_unsigned(slices[1], 0xffffu,
                &command->values[1]) != 0) ||
            (command->values[0] > command->values[1])) {
            return -1;
        }
        if ((slices[2].length == 1u) && (slices[2].data[0] == '0')) {
            command->value_count = 2u;
            return 0;
        }
        command->value_count = 3u;
        return btstack_at_parse_uuid(slices[2], &command->uuid);

    case BTSTACK_AT_COMMAND_GATTCR:
        return (count == 1u) ?
            btstack_at_validate_single_unsigned(slices[0], 0xffffu, command) : -1;

    case BTSTACK_AT_COMMAND_GATTCW:
        if ((count != 3u) ||
            (btstack_at_parse_unsigned(slices[0], 0xffffu,
                &command->values[0]) != 0) ||
            (btstack_at_parse_unsigned(slices[1], 1u,
                &command->values[1]) != 0) ||
            (btstack_at_parse_hex_bytes(slices[2], command->binary,
                sizeof(command->binary), &command->binary_length) != 0)) {
            return -1;
        }
        command->value_count = 2u;
        return 0;

    case BTSTACK_AT_COMMAND_GATTCSUB:
        if ((count != 3u) ||
            (btstack_at_parse_unsigned(slices[0], 0xffffu,
                &command->values[0]) != 0) ||
            (btstack_at_parse_unsigned(slices[1], 0xffffu,
                &command->values[1]) != 0) ||
            (btstack_at_parse_unsigned(slices[2], 3u,
                &command->values[2]) != 0)) {
            return -1;
        }
        command->value_count = 3u;
        return 0;

    case BTSTACK_AT_COMMAND_UNSUPPORTED:
        return 0;

    default:
        return -1;
    }
}

int btstack_at_parse(
    const char *input,
    uint16_t length,
    btstack_at_command_t *command)
{
    const btstack_at_command_descriptor_t *descriptor;
    uint16_t index;
    uint16_t name_start;
    uint16_t name_length;
    uint16_t equals_index = 0xffffu;
    uint8_t explicit_query = 0u;

    if ((input == NULL) || (command == NULL) ||
        (length == 0u) || (length >= BTSTACK_AT_COMMAND_MAX)) {
        return -1;
    }

    btstack_at_clear_command(command);

    if ((length >= 2u) &&
        (input[length - 2u] == '\r') &&
        (input[length - 1u] == '\n')) {
        length -= 2u;
    }
    if (length == 0u) {
        return -1;
    }
    for (index = 0u; index < length; index++) {
        if ((input[index] == '\0') ||
            (input[index] == '\r') ||
            (input[index] == '\n')) {
            return -1;
        }
    }

    if ((length == 2u) && (input[0] == 'A') && (input[1] == 'T')) {
        command->id = BTSTACK_AT_COMMAND_AT;
        command->form = BTSTACK_AT_FORM_EXECUTE;
        return 0;
    }
    if ((length > 4u) && (input[length - 1u] == '?')) {
        explicit_query = 1u;
        length--;
    }
    if ((length < 4u) ||
        (input[0] != 'A') ||
        (input[1] != 'T') ||
        (input[2] != '+')) {
        return -1;
    }

    name_start = 3u;
    for (index = name_start; index < length; index++) {
        if (input[index] == '=') {
            equals_index = index;
            break;
        }
        if (!btstack_at_is_command_character(input[index])) {
            return -1;
        }
    }
    name_length = ((equals_index == 0xffffu) ? length : equals_index) -
                  name_start;
    if (name_length == 0u) {
        return -1;
    }

    descriptor = btstack_at_command_find(
        &input[name_start],
        name_length);
    if ((descriptor != NULL) &&
        (descriptor->id != BTSTACK_AT_COMMAND_AT)) {
        command->id = descriptor->id;
    } else {
        descriptor = NULL;
        command->id = BTSTACK_AT_COMMAND_UNSUPPORTED;
    }

    if (equals_index == 0xffffu) {
        if (descriptor == NULL) {
            command->form = BTSTACK_AT_FORM_EXECUTE;
            return 0;
        }
        command->form = explicit_query ? BTSTACK_AT_FORM_QUERY :
            (((descriptor->allowed_forms &
               BTSTACK_AT_FORM_MASK_QUERY) != 0u) ?
                BTSTACK_AT_FORM_QUERY : BTSTACK_AT_FORM_EXECUTE);
        if ((descriptor->allowed_forms &
             (1u << command->form)) == 0u) {
            return -1;
        }
        if ((command->id == BTSTACK_AT_COMMAND_PAIR) ||
            (command->id == BTSTACK_AT_COMMAND_PAGE)) {
            command->values[0] = 1u;
            command->value_count = 1u;
        }
        return 0;
    }

    command->form = BTSTACK_AT_FORM_SET;
    if (equals_index == (length - 1u)) {
        return -1;
    }
    if ((descriptor != NULL) &&
        (((descriptor->allowed_forms & BTSTACK_AT_FORM_MASK_SET) == 0u) ||
         (descriptor->validation == BTSTACK_AT_VALIDATE_NONE))) {
        return -1;
    }
    return btstack_at_validate_set(
        command,
        &input[equals_index + 1u],
        length - equals_index - 1u);
}
