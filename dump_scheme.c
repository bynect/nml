
static MAC_INLINE inline void
dump_type_pretty(const Type type, uint32_t fun_depth, uint32_t tuple_depth, const Str_Tab *tab)
{
	const Type_Tag tag = TYPE_UNTAG(type);

	if (tag == TAG_FUN)
	{
		const Type_Fun *fun = TYPE_PTR(type);
		if (fun_depth > 0)
		{
			fprintf(stdout, "(");
		}

		dump_type_pretty(fun->par, fun_depth + 1, tuple_depth, tab);
		while (TYPE_UNTAG(fun->ret) == TAG_FUN)
		{
			fun = TYPE_PTR(fun->ret);
			fprintf(stdout, " -> ");
			dump_type_pretty(fun->par, fun_depth + 1, tuple_depth, tab);
		}

		fprintf(stdout, " -> ");
		dump_type_pretty(fun->ret, fun_depth + 1, tuple_depth, tab);
		if (fun_depth > 0)
		{
			fprintf(stdout, ")");
		}
	}
	else if (tag == TAG_TUPLE)
	{
		Type_Tuple *tuple = TYPE_PTR(type);
		if (tuple_depth > 0)
		{
			fprintf(stdout, "(");
		}

		for (size_t i = 0; i < tuple->len - 1; ++i)
		{
			dump_type_pretty(tuple->items[i], fun_depth + 1, tuple_depth + 1, tab);
			fprintf(stdout, " * ");
		}

		dump_type_pretty(tuple->items[tuple->len - 1], fun_depth + 1, tuple_depth + 1, tab);
		if (tuple_depth > 0)
		{
			fprintf(stdout, ")");
		}
	}
	else if (tag == TAG_VAR)
	{
		Type_Var *var = TYPE_PTR(type);
		Sized_Str name = var->var;

		if (tab != NULL)
		{
			str_tab_get(tab, var->var, &name);
		}
		fprintf(stdout, "%.*s", (uint32_t)name.len, name.str);
	}
	else
	{
		dump_tag_pretty(tag);
	}
}

void
dump_type_tag(const Type_Tag tag)
{
	dump_tag_pretty(tag);
	fprintf(stdout, "\n");
}

void
dump_type(const Type type)
{
	dump_type_pretty(type, 0, 0, NULL);
	fprintf(stdout, "\n");
}

static MAC_INLINE inline Sized_Str
dump_pretty_var(uint32_t i)
{
	static const char alpha[] = "abcdefghijklmnopqrstuvwxyz";
	return STR_WLEN(alpha + (i % sizeof(alpha)), 1);
}

void
dump_type_scheme(const Type_Scheme scheme)
{
	if (scheme.len != 0)
	{
		Str_Tab tab;
		str_tab_init(&tab, (Hash_Default)&scheme);

		fprintf(stdout, "forall");
		for (size_t i = 0; i < scheme.len; ++i)
		{
			Sized_Str var;
			if (i <= 25)
			{
				var = dump_pretty_var(i);
				str_tab_insert(&tab, scheme.vars[i], var);
			}
			fprintf(stdout, " %.*s", (uint32_t)var.len, var.str);
		}
		fprintf(stdout, ". ");

		dump_type_pretty(scheme.type, 0, 0, &tab);
		str_tab_free(&tab);
		fprintf(stdout, "\n");
	}
	else
	{
		dump_type(scheme.type);
	}
}
