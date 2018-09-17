/*
 * Based on:
 *   - include/linux/circ_buf.h
 *   - Documentation/core-api/circular-buffers.rst
 */

struct circ_buf {
	char *items;
	int head;
	int tail;
};

/* Return count in buffer. */
#define CIRC_CNT(head, tail, size)	(((head) - (tail)) & ((size) - 1))
/* Return space available, from 0 to (size - 1). */
#define CIRC_SPACE(head, tail, size)	CIRC_CNT((tail), ((head) + 1), (size))

void producer(struct circ_buf *buf, char item)
{
	int head, tail;

	/* serialize producers */
	spin_lock(&producer_lock);

	head = READ_ONCE(buf->head);
	tail = READ_ONCE(buf->tail);

	if (CIRC_SPACE(head, tail, BUF_SIZE) > 0) {
		/* insert one item into the buffer */
		buf->items[head] = item;
		WRITE_ONCE(buf->head, (head + 1) & (BUF_SIZE - 1));
	}

	spin_unlock(&producer_lock);
}

void consumer(struct circ_buf *buf, char *item)
{
	int head, tail;

	/* serialize consumers */
	spin_lock(&consumer_lock);

	head = READ_ONCE(buf->head);
	tail = READ_ONCE(buf->tail);

	if (CIRC_CNT(head, tail, BUF_SIZE) > 0) {
		/* extract one item from the buffer */
		*item = buf->items[tail];
		WRITE_ONCE(buf->tail, (tail + 1) & (BUF_SIZE - 1));
	}

	spin_unlock(&consumer_lock);
}
