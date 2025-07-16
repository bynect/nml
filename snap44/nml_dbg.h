#ifndef NML_DBG_H
#define NML_DBG_H

#include <stdint.h>
#include <stddef.h>

#include "nml_mac.h"
#include "nml_str.h"

typedef struct {
	Sized_Str name;
	Sized_Str src;
} Source;

typedef struct {
	uint16_t line;
	uint16_t col;
	const Source *src;
} Location;

static MAC_CONST inline Location
location_new(uint16_t line, uint16_t col, const Source *src)
{
	return (Location) {
		.line = line,
		.col = col,
		.src = src,
	};
}

#endif
