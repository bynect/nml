#ifndef BYTEBUF_H
#define BYTEBUF_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
	uint8_t *bytes;
	size_t len;
	size_t size;
} Byte_Buf;

void bytebuf_init(Byte_Buf *buf);

void bytebuf_free(Byte_Buf *buf);

void bytebuf_write(Byte_Buf *buf, uint8_t byte);

void bytebuf_writes(Byte_Buf *buf, const uint8_t *const bytes, const size_t len);

#endif
