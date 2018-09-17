/*
 * Based on:
 *   - include/linux/wait.h
 *   - kernel/sched/wait.c
 *   - kernel/sched/core.c
 */

void waiter(struct wait_queue_head *wq_head, bool *condition)
{
	DEFINE_WAIT(wait);

	for (;;) {
		/* in prepare_to_wait(wq_head, &wait, TASK_INTERRUPTIBLE) */
		unsigned long flags;

		spin_lock_irqsave(&wq_head->lock, flags);
		if (list_empty(&wait.entry))
			__add_wait_queue(wq_head, &wait);
		set_current_state(TASK_INTERRUPTIBLE);
		spin_unlock_irqrestore(&wq_head->lock, flags);

		if (*condition)
			break;
		schedule();
	}
	finish_wait(wq_head, &wait);
}

void waker(struct wait_queue_head *wq_head, bool *condition)
{
	*condition = true;

	if (waitqueue_active(wq_head)) {
		/* in wake_up(wq_head, TASK_NORMAL, 0, NULL) */
		wait_queue_entry_t *curr, *next;
		unsigned long wq_flags;

		spin_lock_irqsave(&wq_head->lock, wq_flags);
		curr = list_first_entry(wq_head, wait_queue_entry_t, entry);
		list_for_each_entry_safe_from(curr, next, wq_head, entry) {
			struct task_struct *task = curr->private;
			/* in try_to_wake_up(task, TASK_NORMAL, 0) */
			unsigned long pi_flags;

			raw_spin_lock_irqsafe(&task->pi_lock, pi_flags);
			smp_mb__after_spinlock();
			if (task->state & TASK_NORMAL) {
				task->state = TASK_RUNNING;
				/* enqueue(task) */
			}
			raw_spin_unlock_irqrestore(&task->pi_lock, pi_flags);
		}
		spin_unlock_irqrestore(&wq_head->lock, wq_flags);
	}
}
