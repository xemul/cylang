#define _GNU_SOURCE
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static inline bool is_space_at(const char *ptr)
{
	char c = *ptr;

	return c == ' ' || c == '\t' || c == '\n';
}

static inline bool is_alpha(const char v)
{
	return (v >= 'a' && v <= 'z') || (v >= 'A' && v <= 'Z');
}

static inline bool is_num(const char v)
{
	return (v >= '0' && v <= '9');
}

/* These privs are checked across all tokens and should be unique */
#define OP_CBLOCK	0x70000001
#define OP_CBLOCK_END	0x70000002
#define OP_CBLOCK_NOP	0x70000004
#define OP_COMMENT	0x7fffffff

struct cy_cblock;
struct cy_ctoken;
struct cy_token;

struct cy_file {
	char *buf;
	char *cur;
	char *end;
	unsigned int ln; /* line number */
	unsigned int tn; /* token number */
	bool verbose;

	struct cy_cblock *main;
	struct cy_ctoken *nxt;
};

void show_token_err(struct cy_token *, const char *fmt, ...);

struct cy_type {
	char *ts;
	int (*eval)(struct cy_token *, struct cy_file *);
	unsigned long priv;
};

#include "list.h"
#include "rbtree.h"

struct cy_list {
	struct list_head h;
};

struct cy_map {
	struct rb_root r;
};

struct cy_cblock {
	struct list_head h;
};

struct cy_stream {
	FILE *f;
};

struct cy_value {
	unsigned int t;
	union {
		long			v_i;
		const char		*v_str;
		const char		*v_sym;
		struct cy_list		*v_list;
		struct cy_map		*v_map;
		struct cy_cblock	*v_cblk;
		bool			v_bool;
		int			v_sn;
		struct cy_stream	*v_stream;
	};
};

struct cy_token {
	const char *val;
	struct cy_type *typ;
	unsigned int ln, tn;

	struct cy_value v;
};

struct cy_ctoken {
	struct list_head l;
	struct cy_token t;
};

struct cy_list_value {
	struct list_head l;
	struct cy_value v;
};

struct cy_map_value {
	const char *key;
	struct rb_node n;
	struct cy_value v;
};

int map_add_value(const char *key, struct cy_value *v, struct rb_root *root, bool upsert);
struct cy_map_value *find_in_map(struct rb_root *root, const char *key, unsigned klen);

enum {
	CY_V_NOVALUE = 0,
	CY_V_NUMBER,
	CY_V_STRING,
	CY_V_LIST,
	CY_V_MAP,
	CY_V_CBLOCK,
	CY_V_BOOL,
	CY_V_STREAM,
};

#define CY_V_TERMINATOR	0x70000000
#define CY_V_LIST_END	(CY_V_LIST   | CY_V_TERMINATOR)
#define CY_V_MAP_END	(CY_V_MAP    | CY_V_TERMINATOR)
#define CY_V_CBLOCK_END	(CY_V_CBLOCK | CY_V_TERMINATOR)

char *vtype2s(unsigned int);
int cy_next(struct cy_file *cf, struct cy_token *t);

static inline int cy_eval_next(struct cy_file *f, struct cy_token *t)
{
	int ret;

	ret = cy_next(f, t);
	if (ret < 0) {
		fprintf(stderr, "FATAL: Next token error\n");
		exit(1);
	}

	if (ret > 0 && t->typ->eval)
		ret = t->typ->eval(t, f);

	return ret;
}

static inline int cy_eval_next_x(struct cy_file *f, struct cy_token *t, int want_type)
{
	int ret;

	ret = cy_eval_next(f, t);
	if (ret == 1 && t->v.t != want_type) {
		show_token_err(t, "Expected %s token, got %s",
				vtype2s(want_type), vtype2s(t->v.t));
		ret = -1;
	}

	return ret;
}


void set_cursor(struct cy_value *v);

struct cy_file *cy_open(const char *filename, struct cy_token *mt, bool verbose);
int cy_resolve(struct cy_token *t);

int try_resolve_symbol(struct cy_token *t);
int try_resolve_cvalue(struct cy_token *t);
int try_resolve_stream(struct cy_token *t);
int try_deref_symbol(const char *val, struct cy_value *sv, bool strict);
bool is_nop_token(struct cy_token *t);
bool is_symbol_token(struct cy_token *t);

void init_resolver(void);
struct cy_command {
	char *name;
	struct cy_type t;
	struct cy_command *n;
	struct cy_command *l;
};
void add_commands(struct cy_command *);
void init_aryth(void);
void init_declare(void);
void init_show(void);
void init_list(void);
void init_map(void);
void init_string(void);
void init_cblocks(void);
void init_cond(void);
void init_misc(void);
void init_stream(void);
void init_iter(void);

int try_resolve_sym(struct cy_token *t);

bool cy_compare(struct cy_value *v1, struct cy_value *v2);
int cy_list_del(struct cy_value *l, struct cy_value *i);
int cy_list_splice(struct cy_value *l1, struct cy_value *l2, struct cy_value *res);
bool check_in_list(struct cy_list *l, struct cy_value *);
int cy_map_del(struct cy_value *m, struct cy_value *k);
int dereference_list_elem(struct cy_token *t, struct cy_value *l, struct cy_value *kv);
int dereference_map_elem(struct cy_token *t, struct cy_value *m, struct cy_value *kv);

void show_commands(void);
void init_tokenizer(struct cy_file *f, struct cy_cblock *blks);
int cy_call_cblock(struct cy_token *ct, struct cy_file *f, struct cy_value *rv);

extern struct cy_value symbols;
