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
        return e->aval->eval(e->aval, y, m, ok);
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

time_t
expr_min(expr *e)
{
    time_t a, b;
    switch (e->type) {
    case EXPR_ADD:
    case EXPR_SUB:
    case EXPR_MUL:
    case EXPR_DIV:
    case EXPR_EXP:
        a = expr_min(e->left);
        b = expr_min(e->right);
        return min(a, b);
    case EXPR_NUM:
        /* Constants have infinite range */
        return LONG_MAX;
    case EXPR_ID:
        return e->aval->mindate;
    }
}

static range
intersect(range *a, range *b)
{
    range r;

    if (a->end - a->start < 0) {
        r.start = r.end = 0;
    } else {
        r.start = max(a->start, b->start);
        r.end = min(a->end, b->end);
    }
    return r;
}

static void
print_range(range *r)
{
    char buf1[256];
    char buf2[256];
    struct tm tm;

    ctime_r(&r->start, buf1);
    ctime_r(&r->end, buf2);

    fprintf(stderr, "[%s, %s]", buf1, buf2);
}

range
expr_range(expr *e)
{
    range a, b;
    range r;
    /* An empty range r is one where r.end - r.start <= 0 */
    switch (e->type) {
    case EXPR_ADD:
    case EXPR_SUB:
    case EXPR_MUL:
    case EXPR_DIV:
    case EXPR_EXP:
        /* TODO: Return intersection of left and right ranges */
        /* printf("Evaluating "); expr_print(stdout, e); */
        a = expr_range(e->left);
        /* printf("Left side "); print_range(&a); */
        b = expr_range(e->right);
        /* printf("Right side "); print_range(&b); */
        r = intersect(&a, &b);
        break;
    case EXPR_NUM:
        /* TODO: Return infinite range */
        r.start = LONG_MIN;
        r.end = LONG_MAX;
        break;
    case EXPR_ID:
        /* TODO: Return range of account or var */
        r.start = e->aval->mindate;
        r.end = e->aval->maxdate;
        break;
    }
    return r;
}

static double
eval_var(account *a, int y, int m, int *found)
{
    return eval(a->exp, y, m, found);
}

account *
tracker_new(char *name, char *longname, expr *e)
{
    account *a;

    assert(name && "name is null");
    assert(longname && "long name is null");
    assert(e && "expr is null");

    /* fprintf(stderr, "Making new tracker %s\n", name); */
    a = account_new(EXPENSE, name, longname);
    if (a == NULL) {
        return NULL;
    }
    a->typ = VAR;
    a->exp = e;
    a->eval = eval_var;
    return a;
}
