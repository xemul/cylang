#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "cy.h"

static struct cy_type cy_number = { .ts = "integer", };
static struct cy_type cy_string	= { .ts = "string", };

static int try_resolve_number(struct cy_token *t)
{
	const char *aux = t->val;
	int base = 0;

	if (aux[0] == '-')
		aux++;

	if (aux[0] == '0') {
		if (aux[1] == 'x') {
			base = 16;
		} else {
			base = 8;
		}
	} else {
		base = 10;
	}

	t->v.v_i = strtol(t->val, (char **)&aux, base);
	if (*aux != '\0')
		return 0;

	t->typ = &cy_number;
	t->v.t = CY_V_NUMBER;
	return 1;
}

int try_resolve_sym(struct cy_token *t)
{
	if (try_resolve_number(t))
		return 1;

	if (t->val[0] == '"' || t->val[0] == '\'') {
		t->typ = &cy_string;
		t->v.t = CY_V_STRING;
		t->v.v_str = t->val + 1;
		return 1;
	}

	if (try_resolve_symbol(t))
		return 1;

	return 0;
}
