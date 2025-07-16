

	if (err->loc.line != 0)
	{
		Sized_Str src = err->loc.src->src;

		size_t start = 0;
		size_t end = 0;

		for (size_t i = 0, line = 1; i < src.len; ++i)
		{
			if (src.str[i] == '\n')
			{
				++line;
			}

			if (line == err->loc.line)
			{
				start = i;
				while (i < src.len && src.str[i] != '\n')
				{
					++i;
				}
				end = i;
				break;
			}
		}

		int32_t pad = fprintf(stdout, "%d | ", err->loc.line);
		dump_sized_str(STR_WLEN(src.str + start, end - start));
		fprintf(stdout, "%*s|%*s^\n", pad - 2, "",  err->loc.col, "");
	}
