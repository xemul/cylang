#include "cy.h"

/*
 * CBlocks we have
 *
 * ? cond { then } { else }       -- if
 * ?? { cond1 { } cond2 { } ... } -- select
 * @( list { }                    -- list loop
 * @? cond { }                    -- cond loop
 * -> { }                         -- call
 *
 * <- stops the block and makes the owning command evaluate into the value
 * <? OP checks the operand to be not NOVALUE and returns it otherwise
 */

static int eval_cblock_end(struct cy_token *t, struct cy_file *f)
{
	show_token_err(t, "Dangling cblock terminator");
	return -1;
}

static int eval_call(struct cy_token *t, struct cy_file *f)
{
	int ret;
	struct cy_token ct, mt;
	struct cy_value os = { .t = CY_V_NOVALUE };

	if (cy_eval_next_x(f, &ct, CY_V_CBLOCK) <= 0)
		return -1;
	if (cy_eval_next_x(f, &mt, CY_V_MAP) <= 0)
		return -1;

	if (mt.v.v_map != symbols.v_map) {
		os = symbols;
		symbols = mt.v;
	}

	ret = cy_call_cblock(&ct, f, &t->v);
	if (os.t != CY_V_NOVALUE)
		symbols = os;
	if (ret < 0)
		return ret;

	return 1;
}

static int eval_return(struct cy_token *t, struct cy_file *f)
{
	struct cy_token rt;

	if (cy_eval_next(f, &rt) <= 0)
		return -1;

	t->v = rt.v;
	return 2;
}

static int eval_cond_return(struct cy_token *t, struct cy_file *f)
{
	struct cy_token rt;

	if (cy_eval_next(f, &rt) <= 0)
		return -1;

	if (cy_empty_value(&rt.v))
		return 1;

	t->v = rt.v;
	return 2;
}

static int eval_empty_return(struct cy_token *t, struct cy_file *f)
{
	struct cy_token rt;

	if (cy_eval_next(f, &rt) <= 0)
		return -1;

	return cy_empty_value(&rt.v) ? 2 : 1;
}

int cy_call_cblock(struct cy_token *ct, struct cy_file *f, struct cy_value *rv)
{
	int ret;
	struct cy_cblock *c_list;
	struct cy_ctoken *c_nxt;

	c_list = f->main;
	c_nxt = f->nxt;
	init_tokenizer(f, ct->v.v_cblk);

	while (1) {
		struct cy_token t = {};

		ret = cy_eval_next(f, &t);
		if (ret == 1)
			continue;
		if (ret == 0)
			ret = 1;
		if (ret == 2)
			*rv = t.v;
		break;
	}

	f->main = c_list;
	f->nxt = c_nxt;

	return ret;
}

static struct cy_command cmd_cblock[] = {
	{ .name = "{", .t = { .ts = "cblock start", .priv = OP_CBLOCK, }, },
	{ .name = "}", .t = { .ts = "cblock end", .eval = eval_cblock_end, .priv = OP_CBLOCK_END, }, },
	{ .name = "->", .t = { .ts = "call", .eval = eval_call, }, },
	{ .name = ".", .t = { .ts = "nop", .priv = OP_CBLOCK_NOP, }, },
	{ .name = "<-", .t = { .ts = "return", .eval = eval_return, }, },
	{ .name = "<!", .t = { .ts = "value return", .eval = eval_cond_return, }, },
	{ .name = "<.", .t = { .ts = "empty return", .eval = eval_empty_return, }, },
	{}
};

bool is_nop_token(struct cy_token *t)
{
	return t->typ->priv == OP_CBLOCK_NOP;
}

void init_cblocks(void)
{
	add_commands(cmd_cblock);
}
