#pragma once

#include <stddef.h>
#include <stdint.h>

/**
 * Rucksack config structure. Config entries will be interpreted by the unpacker.
 */
struct rucksack_config_entry {
    /**
     * They key of the entry
     */
    void *key;

    /**
     * The size of the data of they key pointer
     */
    uint32_t key_size;

    /**
     * The value of the entry
     */
    void *value;

    /**
     * The size of the data of the value pointer
     */
    uint32_t value_size;
};

typedef struct rucksack_config_entry rucksack_config_entry_t;

/**
 * Calculates the size of the configuration entries.
 *
 * @param entries array of configuration entries, the last entry must have a key value of NULL
 * @return the calculated size of the config in bytes
 */
size_t rucksack_calculate_config_size(const rucksack_config_entry_t *entries);

/**
 * Encodes the rucksack config into a buffer.
 *
 * @param entries the entries to encode
 * @param out the buffer to write to, must have at least the size of rucksack_calculate_config_size(entries)
 */
void rucksack_encode_config(const rucksack_config_entry_t *entries, void *out);

/**
 * Decodes the rucksack config from a buffer into a config array.
 *
 * @param config the config buffer to decode
 * @return the config array, should be deallocated using rucksack_free_config, the last entry has a key value of NULL
 */
rucksack_config_entry_t *rucksack_decode_config(const void *config);

/**
 * Frees the config array.
 *
 * @param entries the config entries array to free
 */
void rucksack_free_config(rucksack_config_entry_t *entries);
