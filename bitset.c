/* Copyright (c) 2017 Jonas van den Berg <jonas.vanen@gmail.com>
 * 
 * bitset is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "bitset.h"
#include "internal.c"

/* bitset_new
 |   creates a new, empty [struct bitset];
 |   returns a pointer to the allocated struct
 */
struct bitset *bitset_new()
{
	struct bitset *set = calloc(1, sizeof(struct bitset));
	if (!set)
		return NULL;
	return set;
}

/* bitset_malloc(num, clear):
 |   creates a new [struct bitset] and allocates enough
 |   memory to hold the specified amount of bits;
 |   returns a pointer to the allocated struct
 | num:   number of bits the bitset should be able hold
 | clear: flag that indicates if the memory should be zeroed out
 */
struct bitset *bitset_malloc(size_t num, unsigned int clear)
{
	struct bitset *set = malloc(sizeof(struct bitset));
	if (!set)
		return NULL;
	size_t bytes = bitset_internal_bytes(num);
	set->data = bitset_internal_alloc(clear, bytes, sizeof(unsigned char));
	if (!set->data)
		return NULL;
	set->capacity = bitset_internal_capacity(bytes);
	set->size = num;
	return set;
}

/* bitset_free(set):
 |   frees memory associated with the given bitset
 | set: pointer to a [struct bitset]
 */
void bitset_free(struct bitset *set)
{
	free(set->data);
	free(set);
}

/* bitset_rclear(set, begin, end)
 |   clears the bits inside the given range (inclusive): begin to (end - 1);
 |   returns the number of bits cleared
 | set:   valid pointer to a [struct bitset]
 | begin: index of the first bit (inclusive)
 | end:   index of the ending bit (exclusive)
 */
size_t bitset_rclear(struct bitset *set, size_t begin, size_t end)
{
	unsigned int begin_shift = begin & 0x7;
	unsigned int end_shift = end & 0x7;
	unsigned char *begin_ptr = bitset_byte_at(set, begin);
	unsigned char *end_ptr = bitset_byte_at(set, end);

	if (begin_ptr == end_ptr)
		*begin_ptr &= ~(~(~0 << (end - begin)) << begin);
	else {
		unsigned char *first = begin_ptr + !!begin_shift;
		size_t size = end_ptr - first;
		if (size)
			memset(first, 0, size);
		if (begin_shift)
			*begin_ptr &= ~(~0 << begin_shift);
		*end_ptr &= ~0 << end_shift;
	}

	return end - begin;
}

/* bitset_resize(set, size)
 |   resizes the bitset to the specified amount of bits;
 |   returns the change in size (positive: increase, negative: decrease)
 | set:  valid pointer to a [struct bitset]
 | size: new size
 */
intmax_t bitset_resize(struct bitset *set, size_t size)
{
	intmax_t diff = set->size - size;
	size_t bytes = bitset_internal_bytes(size);
	set->data = realloc(set->data, bytes);
	if (!set->data)
		return 0;
	set->capacity = bitset_internal_capacity(bytes);
	set->size = size;
	return diff;
}

/* bitset_cresize(set, size)
 |   resizes the bitset to the specified amount of bits and
 |   clears newly allocated memory;
 |   returns the change in size (positive: increase, negative: decrease)
 | set:  valid pointer to a [struct bitset]
 | size: new size
 */
intmax_t bitset_cresize(struct bitset *set, size_t size)
{
	size_t old_size = set->size;
	intmax_t diff = bitset_resize(set, size);
	if (size > old_size)
		bitset_rclear(set, old_size, set->size);
	return diff;
}

/* bitset_byte_at(set, index):
 |   returns a pointer to the byte that contains the bit at the specified index
 | set:   valid pointer to a [struct bitset]
 | index: the index of a bit in the bitset
 */
unsigned char *bitset_byte_at(struct bitset *set, size_t index)
{
	return set->data + (index / 8);
}

/* bitset_set(set, index, state):
 |   sets a bit to the specified state
 | set:   valid pointer to a [struct bitset]
 | index: the offset of the bit in the bitset
 | state: a boolean value expressing the specified bit's new state
 */
void bitset_set(struct bitset *set, size_t index, unsigned int state)
{
	unsigned char *entry = bitset_byte_at(set, index);
	*entry ^= (-!!state ^ *entry) & (1 << (index & 0x7));
}

/* bitset_get(set, index):
 |   gets the state of a bit
 | set:   valid pointer to a [struct bitset]
 | index: the offset of the bit in the bitset
 */
unsigned int bitset_get(struct bitset *set, size_t index)
{
	return *bitset_byte_at(set, index) & (1 << (index & 0x7));
}

/* bitset_write(set, index, size, seq):
 |   writes the specified amount of bits;
 |   returns the number of bits written - that is the size argument
 | set:   valid pointer to a [struct bitset]
 | index: the offset at which the writing should start
 | size:  the amount of bits to copy
 | seq:   pointer to a sequence of bits that should be written
 */
size_t bitset_write(struct bitset *set, size_t index,
                    unsigned char *seq, size_t size)
{
	size_t tmp = size;
	unsigned int shift = index & 0x7;
	register unsigned int mask = ~0 << shift;
	unsigned char *entry = bitset_byte_at(set, index);

	for (; size >> 3; size -= 8, ++seq) {
		*entry &= ~mask;
		*entry++ |= *seq << shift;
		if (shift) {
			*entry &= mask;
			*entry |= (*seq >> (8 - shift)) & ~mask;
		}
	}

	if (size) {
		unsigned int end = shift + size;
		if (end < 8)
			mask = ~(~0 << size) << shift;

		*entry &= ~mask;
		*entry++ |= (*seq << shift) & mask;

		if (end > 8) {
			mask >>= 8 - size; // will not shift zeroes in
			*entry &= mask;
			*entry |= (*seq >> (8 - shift)) & ~mask;
		}
	}

	return tmp;
}

/* bitset_read(set, index, size, seq):
 |   reads the specified range of bits and copys them into seq;
 |   returns the number of bits read - that is the size argument
 | set:   valid pointer to a [struct bitset]
 | index: the offset at which the reading should start
 | size:  the amount of bits to copy
 | seq:   pointer to a sufficient amount of memory to
 |          hold the read bit sequence
 */
size_t bitset_read(struct bitset *set, size_t index,
                   unsigned char *seq, size_t size)
{
	size_t tmp = size;
	unsigned int shift = index & 0x7;
	register unsigned int mask = ~0 << shift;
	unsigned char *entry = bitset_byte_at(set, index);

	for (; size >> 3; size -= 8, ++seq) {
		*seq |= (*entry++ & mask) >> shift;
		if (shift)
			*seq |= (*entry & ~mask) << (8 - shift);
	}

	if (size) {
		unsigned int end = shift + size;
		if (end < 8)
			mask = ~(~0 << size) << shift;

		*seq |= (*entry++ & mask) >> shift;

		if (end > 8) {
			mask >>= 8 - size; // will not shift zeroes in
			*seq |= (*entry & ~mask) << (8 - shift);
		}
	}

	return tmp;
}
