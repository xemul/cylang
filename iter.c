#include "cy.h"

static int convert_list(struct cy_token *t, struct cy_token *lht, struct cy_token *bt, struct cy_file *f)
{
	struct cy_list_value *lv;

	t->v.t = CY_V_LIST;
	t->v.v_list = malloc(sizeof(struct list_head));
	INIT_LIST_HEAD(&t->v.v_list->h);

	list_for_each_entry(lv, &lht->v.v_list->h, l) {
		int ret;
		struct cy_value rv = {};
		struct cy_list_value *nv;

		set_cursor(&lv->v);
		ret = cy_call_cblock(bt, f, &rv);
		if (ret <= 0)
			return -1;

		if (ret == 2) {
			nv = malloc(sizeof(*lv));
			nv->v = rv;
			list_add_tail(&nv->l, &t->v.v_list->h);
		}
	}

	set_cursor(NULL);

	return 1;
}

static int convert_map(struct cy_token *t, struct cy_token *mt, struct cy_token *bt, struct cy_file *f)
{
	struct rb_node *n;

	t->v.t = CY_V_MAP;
	t->v.v_map = malloc(sizeof(struct rb_root));
	t->v.v_map->r = RB_ROOT;

	for (n = rb_first(&mt->v.v_map->r); n != NULL; n = rb_next(n)) {
		int ret;
		struct cy_value rv = {};
		struct cy_map_value *mv;

		mv = rb_entry(n, struct cy_map_value, n);
		set_cursor(&mv->v);
		ret = cy_call_cblock(bt, f, &rv);
		if (ret <= 0)
			return -1;

		if (ret == 2) {
			if (map_add_value(mv->key, &rv, &t->v.v_map->r, false) <= 0)
				return -1;
		}
	}

	set_cursor(NULL);

	return 1;
}

static int convert_cblock(struct cy_token *t, struct cy_token *gt, struct cy_token *bt, struct cy_file *f)
{
	int ret;

	t->v.t = CY_V_LIST;
	t->v.v_list = malloc(sizeof(struct list_head));
	INIT_LIST_HEAD(&t->v.v_list->h);

	while (1) {
		struct cy_value rv = {};

		ret = cy_call_cblock(gt, f, &rv);
		if (ret <= 0)
			return ret;

		if (ret != 2 || rv.t == CY_V_NOVALUE)
			break;

		set_cursor(&rv);
		ret = cy_call_cblock(bt, f, &rv);
		if (ret <= 0)
			return -1;

		if (ret == 2) {
			struct cy_list_value *nv;

			nv = malloc(sizeof(*nv));
			nv->v = rv;
			list_add_tail(&nv->l, &t->v.v_list->h);
		}
	}

	set_cursor(NULL);

	return 1;
}

static int eval_convert(struct cy_token *t, struct cy_file *f)
{
	struct cy_token st, bt;

	if (cy_eval_next(f, &st) <= 0)
		return -1;
	if (cy_eval_next_x(f, &bt, CY_V_CBLOCK) <= 0)
		return -1;

	switch (st.v.t) {
	case CY_V_LIST:
		return convert_list(t, &st, &bt, f);
	case CY_V_MAP:
		return convert_map(t, &st, &bt, f);
	case CY_V_CBLOCK:
		return convert_cblock(t, &st, &bt, f);
	}

	show_token_err(t, "Bad object %s to map", vtype2s(st.v.t));
	return -1;
}

static struct cy_command cmd_iter[] = {
	{ .name = "||", .t = { .ts = "map/filter collection", .eval = eval_convert, }, },
	{}
};

void init_iter(void)
{
	add_commands(cmd_iter);
}
