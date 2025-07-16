#ifndef NML_HASH_H
#define NML_HASH_H

#include <stdint.h>
#include <stddef.h>

#include "nml_mac.h"

typedef uint64_t Hash_64;
typedef uint32_t Hash_32;

#ifdef MAC_BIT64
typedef Hash_64 Hash_Default;
#else
typedef Hash_32 Hash_Default;
#endif


MAC_HOT Hash_64 hash_xxh64(const uint8_t *data, size_t len, Hash_64 seed);

MAC_HOT Hash_32 hash_xxh32(const uint8_t *data, size_t len, Hash_32 seed);

static MAC_INLINE inline Hash_Default
hash_default(const uint8_t *data, size_t len, Hash_Default seed)
{
#ifdef MAC_BIT64
	return hash_xxh64(data, len, seed);
#else
	return hash_xxh32(data, len, seed);
#endif
}

#endif
