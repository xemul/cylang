#include "cy.h"

static void set_stream(struct cy_token *t, FILE *f)
{
	t->v.t = CY_V_STREAM;
	t->v.v_stream = malloc(sizeof(struct cy_stream));
	t->v.v_stream->f = f;
}

static struct cy_type cy_stdio = { .ts = "stdio", };

int try_resolve_stream(struct cy_token *t)
{
	FILE *std = NULL;

	if (t->val[1] == '<' && t->val[2] == '\0')
		std = stdin;
	else if (t->val[1] == '>') {
		if (t->val[2] == '\0')
			std = stdout;
		else if (t->val[2] == '!' && t->val[3] == '\0')
			std = stderr;
	}

	if (std == NULL)
		return 0;

	t->typ = &cy_stdio;
	set_stream(t, std);
	return 1;
}

#define OPEN_IN		1
#define OPEN_OUT	2
#define OPEN_INOUT	3

static int eval_open(struct cy_token *t, struct cy_file *f)
{
	FILE *file;
	struct cy_token ft;

	if (cy_eval_next_x(f, &ft, CY_V_STRING) <= 0)
		return -1;

	file = fopen(ft.v.v_str, (char *)t->typ->priv);
	if (file == NULL) {
		t->v.t = CY_V_NOVALUE;
		return 1;
	}

	set_stream(t, file);
	return 1;
}

struct stream_format {
	const char *f;
	int (*read)(struct cy_stream *st, struct cy_value *v);
};

static int read_line(struct cy_stream *st, struct cy_value *v)
{
	char *str = NULL;
	size_t len = 0;
	ssize_t sz;

	sz = getline(&str, &len, st->f);
	if (sz <= 0)
		return feof(st->f) ? 1 : -1;

	v->t = CY_V_STRING;
	v->v_str = str;
	str[sz - 1] = '\0';
	return 1;
}

#define READ_NUMBER(st, v, type) do {			\
	type val;					\
	if (fread(&val, sizeof(val), 1, st->f) == 0)	\
		return feof(st->f) ? 1 : -1;		\
	v->t = CY_V_NUMBER;				\
	v->v_i = val;					\
	return 1;					\
} while (0)


static int read_byte(struct cy_stream *st, struct cy_value *v) { READ_NUMBER(st, v, unsigned char); }
static int read_short(struct cy_stream *st, struct cy_value *v) { READ_NUMBER(st, v, unsigned short); }
static int read_int(struct cy_stream *st, struct cy_value *v) { READ_NUMBER(st, v, unsigned int); }
static int read_long(struct cy_stream *st, struct cy_value *v) { READ_NUMBER(st, v, unsigned long); }

static struct stream_format formats[] = {
	{ .f = "ln", .read = read_line, },
	{ .f = "c",  .read = read_byte, },
	{ .f = "s",  .read = read_short, },
	{ .f = "i",  .read = read_int, },
	{ .f = "l",  .read = read_long, },
	{ },
};

static struct stream_format *get_format(const char *fn)
{
	int i;

	for (i = 0; formats[i].f != NULL; i++)
		if (!strcmp(formats[i].f, fn))
			return &formats[i];

	return NULL;
}

static int eval_read(struct cy_token *t, struct cy_file *f)
{
	struct cy_token ft, sft;
	struct stream_format *sf;

	if (cy_eval_next_x(f, &ft, CY_V_STREAM) <= 0)
		return -1;
	if (cy_eval_next_x(f, &sft, CY_V_STRING) <= 0)
		return -1;

	if (feof(ft.v.v_stream->f))
		return 1;

	sf = get_format(sft.v.v_str);
	if (!sf) {
		show_token_err(t, "Unknown read format %s", sft.v.v_str);
		return -1;
	}

	return sf->read(ft.v.v_stream, &t->v);
}

static int eval_write(struct cy_token *t, struct cy_file *f)
{
	show_token_err(t, "Write not implemented");
	return -1;
}

static struct cy_command cmd_stream[] = {
	{ .name = "<~",   .t = { .ts = "open r",  .eval = eval_open, .priv = (unsigned long)"r", }, },  /* RDONLY */
	{ .name = "<~>",  .t = { .ts = "open r+", .eval = eval_open, .priv = (unsigned long)"r+", }, }, /* RDWR */
	{ .name = "~>",   .t = { .ts = "open w",  .eval = eval_open, .priv = (unsigned long)"w", }, },  /* WRONLY CREAT TRUNC */
	{ .name = "<->",  .t = { .ts = "open w+", .eval = eval_open, .priv = (unsigned long)"w+", }, }, /* RDWR   CREAT TRUNC */
	{ .name = "->>",  .t = { .ts = "open a",  .eval = eval_open, .priv = (unsigned long)"a", }, },  /* WRONLY CREAT APPEND */
	{ .name = "<->>", .t = { .ts = "open a+", .eval = eval_open, .priv = (unsigned long)"a+", }, }, /* RDWR   CREAT APPEND */

	{ .name = "<<", .t = { .ts = "read", .eval = eval_read, }, },
	{ .name = ">>", .t = { .ts = "write", .eval = eval_write, }, },
	{ },
};

void init_stream(void)
{
	add_commands(cmd_stream);
}
