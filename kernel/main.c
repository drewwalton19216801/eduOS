/*
 * Copyright (c) 2010, Stefan Lankes, RWTH Aachen University
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the University nor the names of its contributors
 *      may be used to endorse or promote products derived from this
 *      software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <eduos/stddef.h>
#include <eduos/stdio.h>
#include <eduos/string.h>
#include <eduos/time.h>
#include <eduos/tasks.h>
#include <eduos/processor.h>
#include <eduos/tasks.h>
#include <eduos/syscall.h>
#include <eduos/memory.h>
#include <eduos/vma.h>
#include <eduos/fs.h>

#include <asm/irq.h>
#include <asm/atomic.h>
#include <asm/page.h>
#include <asm/uart.h>

/*
 * Note that linker symbols are not variables, they have no memory allocated for
 * maintaining a value, rather their address is their value.
 */
extern const void kernel_start;
extern const void kernel_end;
extern const void bss_start;
extern const void bss_end;
extern char __BUILD_DATE;
extern char __BUILD_TIME;

/* Page frame counters */
extern atomic_int32_t total_pages;
extern atomic_int32_t total_allocated_pages;
extern atomic_int32_t total_available_pages;

#if 0
// Demo of a user-level task
static void NORETURN userfoo(void* arg)
{
	char str[] = "hello from userfoo\n";

	SYSCALL3(__NR_write, 0, str, 20);
	SYSCALL1(__NR_exit, 0);

	while(1) ;
}

static char ustack[KERNEL_STACK_SIZE]  __attribute__ ((aligned (PAGE_SIZE)));

static int wrapper(void* arg)
{
	size_t* stack = (size_t*) (ustack+KERNEL_STACK_SIZE-16);

	memset(ustack, 0xCD, KERNEL_STACK_SIZE);
	*stack-- = (size_t) arg;
	*stack = (size_t) NULL; // put exit function as caller on the stack

#if 0
	// this triggers a page fault because a user task is not able to access the kernel space
	return jump_to_user_code((size_t) userfoo, (size_t) stack);
#else
	// dirty hack, map userfoo to the user space
	size_t phys = virt_to_phys(((size_t) userfoo) & PAGE_MASK);
	size_t vuserfoo = KERNEL_SPACE+PAGE_SIZE;
	page_map(vuserfoo, phys, 2, PG_PRESENT | PG_USER);
	vuserfoo += (size_t)userfoo & (PAGE_SIZE-1);

	// dirty hack, map ustack to the user space
	phys = virt_to_phys((size_t) ustack);
	size_t vstack = 3*KERNEL_SPACE;
	page_map(vstack, phys, KERNEL_STACK_SIZE >> PAGE_BITS, PG_PRESENT | PG_RW | PG_USER);
	vstack = (vstack + KERNEL_STACK_SIZE - 16);

	return jump_to_user_code(vuserfoo, vstack);
#endif
}
#endif

static int foo(void* arg)
{
	int i;

	for(i=0; i<2; i++) {
		kprintf("hello from %s\n", (char*) arg);
	}

	// demo of an exception
	/*i = 0;
	i = 32 / i;
	kprintf("i = %d\n", i);*/

	return 0;
}

static int eduos_init(void)
{
	// initialize .bss section
	memset((void*)&bss_start, 0x00, ((size_t) &bss_end - (size_t) &bss_start));

	koutput_init();
	system_init();
	irq_init();
	timer_init();
	multitasking_init();
	memory_init();
#ifdef CONFIG_UART
	uart_init();
#endif
	initrd_init();

	return 0;
}

int main(void)
{
	char* argv1[] = {"/bin/shell", NULL};

	eduos_init();
	system_calibration(); // enables also interrupts

	kprintf("This is eduOS %s Build %u, %u\n", EDUOS_VERSION, &__BUILD_DATE, &__BUILD_TIME);
	kprintf("Kernel starts at %p and ends at %p\n", &kernel_start, &kernel_end);
	kprintf("Processor frequency: %u MHz\n", get_cpu_frequency());
	kprintf("Total memory: %lu KiB\n", atomic_int32_read(&total_pages) * PAGE_SIZE / 1024);
	kprintf("Current allocated memory: %lu KiB\n", atomic_int32_read(&total_allocated_pages) * PAGE_SIZE / 1024);
	kprintf("Current available memory: %lu KiB\n", atomic_int32_read(&total_available_pages) * PAGE_SIZE / 1024);

	//vma_dump();

	create_user_task(NULL, "/bin/shell", argv1);

#if 0
	kputs("Filesystem:\n");
	list_fs(fs_root, 1);
#endif

	// x64: wrapper maps function to user space to start a user-space task
	//create_kernel_task(NULL, wrapper, "userfoo", NORMAL_PRIO);

	while(1) { 
		HALT;
	}

	return 0;
}
