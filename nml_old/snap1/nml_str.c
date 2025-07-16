
Sized_Str
str_format(Arena *arena, const char *fmt, ...)
{
	va_list args, copy;
	va_start(args, fmt);
	va_copy(copy, args);

	size_t len = vsnprintf(NULL, 0, fmt, copy);
	char *str = arena_alloc(arena, len + 1);
	vsnprintf(str, len + 1, fmt, args);

	va_end(copy);
	va_end(args);
	return STR_WLEN(str, len);
}

