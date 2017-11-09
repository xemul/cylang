#include "cy.h"

static int show_list(struct list_head *h, char tc, struct cy_token *);
static int show_map(struct rb_root *r, char tc, struct cy_token *);

static int show_value(struct cy_value *v, char tc, struct cy_token *st)
{
	switch (v->t) {
	default:
		show_token_err(st, "Can't show %s", vtype2s(v->t));
		return -1;
	case CY_V_NOVALUE:
		printf("NOVALUE%c", tc);
		break;
	case CY_V_NUMBER:
		printf("%li%c", v->v_i, tc);
		break;
	case CY_V_STRING:
		printf("\"%s\"%c", v->v_str, tc);
		break;
	case CY_V_LIST:
		show_list(&v->v_list->h, tc, st);
		break;
	case CY_V_MAP:
		show_map(&v->v_map->r, tc, st);
		break;
	case CY_V_BOOL:
		printf("%s%c", v->v_bool ? "TRUE" : "FALSE", tc);
		break;
	case CY_V_CBLOCK:
		printf("$CBLOCK%c", tc);
		break;
	}

	return 1;
}

static int show_list(struct list_head *h, char tc, struct cy_token *st)
{
	struct cy_list_value *lv;
	int ret;

	printf("(");
	list_for_each_entry(lv, h, l) {
		ret = show_value(&lv->v, ',', st);
		if (ret <= 0)
			return ret;
	}
	printf(")%c", tc);

	return 1;
}

static int show_map(struct rb_root *r, char tc, struct cy_token *st)
{
	struct rb_node *n;
	int ret;

	printf("{");
	for (n = rb_first(r); n != NULL; n = rb_next(n)) {
		struct cy_map_value *mv;

		mv = rb_entry(n, struct cy_map_value, n);
		printf("%s:", mv->key);
		ret = show_value(&mv->v, ',', st);
		if (ret <= 0)
			return ret;
	}
	printf("}%c", tc);

	return 1;
}

static int eval_show(struct cy_token *t, struct cy_file *f)
{
	struct cy_token st;

	if (cy_eval_next(f, &st) <= 0)
		return -1;

	return show_value(&st.v, '\n', t);
}

static int eval_print(struct cy_token *t, struct cy_file *f)
{
	struct cy_token st;

	if (cy_eval_next_x(f, &st, CY_V_STRING) <= 0)
		return -1;

	printf("%s\n", st.v.v_str);
	return 1;
}

static struct cy_command cmd_show[] = {
	{ .name = "``", .t = { .ts = "show", .eval = eval_show, }, },
	{ .name = "`", .t = { .ts = "print", .eval = eval_print, }, },
};

void init_show(void)
{
	add_commands(cmd_show);
}
