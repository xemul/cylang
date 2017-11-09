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

static int eval_map(struct cy_token *t, struct cy_file *f)
{
	struct cy_token st;

	if (cy_eval_next(f, &st) <= 0)
		return -1;

	if (st.v.t == CY_V_LIST)
		return map_list(t, &st, f);

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
	{ .name = "|~", .t = { .ts = "map", .eval = eval_map, }, },
	{ .name = "|:", .t = { .ts = "filter", .eval = eval_filter, }, },
};

void init_iter(void)
{
	add_commands(cmd_iter);
}
