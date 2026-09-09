#include "profiles/btstack_vcard_parser.h"

#include <string.h>

static int ascii_upper(int value)
{
    if (value >= 'a' && value <= 'z') {
        return value - ('a' - 'A');
    }
    return value;
}

static int span_equals(const char *text, size_t length, const char *expected)
{
    size_t i;

    if (strlen(expected) != length) {
        return 0;
    }
    for (i = 0u; i < length; ++i) {
        if (ascii_upper((unsigned char) text[i]) !=
            ascii_upper((unsigned char) expected[i])) {
            return 0;
        }
    }
    return 1;
}

static int line_equals(const btstack_vcard_parser_t *parser,
                       const char *expected)
{
    return !parser->line_overflow &&
           span_equals(parser->logical_line,
                       parser->logical_line_length, expected);
}

static void reset_record(btstack_vcard_parser_t *parser)
{
    memset(&parser->record, 0, sizeof(parser->record));
    memset(parser->n_fallback, 0, sizeof(parser->n_fallback));
    parser->in_record = 0u;
    parser->record_invalid = 0u;
    parser->have_fn = 0u;
}

static void start_record(btstack_vcard_parser_t *parser)
{
    reset_record(parser);
    parser->in_record = 1u;
}

static void invalidate_record(btstack_vcard_parser_t *parser)
{
    if (!parser->record_invalid) {
        ++parser->parser_error_count;
        parser->record_invalid = 1u;
    }
}

static int hex_value(char value)
{
    if (value >= '0' && value <= '9') {
        return value - '0';
    }
    if (value >= 'a' && value <= 'f') {
        return value - 'a' + 10;
    }
    if (value >= 'A' && value <= 'F') {
        return value - 'A' + 10;
    }
    return -1;
}

static const char *find_unquoted(const char *text, size_t length, char wanted)
{
    int in_quotes = 0;
    size_t i;

    for (i = 0u; i < length; ++i) {
        if (text[i] == '"') {
            in_quotes = !in_quotes;
        } else if (!in_quotes && text[i] == wanted) {
            return text + i;
        }
    }
    return NULL;
}

static int parameter_value_has_token(const char *value, size_t value_length,
                                     const char *token)
{
    size_t start = 0u;
    size_t end = value_length;
    size_t token_start;
    size_t i;

    while (start < end && (value[start] == ' ' || value[start] == '\t')) {
        ++start;
    }
    while (end > start && (value[end - 1u] == ' ' ||
                           value[end - 1u] == '\t')) {
        --end;
    }
    if (end - start >= 2u && value[start] == '"' && value[end - 1u] == '"') {
        ++start;
        --end;
    }
    token_start = start;
    for (i = start; i <= end; ++i) {
        if (i == end || value[i] == ',') {
            size_t token_end = i;
            while (token_start < token_end &&
                   (value[token_start] == ' ' || value[token_start] == '\t')) {
                ++token_start;
            }
            while (token_end > token_start &&
                   (value[token_end - 1u] == ' ' ||
                    value[token_end - 1u] == '\t')) {
                --token_end;
            }
            if (span_equals(value + token_start, token_end - token_start,
                            token)) {
                return 1;
            }
            token_start = i + 1u;
        }
    }
    return 0;
}

static int header_parameter_has_token(const char *header,
                                      size_t header_length,
                                      const char *parameter_name,
                                      const char *token,
                                      int allow_bare)
{
    const char *first_parameter = find_unquoted(header, header_length, ';');
    size_t position;

    if (first_parameter == NULL) {
        return 0;
    }
    position = (size_t) (first_parameter - header) + 1u;
    while (position < header_length) {
        const char *segment_end_ptr = find_unquoted(header + position,
                                                    header_length - position,
                                                    ';');
        size_t segment_end = segment_end_ptr == NULL ? header_length :
                             (size_t) (segment_end_ptr - header);
        const char *equals = find_unquoted(header + position,
                                           segment_end - position, '=');

        if (equals == NULL) {
            if (allow_bare &&
                parameter_value_has_token(header + position,
                                          segment_end - position, token)) {
                return 1;
            }
        } else {
            size_t equals_index = (size_t) (equals - header);
            size_t name_start = position;
            size_t name_end = equals_index;
            while (name_start < name_end &&
                   (header[name_start] == ' ' || header[name_start] == '\t')) {
                ++name_start;
            }
            while (name_end > name_start &&
                   (header[name_end - 1u] == ' ' ||
                    header[name_end - 1u] == '\t')) {
                --name_end;
            }
            if (span_equals(header + name_start, name_end - name_start,
                            parameter_name) &&
                parameter_value_has_token(header + equals_index + 1u,
                                          segment_end - equals_index - 1u,
                                          token)) {
                return 1;
            }
        }
        position = segment_end + 1u;
    }
    return 0;
}

static int is_quoted_printable_line(const btstack_vcard_parser_t *parser)
{
    const char *colon = find_unquoted(parser->logical_line,
                                      parser->logical_line_length, ':');
    size_t header_length;

    if (colon == NULL) {
        return 0;
    }
    header_length = (size_t) (colon - parser->logical_line);
    return header_parameter_has_token(parser->logical_line, header_length,
                                      "ENCODING", "QUOTED-PRINTABLE", 0);
}

static size_t utf8_valid_prefix(const char *text, size_t length)
{
    const unsigned char *bytes = (const unsigned char *) text;
    size_t index = 0u;

    while (index < length) {
        unsigned char first = bytes[index];
        size_t continuation_count;
        size_t i;

        if (first <= 0x7fu) {
            ++index;
            continue;
        }
        if (first >= 0xc2u && first <= 0xdfu) {
            continuation_count = 1u;
        } else if (first >= 0xe0u && first <= 0xefu) {
            continuation_count = 2u;
        } else if (first >= 0xf0u && first <= 0xf4u) {
            continuation_count = 3u;
        } else {
            break;
        }
        if (index + continuation_count >= length) {
            break;
        }
        if ((first == 0xe0u && bytes[index + 1u] < 0xa0u) ||
            (first == 0xedu && bytes[index + 1u] > 0x9fu) ||
            (first == 0xf0u && bytes[index + 1u] < 0x90u) ||
            (first == 0xf4u && bytes[index + 1u] > 0x8fu)) {
            break;
        }
        for (i = 1u; i <= continuation_count; ++i) {
            if (bytes[index + i] < 0x80u || bytes[index + i] > 0xbfu) {
                return index;
            }
        }
        index += continuation_count + 1u;
    }
    return index;
}

static void append_line_byte(btstack_vcard_parser_t *parser, uint8_t value)
{
    if (parser->logical_line_length < BTSTACK_VCARD_LOGICAL_LINE_MAX) {
        parser->logical_line[parser->logical_line_length++] = (char) value;
        parser->logical_line[parser->logical_line_length] = '\0';
    } else {
        parser->line_overflow = 1u;
    }
}

static void note_physical_byte(btstack_vcard_parser_t *parser, uint8_t value)
{
    parser->physical_last_byte = value;
    parser->physical_has_byte = 1u;
}

static void clear_line(btstack_vcard_parser_t *parser)
{
    parser->logical_line_length = 0u;
    parser->logical_line[0] = '\0';
    parser->line_pending = 0u;
    parser->line_overflow = 0u;
    parser->pending_qp_soft_break = 0u;
}

static size_t decode_value(const char *source, size_t source_length,
                           int quoted_printable, int source_truncated,
                           char *destination,
                           size_t destination_max, uint8_t *truncated,
                           int *valid)
{
    size_t source_index = 0u;
    size_t destination_length = 0u;

    *valid = 1;
    while (source_index < source_length) {
        unsigned char value;

        if (quoted_printable && source[source_index] == '=') {
            int high;
            int low;
            if (source_index + 2u >= source_length) {
                if (source_truncated) {
                    *truncated = 1u;
                } else {
                    *valid = 0;
                }
                break;
            }
            high = hex_value(source[source_index + 1u]);
            low = hex_value(source[source_index + 2u]);
            if (high < 0 || low < 0) {
                *valid = 0;
                break;
            }
            value = (unsigned char) ((high << 4) | low);
            source_index += 3u;
        } else if (!quoted_printable && source[source_index] == '\\' &&
                   source_index + 1u < source_length) {
            char escaped = source[source_index + 1u];
            if (escaped == 'n' || escaped == 'N') {
                value = (unsigned char) '\n';
            } else {
                value = (unsigned char) escaped;
            }
            source_index += 2u;
        } else {
            value = (unsigned char) source[source_index++];
        }

        if (destination_length < destination_max) {
            destination[destination_length++] = (char) value;
        } else {
            *truncated = 1u;
        }
    }
    {
        size_t valid_prefix = utf8_valid_prefix(destination,
                                                destination_length);
        if (valid_prefix != destination_length) {
            destination_length = valid_prefix;
            *truncated = 1u;
        }
    }
    destination[destination_length] = '\0';
    return destination_length;
}

static void set_n_fallback(btstack_vcard_parser_t *parser,
                           const char *source, size_t source_length,
                           int quoted_printable, int source_truncated)
{
    char family[BTSTACK_VCARD_NAME_MAX + 1u];
    char given[BTSTACK_VCARD_NAME_MAX + 1u];
    size_t first_separator = source_length;
    size_t second_separator = source_length;
    size_t family_length;
    size_t given_length;
    size_t output_length = 0u;
    size_t index = 0u;
    int valid = 1;

    while (index < source_length) {
        if (!quoted_printable && source[index] == '\\' &&
            index + 1u < source_length) {
            index += 2u;
            continue;
        }
        if (source[index] == ';') {
            first_separator = index;
            break;
        }
        ++index;
    }
    index = first_separator < source_length ? first_separator + 1u :
                                              source_length;
    while (index < source_length) {
        if (!quoted_printable && source[index] == '\\' &&
            index + 1u < source_length) {
            index += 2u;
            continue;
        }
        if (source[index] == ';') {
            second_separator = index;
            break;
        }
        ++index;
    }

    family_length = decode_value(source, first_separator, quoted_printable,
                                 source_truncated &&
                                 first_separator == source_length,
                                 family, BTSTACK_VCARD_NAME_MAX,
                                 &parser->record.truncated, &valid);
    if (!valid) {
        invalidate_record(parser);
        return;
    }
    given_length = decode_value(source +
                                (first_separator < source_length ?
                                 first_separator + 1u : source_length),
                                second_separator -
                                (first_separator < source_length ?
                                 first_separator + 1u : source_length),
                                quoted_printable,
                                source_truncated &&
                                second_separator == source_length,
                                given,
                                BTSTACK_VCARD_NAME_MAX,
                                &parser->record.truncated, &valid);
    if (!valid) {
        invalidate_record(parser);
        return;
    }

    if (given_length != 0u) {
        size_t copy_length = given_length;
        if (copy_length > BTSTACK_VCARD_NAME_MAX) {
            copy_length = BTSTACK_VCARD_NAME_MAX;
            parser->record.truncated = 1u;
        }
        memcpy(parser->n_fallback, given, copy_length);
        output_length = copy_length;
    }
    if (family_length != 0u) {
        size_t available;
        size_t copy_length;
        if (output_length != 0u && output_length < BTSTACK_VCARD_NAME_MAX) {
            parser->n_fallback[output_length++] = ' ';
        } else if (output_length != 0u) {
            parser->record.truncated = 1u;
        }
        available = BTSTACK_VCARD_NAME_MAX - output_length;
        copy_length = family_length < available ? family_length : available;
        copy_length = utf8_valid_prefix(family, copy_length);
        memcpy(parser->n_fallback + output_length, family, copy_length);
        output_length += copy_length;
        if (copy_length != family_length) {
            parser->record.truncated = 1u;
        }
    }
    parser->n_fallback[output_length] = '\0';
}

static uint8_t telephone_flags(const char *header, size_t header_length)
{
    uint8_t flags = 0u;

    if (header_parameter_has_token(header, header_length, "TYPE", "HOME", 1)) {
        flags |= BTSTACK_VCARD_TEL_HOME;
    }
    if (header_parameter_has_token(header, header_length, "TYPE", "WORK", 1)) {
        flags |= BTSTACK_VCARD_TEL_WORK;
    }
    if (header_parameter_has_token(header, header_length, "TYPE", "CELL", 1)) {
        flags |= BTSTACK_VCARD_TEL_CELL;
    }
    if (header_parameter_has_token(header, header_length, "TYPE", "FAX", 1)) {
        flags |= BTSTACK_VCARD_TEL_FAX;
    }
    if (header_parameter_has_token(header, header_length, "TYPE", "VOICE", 1)) {
        flags |= BTSTACK_VCARD_TEL_VOICE;
    }
    if (header_parameter_has_token(header, header_length, "TYPE", "OTHER", 1) ||
        flags == 0u) {
        flags |= BTSTACK_VCARD_TEL_OTHER;
    }
    return flags;
}

static btstack_vcard_direction_t call_direction(const char *header,
                                                size_t header_length)
{
    if (header_parameter_has_token(header, header_length, "TYPE", "RECEIVED", 1)) {
        return BTSTACK_VCARD_DIRECTION_INCOMING;
    }
    if (header_parameter_has_token(header, header_length, "TYPE", "DIALED", 1)) {
        return BTSTACK_VCARD_DIRECTION_OUTGOING;
    }
    if (header_parameter_has_token(header, header_length, "TYPE", "MISSED", 1)) {
        return BTSTACK_VCARD_DIRECTION_MISSED;
    }
    return BTSTACK_VCARD_DIRECTION_NONE;
}

static void clear_line(btstack_vcard_parser_t *parser);

static btstack_vcard_parser_result_t handle_end(btstack_vcard_parser_t *parser)
{
    btstack_vcard_record_t completed_record;
    uint8_t accepted = 1u;

    if (!parser->in_record) {
        ++parser->parser_error_count;
        return BTSTACK_VCARD_PARSER_OK;
    }
    if (!parser->record_invalid) {
        if (!parser->have_fn && parser->n_fallback[0] != '\0') {
            memcpy(parser->record.name, parser->n_fallback,
                   sizeof(parser->record.name));
        }
        completed_record = parser->record;
        reset_record(parser);
        clear_line(parser);
        if (parser->record_callback != NULL) {
            parser->in_callback = 1u;
            accepted = parser->record_callback(parser->callback_context,
                                               &completed_record);
            parser->in_callback = 0u;
        }
    } else {
        reset_record(parser);
    }
    return accepted ? BTSTACK_VCARD_PARSER_OK :
                      BTSTACK_VCARD_PARSER_PAUSED;
}

static int property_is(const char *line, size_t property_length,
                       const char *property)
{
    return span_equals(line, property_length, property);
}

static btstack_vcard_parser_result_t handle_property(
    btstack_vcard_parser_t *parser)
{
    const char *line = parser->logical_line;
    size_t line_length = parser->logical_line_length;
    const char *colon = find_unquoted(line, line_length, ':');
    const char *semicolon;
    size_t header_length;
    size_t property_length;
    const char *property;
    const char *value;
    size_t value_length;
    int quoted_printable;
    int valid = 1;

    if (colon == NULL) {
        invalidate_record(parser);
        return BTSTACK_VCARD_PARSER_OK;
    }
    header_length = (size_t) (colon - line);
    semicolon = (const char *) memchr(line, ';', header_length);
    property_length = semicolon == NULL ? header_length :
                      (size_t) (semicolon - line);
    property = line;
    {
        const char *group_separator = NULL;
        size_t i;
        for (i = 0u; i < property_length; ++i) {
            if (line[i] == '.') {
                group_separator = line + i;
            }
        }
        if (group_separator != NULL) {
            property = group_separator + 1;
            property_length -= (size_t) (property - line);
        }
    }
    value = colon + 1;
    value_length = line_length - header_length - 1u;
    quoted_printable = header_parameter_has_token(line, header_length,
                                                  "ENCODING",
                                                  "QUOTED-PRINTABLE", 0);

    if (property_is(property, property_length, "FN")) {
        parser->record.name[0] = '\0';
        (void) decode_value(value, value_length, quoted_printable,
                            parser->line_overflow,
                            parser->record.name, BTSTACK_VCARD_NAME_MAX,
                            &parser->record.truncated, &valid);
        if (parser->line_overflow) {
            parser->record.truncated = 1u;
        }
        if (!valid) {
            invalidate_record(parser);
        } else {
            parser->have_fn = 1u;
        }
    } else if (property_is(property, property_length, "N")) {
        set_n_fallback(parser, value, value_length, quoted_printable,
                       parser->line_overflow);
        if (parser->line_overflow) {
            parser->record.truncated = 1u;
        }
    } else if (property_is(property, property_length, "TEL")) {
        if (parser->record.number_count >= BTSTACK_VCARD_NUMBER_COUNT) {
            parser->record.truncated = 1u;
        } else {
            btstack_vcard_number_t *number =
                &parser->record.numbers[parser->record.number_count];
            number->type_flags = telephone_flags(line, header_length);
            (void) decode_value(value, value_length, quoted_printable,
                                parser->line_overflow,
                                number->value, BTSTACK_VCARD_NUMBER_MAX,
                                &parser->record.truncated, &valid);
            if (!valid) {
                invalidate_record(parser);
            } else {
                ++parser->record.number_count;
            }
        }
        if (parser->line_overflow) {
            parser->record.truncated = 1u;
        }
    } else if (property_is(property, property_length,
                           "X-IRMC-CALL-DATETIME")) {
        parser->record.direction = call_direction(line, header_length);
        (void) decode_value(value, value_length, quoted_printable,
                            parser->line_overflow,
                            parser->record.call_timestamp,
                            BTSTACK_VCARD_TIMESTAMP_MAX,
                            &parser->record.truncated, &valid);
        if (parser->line_overflow) {
            parser->record.truncated = 1u;
        }
        if (!valid) {
            invalidate_record(parser);
        }
    }
    return BTSTACK_VCARD_PARSER_OK;
}

static btstack_vcard_parser_result_t handle_line(
    btstack_vcard_parser_t *parser)
{
    if (parser->logical_line_length == 0u && !parser->line_overflow) {
        return BTSTACK_VCARD_PARSER_OK;
    }
    if (line_equals(parser, "BEGIN:VCARD")) {
        if (parser->in_record) {
            invalidate_record(parser);
        }
        start_record(parser);
        return BTSTACK_VCARD_PARSER_OK;
    }
    if (line_equals(parser, "END:VCARD")) {
        return handle_end(parser);
    }
    if (!parser->in_record || parser->record_invalid) {
        return BTSTACK_VCARD_PARSER_OK;
    }
    return handle_property(parser);
}

static btstack_vcard_parser_result_t flush_line(
    btstack_vcard_parser_t *parser)
{
    btstack_vcard_parser_result_t result = handle_line(parser);

    clear_line(parser);
    return result;
}

static btstack_vcard_parser_result_t end_physical_line(
    btstack_vcard_parser_t *parser)
{
    parser->pending_qp_soft_break =
        parser->physical_has_byte && parser->physical_last_byte == '=' &&
        is_quoted_printable_line(parser);
    parser->physical_has_byte = 0u;
    parser->line_pending = 1u;
    if (line_equals(parser, "BEGIN:VCARD") ||
        line_equals(parser, "END:VCARD")) {
        return flush_line(parser);
    }
    return BTSTACK_VCARD_PARSER_OK;
}

void btstack_vcard_parser_init(
    btstack_vcard_parser_t *parser,
    btstack_vcard_record_callback_t record_callback,
    void *context)
{
    if (parser == NULL) {
        return;
    }
    memset(parser, 0, sizeof(*parser));
    parser->record_callback = record_callback;
    parser->callback_context = context;
}

btstack_vcard_parser_result_t btstack_vcard_parser_process(
    btstack_vcard_parser_t *parser,
    const uint8_t *bytes,
    size_t length,
    size_t *consumed_out)
{
    size_t index = 0u;

    if (consumed_out != NULL) {
        *consumed_out = 0u;
    }
    if (parser == NULL || consumed_out == NULL || parser->in_callback ||
        (bytes == NULL && length != 0u)) {
        return BTSTACK_VCARD_PARSER_ERROR;
    }

    while (index < length) {
        uint8_t value = bytes[index];
        btstack_vcard_parser_result_t result;

        if (parser->saw_cr) {
            parser->saw_cr = 0u;
            if (value == '\n') {
                ++index;
                result = end_physical_line(parser);
                if (result != BTSTACK_VCARD_PARSER_OK) {
                    *consumed_out = index;
                    return result;
                }
                continue;
            }
            result = end_physical_line(parser);
            if (result != BTSTACK_VCARD_PARSER_OK) {
                *consumed_out = index;
                return result;
            }
        }

        if (parser->line_pending) {
            int quoted_printable_continuation =
                parser->pending_qp_soft_break;
            if (quoted_printable_continuation) {
                if (parser->logical_line_length != 0u &&
                    parser->logical_line[parser->logical_line_length - 1u] == '=') {
                    --parser->logical_line_length;
                    parser->logical_line[parser->logical_line_length] = '\0';
                }
                parser->line_pending = 0u;
                parser->pending_qp_soft_break = 0u;
                if (value == ' ' || value == '\t') {
                    note_physical_byte(parser, value);
                    ++index;
                    continue;
                }
            } else if (value == ' ' || value == '\t') {
                parser->line_pending = 0u;
                parser->pending_qp_soft_break = 0u;
                note_physical_byte(parser, value);
                ++index;
                continue;
            } else {
                result = flush_line(parser);
                if (result != BTSTACK_VCARD_PARSER_OK) {
                    *consumed_out = index;
                    return result;
                }
            }
        }

        if (value == '\r') {
            parser->saw_cr = 1u;
            ++index;
        } else if (value == '\n') {
            ++index;
            result = end_physical_line(parser);
            if (result != BTSTACK_VCARD_PARSER_OK) {
                *consumed_out = index;
                return result;
            }
        } else {
            note_physical_byte(parser, value);
            append_line_byte(parser, value);
            ++index;
        }
    }
    *consumed_out = index;
    return BTSTACK_VCARD_PARSER_OK;
}

btstack_vcard_parser_result_t btstack_vcard_parser_finish(
    btstack_vcard_parser_t *parser)
{
    btstack_vcard_parser_result_t result;

    if (parser == NULL || parser->in_callback) {
        return BTSTACK_VCARD_PARSER_ERROR;
    }
    if (parser->saw_cr) {
        parser->saw_cr = 0u;
        result = end_physical_line(parser);
        if (result != BTSTACK_VCARD_PARSER_OK) {
            return result;
        }
    } else if (!parser->line_pending &&
               (parser->logical_line_length != 0u || parser->line_overflow)) {
        parser->line_pending = 1u;
    }
    if (parser->line_pending) {
        result = flush_line(parser);
        if (result != BTSTACK_VCARD_PARSER_OK) {
            return result;
        }
    }
    if (parser->in_record) {
        if (!parser->record_invalid) {
            ++parser->parser_error_count;
        }
        reset_record(parser);
        return BTSTACK_VCARD_PARSER_ERROR;
    }
    return BTSTACK_VCARD_PARSER_OK;
}

uint32_t btstack_vcard_parser_error_count(
    const btstack_vcard_parser_t *parser)
{
    return parser == NULL ? 0u : parser->parser_error_count;
}
