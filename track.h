enum expr_type {
    EXPR_ADD = '+',
    EXPR_SUB = '-',
    EXPR_MUL = '*',
    EXPR_DIV = '/',
    EXPR_EXP = '^',
    EXPR_NUM,
    EXPR_ID
};
typedef enum expr_type expr_type;

struct account;
typedef struct account account;

struct expr {
    expr_type type;
    double ival;
    account *aval;
    struct expr *left;
    struct expr *right;
};
typedef struct expr expr;

double eval(expr *e, int year, int month, int *ok);
expr *expr_new(expr_type type, account *aval, long ival, expr *left, expr *right);
account *tracker_new(char *name, char *longname, expr *e);
void expr_print(FILE *fp, expr *e);
