#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "profiles/btstack_vcard_parser.h"

#define ARRAY_COUNT(a) (sizeof(a) / sizeof((a)[0]))
#define CHECK(condition)                                                        \
    do {                                                                        \
        if (!(condition)) {                                                     \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__,           \
                    #condition);                                                \
            return 0;                                                           \
        }                                                                       \
    } while (0)

typedef struct {
    btstack_vcard_record_t records[8];
    size_t count;
    size_t reject_count;
} record_sink_t;

static uint8_t collect_record(void *context, const btstack_vcard_record_t *record)
{
    record_sink_t *sink = (record_sink_t *) context;

    if (sink->reject_count != 0u) {
        --sink->reject_count;
        return 0u;
    }
    if (sink->count >= ARRAY_COUNT(sink->records)) {
        return 0u;
    }
    sink->records[sink->count++] = *record;
    return 1u;
}

typedef struct {
    btstack_vcard_parser_t *parser;
    btstack_vcard_record_t records[4];
    size_t count;
    uint8_t attempted_reentry;
    btstack_vcard_parser_result_t reentry_result;
    size_t reentry_consumed;
} reentrant_sink_t;

static uint8_t collect_with_reentry(void *context,
                                    const btstack_vcard_record_t *record)
{
    static const char injected[] =
        "BEGIN:VCARD\r\nFN:Injected\r\nTEL:9\r\nEND:VCARD\r\n";
    reentrant_sink_t *sink = (reentrant_sink_t *) context;

    if (sink->count < ARRAY_COUNT(sink->records)) {
        sink->records[sink->count++] = *record;
    }
    if (!sink->attempted_reentry) {
        sink->attempted_reentry = 1u;
        sink->reentry_result = btstack_vcard_parser_process(
            sink->parser, (const uint8_t *) injected, strlen(injected),
            &sink->reentry_consumed);
    }
    return 1u;
}

static int feed_all(btstack_vcard_parser_t *parser, const char *text)
{
    size_t consumed = 0u;
    btstack_vcard_parser_result_t result = btstack_vcard_parser_process(
        parser, (const uint8_t *) text, strlen(text), &consumed);

    return result == BTSTACK_VCARD_PARSER_OK && consumed == strlen(text);
}

static int is_valid_utf8(const char *text)
{
    const unsigned char *bytes = (const unsigned char *) text;
    size_t index = 0u;
    size_t length = strlen(text);

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
            return 0;
        }
        if (index + continuation_count >= length) {
            return 0;
        }
        if (first == 0xe0u && bytes[index + 1u] < 0xa0u) {
            return 0;
        }
        if (first == 0xedu && bytes[index + 1u] > 0x9fu) {
            return 0;
        }
        if (first == 0xf0u && bytes[index + 1u] < 0x90u) {
            return 0;
        }
        if (first == 0xf4u && bytes[index + 1u] > 0x8fu) {
            return 0;
        }
        for (i = 1u; i <= continuation_count; ++i) {
            if (bytes[index + i] < 0x80u || bytes[index + i] > 0xbfu) {
                return 0;
            }
        }
        index += continuation_count + 1u;
    }
    return 1;
}

static int test_versions_name_precedence_and_fallback(void)
{
    static const char input[] =
        "BEGIN:VCARD\r\nVERSION:2.1\r\nN:Doe;Jane;;;\r\nFN:Jane Doe\r\n"
        "TEL;HOME:111\r\nEND:VCARD\r\n"
        "BEGIN:VCARD\r\nVERSION:3.0\r\nN:Smith;John;;;\r\n"
        "TEL;TYPE=WORK:222\r\nEND:VCARD\r\n";
    btstack_vcard_parser_t parser;
    record_sink_t sink = {0};

    btstack_vcard_parser_init(&parser, collect_record, &sink);
    CHECK(feed_all(&parser, input));
    CHECK(btstack_vcard_parser_finish(&parser) == BTSTACK_VCARD_PARSER_OK);
    CHECK(sink.count == 2u);
    CHECK(strcmp(sink.records[0].name, "Jane Doe") == 0);
    CHECK(strcmp(sink.records[1].name, "John Smith") == 0);
    CHECK(sink.records[0].numbers[0].type_flags == BTSTACK_VCARD_TEL_HOME);
    CHECK(sink.records[1].numbers[0].type_flags == BTSTACK_VCARD_TEL_WORK);
    return 1;
}

static int parse_with_split(const char *input, size_t split,
                            record_sink_t *sink, uint32_t *errors)
{
    btstack_vcard_parser_t parser;
    size_t consumed = 0u;
    size_t length = strlen(input);

    btstack_vcard_parser_init(&parser, collect_record, sink);
    CHECK(btstack_vcard_parser_process(&parser, (const uint8_t *) input, split,
                                       &consumed) == BTSTACK_VCARD_PARSER_OK);
    CHECK(consumed == split);
    CHECK(btstack_vcard_parser_process(&parser,
                                       (const uint8_t *) input + split,
                                       length - split, &consumed) ==
          BTSTACK_VCARD_PARSER_OK);
    CHECK(consumed == length - split);
    CHECK(btstack_vcard_parser_finish(&parser) == BTSTACK_VCARD_PARSER_OK);
    *errors = btstack_vcard_parser_error_count(&parser);
    return 1;
}

static int test_every_two_chunk_split_matches_baseline(void)
{
    static const char input[] =
        "BEGIN:VCARD\r\nVERSION:3.0\r\nFN:Split Test\r\n"
        "TEL;TYPE=CELL,VOICE:12345\r\nEND:VCARD\r\n";
    record_sink_t baseline = {0};
    uint32_t baseline_errors = 0u;
    size_t split;

    CHECK(parse_with_split(input, strlen(input), &baseline, &baseline_errors));
    CHECK(baseline.count == 1u);
    for (split = 0u; split <= strlen(input); ++split) {
        record_sink_t actual = {0};
        uint32_t actual_errors = 0u;
        CHECK(parse_with_split(input, split, &actual, &actual_errors));
        CHECK(actual.count == baseline.count);
        CHECK(actual_errors == baseline_errors);
        CHECK(memcmp(&actual.records[0], &baseline.records[0],
                     sizeof(actual.records[0])) == 0);
    }
    return 1;
}

static int test_complex_qp_fold_utf8_every_two_chunk_split(void)
{
    static const char input[] =
        "BEGIN:VCARD\r\nVERSION:2.1\r\n"
        "FN;ENCODING=QUOTED-PRINTABLE:=E5=BC=A0=\r\n=E4=B8=89\r\n"
        "TEL;TYPE=HOME,\r\n VOICE:12345\r\nEND:VCARD\r\n";
    record_sink_t baseline = {0};
    uint32_t baseline_errors = 0u;
    size_t split;

    CHECK(parse_with_split(input, strlen(input), &baseline, &baseline_errors));
    CHECK(baseline.count == 1u);
    CHECK(strcmp(baseline.records[0].name,
                 "\xE5\xBC\xA0\xE4\xB8\x89") == 0);
    for (split = 0u; split <= strlen(input); ++split) {
        record_sink_t actual = {0};
        uint32_t actual_errors = 0u;
        CHECK(parse_with_split(input, split, &actual, &actual_errors));
        CHECK(actual.count == 1u);
        CHECK(actual_errors == baseline_errors);
        CHECK(memcmp(&actual.records[0], &baseline.records[0],
                     sizeof(actual.records[0])) == 0);
    }
    return 1;
}

static int test_crlf_folding_and_utf8(void)
{
    static const char part1[] =
        "BEGIN:VCARD\r\nVERSION:3.0\r\nFN:\xE5\xBC\xA0";
    static const char part2[] =
        "\xE4\xB8\x89 and\r";
    static const char part3[] =
        "\n  Friends\r\n\tClub\r\nTEL:9\r\nEND:VCARD\r\n";
    btstack_vcard_parser_t parser;
    record_sink_t sink = {0};
    size_t consumed = 0u;

    btstack_vcard_parser_init(&parser, collect_record, &sink);
    CHECK(btstack_vcard_parser_process(&parser, (const uint8_t *) part1,
                                       strlen(part1), &consumed) ==
          BTSTACK_VCARD_PARSER_OK);
    CHECK(btstack_vcard_parser_process(&parser, (const uint8_t *) part2,
                                       strlen(part2), &consumed) ==
          BTSTACK_VCARD_PARSER_OK);
    CHECK(btstack_vcard_parser_process(&parser, (const uint8_t *) part3,
                                       strlen(part3), &consumed) ==
          BTSTACK_VCARD_PARSER_OK);
    CHECK(btstack_vcard_parser_finish(&parser) == BTSTACK_VCARD_PARSER_OK);
    CHECK(sink.count == 1u);
    CHECK(strcmp(sink.records[0].name,
                 "\xE5\xBC\xA0\xE4\xB8\x89 and FriendsClub") == 0);
    return 1;
}

static int test_backslash_escaped_values(void)
{
    static const char input[] =
        "BEGIN:VCARD\r\nVERSION:3.0\r\n"
        "FN:Line\\nComma\\, Semi\\; Slash\\\\Done\r\n"
        "TEL:12\\,34\r\nEND:VCARD\r\n"
        "BEGIN:VCARD\r\nVERSION:3.0\r\n"
        "N:Fam\\;ily;Gi\\,ven;;;\r\nEND:VCARD\r\n";
    btstack_vcard_parser_t parser;
    record_sink_t sink = {0};

    btstack_vcard_parser_init(&parser, collect_record, &sink);
    CHECK(feed_all(&parser, input));
    CHECK(btstack_vcard_parser_finish(&parser) == BTSTACK_VCARD_PARSER_OK);
    CHECK(strcmp(sink.records[0].name,
                 "Line\nComma, Semi; Slash\\Done") == 0);
    CHECK(strcmp(sink.records[0].numbers[0].value, "12,34") == 0);
    CHECK(sink.count == 2u);
    CHECK(strcmp(sink.records[1].name, "Gi,ven Fam;ily") == 0);
    return 1;
}

static int test_quoted_printable_hex_and_soft_break(void)
{
    static const char input[] =
        "BEGIN:VCARD\r\nVERSION:2.1\r\n"
        "FN;ENCODING=QUOTED-PRINTABLE:=E5=BC=A0=\r\n=E4=B8=89\r\n"
        "TEL;HOME:100\r\nEND:VCARD\r\n";
    btstack_vcard_parser_t parser;
    record_sink_t sink = {0};

    btstack_vcard_parser_init(&parser, collect_record, &sink);
    CHECK(feed_all(&parser, input));
    CHECK(btstack_vcard_parser_finish(&parser) == BTSTACK_VCARD_PARSER_OK);
    CHECK(sink.count == 1u);
    CHECK(strcmp(sink.records[0].name, "\xE5\xBC\xA0\xE4\xB8\x89") == 0);
    return 1;
}

static int test_numbers_types_and_truncation(void)
{
    static const char input[] =
        "BEGIN:VCARD\r\nVERSION:3.0\r\n"
        "FN:This name is deliberately made much longer than forty eight bytes total\r\n"
        "TEL;TYPE=HOME,VOICE:111\r\nTEL;TYPE=WORK,FAX:222\r\n"
        "TEL;CELL:333\r\nTEL;TYPE=OTHER:444\r\nTEL:555\r\n"
        "END:VCARD\r\n";
    btstack_vcard_parser_t parser;
    record_sink_t sink = {0};

    btstack_vcard_parser_init(&parser, collect_record, &sink);
    CHECK(feed_all(&parser, input));
    CHECK(btstack_vcard_parser_finish(&parser) == BTSTACK_VCARD_PARSER_OK);
    CHECK(sink.count == 1u);
    CHECK(sink.records[0].number_count == BTSTACK_VCARD_NUMBER_COUNT);
    CHECK((sink.records[0].numbers[0].type_flags &
           (BTSTACK_VCARD_TEL_HOME | BTSTACK_VCARD_TEL_VOICE)) ==
          (BTSTACK_VCARD_TEL_HOME | BTSTACK_VCARD_TEL_VOICE));
    CHECK((sink.records[0].numbers[1].type_flags &
           (BTSTACK_VCARD_TEL_WORK | BTSTACK_VCARD_TEL_FAX)) ==
          (BTSTACK_VCARD_TEL_WORK | BTSTACK_VCARD_TEL_FAX));
    CHECK(sink.records[0].numbers[2].type_flags == BTSTACK_VCARD_TEL_CELL);
    CHECK(sink.records[0].numbers[3].type_flags == BTSTACK_VCARD_TEL_OTHER);
    CHECK(strlen(sink.records[0].name) == BTSTACK_VCARD_NAME_MAX);
    CHECK(sink.records[0].truncated != 0u);
    return 1;
}

static int test_call_datetime_directions(void)
{
    static const char input[] =
        "BEGIN:VCARD\r\nFN:In\r\nX-IRMC-CALL-DATETIME;RECEIVED:20260811T010203\r\nEND:VCARD\r\n"
        "BEGIN:VCARD\r\nFN:Out\r\nX-IRMC-CALL-DATETIME;DIALED:20260811T020304\r\nEND:VCARD\r\n"
        "BEGIN:VCARD\r\nFN:Miss\r\nX-IRMC-CALL-DATETIME;MISSED:20260811T030405\r\nEND:VCARD\r\n";
    btstack_vcard_parser_t parser;
    record_sink_t sink = {0};

    btstack_vcard_parser_init(&parser, collect_record, &sink);
    CHECK(feed_all(&parser, input));
    CHECK(btstack_vcard_parser_finish(&parser) == BTSTACK_VCARD_PARSER_OK);
    CHECK(sink.count == 3u);
    CHECK(sink.records[0].direction == BTSTACK_VCARD_DIRECTION_INCOMING);
    CHECK(sink.records[1].direction == BTSTACK_VCARD_DIRECTION_OUTGOING);
    CHECK(sink.records[2].direction == BTSTACK_VCARD_DIRECTION_MISSED);
    CHECK(strcmp(sink.records[2].call_timestamp, "20260811T030405") == 0);
    return 1;
}

static int test_long_photo_is_ignored(void)
{
    btstack_vcard_parser_t parser;
    record_sink_t sink = {0};
    size_t consumed = 0u;
    char photo_chunk[257];
    size_t i;

    memset(photo_chunk, 'A', sizeof(photo_chunk));
    btstack_vcard_parser_init(&parser, collect_record, &sink);
    CHECK(feed_all(&parser, "BEGIN:VCARD\r\nFN:Photo User\r\nPHOTO;ENCODING=b:"));
    for (i = 0u; i < 40u; ++i) {
        CHECK(btstack_vcard_parser_process(&parser,
                                           (const uint8_t *) photo_chunk,
                                           sizeof(photo_chunk), &consumed) ==
              BTSTACK_VCARD_PARSER_OK);
        CHECK(consumed == sizeof(photo_chunk));
    }
    CHECK(feed_all(&parser, "\r\nTEL:7\r\nEND:VCARD\r\n"));
    CHECK(btstack_vcard_parser_finish(&parser) == BTSTACK_VCARD_PARSER_OK);
    CHECK(sink.count == 1u);
    CHECK(strcmp(sink.records[0].name, "Photo User") == 0);
    CHECK(sink.records[0].truncated == 0u);
    return 1;
}

static int test_qp_photo_soft_break_after_buffer_overflow_is_ignored(void)
{
    static const char prefix[] =
        "BEGIN:VCARD\r\nFN:QP Photo\r\n"
        "PHOTO;ENCODING=QUOTED-PRINTABLE:";
    static const char suffix[] =
        "=\r\nB\r\nTEL:70\r\nEND:VCARD\r\n";
    char photo_data[300];
    btstack_vcard_parser_t parser;
    record_sink_t sink = {0};
    size_t consumed = 0u;

    memset(photo_data, 'A', sizeof(photo_data));
    btstack_vcard_parser_init(&parser, collect_record, &sink);
    CHECK(feed_all(&parser, prefix));
    CHECK(btstack_vcard_parser_process(&parser,
                                       (const uint8_t *) photo_data,
                                       sizeof(photo_data), &consumed) ==
          BTSTACK_VCARD_PARSER_OK);
    CHECK(consumed == sizeof(photo_data));
    CHECK(feed_all(&parser, suffix));
    CHECK(btstack_vcard_parser_finish(&parser) == BTSTACK_VCARD_PARSER_OK);
    CHECK(sink.count == 1u);
    CHECK(strcmp(sink.records[0].name, "QP Photo") == 0);
    CHECK(strcmp(sink.records[0].numbers[0].value, "70") == 0);
    CHECK(sink.records[0].truncated == 0u);
    return 1;
}

static int test_utf8_truncation_keeps_only_valid_codepoints(void)
{
    char input[320];
    char boundary_name[64];
    char invalid_name[] = {'G', 'o', 'o', 'd', (char) 0xe5, 'X', 'B', 'a',
                           'd', '\0'};
    btstack_vcard_parser_t parser;
    record_sink_t sink = {0};

    memset(boundary_name, 'A', 47u);
    boundary_name[47] = (char) 0xe5;
    boundary_name[48] = (char) 0xbc;
    boundary_name[49] = (char) 0xa0;
    boundary_name[50] = 'Z';
    boundary_name[51] = '\0';
    (void) snprintf(input, sizeof(input),
                    "BEGIN:VCARD\r\nFN:%s\r\nEND:VCARD\r\n"
                    "BEGIN:VCARD\r\nFN:%s\r\nEND:VCARD\r\n",
                    boundary_name, invalid_name);

    btstack_vcard_parser_init(&parser, collect_record, &sink);
    CHECK(feed_all(&parser, input));
    CHECK(btstack_vcard_parser_finish(&parser) == BTSTACK_VCARD_PARSER_OK);
    CHECK(sink.count == 2u);
    CHECK(sink.records[0].truncated != 0u);
    CHECK(strlen(sink.records[0].name) == 47u);
    CHECK(is_valid_utf8(sink.records[0].name));
    CHECK(sink.records[1].truncated != 0u);
    CHECK(strcmp(sink.records[1].name, "Good") == 0);
    CHECK(is_valid_utf8(sink.records[1].name));
    return 1;
}

static int test_n_fallback_composition_does_not_split_utf8(void)
{
    char input[192];
    char given[47];
    btstack_vcard_parser_t parser;
    record_sink_t sink = {0};

    memset(given, 'A', 46u);
    given[46] = '\0';
    (void) snprintf(input, sizeof(input),
                    "BEGIN:VCARD\r\nN:\xE4\xB8\xAD;%s;;;\r\n"
                    "END:VCARD\r\n",
                    given);
    btstack_vcard_parser_init(&parser, collect_record, &sink);
    CHECK(feed_all(&parser, input));
    CHECK(btstack_vcard_parser_finish(&parser) == BTSTACK_VCARD_PARSER_OK);
    CHECK(sink.count == 1u);
    CHECK(sink.records[0].truncated != 0u);
    CHECK(strlen(sink.records[0].name) == 47u);
    CHECK(sink.records[0].name[46] == ' ');
    CHECK(is_valid_utf8(sink.records[0].name));
    return 1;
}

static int test_overflowed_qp_fn_treats_partial_triplet_as_truncation(void)
{
    static const char prefix[] =
        "BEGIN:VCARD\r\nFN;ENCODING=QUOTED-PRINTABLE:";
    static const char suffix[] = "\r\nTEL:1\r\nEND:VCARD\r\n";
    char input[512];
    char *cursor = input;
    size_t i;
    btstack_vcard_parser_t parser;
    record_sink_t sink = {0};

    memcpy(cursor, prefix, sizeof(prefix) - 1u);
    cursor += sizeof(prefix) - 1u;
    for (i = 0u; i < 100u; ++i) {
        memcpy(cursor, "=41", 3u);
        cursor += 3u;
    }
    memcpy(cursor, suffix, sizeof(suffix));

    btstack_vcard_parser_init(&parser, collect_record, &sink);
    CHECK(feed_all(&parser, input));
    CHECK(btstack_vcard_parser_finish(&parser) == BTSTACK_VCARD_PARSER_OK);
    CHECK(sink.count == 1u);
    CHECK(sink.records[0].truncated != 0u);
    CHECK(strlen(sink.records[0].name) == BTSTACK_VCARD_NAME_MAX);
    for (i = 0u; i < BTSTACK_VCARD_NAME_MAX; ++i) {
        CHECK(sink.records[0].name[i] == 'A');
    }
    return 1;
}

static int test_nonoverflowed_partial_qp_triplet_is_malformed(void)
{
    static const char input[] =
        "BEGIN:VCARD\r\nFN;ENCODING=QUOTED-PRINTABLE:Broken=4\r\n"
        "TEL:1\r\nEND:VCARD\r\n";
    btstack_vcard_parser_t parser;
    record_sink_t sink = {0};

    btstack_vcard_parser_init(&parser, collect_record, &sink);
    CHECK(feed_all(&parser, input));
    CHECK(btstack_vcard_parser_finish(&parser) == BTSTACK_VCARD_PARSER_OK);
    CHECK(sink.count == 0u);
    CHECK(btstack_vcard_parser_error_count(&parser) == 1u);
    return 1;
}

static int test_grouped_tel_and_parameter_grammar(void)
{
    static const char input[] =
        "BEGIN:VCARD\r\nFN:Grammar\r\n"
        "item1.TEL;TYPE=CELL:111\r\n"
        "TEL;X-LABEL=HOME:222\r\n"
        "TEL;TYPE=\"HOME,VOICE\":333\r\n"
        "TEL;TYPE=WORK;X-LABEL=\"Desk:Main\":444\r\n"
        "END:VCARD\r\n";
    btstack_vcard_parser_t parser;
    record_sink_t sink = {0};

    btstack_vcard_parser_init(&parser, collect_record, &sink);
    CHECK(feed_all(&parser, input));
    CHECK(btstack_vcard_parser_finish(&parser) == BTSTACK_VCARD_PARSER_OK);
    CHECK(sink.count == 1u);
    CHECK(sink.records[0].number_count == 4u);
    CHECK(strcmp(sink.records[0].numbers[0].value, "111") == 0);
    CHECK(sink.records[0].numbers[0].type_flags == BTSTACK_VCARD_TEL_CELL);
    CHECK(strcmp(sink.records[0].numbers[1].value, "222") == 0);
    CHECK(sink.records[0].numbers[1].type_flags == BTSTACK_VCARD_TEL_OTHER);
    CHECK(strcmp(sink.records[0].numbers[2].value, "333") == 0);
    CHECK(sink.records[0].numbers[2].type_flags ==
          (BTSTACK_VCARD_TEL_HOME | BTSTACK_VCARD_TEL_VOICE));
    CHECK(strcmp(sink.records[0].numbers[3].value, "444") == 0);
    CHECK(sink.records[0].numbers[3].type_flags == BTSTACK_VCARD_TEL_WORK);
    return 1;
}

static int test_malformed_record_recovers_at_next_begin(void)
{
    static const char input[] =
        "BEGIN:VCARD\r\nFN:Bad\r\nBROKEN-LINE\r\nEND:VCARD\r\n"
        "BEGIN:VCARD\r\nFN:Good\r\nTEL:8\r\nEND:VCARD";
    btstack_vcard_parser_t parser;
    record_sink_t sink = {0};

    btstack_vcard_parser_init(&parser, collect_record, &sink);
    CHECK(feed_all(&parser, input));
    CHECK(btstack_vcard_parser_finish(&parser) == BTSTACK_VCARD_PARSER_OK);
    CHECK(sink.count == 1u);
    CHECK(strcmp(sink.records[0].name, "Good") == 0);
    CHECK(btstack_vcard_parser_error_count(&parser) == 1u);
    return 1;
}

static int test_pause_consumed_boundary_and_replay(void)
{
    static const char first[] =
        "BEGIN:VCARD\r\nFN:First\r\nTEL:1\r\nEND:VCARD\r\n";
    static const char second[] =
        "BEGIN:VCARD\r\nFN:Second\r\nTEL:2\r\nEND:VCARD\r\n";
    char input[sizeof(first) + sizeof(second)];
    btstack_vcard_parser_t parser;
    record_sink_t sink = {0};
    size_t consumed = 0u;
    size_t total_length;

    (void) snprintf(input, sizeof(input), "%s%s", first, second);
    total_length = strlen(input);
    sink.reject_count = 1u;
    btstack_vcard_parser_init(&parser, collect_record, &sink);
    CHECK(btstack_vcard_parser_process(&parser, (const uint8_t *) input,
                                       strlen(first) - 1u, &consumed) ==
          BTSTACK_VCARD_PARSER_OK);
    CHECK(consumed == strlen(first) - 1u);
    CHECK(btstack_vcard_parser_process(
              &parser, (const uint8_t *) input + strlen(first) - 1u,
              total_length - strlen(first) + 1u, &consumed) ==
          BTSTACK_VCARD_PARSER_PAUSED);
    CHECK(consumed == 1u);
    CHECK(sink.count == 0u);
    CHECK(btstack_vcard_parser_process(&parser,
                                       (const uint8_t *) input + strlen(first),
                                       total_length - strlen(first),
                                       &consumed) ==
          BTSTACK_VCARD_PARSER_OK);
    CHECK(consumed == total_length - strlen(first));
    CHECK(btstack_vcard_parser_finish(&parser) == BTSTACK_VCARD_PARSER_OK);
    CHECK(sink.count == 1u);
    CHECK(strcmp(sink.records[0].name, "Second") == 0);
    return 1;
}

static int test_callback_reentry_is_rejected_without_duplicate(void)
{
    static const char input[] =
        "BEGIN:VCARD\r\nFN:First\r\nTEL:1\r\nEND:VCARD\r\n"
        "BEGIN:VCARD\r\nFN:Second\r\nTEL:2\r\nEND:VCARD\r\n";
    btstack_vcard_parser_t parser;
    reentrant_sink_t sink = {0};
    size_t consumed = 0u;

    sink.parser = &parser;
    btstack_vcard_parser_init(&parser, collect_with_reentry, &sink);
    CHECK(btstack_vcard_parser_process(&parser, (const uint8_t *) input,
                                       strlen(input), &consumed) ==
          BTSTACK_VCARD_PARSER_OK);
    CHECK(consumed == strlen(input));
    CHECK(btstack_vcard_parser_finish(&parser) == BTSTACK_VCARD_PARSER_OK);
    CHECK(sink.reentry_result == BTSTACK_VCARD_PARSER_ERROR);
    CHECK(sink.reentry_consumed == 0u);
    CHECK(sink.count == 2u);
    CHECK(strcmp(sink.records[0].name, "First") == 0);
    CHECK(strcmp(sink.records[1].name, "Second") == 0);
    return 1;
}

typedef int (*test_fn_t)(void);

int main(void)
{
    static const test_fn_t tests[] = {
        test_versions_name_precedence_and_fallback,
        test_every_two_chunk_split_matches_baseline,
        test_complex_qp_fold_utf8_every_two_chunk_split,
        test_crlf_folding_and_utf8,
        test_backslash_escaped_values,
        test_quoted_printable_hex_and_soft_break,
        test_numbers_types_and_truncation,
        test_call_datetime_directions,
        test_long_photo_is_ignored,
        test_qp_photo_soft_break_after_buffer_overflow_is_ignored,
        test_utf8_truncation_keeps_only_valid_codepoints,
        test_n_fallback_composition_does_not_split_utf8,
        test_overflowed_qp_fn_treats_partial_triplet_as_truncation,
        test_nonoverflowed_partial_qp_triplet_is_malformed,
        test_grouped_tel_and_parameter_grammar,
        test_malformed_record_recovers_at_next_begin,
        test_pause_consumed_boundary_and_replay,
        test_callback_reentry_is_rejected_without_duplicate
    };
    size_t i;

    for (i = 0u; i < ARRAY_COUNT(tests); ++i) {
        if (!tests[i]()) {
            return EXIT_FAILURE;
        }
    }
    printf("PASS: %u vCard parser tests\n", (unsigned int) ARRAY_COUNT(tests));
    return EXIT_SUCCESS;
}
