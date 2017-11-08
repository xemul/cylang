#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "cy.h"

#define CY_BUF_SIZE	1024

static struct cy_file *cy_read(const char *filename)
{
	int fd, ret;
	unsigned long len;
	struct cy_file *cf;

	cf = malloc(sizeof(*cf));

	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		free(cf);
		perror("Can't open file");
		return NULL;
	}

	cf->buf = malloc(CY_BUF_SIZE);
	len = 0;

	while (1) {
		ret = read(fd, cf->buf + len, CY_BUF_SIZE);
		if (ret < 0) {
			free(cf->buf);
			free(cf);
			close(fd);
			perror("Can't read file");
			return NULL;
		}

		if (ret == 0) {
			close(fd);
			cf->cur = cf->buf;
			cf->end = cf->buf + len;
			cf->ln = 1;
			cf->tn = 1;
			return cf;
		}

		len += ret;
		cf->buf = realloc(cf->buf, len + CY_BUF_SIZE);
	}
}

static int cy_advance(struct cy_file *cf, struct cy_token *t)
{
	char span_start = '\0';

	t->typ = NULL;
	t->v.t = CY_V_NOVALUE;

	while (1) {
		if (cf->cur == cf->end)
			return 0;

		if (is_space_at(cf->cur)) {
			if (*cf->cur == '\n') {
				cf->ln++;
				cf->tn = 1;
			}

			cf->cur++;
			continue;
		}

		break;
	}

	t->val = cf->cur;
	t->ln = cf->ln;
	t->tn = cf->tn;
	if (*cf->cur == '"' || *cf->cur == '\'' || *cf->cur == '#') {
		/* This is a spanning token. It ends at the next peer value. */
		span_start = *cf->cur;
		cf->cur++;
	}

	while (1) {
		if (cf->cur >= cf->end) {
			fprintf(stderr, "Parse error -- can't find token end\n");
			return -1;
		}

		if (span_start == '\0') {
			if (!is_space_at(cf->cur)) {
				cf->cur++;
				continue;
			}
		} else {
			if (*cf->cur != span_start) {
				if (*cf->cur == '\n') {
					cf->ln++;
					cf->tn = 1;
				}

				cf->cur++;
				continue;
			}
		}


		if (*cf->cur == '\n') {
			cf->ln++;
			cf->tn = 1;
		} else
			cf->tn++;

		*cf->cur = '\0';
		cf->cur++;

		if (cf->verbose)
			printf("Found token %s @%d.%d\n", t->val, t->ln, t->tn);
		break;
	}

	return 1;
}

static inline int cy_read_next_token(struct cy_file *cf, struct cy_token *t)
{
	int ret;

	ret = cy_advance(cf, t);
	if (ret == 1)
		ret = cy_resolve(t);

	return ret;
}

static int read_cblocks(struct cy_token *t, struct cy_file *f)
{
	int ret;

	t->v.t = CY_V_CBLOCK;
	t->v.v_cblk = malloc(sizeof(struct cy_cblock));
	INIT_LIST_HEAD(&t->v.v_cblk->h);

	while (1) {
		struct cy_ctoken *cb;

		cb = malloc(sizeof(*cb));

		ret = cy_read_next_token(f, &cb->t);
		if (ret <= 0) {
			free(cb);
			return ret;
		}

		if (cb->t.typ->priv == OP_CBLOCK) {
			ret = read_cblocks(&cb->t, f);
			if (ret <= 0) {
				free(cb);
				return ret;
			}
		} else if (cb->t.typ->priv == OP_CBLOCK_END) {
			free(cb);
			return 1;
		} else if (cb->t.typ->priv == OP_COMMENT) {
			free(cb);
			continue;
		}

		list_add_tail(&cb->l, &t->v.v_cblk->h);
	}
}

struct cy_file *cy_open(const char *filename, struct cy_token *mt, bool verbose)
{
	int ret;
	struct cy_file *cf;

	cf = cy_read(filename);
	if (!cf)
		return NULL;

	cf->verbose = verbose;
	cf->main = NULL;
	ret = read_cblocks(mt, cf);
	if (ret < 0) {
		fprintf(stderr, "Can't parse file");
		return NULL;
	}

	return cf;
}

void init_tokenizer(struct cy_file *f, struct cy_cblock *blks)
{
	f->main = blks;
	if (!list_empty(&f->main->h))
		f->nxt = list_first_entry(&f->main->h, struct cy_ctoken, l);
	else
		f->nxt = NULL;
}

int cy_next(struct cy_file *cf, struct cy_token *t)
{
	struct list_head *n;

	if (cf->nxt == NULL)
		return 0;

	*t = cf->nxt->t;
	if (t->typ->eval != NULL)
		t->v.t = CY_V_NOVALUE;

	n = cf->nxt->l.next;
	if (n == &cf->main->h)
		cf->nxt = NULL;
	else
		cf->nxt = list_entry(n, struct cy_ctoken, l);

	return 1;
}
