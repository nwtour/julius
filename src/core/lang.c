#include "core/lang.h"

#include "core/buffer.h"
#include "core/io.h"
#include "core/string.h"

#include <stdlib.h>

#define MAX_TEXT_ENTRIES 1000
#define MAX_TEXT_DATA 200000
#define MIN_TEXT_SIZE (28 + MAX_TEXT_ENTRIES * 8)
#define MAX_TEXT_SIZE (MIN_TEXT_SIZE + MAX_TEXT_DATA)

#define MAX_MESSAGE_ENTRIES 400
#define MAX_MESSAGE_DATA 360000
#define MIN_MESSAGE_SIZE 32024
#define MAX_MESSAGE_SIZE (MIN_MESSAGE_SIZE + MAX_MESSAGE_DATA)

#define BUFFER_SIZE 400000

static const uint8_t NEW_GAME_POLISH[] = { 0x4e, 0x6f, 0x77, 0x61, 0x20, 0x67, 0x72, 0x61, 0 };
static const uint8_t NEW_GAME_RUSSIAN[] = { 0xcd, 0xee, 0xe2, 0xe0, 0xff, 0x20, 0xe8, 0xe3, 0xf0, 0xe0, 0 };

static struct {
    struct {
        int32_t offset;
        int32_t in_use;
    } text_entries[MAX_TEXT_ENTRIES];
    uint8_t text_data[MAX_TEXT_DATA];

    lang_message message_entries[MAX_MESSAGE_ENTRIES];
    uint8_t message_data[MAX_MESSAGE_DATA];

    encoding_type encoding;
} data;

static void parse_text(buffer *buf)
{
    buffer_skip(buf, 28); // header
    for (int i = 0; i < MAX_TEXT_ENTRIES; i++) {
        data.text_entries[i].offset = buffer_read_i32(buf);
        data.text_entries[i].in_use = buffer_read_i32(buf);
    }
    buffer_read_raw(buf, data.text_data, MAX_TEXT_DATA);
}

static int load_text(const char *filename, uint8_t *buf_data)
{
    buffer buf;
    int filesize = io_read_file_into_buffer(filename, buf_data, BUFFER_SIZE);
    if (filesize < MIN_TEXT_SIZE || filesize > MAX_TEXT_SIZE) {
        return 0;
    }
    buffer_init(&buf, buf_data, filesize);
    parse_text(&buf);
    return 1;
}

static uint8_t *get_message_text(int32_t offset)
{
    if (!offset) {
        return 0;
    }
    return &data.message_data[offset];
}

static void parse_message(buffer *buf)
{
    buffer_skip(buf, 24); // header
    for (int i = 0; i < MAX_MESSAGE_ENTRIES; i++) {
        lang_message *m = &data.message_entries[i];
        m->type = buffer_read_i16(buf);
        m->message_type = buffer_read_i16(buf);
        buffer_skip(buf, 2);
        m->x = buffer_read_i16(buf);
        m->y = buffer_read_i16(buf);
        m->width_blocks = buffer_read_i16(buf);
        m->height_blocks = buffer_read_i16(buf);
        m->image1.id = buffer_read_i16(buf);
        m->image1.x = buffer_read_i16(buf);
        m->image1.y = buffer_read_i16(buf);
        m->image2.id = buffer_read_i16(buf);
        m->image2.x = buffer_read_i16(buf);
        m->image2.y = buffer_read_i16(buf);
        m->title.x = buffer_read_i16(buf);
        m->title.y = buffer_read_i16(buf);
        m->subtitle.x = buffer_read_i16(buf);
        m->subtitle.y = buffer_read_i16(buf);
        buffer_skip(buf, 4);
        m->video.x = buffer_read_i16(buf);
        m->video.y = buffer_read_i16(buf);
        buffer_skip(buf, 14);
        m->urgent = buffer_read_i32(buf);
        
        m->video.text = get_message_text(buffer_read_i32(buf));
        buffer_skip(buf, 4);
        m->title.text = get_message_text(buffer_read_i32(buf));
        m->subtitle.text = get_message_text(buffer_read_i32(buf));
        m->content.text = get_message_text(buffer_read_i32(buf));
    }
    buffer_read_raw(buf, &data.message_data, MAX_MESSAGE_DATA);
}

static int load_message(const char *filename, uint8_t *data)
{
    buffer buf;
    int filesize = io_read_file_into_buffer(filename, data, BUFFER_SIZE);
    if (filesize < MIN_MESSAGE_SIZE || filesize > MAX_MESSAGE_SIZE) {
        return 0;
    }
    buffer_init(&buf, data, filesize);
    parse_message(&buf);
    return 1;
}

static void determine_encoding(void)
{
    // A really dirty way to 'detect' encoding:
    // - Windows-1250 (Central/Eastern Europe) is used in Polish only
    // - Windows-1251 (Cyrillic) is used in Russian only
    // - Windows-1252 (Western Europe) is used in all other languages
    // Check if the string for "New game" is Polish or Russian
    const uint8_t *new_game_string = lang_get_string(1, 1);
    if (string_equals(NEW_GAME_POLISH, new_game_string)) {
        data.encoding = ENCODING_EASTERN_EUROPE;
    } else if (string_equals(NEW_GAME_RUSSIAN, new_game_string)) {
        data.encoding = ENCODING_CYRILLIC;
    } else {
        data.encoding = ENCODING_WESTERN_EUROPE;
    }
}

int lang_load(const char *text_filename, const char *message_filename)
{
    uint8_t *data = (uint8_t *) malloc(BUFFER_SIZE);
    if (!data) {
        return 0;
    }
    int success = load_text(text_filename, data) && load_message(message_filename, data);
    free(data);
    if (success) {
        determine_encoding();
    }
    return success;
}

encoding_type lang_encoding(void)
{
    return data.encoding;
}

const uint8_t *lang_get_string(int group, int index)
{
    const uint8_t *str = &data.text_data[data.text_entries[group].offset];
    uint8_t prev = 0;
    while (index > 0) {
        if (!*str && (prev >= ' ' || prev == 0)) {
            --index;
        }
        prev = *str;
        ++str;
    }
    while (*str < ' ') { // skip non-printables
        ++str;
    }
    return str;
}

const lang_message *lang_get_message(int id)
{
    return &data.message_entries[id];
}
