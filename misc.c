#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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

static struct cy_command cmd_misc[] = {
	{ .name = "@",  { .ts = "in", .eval = eval_in, }, },
	{ .name = "$", .t = { .ts = "sizeof", .eval = eval_sizeof, }, },
	{ .name = ";", .t = { .ts = "default value", .eval = eval_default_value, }, },
};

void init_misc(void)
{
	srand(time(NULL));
	add_commands(cmd_misc);
}