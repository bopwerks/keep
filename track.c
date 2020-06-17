#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "account.h"
#include "track.h"

double
eval(expr *e, int y, int m, int *ok)
{
    double left;
    double right;
    /* expr_print(stderr, e); */
    switch (e->type) {
    case EXPR_ADD:
        left = eval(e->left, y, m, ok);
        right = eval(e->right, y, m, ok);
        return left + right;
    case EXPR_SUB:
        left = eval(e->left, y, m, ok);
        right = eval(e->right, y, m, ok);
        return left - right;
    case EXPR_MUL:
        left = eval(e->left, y, m, ok);
        right = eval(e->right, y, m, ok);
        return left * right;
    case EXPR_DIV:
        left = eval(e->left, y, m, ok);
        right = eval(e->right, y, m, ok);
        return left / right;
    case EXPR_EXP:
        left = eval(e->left, y, m, ok);
        right = eval(e->right, y, m, ok);
        return pow(left, right);
    case EXPR_NUM:
        return e->ival;
    case EXPR_ID:
        left = account_eval(e->aval, y, m, ok);
        return left;
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
expr_new(expr_type type, account *aval, long ival, expr *left, expr *right)
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

account *
tracker_new(char *name, char *longname, expr *e)
{
    account *a;

    assert(name && "name is null");
    assert(longname && "long name is null");
    assert(e && "expr is null");

    a = calloc(1, sizeof *a);
    if (a == NULL) {
        return NULL;
    }
    a->typ = VAR;
    a->name = malloc(strlen(name)+1);
    if (a->name == NULL) {
        free(a);
        return NULL;
    }
    strcpy(a->name, name);
    a->longname = malloc(strlen(longname)+1);
    if (a->longname == NULL) {
        free(a->name);
        free(a);
        return NULL;
    }
    strcpy(a->longname, longname);
    a->exp = e;
    accounts[naccounts++] = a;
    return a;
}
