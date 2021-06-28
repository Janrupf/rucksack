#pragma once

#include <7z.h>
#include <stdint.h>

/**
 * Opens a 7zip stream to the own executable.
 *
 * @param config a pointer to which the config data pointer will be written to
 * @return the opened stream
 */
ISeekInStream *rucksack_unpacker_open_stream(void const **config);

/**
 * Closes the 7zip stream to the own executable.
 *
 * @param stream the stream to close
 */
void rucksack_unpacker_close_stream(const ISeekInStream *stream);

/**
 * Creates the directory specified by the name.
 *
 * @param name the name of the directory to create
 * @param attributes the attributes to apply to the directory
 */
void rucksack_unpacker_create_directory(const uint16_t *name, uint32_t attributes);

/**
 * Writes a file.
 *
 * @param name the name of the file to write
 * @param data the buffer to read the data to write from
 * @param size the size of the data buffer
 * @param attributes the attributes to apply to the file
 */
void rucksack_unpacker_write_file(const uint16_t *name, const void *data, size_t size, uint32_t attributes);

/**
 * Executes a file.
 *
 * @param name the name of the file to execute
 */
void rucksack_unpacker_execute_file(const char *name);
