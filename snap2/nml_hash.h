#ifndef NML_HASH_H
#define NML_HASH_H

#include <stdint.h>
#include <stddef.h>

#include "nml_mac.h"

typedef uint64_t Hash_64;
typedef uint32_t Hash_32;

#ifdef MAC_BIT64
typedef Hash_64 Hash_Default;

#define HASH_DEFAULT(data, len, seed)	hash_xxh64(data, len, seed)
#else
typedef Hash_32 Hash_Default;

#define HASH_DEFAULT(data, len, seed)	hash_xxh32(data, len, seed)
#endif

MAC_HOT Hash_64 hash_xxh64(const uint8_t *data, size_t len, Hash_64 seed);

MAC_HOT Hash_32 hash_xxh32(const uint8_t *data, size_t len, Hash_32 seed);

#endif
