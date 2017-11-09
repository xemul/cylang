#include "cy.h"

static unsigned int map_depth = 0;

static int eval_map_end(struct cy_token *t, struct cy_file *f)
{
	if (map_depth > 0) {
		t->v.t = CY_V_MAP_END;
		return 1;
	}

	show_token_err(t, "Dangling map terminator");
	return -1;
}

int map_add_value(const char *key, struct cy_value *v, struct rb_root *root, bool upsert)
{
	struct rb_node *node = root->rb_node, *parent = NULL;
	struct rb_node **new = &root->rb_node;
	struct cy_map_value *mv;

	while (node) {
		struct cy_map_value *this;
		int r;

		parent = *new;
		this = rb_entry(node, struct cy_map_value, n);
		r = strcmp(this->key, key);
		if (r == 0) {
			if (!upsert)
				return -1;

			this->v = *v;
			return 0;
		}
		if (r > 0)
			node = node->rb_left, new = &((*new)->rb_left);
		else
			node = node->rb_right, new = &((*new)->rb_right);
	}

	mv = malloc(sizeof(*mv));
	mv->key = key;
	mv->v = *v;

	rb_link_and_balance(root, &mv->n, parent, new);
	return 1;
}

static int map_key(struct cy_file *f, const char **keyp)
{
	int ret;
	struct cy_token kt;

	ret = cy_next(f, &kt);
	if (ret <= 0)
		return -1;

	/*
	 * Symbols as keys are accepted as is, w/o derefernece,
	 * for simplicity and nicety in declaring objects. To
	 * push dereferenced string as key use + operator.
	 */

	if (is_symbol_token(&kt)) {
		*keyp = kt.v.v_sym;
		return 1;
	}

	if (kt.typ->eval) {
		ret = kt.typ->eval(&kt, f);
		if (ret <= 0)
			return -1;
	}

	if (kt.v.t & CY_V_TERMINATOR) {
		if (kt.v.t == CY_V_MAP_END)
			return 0;
		else {
			show_token_err(&kt, "Unexpected map terminator");
			return -1;
		}
	}

	if (kt.v.t != CY_V_STRING) {
		show_token_err(&kt, "Expected string as map key, got %s", vtype2s(kt.v.t));
		return -1;
	}

	*keyp = kt.v.v_str;
	return 1;
}

static int eval_map(struct cy_token *t, struct cy_file *f)
{
	t->v.t = CY_V_MAP;
	t->v.v_map = malloc(sizeof(struct rb_root));
	t->v.v_map->r = RB_ROOT;
	
	map_depth++;
	while (1) {
		int ret;
		const char *key;
		struct cy_token vt;

		ret = map_key(f, &key);
		if (ret < 0)
			goto er;
		if (ret == 0) {
			map_depth--;
			return 1;
		}

		if (cy_eval_next(f, &vt) <= 0)
			goto er;

		if (vt.v.t & CY_V_TERMINATOR)
			goto er;

		if (map_add_value(key, &vt.v, &t->v.v_map->r, false) < 0)
			goto er;

		continue;

er:
		show_token_err(t, "Error processing map entry");
		map_depth--;
		return -1;
	}
}

struct cy_map_value *find_in_map(struct rb_root *root, const char *key, unsigned klen)
{
	struct rb_node *node = root->rb_node;

	while (node) {
		struct cy_map_value *this;
		int r;

		this = rb_entry(node, struct cy_map_value, n);
		if (klen)
			r = strncmp(this->key, key, klen);
		else
			r = strcmp(this->key, key);
		if (r == 0)
			return this;

		if (r > 0)
			node = node->rb_left;
		else
			node = node->rb_right;
	}

	fprintf(stderr, "Can't find map entry\n");
	return NULL;
}

int dereference_map_elem(struct cy_token *t, struct cy_value *m, struct cy_value *kv)
{
	struct cy_map_value *mv;

	if (kv->t != CY_V_STRING) {
		show_token_err(t, "Nested map resolve is not a string");
		return -1;
	}

	mv = find_in_map(&m->v_map->r, kv->v_str, 0);
	if (mv)
		t->v = mv->v;
	else
		t->v.t = CY_V_NOVALUE;

	return 1;
}

int cy_map_del(struct cy_value *m, struct cy_value *k)
{
	struct rb_root *root = &m->v_map->r;
	struct cy_map_value *mv;

	mv = find_in_map(root, k->v_str, 0);
	if (!mv)
		return -1;

	rb_erase(&mv->n, root);
	return 1;
}

static struct cy_command cmd_map[] = {
	{ .name = "[", .t = { .ts = "map", .eval = eval_map, }, },
	{ .name = "]", .t = { .ts = "map_end", .eval = eval_map_end, }, },
	{}
};

void init_map(void)
{
	add_commands(cmd_map);
}
