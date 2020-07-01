#include <limits.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "account.h"
#include "track.h"

#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))

static double
eval(expr *e, int y, int m)
{
    double left;
    double right;
    switch (e->type) {
    case EXPR_ADD:
        left = eval(e->left, y, m);
        right = eval(e->right, y, m);
        return left + right;
    case EXPR_SUB:
        left = eval(e->left, y, m);
        right = eval(e->right, y, m);
        return left - right;
    case EXPR_MUL:
        left = eval(e->left, y, m);
        right = eval(e->right, y, m);
        /* printf("Multiplying "); */
        /* expr_print(stdout, e->left); */
        /* printf(" and "); */
        /* expr_print(stdout, e->right); */
        /* printf("\n"); */
        return left * right;
    case EXPR_DIV:
        left = eval(e->left, y, m);
        right = eval(e->right, y, m);
        return left / right;
    case EXPR_EXP:
        left = eval(e->left, y, m);
        right = eval(e->right, y, m);
        return pow(left, right);
    case EXPR_NUM:
        return e->ival;
    case EXPR_ID:
        return e->aval->eval(e->aval, y, m);
    }
}

static void
expr_print_rec(FILE *fp, expr *e)
{
    assert(fp != NULL);
    assert(e != NULL);

    if (e->type == EXPR_NUM) {
        fprintf(fp, "%g", e->ival);
    } else if (e->type == EXPR_ID) {
        fprintf(fp, "%s", e->aval->name);
    } else {
        fprintf(fp, "(%c ", e->type);
        expr_print_rec(fp, e->left);
        fputc(' ', fp);
        expr_print_rec(fp, e->right);
        fputc(')', fp);
    }
}

void
expr_print(FILE *fp, expr *e)
{
    expr_print_rec(fp, e);
    fputc('\n', fp);
}

expr *
expr_new(expr_type type, account *aval, double ival, expr *left, expr *right)
{
    expr *e;

    e = malloc(sizeof *e);
    if (e == NULL) {
        return NULL;
    }
    e->type = type;
    e->aval = aval;
    e->ival = ival;
    e->left = left;
    e->right = right;
    return e;
}

static double
eval_var(account *a, int y, int m)
{
    return eval(a->exp, y, m);
}

account *
tracker_new(char *name, char *longname, expr *e)
{
    account *a;

    assert(name && "name is null");
    assert(longname && "long name is null");
    assert(e && "expr is null");

    a = account_new(EXPENSE, name, longname);
    if (a == NULL) {
        return NULL;
    }
    a->typ = VAR;
    a->exp = e;
    a->eval = eval_var;
    return a;
}

static double
eval_const(account *a, int y, int m)
{
    assert(a != NULL);
    return a->dval;
}


account *
const_new(char *name, double val)
{
    account *a;

    assert(name && "name is null");

    a = calloc(1, sizeof *a + strlen(name) + 1);
    if (a == NULL)
        return NULL;
    a->typ = CONST;
    a->name = a->longname = ((char *) a) + (sizeof *a);
    strcpy(a->name, name);
    a->dval = val;
    a->eval = eval_const;
    accounts[naccounts++] = a;
    return a;
}
