void producer(struct circ_buf *buf, char item)
{
	int head, tail;

	/* serialize producers */
	spin_lock(&producer_lock);

	head = READ_ONCE(buf->head);
	tail = READ_ONCE(buf->tail);

	if (CIRC_SPACE(head, tail, BUF_SIZE) > 0)
		/* insert one item into the buffer */
		buf->items[head] = item; /* A */
		/*
		 * Guarantee that the consumer sees up-to-date items.
		 *
		 * Orders A before B; matches the smp_load_acquire()
		 * in consumer() (C -> D).
		 *
		 * Forbids: (B ->rf C) and (D ->fr A)
		 */
		smp_store_release(&buf->head,
				  (head + 1) & (BUF_SIZE - 1)); /* B */
	}

	spin_unlock(&producer_lock);
}

void consumer(struct circ_buf *buf, char *item)
{
	int head, tail;

	/* serialize consumers */
	spin_lock(&consumer_lock);

	head = smp_load_acquire(&buf->head); /* C */
	tail = READ_ONCE(buf->tail);

	if (CIRC_CNT(head, tail, BUF_SIZE) > 0) {
		/* extract one item from the buffer */
		*item = buf->items[tail]; /* D */
		WRITE_ONCE(buf->tail, (tail + 1) & (BUF_SIZE - 1));
	}

	spin_unlock(&consumer_lock);
}
