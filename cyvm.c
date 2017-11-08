#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include "cy.h"

void show_token_err(struct cy_token *t, const char *fmt, ...)
{
	va_list args;

	fprintf(stderr, "Error @%d.%d: ", t->ln, t->tn);

	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);

	fprintf(stderr, "\n");
}

char *vtype2s(unsigned int t)
{
	switch (t) {
	case CY_V_NUMBER:
		return "interer";
	case CY_V_STRING:
		return "string";
	case CY_V_LIST:
		return "list";
	case CY_V_MAP:
		return "map";
	case CY_V_BOOL:
		return "bool";
	case CY_V_CBLOCK:
		return "cblock";
	case CY_V_NOVALUE:
		return "novalue";
	case CY_V_CBLOCK_END:
	case CY_V_LIST_END:
	case CY_V_MAP_END:
		return "terminator";
	default:
		return "unknown";
	}
}

static struct cy_list args_list = {
	.h = LIST_HEAD_INIT(args_list.h),
};

static struct cy_value args = {
	.t = CY_V_LIST,
	.v_list = &args_list,
};

static void init_args(int argc, char **argv)
{
	int i;

	for (i = 0; i < argc; i++) {
		struct cy_list_value *lv;

		lv = malloc(sizeof(*lv));
		lv->v.t = CY_V_STRING;
		lv->v.v_str = argv[i];
		list_add_tail(&lv->l, &args_list.h);
	}

	map_add_value("Args", &args, &symbols.v_map->r, true);
}

static void init_envs(char **env)
{
}

int main(int argc, char **argv, char **env)
{
	struct cy_file *cf;
	struct cy_token mt;
	struct cy_value rv = {};

	init_resolver();

	if (argc < 2)
		goto usage;

	if (!strcmp(argv[1], "-c")) {
		show_commands();
		return 0;
	}

	if (!strcmp(argv[1], "-r")) {
		if (argc < 3)
			goto usage;

		init_args(argc - 2, argv + 2);
		init_envs(env);

		cf = cy_open(argv[2], &mt, false);
		if (!cf)
			return -1;

		return cy_call_cblock(&mt, cf, &rv) >= 0 ? 0 : 1;
	}

	if (!strcmp(argv[1], "-t")) {
		if (argc < 3)
			goto usage;

		cf = cy_open(argv[2], &mt, true);
		if (!cf)
			return -1;

		return 0;
	}

usage:
	fprintf(stderr, "Use -c, -r <filename> or -t <filename>\n");
	return 1;
}
