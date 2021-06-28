#include <malloc.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "rucksack/unpacker/unpacker.h"

#ifdef __APPLE__
#    include <mach-o/dyld.h>
#else
#    include <sys/unistd.h>
#endif

#define PATH_BUFFER_LENGTH 4096

static const char *find_own_executable_path() {
    char *own_executable_path = malloc(PATH_BUFFER_LENGTH);

#ifdef __APPLE__
    uint32_t buffer_size = PATH_BUFFER_LENGTH;

    if(_NSGetExecutablePath(own_executable_path, &buffer_size) == -1) {
        free(own_executable_path);
        own_executable_path = malloc(buffer_size);

        _NSGetExecutablePath(own_executable_path, &buffer_size);
    }
#else
    ssize_t length = readlink("/proc/self/exe", own_executable_path, PATH_BUFFER_LENGTH - 1);

    if(length == -1) {
        perror("readlink(\"/proc/self/exe\") failed");
        exit(1);
    }

    own_executable_path[length] = '\0';
#endif

    return own_executable_path;
}

struct rucksack_unpacker_stream {
    ISeekInStream vt;

    FILE *file;

    uint64_t min_position;
    uint64_t current_position;
    uint64_t max_position;
};

typedef struct rucksack_unpacker_stream rucksack_unpacker_stream_t;

static SRes stream_read(const ISeekInStream *p, void *buf, size_t *size) {
    rucksack_unpacker_stream_t *stream = CONTAINER_FROM_VTBL(p, rucksack_unpacker_stream_t, vt);

    if(stream->current_position + *size > stream->max_position) {
        *size = (stream->current_position + *size) - stream->max_position;
    }

    *size = fread(buf, 1, *size, stream->file);

    long new_pos = ftell(stream->file);
    if(new_pos < 0) {
        return errno;
    }

    stream->current_position = new_pos;

    if(feof(stream->file)) {
        return SZ_ERROR_INPUT_EOF;
    } else if(ferror(stream->file)) {
        return SZ_ERROR_READ;
    }

    return SZ_OK;
}

static SRes stream_seek(const ISeekInStream *p, Int64 *pos, ESzSeek origin) {
    rucksack_unpacker_stream_t *stream = CONTAINER_FROM_VTBL(p, rucksack_unpacker_stream_t, vt);

    uint64_t new_position;

    switch(origin) {
        case SZ_SEEK_SET:
            new_position = stream->min_position + *pos;
            break;

        case SZ_SEEK_CUR:
            new_position = stream->current_position + (*pos);
            break;

        case SZ_SEEK_END:
            new_position = stream->max_position;
            break;

        default:
            return SZ_ERROR_PARAM;
    }

    int result = fseek(stream->file, (int64_t) new_position, SEEK_SET);
    if(result == -1) {
        return errno;
    }

    stream->current_position = new_position;
    *pos = (int64_t) (stream->current_position - stream->min_position);
    return SZ_OK;
}

static void init_stream(
    rucksack_unpacker_stream_t *stream, FILE *file, uint64_t start_position, uint64_t max_position) {
    stream->vt.Read = stream_read;
    stream->vt.Seek = stream_seek;

    fseek(file, (int64_t) start_position, SEEK_SET);
    stream->file = file;
    stream->min_position = start_position;
    stream->current_position = start_position;
    stream->max_position = max_position;
}

// Unix implementation
ISeekInStream *rucksack_unpacker_open_stream(void const **config) {
    const char *own_executable_path = find_own_executable_path();

    FILE *opened = fopen(own_executable_path, "rb");
    if(!opened) {
        perror("fopen failed");
        exit(1);
    }

    fseek(opened, 0, SEEK_END);
    size_t file_size = ftell(opened);
    fseek(opened, -8, SEEK_CUR);

    char magic_buffer[9];
    size_t read_count = fread(magic_buffer, 1, 8, opened);
    if(read_count != 8) {
        fprintf(stderr, "Failed to read magic\n");
        exit(1);
    }

    magic_buffer[8] = '\0';
    if(strcmp(magic_buffer, "rucksack") != 0) {
        fprintf(stderr, "Invalid magic, expected 'rucksack', got '%s'\n", magic_buffer);
        exit(1);
    }

    fseek(opened, -24, SEEK_CUR);

    uint64_t size_buffers[2];
    read_count = fread(size_buffers, 1, 16, opened);
    if(read_count != 16) {
        fprintf(stderr, "Failed to read size buffers\n");
        exit(1);
    }

    uint64_t data_size = size_buffers[0];
    uint64_t config_size = size_buffers[1];

    uint64_t rucksack_size = data_size + config_size;
    fseek(opened, -(16 + (int64_t) config_size), SEEK_CUR);

    void *config_buffer = config_size ? malloc(config_size) : NULL;
    if(config_size != 0) {
        read_count = fread(config_buffer, 1, config_size, opened);

        if(read_count != config_size) {
            fprintf(stderr, "Expected to read %lu bytes for config, but only read %zu\n", config_size, read_count);
            exit(1);
        }
    }

    *config = config_buffer;

    rucksack_unpacker_stream_t *stream = malloc(sizeof(rucksack_unpacker_stream_t));
    init_stream(stream, opened, file_size - rucksack_size - 24, file_size - 24 - config_size);

    return &stream->vt;
}

void rucksack_unpacker_close_stream(const ISeekInStream *p) {
    rucksack_unpacker_stream_t *stream = CONTAINER_FROM_VTBL(p, rucksack_unpacker_stream_t, vt);
    fclose(stream->file);
}
