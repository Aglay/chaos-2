/* ------------------------------------------------------------------------ *\
**
**  This file is part of the Chaos Kernel, and is made available under
**  the terms of the GNU General Public License version 2.
**
**  Copyright (C) 2017 - Benjamin Grange <benjamin.grange@epitech.eu>
**
\* ------------------------------------------------------------------------ */

#include <kernel/init.h>
#include <kernel/pmm.h>
#include <arch/x86/vmm.h>
#include <arch/x86/asm.h>
#include <string.h>
#include <stdio.h>

/*
** WARNING:
**
** All the functions in this file are basic primitives that do not try to lock any mutex.
** More high-level code should do it instead, like the mmap() function.
*/

/*
** Returns the address of the current page directory
** This takes advantages of the recursive mapping we set up.
*/
static inline struct page_dir *
get_pagedir(void)
{
	return ((struct page_dir *)0xFFFFF000ul);
}

/*
** Returns the address of the page table at index 'x'.
** This takes advantages of the recursive mapping we set up.
*/
static inline struct page_table *
get_pagetable(uint x)
{
	return ((struct page_table *)(0xFFC00000ul | (((x) & 0x3FF) << 12u)));
}

/*
** Returns the index within the page directory of the page table containg the
** given address.
*/
static inline uint
get_pd_idx(virtaddr_t va)
{
	return ((uintptr)va >> 22u);
}

/*
** Returns the index within the page table of the page the
** given address belongs to.
*/
static inline uint
get_pt_idx(virtaddr_t va)
{
	return (((uintptr)(va) >> 12u) & 0x3FF);
}

/*
** Returns the address of the page with given page directory index
** and given page table index.
*/
static inline virtaddr_t
get_virtaddr(uint pdidx, uint ptidx)
{
	return ((virtaddr_t)((pdidx) << 22u | (ptidx) << 12u));
}

/*
** Used for debugging purposes. Dumps the page table at given index.
*/
static void
vmm_dump_pagetable(uint pd_idx)
{
	struct page_table *pt;
	uint j;

	j = 0;
	pt = get_pagetable(pd_idx);
	while (j < 1024u)
	{
		if (pt->entries[j].present) {
			printf("\t[%4u] [%p -> %p] %s %c %c %c %c %c\n",
				j,
				(virtaddr_t)((pd_idx << 22u) | (j << 12u)),
				(virtaddr_t)(pt->entries[j].frame << 12ul),
				"RO\0RW" + pt->entries[j].rw * 3,
				"ku"[pt->entries[j].user],
				"-w"[pt->entries[j].wtrough],
				"-d"[pt->entries[j].cache],
				"-a"[pt->entries[j].accessed],
				"-d"[pt->entries[j].dirty]);
		}
		++j;
	}
}

/*
** Used for debugging purposes. Dumps the current page directory.
*/
void
vmm_dump_mem(void)
{
	uint i;
	struct page_dir *pd;

	pd = get_pagedir();
	for (i = 0; i < 1024u; ++i)
	{
		if (pd->entries[i].present)
		{
			printf("[%4u] [%p -> %p] %s %c %c %c %c %c\n",
				i,
				(virtaddr_t)(i << 22u),
				(virtaddr_t)(pd->entries[i].frame << 12ul),
				"RO\0RW" + pd->entries[i].rw * 3,
				"ku"[pd->entries[i].user],
				"-w"[pd->entries[i].wtrough],
				"-d"[pd->entries[i].cache],
				"-a"[pd->entries[i].accessed],
				"-H"[pd->entries[i].size]);
			if (i != 1023) {
				vmm_dump_pagetable(i);
			}
		}
	}
}

/*
** Tells if the given virtual address is mapped.
*/
bool
vmm_is_mapped(virtaddr_t va)
{
	struct pagedir_entry *pde;
	struct pagetable_entry *pte;

	assert_vmm(IS_PAGE_ALIGNED(va));
	pde = get_pagedir()->entries + get_pd_idx(va);
	if (!pde->present) {
		return (false);
	}
	pte = get_pagetable(get_pd_idx(va))->entries + get_pt_idx(va);
	return (pte->present);
}

/*
** Returns the frame that the given address is mapped to, or NULL_FRAME
** if it's not mapped.
*/
physaddr_t
vmm_get_frame(virtaddr_t va)
{
	struct pagedir_entry *pde;
	struct pagetable_entry *pte;

	assert_vmm(IS_PAGE_ALIGNED(va));
	pde = get_pagedir()->entries + get_pd_idx(va);
	if (!pde->present) {
		return (NULL_FRAME);
	}
	pte = get_pagetable(get_pd_idx(va))->entries + get_pt_idx(va);
	return (pte->frame << 12u);
}

/*
** Maps a physical address to a virtual one.
*/
status_t
vmm_map_physical(virtaddr_t va, physaddr_t pa, mmap_flags_t flags)
{
	struct pagedir_entry *pde;
	struct pagetable_entry *pte;
	struct page_table *pt;

	assert_vmm(IS_PAGE_ALIGNED(va));
	assert_vmm(IS_PAGE_ALIGNED(pa));
	pde = get_pagedir()->entries + get_pd_idx(va);
	pt = get_pagetable(get_pd_idx(va));
	if (pde->present == false)
	{
		pde->value = alloc_frame();
		if (pde->value == NULL_FRAME) {
			return (ERR_NO_MEMORY);
		}
		pde->present = true;
		pde->rw = true;
		pde->user = (bool)(flags & MMAP_USER);
		pde->accessed = false;
		invlpg(pt);
		memset(pt, 0, PAGE_SIZE);
	}
	pte = pt->entries + get_pt_idx(va);

	if (pte->present)
	{
		/* If MMAP_REMAP, free old frame. Else, throw an error. */
		if (!(flags & MMAP_REMAP)) {
			return (ERR_ALREADY_MAPPED);
		}
		free_frame(pte->frame << 12u);
	}
	pte->value = pa;
	pte->present = true;
	pte->rw = (bool)(flags & MMAP_WRITE);
	pte->user = (bool)(flags & MMAP_USER);
	pte->accessed = false;
	pte->dirty = 0;
	invlpg(va);
	return (OK);
}

/*
** Maps the given virtual address to a random physical addresses.
*/
status_t
vmm_map_virtual(virtaddr_t va, mmap_flags_t flags)
{
	physaddr_t pa;
	status_t s;

	pa = alloc_frame();
	if (pa != NULL_FRAME)
	{
		s = vmm_map_physical(va, pa, flags);
		if (s == OK) {
#if KCONFIG_DEBUG_VMM
			/* Clean the new page with random shitty values */
			memset(va, 42, PAGE_SIZE);
#endif /* KCONFIG_DEBUG_VMM */
			return (OK);
		}
		free_frame(pa);
		return (s);
	}
	return (ERR_NO_MEMORY);
}

/*
** Unmaps a virtual address
*/
void
vmm_unmap(virtaddr_t va, munmap_flags_t flags)
{
	struct pagedir_entry *pde;
	struct pagetable_entry *pte;

	pde = get_pagedir()->entries + get_pd_idx(va);
	if (pde->present) {
		pte = get_pagetable(get_pd_idx(va))->entries + get_pt_idx(va);
		if (pte->present)
		{
			if (!(flags & MUNMAP_DONTFREE)) {
				free_frame(pte->frame << 12u);
			}
			pte->value = 0;
			invlpg(va);
		}
	}
}

/*
** Inits the arch-dependant virtual memory manager, and then calls
** the arch-independant virtual memory manager init function.
*/
static void
arch_vmm_init(void)
{
	/* Some assertions that can't be static_assert() */
	assert(IS_PAGE_ALIGNED(KERNEL_VIRTUAL_BASE));
	assert(IS_PAGE_ALIGNED(KERNEL_VIRTUAL_END));
	assert(IS_PAGE_ALIGNED(KERNEL_PHYSICAL_END));

	vmm_init();
}

NEW_INIT_HOOK(arch_vmm_init, &arch_vmm_init, INIT_LEVEL_VMM);
