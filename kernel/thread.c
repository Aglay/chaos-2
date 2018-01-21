/* ------------------------------------------------------------------------ *\
**
**  This file is part of the Chaos Kernel, and is made available under
**  the terms of the GNU General Public License version 2.
**
**  Copyright (C) 2017 - Benjamin Grange <benjamin.grange@epitech.eu>
**
\* ------------------------------------------------------------------------ */

#include <kconfig.h>
#include <kernel/thread.h>
#include <kernel/vaspace.h>
#include <kernel/rwlock.h>
#include <kernel/kalloc.h>
#include <string.h>

/* Thread table */
struct thread thread_table[KCONFIG_MAX_THREADS] = { 0 };

/* The virtual address space for the kthread process. */
static struct vaspace kthread_vaspace;

/*
** Sets the name of the given thread.
*/
static void
thread_set_name(struct thread *t, char const *name)
{
	strncpy(t->name, name, sizeof(t->name) - 1);
	t->name[sizeof(t->name) - 1] = '\0';
}

/*
** Creates both user and kernel stacks for the current thread.
**
** This assumes the current thread and the current virtual address space are both
** locked as writers.
*/
status_t
thread_create_stacks(void)
{
	struct thread *t;

	t = current_cpu()->thread;
	assert_thread(t->state == EMBRYO);
	assert_thread(!t->stack);
	assert_thread(!t->kstack);

	t->stack = vaspace_new_random_vseg(
		KCONFIG_THREAD_STACK_SIZE * PAGE_SIZE,
		MMAP_USER | MMAP_WRITE
	);
	if (!t->stack) {
		return (ERR_NO_MEMORY);
	}
	t->kstack = kalloc(KCONFIG_KERNEL_STACK_SIZE * PAGE_SIZE);
	if (!t->kstack) {
		/* TODO FIXME Free thread stack here */
		return (ERR_NO_MEMORY);
	}
	t->stack_top = (uchar *)t->stack + KCONFIG_THREAD_STACK_SIZE * PAGE_SIZE;
	t->stack_saved = t->stack_top;
	t->kstack_top = (uchar *)t->kstack + KCONFIG_KERNEL_STACK_SIZE * PAGE_SIZE;
	return (OK);
}

/*
** Attaches the given virtual space to the given thread.
** This assumes there is no previous virtual space already attached.
**
** This assumes that the given thread 'thread' and the given
** virtual memory space 'vaspace' are locked as writers (or there is no
** need to do so).
*/
static void
thread_attach_vaspace(struct thread *t, struct vaspace *vaspace)
{
	assert_thread(!t->vaspace);

	t->vaspace = vaspace;
	++t->vaspace->count;
}

/*
** Put us in a thread context
** Called directly from kmain()
*/
void
thread_early_init(void)
{
	struct thread *t;
	size_t i;

	/* Init thread table */
	i = 0;
	while (i < KCONFIG_MAX_THREADS) {
		spinlock_init(&thread_table[i].lock);
		thread_table[i].state = NONE;
		++i;
	}

	/* Init boot thread */
	t = thread_table;
	spinlock_acquire(&t->lock);
	{
		thread_set_name(t, "boot");
		t->state = EMBRYO;
	}
	spinlock_release(&t->lock);
}

/*
** Sets up the kthread thread
*/
void
thread_init(void)
{
	struct thread *t;

	t = thread_table;
	spinlock_acquire(&t->lock);
	{
		thread_set_name(t, "kthread");

		/* Setup vaspace */
		assert_eq(vaspace_init(&kthread_vaspace), OK);
		rwlock_acquire_write(&kthread_vaspace.rwlock);
		{
			thread_attach_vaspace(t, &kthread_vaspace);
		}
		rwlock_release_write(&kthread_vaspace.rwlock);
	}
	spinlock_release(&t->lock);
}
