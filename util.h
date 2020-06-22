/* Return distance of [start, end) in months */
int monthdist(time_t start, time_t end);

/* Return non-zero if the block at *addr was successfully resized, zero otherwise */
int array_grow(void **addr, unsigned nelem, unsigned size);

/* Given a dollar amount as a string, return the amount in cents */
long cents(char *num);

/* Return the name of month i for i in [0, 11] */
char *monthname(int i);
