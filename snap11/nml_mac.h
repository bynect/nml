#ifndef NML_MAC_H
#define NML_MAC_H

#if defined(__i386) || defined(__i386__) || defined(_M_IX86)
#define MAC_ARCHX86
#elif defined(__x86_64__) || defined(__x86_64) || defined(_M_X64) || defined(_M_AMD64)
#define MAC_ARCHX64
#define MAC_BIT64
#else
#include <stdint.h>

#if INTPTR_MAX == INT64_MAX
#define MAC_BIT64
#endif
#endif

#if (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__) || \
	defined(__BIG_ENDIAN__) || defined(__BIG_ENDIAN) || defined(_BIG_ENDIAN)
#define MAC_BE
#else
#define MAC_LE
#endif

#if defined(MAC_PREFETCH) && ( defined(MAC_ARCHX86) || defined(MAC_ARCHX64))
#include <emmintrin.h>

#define MAC_PREFETCH_ONCE(addr, _) \
							_mm_prefetch((char *)(addr), _MM_HINT_NTA)

#define MAC_PREFETCH_CACHE(addr, _) \
							_mm_prefetch((char *)(addr), _MM_HINT_T0)
#else
#define MAC_PREFETCH_ONCE(addr, rw) \
							do { } while (0)

#define MAC_PREFETCH_CACHE(addr, rw) \
							do { } while (0)
#endif

#ifdef __has_builtin
#if __has_builtin(__builtin_expect)
#define MAC_LIKELY(x) 		__builtin_expect(!!(x), 1)
#define MAC_UNLIKELY(x) 	__builtin_expect(!!(x), 0)
#endif

#if __has_builtin(__builtin_prefetch) && \
	!(defined(MAC_PREFETCH_ONCE) && defined(MAC_PREFETCH_CACHE))
#define MAC_PREFETCH_ONCE(addr, rw) \
							__builtin_prefetch((addr), rw, 0)

#define MAC_PREFETCH_CACHE(addr, rw) \
							__builtin_prefetch((addr), rw, 3)
#endif
#endif

#ifndef MAC_LIKELY
#define MAC_LIKELY(x)  		x
#endif

#ifndef MAC_UNLIKELY
#define MAC_UNLIKELY(x)		x
#endif

#ifdef __has_attribute
#if __has_attribute(cold)
#define MAC_COLD		__attribute__((cold))
#endif

#if __has_attribute(hot)
#define MAC_HOT			__attribute__((hot))
#endif

#if __has_attribute(const)
#define MAC_CONST		__attribute__((const))
#endif

#if __has_attribute(packed)
#define MAC_PACKED	__attribute__((packed))
#endif

#if __has_attribute(force_inline)
#define MAC_INLINE	__attribute__((force_inline))
#endif

#if __has_attribute(noinline)
#define MAC_NOINLINE	__attribute__((noinline))
#endif

#if __has_attribute(noreturn)
#define MAC_NORETURN	__attribute__((noreturn))
#endif

#if __has_attribute(returns_nonnull)
#define MAC_NORETNULL	__attribute__((returns_nonnull))
#endif
#endif

#ifndef MAC_COLD
#define MAC_COLD
#endif

#ifndef MAC_HOT
#define MAC_HOT
#endif

#ifndef MAC_CONST
#define MAC_CONST
#endif

#ifndef MAC_PACKED
#define MAC_PACKED
#endif

#ifndef MAC_INLINE
#define MAC_INLINE
#endif

#ifndef MAC_NOINLINE
#define MAC_NOINLINE
#endif

#ifndef MAC_NORETURN
#define MAC_NORETURN
#endif

#ifndef MAC_NORETNULL
#define MAC_NORETNULL
#endif

#define MAC_TOSTR(x) 		#x
#define MAC_CONCAT(a, b)	a##b
#define MAC_STRLEN(x)		(sizeof(x) - 1)
#define MAC_STRSTATIC(x)	(x "")
#define MAC_UNUSED(x)		(void) (x)

#if defined __STDC_VERSION__ && __STDC_VERSION__ >= 201112l
#define MAC_ASSERT(x, msg)	_Static_assert(x, msg)
#else
#define MAC_ASSERT(x, msg)
#endif

#ifdef __GNUC__
#define MAC_COMPUTED_GOTO
#endif

#endif
