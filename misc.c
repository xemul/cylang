#include <time.h>
#include "cy.h"

static int eval_random(struct cy_token *t, struct cy_file *f)
{
	t->v.t = CY_V_NUMBER;
	t->v.v_i = random();
	return 1;
}

static struct cy_type cy_random = { .ts = "random number", .eval = eval_random, };
static struct cy_type cy_const	= { .ts = "constant", };

int try_resolve_cvalue(struct cy_token *t)
{
	const char *val = t->val;

	if (val[1] == '+' && val[2] == '\0') {
		t->typ = &cy_const;
		t->v.t = CY_V_BOOL;
		t->v.v_bool = true;
		return 1;
	}

	if (val[1] == '-' && val[2] == '\0') {
		t->typ = &cy_const;
		t->v.t = CY_V_BOOL;
		t->v.v_bool = false;
		return 1;
	}

	if (val[1] == '?' && val[2] == '\0') {
		t->typ = &cy_random;
		return 1;
	}

	return 0;
}

static int eval_in(struct cy_token *t, struct cy_file *f)
{
	struct cy_token it, ct;

	if (cy_eval_next(f, &it) <= 0)
		return -1;
	if (cy_eval_next(f, &ct) <= 0)
		return -1;

	t->v.t = CY_V_BOOL;

	if (it.v.t == CY_V_LIST) {
		t->v.v_bool = check_in_list(it.v.v_list, &ct.v);
		return 1;
	}

	if (it.v.t == CY_V_MAP) {
		;
		/*
		 * What to check in map? Key or value? Py does check key,
		 * but that's equivalent to just dereferencing the map
		 * element. Also, checking value makes it similar to
		 * checking the list, by re-using the cy_compare() thing.
		 */
	}

	show_token_err(t, "Bad object %s to check value in", vtype2s(it.v.t));
	return -1;
}

static int eval_sizeof(struct cy_token *t, struct cy_file *f)
{
	struct cy_token at;

	if (cy_eval_next(f, &at) <= 0)
		return -1;

	t->v.t = CY_V_NUMBER;

	if (at.v.t == CY_V_STRING) {
		t->v.v_i = strlen(at.v.v_str);
		return 1;
	}

	if (at.v.t == CY_V_LIST) {
		struct cy_list_value *lv;

		t->v.v_i = 0;
		list_for_each_entry(lv, &at.v.v_list->h, l)
			t->v.v_i++;

		return 1;
	}

	if (at.v.t == CY_V_MAP) {
		struct rb_node *n;

		t->v.v_i = 0;
		for (n = rb_first(&at.v.v_map->r); n; n = rb_next(n))
			t->v.v_i++;

		return 1;
	}

	show_token_err(t, "Can't get size of a %s", vtype2s(at.v.t));
	return -1;
}

static int eval_default_value(struct cy_token *t, struct cy_file *f)
{
	struct cy_token a, b;

	if (cy_eval_next(f, &a) <= 0)
		return -1;
	if (cy_eval_next(f, &b) <= 0)
		return -1;

	t->v = (a.v.t != CY_V_NOVALUE ? a.v : b.v);
	return 1;
}

static bool is_empty_value(struct cy_value *v)
{
	switch (v->t) {
	case CY_V_NOVALUE:
		return true;
	case CY_V_BOOL:
		return !v->v_bool;
	case CY_V_LIST:
		return list_empty(&v->v_list->h);
	case CY_V_MAP:
		return RB_EMPTY_ROOT(&v->v_map->r);
	case CY_V_STRING:
		return v->v_str[0] == '\0';
	case CY_V_NUMBER:
		return v->v_i == 0;
	}

	return false;
}

static int eval_empty_is_novalue(struct cy_token *t, struct cy_file *f)
{
	struct cy_token vt;

	if (cy_eval_next(f, &vt) <= 0)
		return -1;

	if (!is_empty_value(&vt.v))
		t->v = vt.v;

	return 1;
}

static int eval_eval(struct cy_token *t, struct cy_file *f)
{
	struct cy_token st;

	if (cy_eval_next_x(f, &st, CY_V_STRING) <= 0)
		return -1;

	st.val = st.v.v_str;
	st.v.t = CY_V_NOVALUE;

	if (cy_resolve(&st) < 0)
		return -1;

	if (st.typ->eval && st.typ->eval(&st, f) <= 0)
		return -1;

	t->v = st.v;
	return 1;
}

static struct cy_command cmd_misc[] = {
	{ .name = "@",  { .ts = "in", .eval = eval_in, }, },
	{ .name = "$", .t = { .ts = "sizeof", .eval = eval_sizeof, }, },

	{ .name = ".|", .t = { .ts = "default value", .eval = eval_default_value, }, },
	{ .name = "-.", .t = { .ts = "no empty", .eval = eval_empty_is_novalue, }, },

	{ .name = "`", .t = { .ts = "eval token", .eval = eval_eval, }, },
	{}
};

void init_misc(void)
{
	srand(time(NULL));
	add_commands(cmd_misc);
}
