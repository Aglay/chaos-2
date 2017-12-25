/* ------------------------------------------------------------------------ *\
**
**  This file is part of the Chaos Kernel, and is made available under
**  the terms of the GNU General Public License version 2.
**
**  Copyright (C) 2017 - Benjamin Grange <benjamin.grange@epitech.eu>
**
\* ------------------------------------------------------------------------ */

#ifndef _KERNEL_VASPACE_H_
# define _KERNEL_VASPACE_H_

# include <chaoserr.h>
# include <kernel/memory.h>
# include <kernel/rwlock.h>
# include <kernel/vseg.h>
# include <kernel/vmm.h>

/*
** Represents the virtual address space shared by a couple of threads.
**
** The golden rule is that must _NEVER_ have two virtual segments
** overlapping.
*/
struct vaspace
{
	/* All the memory segments composing this virtual address space */
	struct vseg vsegs[10]; /* TODO Change this to a struct vseg * when kalloc() */

	/* Number of virtual segments in the array above */
	size_t vseg_size;

	/* RWlock to lock the virtual address space */
	struct rwlock rwlock;

	/* Number of threads sharing this address space */
	uint count;
};

status_t	vaspace_init(struct vaspace *vaspace);
status_t	vaspace_new_vseg(virtaddr_t start, size_t size, mmap_flags_t flags);
status_t	vaspace_add_vseg(virtaddr_t start, virtaddr_t end);
void		vaspace_remove_vseg(size_t idx, munmap_flags_t flags);
void		vaspace_dump(struct vaspace *vaspace);

#endif /* !_KERNEL_VASPACE_H_ */
