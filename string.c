#define _GNU_SOURCE
#include "cy.h"

static char *fmt_tmp_buf;
static int tmp_buf_len;

static void append_buf(const char *str, int len)
{
	fmt_tmp_buf = realloc(fmt_tmp_buf, tmp_buf_len + len + 1);
	strncpy(fmt_tmp_buf + tmp_buf_len, str, len);
	tmp_buf_len += len;
}

static void drop_buf(void)
{
	free(fmt_tmp_buf);
	tmp_buf_len = 0;
}

static char *fixup_buf(void)
{
	char *ret = fmt_tmp_buf;

	ret[tmp_buf_len] = '\0';
	fmt_tmp_buf = NULL;
	tmp_buf_len = 0;

	return ret;
}

static int format_one(char *str, char **end, struct cy_token *st)
{
	int len, ret;

	*end = strchr(str, ')');
	if (*end == NULL) {
		show_token_err(st, "Error scanning format");
		return -1;
	}

	len = *end - str - 1;

	{
		char fbuf[len + 1];
		struct cy_value cv;
		char i_aux[32];

		strncpy(fbuf, str + 1, len);
		fbuf[len] = '\0';

		if (try_deref_symbol(fbuf, &cv, false) < 0)
			return -1;

		switch (cv.t) {
		case CY_V_STRING:
			append_buf(cv.v_str, strlen(cv.v_str));
			break;
		case CY_V_NUMBER:
			ret = sprintf(i_aux, "%d", cv.v_i);
			append_buf(i_aux, ret);
			break;
		default:
			show_token_err(st, "Bad symbol (%s) for format", vtype2s(cv.t));
			return -1;
		}
	}

	return 0;
}

static int format_string(struct cy_token *st, struct cy_value *sv)
{
	const char *s = st->v.v_str;
	char *aux;

	sv->t = CY_V_STRING;

	while (1) {
		aux = strchrnul(s, '\\');
		append_buf(s, aux - s);

		if (*aux == '\0')
			break;

		if (aux[0] == '\\' && aux[1] == '(') {
			if (format_one(aux + 1, &aux, st) < 0) {
				drop_buf();
				return -1;
			}
		}

		s = aux + 1;
	}

	sv->v_str = fixup_buf();
	return 1;
}

static int eval_format(struct cy_token *t, struct cy_file *f)
{
	struct cy_token st;

	if (cy_eval_next_x(f, &st, CY_V_STRING) <= 0)
		return -1;

	return format_string(&st, &t->v);
}

static int decode_string(struct cy_token *st, struct cy_value *v)
{
	char *end;

	v->t = CY_V_NUMBER;
	v->v_i = strtol(st->v.v_str, &end, 0);
	if (*end != '\0') {
		show_token_err(st, "Not a number");
		return -1; /* XXX -- errors */
	}

	return 1;
}

static int eval_atoi(struct cy_token *t, struct cy_file *f)
{
	struct cy_token st;

	if (cy_eval_next_x(f, &st, CY_V_STRING) <= 0)
		return -1;

	return decode_string(&st, &t->v);
}

static int split_string(const char *str, struct cy_value *v)
{
	const char *aux = NULL;

	v->t = CY_V_LIST;
	v->v_list = malloc(sizeof(struct list_head));
	INIT_LIST_HEAD(&v->v_list->h);

	while (*str != '\0') {
		if (is_space_at(str)) {
			str++;
			continue;
		}

		aux = str + 1;
		while (*aux != '\0') {
			if (!is_space_at(aux)) {
				aux++;
				continue;
			}

			break;
		}

		{
			struct cy_list_value *lv;

			lv = malloc(sizeof(*lv));
			lv->v.t = CY_V_STRING;
			lv->v.v_str = strndup(str, aux - str);

			list_add_tail(&lv->l, &v->v_list->h);
		}

		if (*aux != '\0')
			str = aux + 1;
		else
			str = aux;
	}

	return 1;
}

static int eval_split(struct cy_token *t, struct cy_file *f)
{
	struct cy_token st;

	if (cy_eval_next_x(f, &st, CY_V_STRING) <= 0)
		return -1;

	return split_string(st.v.v_str, &t->v);
}

#define OP_CHK_PREFIX	1
#define OP_CHK_SUFFIX	2

static int eval_chk_match(struct cy_token *t, struct cy_file *f)
{
	struct cy_token st, pt;
	unsigned len;

	if (cy_eval_next_x(f, &st, CY_V_STRING) <= 0)
		return -1;
	if (cy_eval_next_x(f, &pt, CY_V_STRING) <= 0)
		return -1;

	len = strlen(pt.v.v_str);
	t->v.t = CY_V_BOOL;

	if (t->typ->priv == OP_CHK_PREFIX)
		t->v.v_bool = (strncmp(st.v.v_str, pt.v.v_str, len) == 0);
	else {
		unsigned slen;

		slen = strlen(st.v.v_str);
		if (slen < len)
			t->v.v_bool = false;
		else
			t->v.v_bool = (strcmp(st.v.v_str + slen - len, pt.v.v_str) == 0);
	}

	return 1;
}

static struct cy_command cmd_format[] = {
	{ .name = "%%", .t = { .ts = "format", .eval = eval_format, }, },
	{ .name = "%~", .t = { .ts = "atoi", .eval = eval_atoi, }, },
	{ .name = "%/", .t = { .ts = "split", .eval = eval_split, }, },
	{ .name = "%^", .t = { .ts = "startswith", .eval = eval_chk_match, .priv = OP_CHK_PREFIX, }, },
	{ .name = "%$", .t = { .ts = "endswith", .eval = eval_chk_match, .priv = OP_CHK_SUFFIX, }, },
	{},
};

void init_string(void)
{
	add_commands(cmd_format);
}
