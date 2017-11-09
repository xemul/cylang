#include "cy.h"

static void copy_list(struct list_head *from, struct list_head *to)
{
	struct cy_list_value *lf, *lt;

	list_for_each_entry(lf, from, l) {
		lt = malloc(sizeof(*lt));
		lt->v = lf->v;
		list_add_tail(&lt->l, to);
	}
}

int cy_list_splice(struct cy_value *l1, struct cy_value *l2, struct cy_value *r)
{
	struct cy_list *rl;

	rl = malloc(sizeof(*rl));
	r->v_list = rl;
	INIT_LIST_HEAD(&rl->h);

	copy_list(&l1->v_list->h, &rl->h);
	copy_list(&l2->v_list->h, &rl->h);

	return 1;
}

static struct cy_list_value *cy_list_find(struct cy_value *lst, int idx)
{
	struct cy_list_value *lv;

	list_for_each_entry(lv, &lst->v_list->h, l) {
		if (idx-- != 0)
			continue;

		return lv;
	}

	return NULL;
}

int cy_list_del(struct cy_value *l, struct cy_value *i)
{
	struct cy_list_value *lv;

	if (i->v_i < 0) {
		fprintf(stderr, "List removal should be by positive index\n");
		return -1;
	}

	lv = cy_list_find(l, i->v_i);
	if (lv) {
		list_del_init(&lv->l);
		return 1;
	}

	return -1;
}

int dereference_list_elem(struct cy_token *t, struct cy_value *l, struct cy_value *kv)
{
	struct cy_list_value *lv;
	int n;
	char *end;

	if (kv->t == CY_V_STRING) {
		n = strtol(kv->v_str, &end, 10);
		if (*end != '\0' || n < 0) {
			show_token_err(t, "Bad list index %s", kv->v_str);
			return -1;
		}
	} else if (kv->t == CY_V_NUMBER)
		n = kv->v_i;
	else {
		show_token_err(t, "Bad list deref %s", vtype2s(kv->t));
		return -1;
	}

	lv = cy_list_find(l, n);
	if (lv)
		t->v = lv->v;
	else
		t->v.t = CY_V_NOVALUE;

	return 1;
}

bool check_in_list(struct cy_list *l, struct cy_value *v)
{
	struct cy_list_value *lv;

	list_for_each_entry(lv, &l->h, l)
		if (cy_compare(v, &lv->v))
			return true;

	return false;
}

static unsigned int list_depth = 0;

static int eval_list_end(struct cy_token *t, struct cy_file *f)
{
	if (list_depth > 0) {
		t->v.t = CY_V_LIST_END;
		return 1;
	}

	show_token_err(t, "Dangling list terminator");
	return -1;
}

static int eval_list(struct cy_token *t, struct cy_file *f)
{
	t->v.t = CY_V_LIST;
	t->v.v_list = malloc(sizeof(struct list_head));
	INIT_LIST_HEAD(&t->v.v_list->h);

	list_depth++;
	while (1) {
		struct cy_token vt;
		struct cy_list_value *lv;

		if (cy_eval_next(f, &vt) <= 0) {
			list_depth--;
			return -1;
		}

		if (vt.v.t & CY_V_TERMINATOR) {
			list_depth--;
			if (vt.v.t == CY_V_LIST_END)
				return 1;
			else {
				show_token_err(&vt, "Unexpected terminator");
				return -1;
			}
		}

		lv = malloc(sizeof(*lv));
		lv->v = vt.v;
		list_add_tail(&lv->l, &t->v.v_list->h);
	}
}

static int eval_list_gen(struct cy_token *t, struct cy_file *f)
{
	struct cy_token gt;
	struct cy_list_value *lv;
	struct cy_ctoken *cur;

	t->v.t = CY_V_LIST;
	t->v.v_list = malloc(sizeof(struct list_head));
	INIT_LIST_HEAD(&t->v.v_list->h);

	cur = f->nxt;
again:
	if (cy_eval_next(f, &gt) <= 0)
		return -1;

	if (!cy_empty_value(&gt.v)) {
		f->nxt = cur;
		lv = malloc(sizeof(*lv));
		lv->v = gt.v;
		list_add_tail(&lv->l, &t->v.v_list->h);
		goto again;
	}

	return 1;
}

#define OP_CUT_HEAD	1
#define OP_CUT_TAIL	2
#define OP_CUT_BOTH	3

static int eval_list_cut(struct cy_token *t, struct cy_file *f)
{
	int i, from, to;
	struct cy_token lht, ct;
	struct cy_list_value *lv;

	if (cy_eval_next_x(f, &lht, CY_V_LIST) <= 0)
		return -1;

	if (cy_eval_next_x(f, &ct, CY_V_NUMBER) <= 0)
		return -1;

	if (t->typ->priv == OP_CUT_BOTH) {
		struct cy_token ct2;

		if (cy_eval_next_x(f, &ct2, CY_V_NUMBER) <= 0)
			return -1;

		from = ct.v.v_i;
		to = ct2.v.v_i;
	} else if (t->typ->priv == OP_CUT_HEAD) {
		from = ct.v.v_i;
		to = 0x7fffffff;
	} else {
		from = 0;
		to = ct.v.v_i;
	}

	t->v.t = CY_V_LIST;
	t->v.v_list = malloc(sizeof(struct list_head));
	INIT_LIST_HEAD(&t->v.v_list->h);

	i = 0;
	list_for_each_entry(lv, &lht.v.v_list->h, l) {
		struct cy_list_value *nv;

		if (i < from || i > to) {
			i++;
			continue;
		}

		nv = malloc(sizeof(*lv));
		nv->v = lv->v;
		list_add_tail(&nv->l, &t->v.v_list->h);
		i++;
	}

	return 1;
}

#define OP_LIST_HEAD	1
#define OP_LIST_TAIL	2

static int eval_list_add(struct cy_token *t, struct cy_file *f)
{
	struct cy_token lst, vt;
	struct cy_list_value *lv;

	if (cy_eval_next_x(f, &lst, CY_V_LIST) <= 0)
		return -1;

	if (cy_eval_next(f, &vt) <= 0)
		return -1;

	lv = malloc(sizeof(*lv));
	lv->v = vt.v;

	if (t->typ->priv == OP_LIST_HEAD)
		list_add(&lv->l, &lst.v.v_list->h);
	else
		list_add_tail(&lv->l, &lst.v.v_list->h);

	return 1;
}

static int eval_list_pop(struct cy_token *t, struct cy_file *f)
{
	struct cy_token lst;
	struct cy_list_value *lv;

	if (cy_eval_next_x(f, &lst, CY_V_LIST) <= 0)
		return -1;

	if (list_empty(&lst.v.v_list->h)) {
		t->v.t = CY_V_NOVALUE;
		return 1;
	}

	if (t->typ->priv == OP_LIST_HEAD)
		lv = list_entry(lst.v.v_list->h.next, struct cy_list_value, l);
	else
		lv = list_entry(lst.v.v_list->h.prev, struct cy_list_value, l);

	list_del_init(&lv->l);
	t->v = lv->v;
	free(lv);

	return 1;
}

static struct cy_command cmd_list[] = {
	{ .name = "(", .t = { .ts = "list start", .eval = eval_list, }, },
	{ .name = ")", .t = { .ts = "list end", .eval = eval_list_end, }, },
	{ .name = "(:", .t = { .ts = "list gen", .eval = eval_list_gen, }, },

	{ .name = "(<", .t = { .ts = "list head", .eval = eval_list_cut, .priv = OP_CUT_TAIL, }, },
	{ .name = "(>", .t = { .ts = "list tail", .eval = eval_list_cut, .priv = OP_CUT_HEAD, }, },
	{ .name = "(<>", .t = { .ts = "list body", .eval = eval_list_cut, .priv = OP_CUT_BOTH, }, },

	{ .name = "+(", .t = { .ts = "list add head", .eval = eval_list_add, .priv = OP_LIST_HEAD, }, },
	{ .name = "+)", .t = { .ts = "list add tail", .eval = eval_list_add, .priv = OP_LIST_TAIL, }, },
	{ .name = "-(", .t = { .ts = "list pop head", .eval = eval_list_pop, .priv = OP_LIST_HEAD, }, },
	{ .name = "-)", .t = { .ts = "list pop tail", .eval = eval_list_pop, .priv = OP_LIST_TAIL, }, },

	{}
};

void init_list(void)
{
	add_commands(cmd_list);
}
