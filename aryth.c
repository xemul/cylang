#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cy.h"

static int eval_2arg_math(struct cy_token *a, struct cy_token *b, struct cy_file *f)
{
	if (cy_eval_next(f, a) <= 0)
		return -1;
	return cy_eval_next_x(f, b, a->v.t);
}

static char *xstrcat(const char *s1, const char *s2)
{
	int l1, l2;
	char *ret;

	l1 = strlen(s1);
	l2 = strlen(s2);
	ret = malloc(l1 + l2 + 1);
	memcpy(ret, s1, l1);
	memcpy(ret + l1, s2, l2 + 1);

	return ret;
}

static int eval_add(struct cy_token *t, struct cy_file *f)
{
	struct cy_token a, b;

	if (eval_2arg_math(&a, &b, f) <= 0)
		return -1;

	if (a.v.t == CY_V_LIST) {
		t->v.t = CY_V_LIST;
		return cy_list_splice(&a.v, &b.v, &t->v);
	}

	if (a.v.t == CY_V_NUMBER) {
		t->v.t = CY_V_NUMBER;
		t->v.v_i = a.v.v_i + b.v.v_i;
		return 1;
	}

	if (a.v.t == CY_V_STRING) {
		t->v.t = CY_V_STRING;
		t->v.v_str = xstrcat(a.v.v_str, b.v.v_str);
		return 1;
	}

	show_token_err(t, "Bad op for add");
	return -1;
}

static int eval_sub(struct cy_token *t, struct cy_file *f)
{
	struct cy_token a, b;

	if (cy_eval_next(f, &a) <= 0)
		return -1;

	if (a.v.t == CY_V_LIST) {
		if (cy_eval_next_x(f, &b, CY_V_NUMBER) <= 0)
			return -1;

		return cy_list_del(&a.v, &b.v);
	}

	if (a.v.t == CY_V_MAP) {
		if (cy_eval_next_x(f, &b, CY_V_STRING) <= 0)
			return -1;

		return cy_map_del(&a.v, &b.v);
	}

	if (a.v.t == CY_V_NUMBER) {
		if (cy_eval_next_x(f, &b, CY_V_NUMBER) <= 0)
			return -1;

		t->v.t = CY_V_NUMBER;
		t->v.v_i = a.v.v_i - b.v.v_i;
		return 1;
	}

	show_token_err(t, "Bad op for sub");
	return -1;
}

#define OP_DIV	1
#define OP_MOD	2
#define OP_MUL	3

static int eval_nr_op(struct cy_token *t, struct cy_file *f)
{
	int ret;
	struct cy_token a, b;

	ret = eval_2arg_math(&a, &b, f);
	if (ret <= 0)
		return -1;

	t->v.t = a.v.t;

	switch (a.v.t) {
	case CY_V_NUMBER:
		if (t->typ->priv == OP_MUL) {
			t->v.v_i = a.v.v_i * b.v.v_i;
		} else {
			if (b.v.v_i == 0) {
				show_token_err(t, "Division by zero");
				return -1;
			}

			if (t->typ->priv == OP_DIV)
				t->v.v_i = a.v.v_i / b.v.v_i;
			else
				t->v.v_i = a.v.v_i % b.v.v_i;
		}
		break;
	default:
		show_token_err(t, "Can't DIV %s", vtype2s(a.v.t));
		return -1;
	}

	return 1;
}

#define OP_AND	1
#define OP_OR	2
#define OP_XOR	4

static int eval_bool_op(struct cy_token *t, struct cy_file *f)
{
	int ret;
	struct cy_token a, b;

	ret = eval_2arg_math(&a, &b, f);
	if (ret <= 0)
		return -1;

	t->v.t = a.v.t;

	switch (t->v.t) {
	case CY_V_BOOL:
		switch (t->typ->priv) {
		case OP_AND: t->v.v_bool = a.v.v_bool && b.v.v_bool; break;
		case OP_OR:  t->v.v_bool = a.v.v_bool || b.v.v_bool; break;
		case OP_XOR: t->v.v_bool = a.v.v_bool ^  b.v.v_bool; break;
		}
		break;
	default:
		show_token_err(t, "Bool command on %s", vtype2s(a.v.t));
		return -1;
	}

	return 1;
}

static int eval_bool_not(struct cy_token *t, struct cy_file *f)
{
	struct cy_token a;

	if (cy_eval_next_x(f, &a, CY_V_BOOL) <= 0)
		return -1;

	t->v.t = CY_V_BOOL;
	t->v.v_bool = !a.v.v_bool;
	return 1;
}

static struct cy_command cmd_aryth[] = {
	{ .name = "+", .t = { .ts = "add", .eval = eval_add, }, },
	{ .name = "-", .t = { .ts = "sub", .eval = eval_sub, }, },

	{ .name = "*", .t = { .ts = "mul", .eval = eval_nr_op, .priv = OP_MUL, }, },
	{ .name = "/", .t = { .ts = "div", .eval = eval_nr_op, .priv = OP_DIV, }, },
	{ .name = "%", .t = { .ts = "mod", .eval = eval_nr_op, .priv = OP_MOD, }, },

	{ .name = "&", .t = { .ts = "and", .eval = eval_bool_op, .priv = OP_AND, }, },
	{ .name = "|", .t = { .ts = "or",  .eval = eval_bool_op, .priv = OP_OR, }, },
	{ .name = "^", .t = { .ts = "xor", .eval = eval_bool_op, .priv = OP_XOR, }, },
	{ .name = "~", .t = { .ts = "not", .eval = eval_bool_not, }, },
	{},
};

void init_aryth(void)
{
	add_commands(cmd_aryth);
}
