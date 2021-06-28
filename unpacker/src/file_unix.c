#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "rucksack/unpacker/unpacker.h"

// Blatantly stolen macros from 7zip source

#define _UTF8_START(n) (0x100 - (1 << (7 - (n))))
#define _UTF8_RANGE(n) (((UInt32) 1) << ((n) *5 + 6))
#define _UTF8_HEAD(n, val) ((Byte) (_UTF8_START(n) + (val >> (6 * (n)))))
#define _UTF8_CHAR(n, val) ((Byte) (0x80 + (((val) >> (6 * (n))) & 0x3F)))

static size_t utf16_strlen(const uint16_t *str) {
    size_t s = 0;

    while(*str != 0) {
        s++;
        str++;
    }

    return s;
}

static size_t utf8_buffer_length(const uint16_t *utf16_string, const uint16_t *utf16_string_end) {
    size_t size = 0;

    while(1) {
        if(utf16_string == utf16_string_end) {
            return size;
        }

        size++;
        uint32_t value = *utf16_string++;

        if(value < 0x80) {
            continue;
        }

        if(value < _UTF8_RANGE(1)) {
            size++;
            continue;
        }

        if(value >= 0xD800 && value < 0xDC00 && utf16_string != utf16_string_end) {
            uint32_t next_value = *utf16_string;

            if(next_value >= 0xDC00 && next_value < 0xE000) {
                utf16_string++;
                size += 3;
                continue;
            }
        }

        size += 2;
    }
}

static char *utf16_to_utf8(const uint16_t *utf16_string) {
    const uint16_t *utf16_string_end = utf16_string + utf16_strlen(utf16_string);
    char *buffer = malloc(utf8_buffer_length(utf16_string, utf16_string_end));

    size_t write_head = 0;
    uint32_t value;

    while(1) {
        if(utf16_string == utf16_string_end) {
            buffer[write_head] = '\0';
            return buffer;
        }

        value = *utf16_string++;

        if(value < 0x80) {
            buffer[write_head++] = (char) value;
            continue;
        }

        if(value < _UTF8_RANGE(1)) {
            buffer[write_head++] = _UTF8_HEAD(1, value);
            buffer[write_head++] = _UTF8_CHAR(0, value);
            continue;
        }

        if(value >= 0xD800 && value < 0xDC00 && utf16_string != utf16_string_end) {
            uint32_t next_value = *utf16_string;

            if(next_value >= 0xDC00 && next_value < 0xE000) {
                utf16_string++;

                value = (((value - 0xD800) << 1) | (next_value - 0xDC00)) + 0x10000;
                buffer[write_head++] = _UTF8_HEAD(3, value);
                buffer[write_head++] = _UTF8_CHAR(2, value);
                buffer[write_head++] = _UTF8_CHAR(1, value);
                buffer[write_head++] = _UTF8_CHAR(0, value);
                continue;
            }
        }

        buffer[write_head++] = _UTF8_HEAD(2, value);
        buffer[write_head++] = _UTF8_CHAR(1, value);
        buffer[write_head++] = _UTF8_CHAR(0, value);
    }
}

static void recursive_mkdir(const char *directory_name) {
    size_t name_length = strlen(directory_name);
    char *temporary_buffer = malloc(name_length + 1);
    strcpy(temporary_buffer, directory_name);

    if(temporary_buffer[name_length - 1] == '/') {
        temporary_buffer[name_length - 1] = '\0';
    }

    for(char *search = temporary_buffer + 1; *search; search++) {
        if(*search == '/') {
            *search = '\0';
            if(mkdir(temporary_buffer, S_IRWXU) == -1 && errno != EEXIST) {
                perror("mkdir failed");
                exit(1);
            }
            *search = '/';
        }
    }

    if(mkdir(temporary_buffer, S_IRWXU) == -1 && errno != EEXIST) {
        perror("mkdir failed");
        exit(1);
    }

    free(temporary_buffer);
}

void rucksack_unpacker_create_directory(const uint16_t *name, uint32_t attributes) {
    char *directory_name = utf16_to_utf8(name);
    recursive_mkdir(directory_name);

    if(attributes & FILE_ATTRIBUTE_UNIX_EXTENSION) {
        uint16_t real_attributes = attributes >> 16;
        chmod(directory_name, real_attributes);
    }

    free(directory_name);
}

void rucksack_unpacker_write_file(const uint16_t *name, const void *data, size_t size, uint32_t attributes) {
    char *file_name = utf16_to_utf8(name);

    FILE *file = fopen(file_name, "wb");
    if(!file) {
        perror("fopen failed");
        exit(1);
    }

    size_t write_result = fwrite(data, 1, size, file);
    if(write_result != size) {
        fprintf(stderr, "Expected %zu bytes to be written, but only wrote %zu bytes\n", size, write_result);
        exit(1);
    }

    fclose(file);

    if(attributes & FILE_ATTRIBUTE_UNIX_EXTENSION) {
        uint16_t real_attributes = attributes >> 16;
        chmod(file_name, real_attributes);
    }

    free(file_name);
}

void rucksack_unpacker_execute_file(const char *name, const char *working_dir) {
    char *argv[2] = {(char *) name, NULL};
    char *full_file_path = realpath(name, NULL);
    if(!full_file_path) {
        perror("Failed to resolve path for file to execute");
        exit(1);
    }

    if(working_dir) {
        if(chdir(working_dir) == -1) {
            perror("Failed to change working directory to start child process");
            exit(1);
        }
    }
    execv(full_file_path, argv);

    perror("execve failed");
    exit(1);
}