#include "cy.h"

#define OP_EQ	1
#define OP_NE	2
#define OP_GE	3
#define OP_GT	4
#define OP_LE	5
#define OP_LT	6

static int cy_compare_t(struct cy_value *v1, struct cy_value *v2, int *cr)
{
	switch (v1->t) {
	case CY_V_NUMBER:
		*cr = v1->v_i - v2->v_i;
		return 0;
	case CY_V_STRING:
		*cr = strcmp(v1->v_str, v2->v_str);
		return 0;
	default:
		return -1;
	}
}

bool cy_compare(struct cy_value *v1, struct cy_value *v2)
{
	int cr;

	if (v1->t != v2->t)
		return false;

	if (cy_compare_t(v1, v2, &cr) < 0)
		return false;

	return cr == 0 ? true : false;
}

static int eval_compare(struct cy_token *t, struct cy_file *f)
{
	int cr;
	struct cy_token a, b;

	if (cy_eval_next(f, &a) <= 0)
		return -1;
	if (cy_eval_next_x(f, &b, a.v.t) <= 0)
		return -1;

	t->v.t = CY_V_BOOL;
	if (cy_compare_t(&a.v, &b.v, &cr) < 0) {
		show_token_err(t, "Comparison of %ss not possible", vtype2s(a.v.t));
		return -1;
	}

	switch (t->typ->priv) {
	case OP_EQ: t->v.v_bool = (cr == 0); break;
	case OP_NE: t->v.v_bool = (cr != 0); break;
	case OP_GT: t->v.v_bool = (cr >  0); break;
	case OP_GE: t->v.v_bool = (cr >= 0); break;
	case OP_LT: t->v.v_bool = (cr <  0); break;
	case OP_LE: t->v.v_bool = (cr <= 0); break;
	}

	return 1;
}

static int get_branch(struct cy_token *t, struct cy_file *f)
{
	if (cy_eval_next(f, t) <= 0)
		return -1;

	if ((t->v.t != CY_V_CBLOCK) && !is_nop_token(t)) {
		show_token_err(t, "Expected cblock/nop branch, got %s", vtype2s(t->v.t));
		return -1;
	}

	return 1;
}

static int call_branch(struct cy_token *t, struct cy_file *f, struct cy_value *rv)
{
	if (is_nop_token(t))
		return 1;
	else
		return cy_call_cblock(t, f, rv);
}

static int eval_if(struct cy_token *t, struct cy_file *f)
{
	int ret;
	struct cy_token t_cond, t_then, t_else;

	if (cy_eval_next_x(f, &t_cond, CY_V_BOOL) <= 0)
		return -1;

	if (get_branch(&t_then, f) <= 0)
		return -1;

	if (get_branch(&t_else, f) <= 0)
		return -1;

	if (t_cond.v.v_bool)
		ret = call_branch(&t_then, f, &t->v);
	else
		ret = call_branch(&t_else, f, &t->v);

	if (ret < 0)
		return ret;

	return 1;
}

static int cy_call_sblock(struct cy_token *ct, struct cy_file *f, struct cy_value *rv)
{
	int ret;
	struct cy_cblock *c_list;
	struct cy_ctoken *c_nxt;

	c_list = f->main;
	c_nxt = f->nxt;
	init_tokenizer(f, ct->v.v_cblk);

	while (1) {
		struct cy_token ct = {};
		struct cy_token bt = {};

		ret = cy_eval_next_x(f, &ct, CY_V_BOOL);
		if (ret <= 0)
			break;

		ret = get_branch(&bt, f);
		if (ret <= 0)
			break;

		if (ct.v.v_bool) {
			ret = call_branch(&bt, f, rv);
			break;
		}
	}

	f->main = c_list;
	f->nxt = c_nxt;

	if (ret < 0)
		return ret;

	return 1;
}

static int eval_select(struct cy_token *t, struct cy_file *f)
{
	struct cy_token sbt;

	if (cy_eval_next_x(f, &sbt, CY_V_CBLOCK) <= 0)
		return -1;

	return cy_call_sblock(&sbt, f, &t->v);
}

static int eval_list_loop(struct cy_token *t, struct cy_token *lt, struct cy_token *bt, struct cy_file *f)
{
	int ret;
	struct cy_list_value *lv;

	list_for_each_entry(lv, &lt->v.v_list->h, l) {
		struct cy_value rv = {};

		set_cursor(&lv->v);
		ret = call_branch(bt, f, &rv);
		if (ret < 0)
			return ret;

		if (ret == 2) {
			t->v = rv;
			break;
		}

		/* XXX if someone removes an element from the list while
		 * we iterate it :(
		 */
		if (list_empty(&lv->l))
			break;
	}
	set_cursor(NULL);

	return 1;
}

static int eval_map_loop(struct cy_token *t, struct cy_token *mt, struct cy_token *bt, struct cy_file *f)
{
	int ret;
	struct rb_node *n;

	for (n = rb_first(&mt->v.v_map->r); n != NULL; n = rb_next(n)) {
		struct cy_map_value *mv;
		struct cy_value rv = {};
		struct cy_value cv = { .t = CY_V_STRING, };

		mv = rb_entry(n, struct cy_map_value, n);
		cv.v_str = mv->key;
		set_cursor(&cv);
		ret = call_branch(bt, f, &rv);
		if (ret < 0)
			return ret;

		if (ret == 2) {
			t->v = rv;
			break;
		}
	}
	set_cursor(NULL);

	return 1;
}

static int eval_cblock_loop(struct cy_token *t, struct cy_token *ct, struct cy_token *bt, struct cy_file *f)
{
	int ret;

	while (1) {
		struct cy_value rv = {};

		ret = cy_call_cblock(ct, f, &rv);
		if (ret <= 0)
			return -1;

		if (ret != 2 || rv.t == CY_V_NOVALUE)
			break;

		set_cursor(&rv);
		ret = call_branch(bt, f, &rv);
		if (ret < 0)
			return ret;

		if (ret == 2) {
			t->v = rv;
			break;
		}
	}
	set_cursor(NULL);

	return 1;
}

static int eval_nuber_loop(struct cy_token *t, struct cy_token *nt, struct cy_token *bt, struct cy_file *f)
{
	int ret, start, stop, inc;
	struct cy_value lc = { .t = CY_V_NUMBER, };
	struct cy_value rv = {};

	if (nt->v.v_i > 0) {
		start = 0;
		stop = nt->v.v_i;
		inc = 1;
	} else if (nt->v.v_i < 0) {
		start = -nt->v.v_i;
		stop = 0;
		inc = -1;
	} else
		return 1;

	for (lc.v_i = start; lc.v_i != stop; lc.v_i += inc) {
		set_cursor(&lc);
		ret = call_branch(bt, f, &rv);
		if (ret < 0)
			return ret;
		if (ret == 2) {
			t->v = rv;
			break;
		}
	}

	return 1;
}

static int eval_loop(struct cy_token *t, struct cy_file *f)
{
	struct cy_token lt, bt;

	if (cy_eval_next(f, &lt) <= 0)
		return -1;

	if (get_branch(&bt, f) <= 0)
		return -1;

	switch (lt.v.t) {
	case CY_V_LIST:
		return eval_list_loop(t, &lt, &bt, f);
	case CY_V_MAP:
		return eval_map_loop(t, &lt, &bt, f);
	case CY_V_CBLOCK:
		return eval_cblock_loop(t, &lt, &bt, f);
	case CY_V_NUMBER:
		return eval_nuber_loop(t, &lt, &bt, f);
	}

	show_token_err(t, "Bad loop object");
	return -1;
}

static struct cy_command cmd_compare[] = {
	/* Checks */
	{ .name = "=",  { .ts = "eq", .eval = eval_compare, .priv = OP_EQ, }, },
	{ .name = "!=", { .ts = "ne", .eval = eval_compare, .priv = OP_NE, }, },
	{ .name = ">",  { .ts = "gt", .eval = eval_compare, .priv = OP_GT, }, },
	{ .name = ">=", { .ts = "ge", .eval = eval_compare, .priv = OP_GE, }, },
	{ .name = "<",  { .ts = "lt", .eval = eval_compare, .priv = OP_LT, }, },
	{ .name = "<=", { .ts = "le", .eval = eval_compare, .priv = OP_LE, }, },

	/* If-s */
	{ .name = "?",  { .ts = "if", .eval = eval_if, }, },
	{ .name = "??", { .ts = "select", .eval = eval_select, }, },

	/* Loops */
	{ .name = "~",  { .ts = "loop", .eval = eval_loop, }, },
	{},
};

void init_cond(void)
{
	add_commands(cmd_compare);
}
