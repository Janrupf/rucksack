#include <malloc.h>
#include <rucksack/rucksack.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void read_data(const char *specifier, uint32_t *size_out, void **data_out) {
    size_t specifier_length = strlen(specifier);

    if(specifier_length < 1) {
        *size_out = 0;
        *data_out = (void *) 0x1;
    } else if(specifier[0] == '@') {
        specifier++;
        specifier_length = strlen(specifier);

        if(specifier_length < 1) {
            fprintf(stderr, "Invalid config specifier @, expected @fileName");
            exit(1);
        }

        FILE *specifier_file = fopen(specifier, "rb");
        if(!specifier_file) {
            fprintf(stderr, "Failed to open specifier file %s\n", specifier);
            exit(1);
        }

        fseek(specifier_file, 0, SEEK_END);
        size_t specifier_file_size = ftell(specifier_file);

        if(specifier_file_size < 1) {
            fclose(specifier_file);

            *size_out = 0;
            *data_out = (void *) 0x1;
        } else {
            *size_out = specifier_file_size;
            *data_out = malloc(specifier_file_size);

            fseek(specifier_file, 0, SEEK_SET);
            size_t read = fread(*data_out, 1, specifier_file_size, specifier_file);
            fclose(specifier_file);

            if(read != specifier_file_size) {
                fprintf(
                    stderr,
                    "Expected to read %zu bytes from specifier file, but read %zu instead\n",
                    specifier_file_size,
                    read);
                exit(1);
            }
        }
    } else {
        *size_out = specifier_length;
        *data_out = malloc(specifier_length);
        memcpy(*data_out, specifier, specifier_length);
    }
}

static void fill_config_entry(uint32_t index, const char **argv, rucksack_config_entry_t *entries) {
    read_data(argv[3 + (index * 2)], &entries[index].key_size, &entries[index].key);
    read_data(argv[3 + ((index * 2) + 1)], &entries[index].value_size, &entries[index].value);
}

int main(int argc, const char **argv) {
    if(argc < 3) {
        fprintf(stderr, "%s <executable> <7z archive> [config-key config-value...]", argv[0]);
        return 1;
    } else if(argc > 3 && ((argc + 1) % 2) != 0) {
        fprintf(stderr, "Expected each config key to have an associated value\n");
        return 1;
    }

    uint32_t config_entry_count = argc > 3 ? (argc - 3) / 2 : 0;

    FILE *exe = fopen(argv[1], "a+b");
    if(!exe) {
        perror("fopen failed");
        return 1;
    }

    fseek(exe, 0, SEEK_END);
    fseek(exe, -8, SEEK_CUR);

    char magic_buffer[9];
    size_t read_count = fread(magic_buffer, 1, 8, exe);
    if(read_count == 8) {
        magic_buffer[8] = '\0';
        if(strcmp(magic_buffer, "rucksack") == 0) {
            fclose(exe);
            fprintf(stderr, "%s already contains the magic 'rucksack'\n", argv[1]);
            exit(1);
        }
    }

    magic_buffer[0] = 'r';
    magic_buffer[1] = 'u';
    magic_buffer[2] = 'c';
    magic_buffer[3] = 'k';
    magic_buffer[4] = 's';
    magic_buffer[5] = 'a';
    magic_buffer[6] = 'c';
    magic_buffer[7] = 'k';
    magic_buffer[8] = '\0';

    FILE *sz = fopen(argv[2], "rb");
    if(!sz) {
        perror("fopen failed");
        return 1;
    }

    fseek(sz, 0, SEEK_END);
    size_t sz_size = ftell(sz);
    fseek(sz, 0, SEEK_SET);

    void *data_buffer = malloc(sz_size);
    size_t io_result = fread(data_buffer, 1, sz_size, sz);
    fclose(sz);

    if(io_result != sz_size) {
        fprintf(stderr, "Expected to read %zu bytes, but read %zu\n", sz_size, io_result);
        return 1;
    }

    io_result = fwrite(data_buffer, 1, sz_size, exe);
    free(data_buffer);

    if(io_result != sz_size) {
        fprintf(stderr, "Expected to write %zu bytes, but wrote %zu\n", sz_size, io_result);
        return 1;
    }

    uint64_t config_size = 0;

    if(config_entry_count != 0) {
        rucksack_config_entry_t *config_entries = malloc(sizeof(rucksack_config_entry_t) * (config_entry_count + 1));

        for(uint32_t i = 0; i < config_entry_count; i++) {
            fill_config_entry(i, argv, config_entries);
        }

        config_entries[config_entry_count].key = NULL;
        config_entries[config_entry_count].key_size = 0;
        config_entries[config_entry_count].value = NULL;
        config_entries[config_entry_count].value_size = 0;

        config_size = rucksack_calculate_config_size(config_entries);
        void *config_buffer = malloc(config_size);
        rucksack_encode_config(config_entries, config_buffer);

        io_result = fwrite(config_buffer, 1, config_size, exe);
        free(config_buffer);

        if(io_result != config_size) {
            fprintf(stderr, "Expected to write %zu bytes, but wrote %zu\n", config_size, io_result);
            return 1;
        }

        rucksack_free_config(config_entries);
    }

    uint64_t data_sizes[2] = {
        sz_size,
        config_size
    };

    io_result = fwrite(data_sizes, 1, 16, exe);
    if(io_result != 16) {
        fprintf(stderr, "Expected to write 16 bytes, but wrote %zu\n", io_result);
        return 1;
    }

    io_result = fwrite(magic_buffer, 1, 8, exe);
    if(io_result != 8) {
        fprintf(stderr, "Expected to write 8 bytes, but wrote %zu\n", io_result);
        return 1;
    }

    fclose(exe);
}