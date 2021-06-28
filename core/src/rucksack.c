#include "rucksack/rucksack.h"

#include <stdlib.h>
#include <string.h>

static void encode_uint32_t(size_t *write_head, char *buf, uint32_t value) {
    buf[(*write_head)++] = (char) (value & 0x000000FF);
    buf[(*write_head)++] = (char) (value & 0x0000FF00);
    buf[(*write_head)++] = (char) (value & 0x00FF0000);
    buf[(*write_head)++] = (char) (value & 0xFF000000);
}

static uint32_t decode_uint32_t(size_t *read_head, const char *buf) {
    uint32_t a = (uint32_t) buf[(*read_head)++];
    uint32_t b = (uint32_t) buf[(*read_head)++];
    uint32_t c = (uint32_t) buf[(*read_head)++];
    uint32_t d = (uint32_t) buf[(*read_head)++];

    uint32_t value = (a) | (b << 8) | (c << 16) | (d << 24);

    return value;
}

size_t rucksack_calculate_config_size(const rucksack_config_entry_t *entries) {
    size_t config_size = 4;

    for(uint32_t i = 0; entries[i].key != NULL; i++) {
        config_size += 8 + entries[i].key_size + entries[i].value_size;
    }

    return config_size;
}

void rucksack_encode_config(const rucksack_config_entry_t *entries, void *out) {
    char *buf = out;

    size_t write_head = 4;

    uint32_t i;
    for(i = 0; entries[i].key != NULL; i++) {
        encode_uint32_t(&write_head, buf, entries[i].key_size);
        memcpy(buf + write_head, entries[i].key, entries[i].key_size);
        write_head += entries[i].key_size;

        encode_uint32_t(&write_head, buf, entries[i].value_size);
        memcpy(buf + write_head, entries[i].value, entries[i].value_size);
        write_head += entries[i].value_size;
    }

    write_head = 0;
    encode_uint32_t(&write_head, buf, i);
}

rucksack_config_entry_t *rucksack_decode_config(const void *config) {
    const char *buf = config;

    size_t read_head = 0;
    uint32_t config_size = decode_uint32_t(&read_head, buf);

    rucksack_config_entry_t *entries = malloc((config_size + 1) * sizeof(rucksack_config_entry_t));

    uint32_t i;
    for(i = 0; i < config_size; ++i) {
        uint32_t key_size = decode_uint32_t(&read_head, buf);
        void *key = key_size == 0 ? ((void *) 0x1) : malloc(key_size);
        memcpy(key, buf + read_head, key_size);
        read_head += key_size;

        uint32_t value_size = decode_uint32_t(&read_head, buf);
        void *value = value_size == 0 ? ((void *) 0x1) : malloc(value_size);
        memcpy(value, buf + read_head, value_size);
        read_head += value_size;

        entries[i].key = key;
        entries[i].key_size = key_size;
        entries[i].value = value;
        entries[i].value_size = value_size;
    }

    entries[i].key = NULL;
    entries[i].key_size = 0;
    entries[i].value = NULL;
    entries[i].value_size = 0;

    return entries;
}

void rucksack_free_config(rucksack_config_entry_t *entries) {
    for(uint32_t i = 0; entries[i].key != NULL; i++) {
        if(entries[i].key_size != 0) {
            free(entries[i].key);
        }

        if(entries[i].value_size != 0) {
            free(entries[i].value);
        }
    }

    free(entries);
}
