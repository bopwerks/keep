/* Return distance of [start, end) in months */
int monthdist(time_t start, time_t end);

/* Return non-zero if the block at *addr was successfully resized, zero otherwise */
int array_grow(void **addr, unsigned nelem, unsigned size);
