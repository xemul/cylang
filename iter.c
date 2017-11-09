#include "cy.h"

static int map_list(struct cy_token *t, struct cy_token *lht, struct cy_file *f)
{
	struct cy_list_value *lv;
	struct cy_ctoken *cur;

	t->v.t = CY_V_LIST;
	t->v.v_list = malloc(sizeof(struct list_head));
	INIT_LIST_HEAD(&t->v.v_list->h);

	cur = f->nxt;
	list_for_each_entry(lv, &lht->v.v_list->h, l) {
		struct cy_token nt;
		struct cy_list_value *nv;

		set_cursor(&lv->v);
		f->nxt = cur;
		if (cy_eval_next(f, &nt) <= 0)
			return -1;

		nv = malloc(sizeof(*lv));
		nv->v = nt.v;
		list_add_tail(&nv->l, &t->v.v_list->h);
	}

	set_cursor(NULL);

	return 1;
}

static int map_map(struct cy_token *t, struct cy_token *mt, struct cy_file *f)
{
	struct rb_node *n;
	struct cy_ctoken *cur;

	t->v.t = CY_V_MAP;
	t->v.v_map = malloc(sizeof(struct rb_root));
	t->v.v_map->r = RB_ROOT;

	cur = f->nxt;
	for (n = rb_first(&mt->v.v_map->r); n != NULL; n = rb_next(n)) {
		struct cy_map_value *mv;
		struct cy_token nt;

		mv = rb_entry(n, struct cy_map_value, n);
		set_cursor(&mv->v);
		f->nxt = cur;
		if (cy_eval_next(f, &nt) <= 0)
			return -1;

		if (map_add_value(mv->key, &nt.v, &t->v.v_map->r, false) <= 0)
			return -1;
	}

	set_cursor(NULL);

	return 1;
}

static int eval_map(struct cy_token *t, struct cy_file *f)
{
	struct cy_token st;

	if (cy_eval_next(f, &st) <= 0)
		return -1;

	if (st.v.t == CY_V_LIST)
		return map_list(t, &st, f);
	if (st.v.t == CY_V_MAP)
		return map_map(t, &st, f);

	show_token_err(t, "Bad object %s to map", vtype2s(st.v.t));
	return -1;
}

static int filter_list(struct cy_token *t, struct cy_token *lht, struct cy_file *f)
{
	struct cy_list_value *lv;
	struct cy_ctoken *cur;

	t->v.t = CY_V_LIST;
	t->v.v_list = malloc(sizeof(struct list_head));
	INIT_LIST_HEAD(&t->v.v_list->h);

	cur = f->nxt;
	list_for_each_entry(lv, &lht->v.v_list->h, l) {
		struct cy_token ft;

		set_cursor(&lv->v);

		f->nxt = cur;
		if (cy_eval_next_x(f, &ft, CY_V_BOOL) <= 0)
			return -1;

		if (ft.v.v_bool) {
			struct cy_list_value *nv;

			nv = malloc(sizeof(*lv));
			nv->v = lv->v;
			list_add_tail(&nv->l, &t->v.v_list->h);
		}
	}

	set_cursor(NULL);

	return 1;
}

static int eval_filter(struct cy_token *t, struct cy_file *f)
{
	struct cy_token st;

	if (cy_eval_next(f, &st) <= 0)
		return -1;

	if (st.v.t == CY_V_LIST)
		return filter_list(t, &st, f);

	show_token_err(t, "Bad object %s to filter", st.v.t);
	return -1;

}

static struct cy_command cmd_iter[] = {
	{ .name = "|~", .t = { .ts = "map collection", .eval = eval_map, }, },
	{ .name = "|:", .t = { .ts = "filter collection", .eval = eval_filter, }, },
};

void init_iter(void)
{
	add_commands(cmd_iter);
}
