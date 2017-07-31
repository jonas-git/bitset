/* Copyright (c) 2017 Jonas van den Berg <jonas.vanen@gmail.com>
 * 
 * bitset is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#ifndef BITSET_H
#define BITSET_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <string.h>

struct bitset {
	unsigned char *data;
	size_t capacity;
	size_t size;
};

/* bitset_bytes(set)
 |   returns the number of bytes stored
 | set: valid pointer to a [struct bitset]
 */
#define bitset_bytes(set) ((set)->capacity / 8)

#define bitset_alloc(num)  bitset_malloc(num, 0)
#define bitset_calloc(num) bitset_malloc(num, 1)

struct bitset *bitset_new();
struct bitset *bitset_malloc(size_t num, unsigned int clear);
struct bitset *bitset_cpy(struct bitset *set);
void bitset_free(struct bitset *set);

size_t bitset_rclear(struct bitset *set, size_t begin, size_t end);

/* bitset_nclear(set, index, size)
 |   clears the specified number of bits: index to (index + size - 1);
 |   returns the number of bits cleared
 | set:   valid pointer to a [struct bitset]
 | index: index of the first bit (inclusive)
 | size:  number of bits to be cleared
 */
static inline
size_t bitset_nclear(struct bitset *set, size_t index, size_t size)
{
	return bitset_rclear(set, index, index + size);
}

/* bitset_clear(set)
 |   clears all bits: 0 to (size - 1);
 |   returns the number of bits cleared
 | set:   valid pointer to a [struct bitset]
 */
static inline
size_t bitset_clear(struct bitset *set)
{
	memset(set->data, 0, bitset_bytes(set));
	return set->size;
}

intmax_t bitset_resize(struct bitset *set, size_t size);
intmax_t bitset_cresize(struct bitset *set, size_t size);

unsigned char *bitset_byte_at(struct bitset *set, size_t index);
void bitset_set(struct bitset *set, size_t index, unsigned int state);
unsigned int bitset_get(struct bitset *set, size_t index);
size_t bitset_write(struct bitset *set, size_t index,
                    unsigned char *seq, size_t size);
size_t bitset_read(struct bitset *set, size_t index,
                   unsigned char *seq, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* BITSET_H */
