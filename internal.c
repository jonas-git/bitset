#define bitset_internal_bytes(bits) \
	((((bits) - 1) >> 3) + 1)

#define bitset_internal_capacity(bytes) \
	((bytes) << 3)

#define bitset_internal_alloc(clear, num, size) \
	((clear) ? calloc(num, size) : malloc((num) * (size)))
