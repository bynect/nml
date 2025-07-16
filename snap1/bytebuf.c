#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "bytebuf.h"
#include "misc.h"

#define GROW_BUFIZE(size) \
	(size == 0 ? 8 : size * 2)

void
bytebuf_init(Byte_Buf *buf)
{
	buf->bytes = NULL;
	buf->len = 0;
	buf->size = 0;
}

void
bytebuf_free(Byte_Buf *buf)
{
	free(buf->bytes);
}

void
bytebuf_write(Byte_Buf *buf, uint8_t byte)
{
	if (buf->len + 1 >= buf->size)
	{
		buf->size = GROW_BUFSIZE(buf->size);
		buf->bytes = realloc(buf->bytes, buf->size);
	}

	buf->bytes[buf->len++] = byte;
}

void
bytebuf_writes(Byte_Buf *buf, const uint8_t *const bytes, const size_t len)
{
	if (buf->len + len >= buf->size)
	{
		const uint64_t plen = misc_nextpow2(len);
		const size_t size = GROW_BUFSIZE(buf->size);
		buf->size = buf->size + len > size ? buf->size + plen : size;
		buf->bytes = realloc(buf->bytes, buf->size);
	}

	memcpy(buf->bytes + buf->len, bytes, len);
	buf->len += len;
}
