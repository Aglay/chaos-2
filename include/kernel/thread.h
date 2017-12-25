/* ------------------------------------------------------------------------ *\
**
**  This file is part of the Chaos Kernel, and is made available under
**  the terms of the GNU General Public License version 2.
**
**  Copyright (C) 2017 - Benjamin Grange <benjamin.grange@epitech.eu>
**
\* ------------------------------------------------------------------------ */

#ifndef _KERNEL_THREAD_H_
# define _KERNEL_THREAD_H_

# include <kconfig.h>
# include <chaosdef.h>
# include <kernel/memory.h>
# include <kernel/rwlock.h>
# include <arch/thread.h>

typedef int tid_t;
typedef int (*thread_main)(void);

# if KCONFIG_DEBUG_THREAD
#  define assert_thread(x) assert(x)
# else
#  define assert_thread(x)
# endif /* KCONFIG_DEBUG_THREAD */

/*
** An enum of all states a thread can be in.
*/
enum			thread_state
{
	NONE = 0,
	EMBRYO,
	SUSPENDED,
	RUNNABLE,
	RUNNING,
	BLOCKED,
	SLEEPING,
	ZOMBIE,
};

/*
** Strings to print each thread state
*/
static char const *thread_state_str[] =
{
	[NONE]		= "NONE",
	[EMBRYO]	= "EMBRYO",
	[SUSPENDED]	= "SUSPENDED",
	[RUNNABLE]	= "RUNNABLE",
	[RUNNING]	= "RUNNING",
	[BLOCKED]	= "BLOCKED",
	[SLEEPING]	= "SLEEPING",
	[ZOMBIE]	= "ZOMBIE",
};

struct			thread
{
	/* Thread basic infos*/
	char name[256];			/* Name */
	tid_t tid;			/* Thread id */
	uchar exit_status;		/* Exit status */
	enum thread_state state;	/* Curent thread state */
	struct thread *parent;		/* Parent thread */

	/* Thread stack */
	virtaddr_t stack;
	size_t stack_size;

	/* arch-dependant stuff */
	struct arch_thread arch;

	/* entry point */
	thread_main entry;

	/* virtual address space */
	struct vaspace *vaspace;

	/* RWlock to lock this thread */
	struct rwlock rwlock;
};

/* Thread table */
extern struct thread thread_table[KCONFIG_MAX_THREADS];

void	thread_early_init(void);
void	thread_init(void);

#endif /* !_KERNEL_THREAD_H_ */
