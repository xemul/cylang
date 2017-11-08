#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "cy.h"

#define MAX_SYM_LEN	64

struct cy_value symbols;
static struct cy_value cursor;

void set_cursor(struct cy_value *v)
{
	if (v == NULL)
		cursor.t = CY_V_NOVALUE;
	else
		cursor = *v;
}

static char *nest_get_key(const char *val, struct cy_value *kv, char *buf)
{
	char *end;
	bool sym = false;

	if (*val == '.') {
		val++;
		sym = true;
	}

	end = strchrnul(val, '.');
	if (*end == '.') {
		strncpy(buf, val, end - val);
		buf[end - val] = '\0';
		val = buf;
		end++;
	}

	if (sym) {
		if (try_deref_symbol(val, kv, true) < 0)
			return NULL;
	} else {
		kv->t = CY_V_STRING;
		kv->v_str = val;
	}

	return end;
}

static int dereference(struct cy_token *t, struct cy_value *into, struct cy_value *last, char *buf)
{
	struct cy_value sv = symbols;
	const char *aux;

	if (last)
		last->t = CY_V_NOVALUE;
	aux = t->v.v_sym;
	if (aux[0] == '_') {
		if (cursor.t == CY_V_NOVALUE) {
			show_token_err(t, "Use of cursor outside of loop");
			return -1;
		}

		sv = cursor;
		aux++;
		if (*aux == '.')
			aux++;
	}

	while (*aux != '\0') {
		const char *nxt;
		struct cy_value kv;
		int ret;

		nxt = nest_get_key(aux, &kv, buf);
		if (nxt == NULL)
			return -1;

		if (last != NULL && *nxt == '\0') {
			*last = kv;
			break;
		}

		switch (sv.t) {
		case CY_V_LIST:
			ret = dereference_list_elem(t, &sv, &kv);
			break;
		case CY_V_MAP:
			ret = dereference_map_elem(t, &sv, &kv);
			break;
		default:
			show_token_err(t, "Unknown symbol (%s) for nesting", vtype2s(sv.t));
			ret = -1;
			break;
		}

		if (ret <= 0)
			return -1;

		sv = t->v;
		aux = nxt;
	}

	*into = sv;
	return 1;
}

static int eval_sym(struct cy_token *t, struct cy_file *f)
{
	char buf[MAX_SYM_LEN + 1];
	return dereference(t, &t->v, NULL, buf);
}

static int eval_curns(struct cy_token *t, struct cy_file *f)
{
	t->v = symbols;
	return 1;
}

static struct cy_type cy_curns = { .ts = "current namespace", .eval = eval_curns, };
static struct cy_type cy_sym	= { .ts = "symbol", .eval = eval_sym,};
static struct cy_type cy_cursor	= { .ts = "cursor", .eval = eval_sym,};

bool is_symbol_token(struct cy_token *t)
{
	return t->typ == &cy_sym;
}

int try_resolve_symbol(struct cy_token *t)
{
	const char *val = t->val, *aux;
	struct cy_type *ret;

	t->v.v_sym = val;

	if (val[0] == '_') {
		if (val[1] == '.')
			/*
			 * this is a _.... construciton which
			 * is valid nested symbol, but requires
			 * additional checking
			 */
			goto sym;

		if (val[1] == '\0') {
			t->typ = &cy_cursor;
			return 1;
		}

		if (val[1] == ':' && val[2] == '\0') {
			t->typ = &cy_curns;
			return 1;
		}

		return try_resolve_cvalue(t);
	}

	if (!is_alpha(val[0]))
		return 0;

sym:
	ret = &cy_sym;
	aux = val; /* sym len counting */
	for (val++; *val != '\0'; val++) {
		if (val - aux >= MAX_SYM_LEN) {
			show_token_err(t, "Too long symbol");
			return 0;
		}

		if (is_alpha(*val))
			continue;
		if (is_num(*val))
			continue;
		if (*val == '_')
			continue;
		if (*val == '.') {
			aux = val;
			continue;
		}

		return 0;
	}

	t->typ = ret;
	return 1;
}

int try_deref_symbol(const char *val, struct cy_value *sv, bool strict)
{
	struct cy_token ft;

	ft.val = val;
	if (!try_resolve_symbol(&ft))
		return -1;

	if (strict && !is_symbol_token(&ft)) {
		fprintf(stderr, "Nested resolve is not a symbol");
		return -1;
	}

	if (ft.typ->eval(&ft, NULL) < 0)
		return -1;

	*sv = ft.v;
	return 0;
}

static int eval_declare(struct cy_token *t, struct cy_file *f)
{
	struct cy_token st, vt;
	struct cy_value in, last;
	char buf[MAX_SYM_LEN + 1];

	if (cy_next(f, &st) <= 0)
		return -1;

	if (!is_symbol_token(&st)) {
		show_token_err(t, "Declaration to non-symbol");
		return -1;
	}

	if (dereference(&st, &in, &last, buf) <= 0)
		return -1;

	if (last.t != CY_V_STRING) {
		show_token_err(t, "Last sym is not a string");
		return -1;
	}

	if (in.t != CY_V_MAP) {
		show_token_err(t, "Can't declare in non-map");
		return -1;
	}

	if (cy_eval_next(f, &vt) <= 0)
		return -1;

	if (map_add_value(strdup(last.v_str), &vt.v, &in.v_map->r, true) < 0)
		return -1;

	return 1;
}

static int eval_nsenter(struct cy_token *t, struct cy_file *f)
{
	struct cy_token nst;

	if (cy_eval_next_x(f, &nst, CY_V_MAP) <= 0)
		return -1;

	symbols = nst.v;
	return 1;
}

static struct cy_command cmd_declare[] = {
	{ .name = "!", .t = { .ts = "declare", .eval = eval_declare, }, },
	{ .name = ":", .t = { .ts = "nsenter", .eval = eval_nsenter, }, },
};

void init_declare(void)
{
	symbols.t = CY_V_MAP;
	symbols.v_map = malloc(sizeof(struct cy_map));
	symbols.v_map->r = RB_ROOT;
	add_commands(cmd_declare);
}
