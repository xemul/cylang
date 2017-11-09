#include "cy.h"

#define CY_CMD_HASH_BITS	5
#define CY_CMD_HASH_SIZE	(1 << CY_CMD_HASH_BITS)
#define CY_CMD_HASH_MASK	(CY_CMD_HASH_SIZE - 1)

static struct cy_command *cmd_hash[CY_CMD_HASH_SIZE];
static struct cy_command *cmd_list, **last = &cmd_list;

static unsigned int strhash(const char *val)
{
	unsigned int ret = 0, s = 0;

	while (1) {
		unsigned int uv;

		if (*val == '\0')
			return ret;
		uv = *val;
		ret += uv << s;
		val++;
		s++;
	}
}

static int try_resolve_command(struct cy_token *t)
{
	unsigned int hv;
	struct cy_command *cmd;

	hv = strhash(t->val) & CY_CMD_HASH_MASK;
	for (cmd = cmd_hash[hv]; cmd != NULL; cmd = cmd->n)
		if (strcmp(cmd->name, t->val) == 0) {
			t->typ = &cmd->t;
			return 1;
		}

	return 0;
}

static struct cy_type comment = { .priv = OP_COMMENT, };

int cy_resolve(struct cy_token *t)
{
	if (try_resolve_command(t))
		return 1;

	if (try_resolve_sym(t))
		return 1;

	if (t->val[0] == '#') {
		t->typ = &comment;
		return 1;
	}

	show_token_err(t, "Can't resolve token value %s", t->val);
	return -1;
}

static void add_command(struct cy_command *cmd)
{
	unsigned int hv;

	hv = strhash(cmd->name) & CY_CMD_HASH_MASK;
	cmd->n = cmd_hash[hv];
	cmd_hash[hv] = cmd;

	*last = cmd;
	last = &cmd->l;
}

void add_commands(struct cy_command *cmds)
{
	int i;

	for (i = 0; cmds[i].name != NULL; i++)
		add_command(&cmds[i]);
}

void init_resolver(void)
{
	init_declare();
	init_show();
	init_aryth();
	init_list();
	init_map();
	init_string();
	init_cblocks();
	init_cond();
	init_misc();
	init_stream();
	init_iter();
}

void show_commands(void)
{
	struct cy_command *cmd;

	for (cmd = cmd_list; cmd != NULL; cmd = cmd->l)
		printf("%24s%5s\n", cmd->t.ts, cmd->name);
}
