/* ------------------------------------------------------------------------ *\
**
**  This file is part of the Chaos Kernel, and is made available under
**  the terms of the GNU General Public License version 2.
**
**  Copyright (C) 2017 - Benjamin Grange <benjamin.grange@epitech.eu>
**
\* ------------------------------------------------------------------------ */

#include <drivers/vga.h>
#include <stdio.h>

/*
** Kernel main entry point
*/
void
kmain(void)
{
	vga_set_color(VGA_BLACK, VGA_WHITE);
	vga_clear();
	printf("Hello Kernel World!");

	/* Halt */
	while (42);
}
