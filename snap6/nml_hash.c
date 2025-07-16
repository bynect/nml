#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "nml_hash.h"
#include "nml_mac.h"

#define HASH_BYTES64		sizeof(Hash_64)
#define HASH_BYTES32		sizeof(Hash_32)

#define HASH_DATALEN64	32
#define HASH_DATALEN32	16

#define HASH_ALIGN64	7
#define HASH_ALIGN32	3
#define HASH_ALIGNED	0

#define HASH_ROTL1		1
#define HASH_ROTL2		7
#define HASH_ROTL3		12
#define HASH_ROTL4		18

#define HASH_ROUND64	31
#define HASH_ROUND32	13

#define HASH_AV64_1		33
#define HASH_AV64_2		29
#define HASH_AV64_3		32

#define HASH_AV32_1		15
#define HASH_AV32_2		13
#define HASH_AV32_3		16

#define HASH_PROC64_1	27
#define HASH_PROC64_2	23
#define HASH_PROC64_3	11

#define HASH_PROC32_1	17
#define HASH_PROC32_2	11

#define HASH_PRIME64_1 	11400714785074694791ull
#define HASH_PRIME64_2	14029467366897019727ull
#define HASH_PRIME64_3	1609587929392839161ull
#define HASH_PRIME64_4	9650029242287828579ull
#define HASH_PRIME64_5	2870177450012600261ull

#define HASH_PRIME32_1 	2654435761u
#define HASH_PRIME32_2	2246822519u
#define HASH_PRIME32_3	3266489917u
#define HASH_PRIME32_4	668265263u
#define HASH_PRIME32_5	374761393u

#ifdef __has_builtin
#if __has_builtin(__builtin_bswap64)
#define hash_swap64		__builtin_bswap64
#endif

#if __has_builtin(__builtin_rotateleft64)
#define hash_rotl64		__builtin_rotateleft64
#endif

#if __has_builtin(__builtin_bswap32)
#define hash_swap32		__builtin_bswap32
#endif

#if __has_builtin(__builtin_rotateleft32)
#define hash_rotl32		__builtin_rotateleft32
#endif
#endif

#ifndef hash_swap64
static MAC_INLINE inline Hash_64
hash_swap64(Hash_64 hsh)
{
	return ((hsh << 56) & 0xff00000000000000ull) |
			((hsh << 40) & 0x00ff000000000000ull) |
			((hsh << 24) & 0x0000ff0000000000ull) |
			((hsh << 8) & 0x000000ff00000000ull) |
			((hsh >> 8) & 0x00000000ff000000ull) |
			((hsh >> 24) & 0x0000000000ff0000ull) |
			((hsh >> 40) & 0x000000000000ff00ull) |
			((hsh >> 56) & 0x00000000000000ffull);
}
#endif

#ifndef hash_rotl64
static MAC_INLINE inline Hash_64
hash_rotl64(Hash_64 hsh, uint32_t bits)
{
	return (hsh << bits) | (hsh >> (64 - bits));
}
#endif

static MAC_INLINE inline Hash_64
hash_fetch64(const uint8_t *data, int_fast32_t align)
{
	if (MAC_LIKELY(align == HASH_ALIGNED))
	{
#ifdef MAC_LE
		return *(const Hash_64 *)data;
#else
		return hash_swap64(*(const Hash_64 *)data);
#endif
	}
	else
	{
		Hash_64 hsh;
		memcpy(&hsh, data, 8);
#ifdef MAC_LE
		return hsh;
#else
		return hash_swap64(hsh);
#endif
	}
}

static MAC_INLINE inline Hash_64
hash_avalanche64(Hash_64 hsh)
{
	hsh ^= hsh >> HASH_AV64_1;
	hsh *= HASH_PRIME64_2;
	hsh ^= hsh >> HASH_AV64_2;
	hsh *= HASH_PRIME64_3;
	hsh ^= hsh >> HASH_AV64_3;
	return hsh;
}

static Hash_64
hash_round64(Hash_64 hsh, Hash_64 next)
{
	hsh += next * HASH_PRIME64_2;
	hsh = hash_rotl64(hsh, HASH_ROUND64);
	hsh *= HASH_PRIME64_1;
	return hsh;
}

static Hash_64
hash_round64_merge(Hash_64 hsh, Hash_64 next)
{
	hsh ^= hash_round64(0, next);
	hsh = hsh * HASH_PRIME64_1 + HASH_PRIME64_4;
	return hsh;
}

#ifndef hash_rotl32
static MAC_HOT MAC_INLINE inline Hash_32
hash_rotl32(Hash_32 hsh, uint32_t bits)
{
	return (hsh << bits) | (hsh >> (32 - bits));
}
#endif

#ifndef hash_swap32
static MAC_INLINE inline Hash_32
hash_swap32(Hash_32 hsh)
{
	return ((hsh << 24) & 0xff000000) |
			((hsh << 8) & 0x00ff0000) |
			((hsh << 8) & 0x0000ff00) |
			((hsh << 24) & 0x000000ff);
}
#endif

static MAC_INLINE inline Hash_32
hash_fetch32(const uint8_t *data, int_fast32_t align)
{
	if (MAC_LIKELY(align == HASH_ALIGNED))
	{
#ifdef MAC_LE
		return *(const Hash_32 *)data;
#else
		return hash_swap32(*(const Hash_32 *)data);
#endif
	}
	else
	{
		Hash_32 hsh;
		memcpy(&hsh, data, 4);
#ifdef MAC_LE
		return hsh;
#else
		return hash_swap32(hsh);
#endif
	}
}

static MAC_INLINE inline Hash_32
hash_avalanche32(Hash_32 hsh)
{
	hsh ^= hsh >> HASH_AV32_1;
	hsh *= HASH_PRIME32_2;
	hsh ^= hsh >> HASH_AV32_2;
	hsh *= HASH_PRIME32_3;
	hsh ^= hsh >> HASH_AV32_3;
	return hsh;
}

static Hash_32
hash_round32(Hash_32 hsh, Hash_32 next)
{
	hsh += next * HASH_PRIME32_2;
	hsh = hash_rotl32(hsh, HASH_ROUND32);
	hsh *= HASH_PRIME32_1;
	return hsh;
}

MAC_HOT Hash_64
hash_xxh64(const uint8_t *data, size_t len, Hash_64 seed)
{
	Hash_64 hsh;
	const int_fast32_t align = (uintptr_t)data & HASH_ALIGN64;

	if (len >= HASH_DATALEN64)
	{
		const uint8_t *const end = data + len - HASH_DATALEN64;

		Hash_64 st1 = seed + HASH_PRIME64_1 + HASH_PRIME64_2;
		Hash_64 st2 = seed + HASH_PRIME64_2;
		Hash_64 st3 = seed;
		Hash_64 st4 = seed - HASH_PRIME64_1;

		do
		{
			st1 = hash_round64(st1, hash_fetch64(data, align));
			data += HASH_BYTES64;
			st2 = hash_round64(st2, hash_fetch64(data, align));
			data += HASH_BYTES64;
			st3 = hash_round64(st3, hash_fetch64(data, align));
			data += HASH_BYTES64;
			st4 = hash_round64(st4, hash_fetch64(data, align));
			data += HASH_BYTES64;
		}
		while (data <= end);

		hsh = hash_rotl64(st1, HASH_ROTL1) + hash_rotl64(st2, HASH_ROTL2) +
				hash_rotl64(st3, HASH_ROTL3) + hash_rotl64(st4, HASH_ROTL4);

		hsh = hash_round64_merge(hsh, st1);
		hsh = hash_round64_merge(hsh, st2);
		hsh = hash_round64_merge(hsh, st3);
		hsh = hash_round64_merge(hsh, st4);
	}
	else
	{
		hsh = seed + HASH_PRIME64_5;
	}

	hsh += len;
	len &= (HASH_DATALEN64 - 1);

	while (len >= 8)
	{
		hsh ^= hash_round64(0, hash_fetch64(data, align));
		hsh = hash_rotl64(hsh, HASH_PROC64_1) * HASH_PRIME64_1 + HASH_PRIME64_4;

		data += HASH_BYTES64;
		len -= HASH_BYTES64;
	}

	while (len >= 4)
	{
		hsh ^= (Hash_64)(hash_fetch32(data, align)) * HASH_PRIME64_1;
		hsh = hash_rotl64(hsh, HASH_PROC64_2) * HASH_PRIME64_2 + HASH_PRIME64_3;

		data += HASH_BYTES32;
		len -= HASH_BYTES32;
	}

	while (len > 0)
	{
		hsh ^= (*data++) * HASH_PRIME64_5;
		hsh = hash_rotl64(hsh, HASH_PROC64_3) * HASH_PRIME64_1;
		--len;
	}
	return hash_avalanche64(hsh);
}

MAC_HOT Hash_32
hash_xxh32(const uint8_t *data, size_t len, Hash_32 seed)
{
	Hash_32 hsh;
	const int_fast32_t align = (uintptr_t)data & HASH_ALIGN32;

	if (len >= HASH_DATALEN32)
	{
		const uint8_t *const end = data + len - HASH_DATALEN32;

		Hash_32 st1 = seed + HASH_PRIME32_1 + HASH_PRIME32_2;
		Hash_32 st2 = seed + HASH_PRIME32_2;
		Hash_32 st3 = seed;
		Hash_32 st4 = seed - HASH_PRIME32_1;

		do
		{
			st1 = hash_round32(st1, hash_fetch32(data, align));
			data += HASH_BYTES32;
			st2 = hash_round32(st2, hash_fetch32(data, align));
			data += HASH_BYTES32;
			st3 = hash_round32(st3, hash_fetch32(data, align));
			data += HASH_BYTES32;
			st4 = hash_round32(st4, hash_fetch32(data, align));
			data += HASH_BYTES32;
		}
		while (data <= end);

		hsh = hash_rotl32(st1, HASH_ROTL1) + hash_rotl32(st2, HASH_ROTL2) +
				hash_rotl32(st3, HASH_ROTL3) + hash_rotl32(st4, HASH_ROTL4);
	}
	else
	{
		hsh = seed + HASH_PRIME32_5;
	}

	hsh += len;
	len &= (HASH_DATALEN32 - 1);

	while (len >= 4)
	{
		hsh += hash_fetch32(data, align) * HASH_PRIME32_3;
		hsh = hash_rotl32(hsh, HASH_PROC32_1) * HASH_PRIME32_4;

		data += HASH_BYTES32;
		len -= HASH_BYTES32;
	}

	while (len > 0)
	{
		hsh += (*data++) * HASH_PRIME32_5;
		hsh = hash_rotl32(hsh, HASH_PROC32_2) * HASH_PRIME32_1;
		--len;
	}
	return hash_avalanche32(hsh);
}
