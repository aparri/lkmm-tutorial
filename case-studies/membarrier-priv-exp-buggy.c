/*
 * Based on:
 *   - kernel/sched/membarrier.c
 *   - kernel/sched/core.c
 *   - arch/powerpc/include/asm/membarrier.h
 */

#define MEMBAR_PRIV_EXP_READY	(1U << 0)
#define MEMBAR_PRIV_EXP		(1U << 1)

void membarrier_register_private_expedited(int flags)
{
	struct mm_struct *mm = current->mm;

	atomic_or(MEMBAR_PRIV_EXP, &mm->membar_state);
	atomic_or(MEMBAR_PRIV_EXP_READY, &mm->membar_state);
}

void membarrier_private_expedited()
{
	int cpu;

	if (!(atomic_read(&current->mm->membar_state) & MEMBAR_PRIV_EXP_READY))
		return -EPERM;

	smp_mb(); /* full barrier on syscall entry */
	for_each_online_cpu(cpu) {
		struct task_struct *task;

		task = task_rcu_dereference(&cpu_rq(cpu)->curr);
		if (task & task->mm == current->mm)
			smp_call_function_single(cpu, ipi_mb, NULL, 1);
	}
	smp_mb(); /* full barrier on syscall exit */
}

void __schedule()
{
	struct rq *rq = cpu_rq(smp_processor_id());
	struct task_struct *prev = rq->curr, *next;
	struct rq_flags rf;

	rq_lock(rq, &rf);
	smp_mb__after_spinlock(); /* full barrier before store to rq->curr */
	next = pick_next_task(rq, prev, &rf);
	if (likely(prev != next)) {
		rq->curr = next;

		/* in context_switch(rq, prev, next, &rf) */
		if (unlikely(atomic_read(&next->mm->membar_state) & MEMBAR_PRIV_EXP))
			smp_mb(); /* full barrier after store to rq->curr */
		/* ... also unlock the rq */
	} else {
		rq_unlock_irq(rq, &rf);
	}
}
