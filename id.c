#include <time.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"

static char **ids;
static int nids;
static int maxids;

static char *
find(char *s, int len)
{
    int i;

    for (i = 0; i < nids; ++i) {
        if (strlen(ids[i]) == len && !strncmp(ids[i], s, len)) {
            return ids[i];
        }
    }
    return NULL;
}

char *
install(char *id, int len)
{
    char *rval;
    char *tmp;
    int i;

    if (ids == NULL) {
        ids = malloc(sizeof *ids);
        if (ids == NULL) {
            return 0;
        }
        nids = 0;
        maxids = 1;
    }
    rval = find(id, len);
    if (rval != NULL) {
        return rval;
    }
    if (nids == maxids) {
        if (!array_grow((void **) &ids, maxids*2, sizeof *ids)) {
            return 0;
        }
        maxids *= 2;
    }
    rval = malloc(len + 1);
    if (rval == NULL) {
        return NULL;
    }
    strncpy(rval, id, len);
    rval[len] = '\0';
    ids[nids] = rval;
    for (i = nids; i > 0 && strcmp(ids[i], ids[i-1]); --i) {
        tmp = ids[i];
        ids[i] = ids[i-1];
        ids[i-1] = tmp;
    }
    nids += 1;
    return rval;
}
