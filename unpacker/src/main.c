#include <7z.h>
#include <7zCrc.h>
#include <memory.h>
#include <rucksack/rucksack.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "rucksack/unpacker/unpacker.h"

#define LOOK_STREAM_BUFFER_SIZE 262144 /* 256 kib */

static void *alloc_7z(ISzAllocPtr allocator, size_t size) {
    return malloc(size);
}

static void free_7z(ISzAllocPtr allocator, void *address) {
    if(address) {
        free(address);
    }
}

static ISzAlloc create_7z_alloc() {
    ISzAlloc alloc;
    alloc.Alloc = alloc_7z;
    alloc.Free = free_7z;

    return alloc;
}

static int extended_memcmp(const void *a, size_t size_a, const void *b, size_t size_b) {
    size_t cmp_size = size_a < size_b ? size_a : size_b;
    return memcmp(a, b, cmp_size);
}

int main(int argc, const char **argv) {
    // Set up 7zip
    CrcGenerateTable();

    ISzAlloc allocator = create_7z_alloc();

    // Open the stream to the own executable
    CLookToRead2 look_stream;
    LookToRead2_CreateVTable(&look_stream, False);

    const void *config_buffer;

    look_stream.buf = malloc(LOOK_STREAM_BUFFER_SIZE);
    look_stream.bufSize = LOOK_STREAM_BUFFER_SIZE;
    look_stream.realStream = rucksack_unpacker_open_stream(&config_buffer);
    LookToRead2_Init(&look_stream);

    rucksack_config_entry_t *config = NULL;
    char *execute_after = NULL;
    char *execute_working_dir = NULL;

    if(config_buffer) {
        config = rucksack_decode_config(config_buffer);

        for(rucksack_config_entry_t *entry = config; entry->key != NULL; entry++) {
            if(extended_memcmp(entry->key, entry->key_size, "exec", 4) == 0) {
                execute_after = malloc(entry->value_size + 1);
                memcpy(execute_after, entry->value, entry->value_size);
                execute_after[entry->value_size] = '\0';
            }

            if(extended_memcmp(entry->key, entry->key_size, "working-dir", 11) == 0) {
                execute_working_dir = malloc(entry->value_size + 1);
                memcpy(execute_working_dir, entry->value, entry->value_size);
                execute_working_dir[entry->value_size] = '\0';
            }
        }
    }

    // Initialize the archive structure
    CSzArEx archive;
    SzArEx_Init(&archive);
    SRes result = SzArEx_Open(&archive, &look_stream.vt, &allocator, &allocator);
    if(result != SZ_OK) {
        fprintf(stderr, "Failed to open archive: %d\n", result);
        return 1;
    }

    // Begin extracting files
    uint32_t block_index;
    Byte *output_buffer = NULL;
    size_t output_buffer_size;

    uint16_t *file_name_buffer = NULL;
    size_t file_name_buffer_size = 0;

    for(uint32_t file_index = 0; file_index < archive.NumFiles; file_index++) {
        size_t file_name_length = SzArEx_GetFileNameUtf16(&archive, file_index, NULL);
        if(file_name_length > file_name_buffer_size) {
            file_name_buffer_size = file_name_length;

            if(file_name_buffer) {
                free(file_name_buffer);
            }

            file_name_buffer = malloc(file_name_buffer_size * sizeof(uint16_t));
        }

        SzArEx_GetFileNameUtf16(&archive, file_index, file_name_buffer);

        if(SzArEx_IsDir(&archive, file_index)) {
            rucksack_unpacker_create_directory(file_name_buffer, archive.Attribs.Vals[file_index]);
            continue;
        }

        size_t extract_offset = 0;
        size_t processed_output_size = 0;

        result = SzArEx_Extract(
            &archive,
            &look_stream.vt,
            file_index,
            &block_index,
            &output_buffer,
            &output_buffer_size,
            &extract_offset,
            &processed_output_size,
            &allocator,
            &allocator);

        if(result != SZ_OK) {
            fprintf(stderr, "Failed to extract a file!\n");
            return 1;
        }

        rucksack_unpacker_write_file(
            file_name_buffer, output_buffer + extract_offset, processed_output_size, archive.Attribs.Vals[file_index]);
    }

    if(output_buffer) {
        free(output_buffer);
    }

    if(file_name_buffer) {
        free(file_name_buffer);
    }

    free(look_stream.buf);

    if(execute_after) {
        rucksack_unpacker_execute_file(execute_after, execute_working_dir);
        free(execute_after);
    }

    if(execute_working_dir) {
        free(execute_working_dir);
    }

    if(config) {
        rucksack_free_config(config);
    }
    rucksack_unpacker_close_stream(look_stream.realStream);

    return 0;
}
