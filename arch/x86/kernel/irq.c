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

/**
 * @author Stefan Lankes
 * @file arch/x86/kernel/irq.c
 * @brief Function definitions for irq.h and a standard IRQ-handler
 *
 *
 */

#include <eduos/stdio.h>
#include <eduos/string.h>
#include <eduos/tasks.h>
#include <eduos/errno.h>
#include <asm/irq.h>
#include <asm/idt.h>
#include <asm/isrs.h>
#include <asm/io.h>

/*
 * These are our own ISRs that point to our special IRQ handler
 * instead of the regular 'fault_handler' function
 */
extern void irq0(void);
extern void irq1(void);
extern void irq2(void);
extern void irq3(void);
extern void irq4(void);
extern void irq5(void);
extern void irq6(void);
extern void irq7(void);
extern void irq8(void);
extern void irq9(void);
extern void irq10(void);
extern void irq11(void);
extern void irq12(void);
extern void irq13(void);
extern void irq14(void);
extern void irq15(void);
extern void irq16(void);
extern void irq17(void);
extern void irq18(void);
extern void irq19(void);
extern void irq20(void);
extern void irq21(void);
extern void irq22(void);
extern void irq23(void);
extern void apic_timer(void);
extern void apic_lint0(void);
extern void apic_lint1(void);
extern void apic_error(void);
extern void apic_svr(void);

#define MAX_HANDLERS	256

/** @brief IRQ handle pointers
 *
 * This array is actually an array of function pointers. We use
 * this to handle custom IRQ handlers for a given IRQ
 */
static void* irq_routines[MAX_HANDLERS] = {[0 ... MAX_HANDLERS-1] = NULL };

/* This installs a custom IRQ handler for the given IRQ */
int irq_install_handler(unsigned int irq, irq_handler_t handler)
{
	if (irq >= MAX_HANDLERS)
		return -EINVAL;

	irq_routines[irq] = handler;

	return 0;
}

/* This clears the handler for a given IRQ */
int irq_uninstall_handler(unsigned int irq)
{
	if (irq >= MAX_HANDLERS)
		return -EINVAL;

	irq_routines[irq] = NULL;

	return 0;
}

/** @brief Remapping IRQs with a couple of IO output operations
 *
 * Normally, IRQs 0 to 7 are mapped to entries 8 to 15. This
 * is a problem in protected mode, because IDT entry 8 is a
 * Double Fault! Without remapping, every time IRQ0 fires,
 * you get a Double Fault Exception, which is NOT what's
 * actually happening. We send commands to the Programmable
 * Interrupt Controller (PICs - also called the 8259's) in
 * order to make IRQ0 to 15 be remapped to IDT entries 32 to
 * 47
 */
static int irq_remap(void)
{
	// outportb(0x20, 0x11);
	// outportb(0xA0, 0x11);
	// outportb(0x21, 0x20);
	// outportb(0xA1, 0x28);
	// outportb(0x21, 0x04);
	// outportb(0xA1, 0x02);
	// outportb(0x21, 0x01);
	// outportb(0xA1, 0x01);
	// outportb(0x21, 0x0);
	// outportb(0xA1, 0x0);
	outportb(PIC1_COMMAND, ICW1_INIT + ICW1_ICW4);	// ICW1
	outportb(PIC2_COMMAND, ICW1_INIT + ICW1_ICW4);	// ICW1
	outportb(PIC1_DATA, PIC1_OFFSET);				// ICW2
	outportb(PIC2_DATA, PIC2_OFFSET);				// ICW2
	outportb(PIC1_DATA, 4);							// ICW3: tell Master PIC that there is a slave PIC at IRQ2(0000 0100)
	outportb(PIC2_DATA, 2);							// ICW3: tell Slave PIC its cascade identity (0000 0010)
	outportb(PIC1_DATA, ICW4_8086);
	outportb(PIC2_DATA, ICW4_8086);

	// write mask enable all interrupt
	outportb(PIC1_DATA, 0x0);
	outportb(PIC2_DATA, 0x0);
	return 0;
}

/** @brief Remap IRQs and install ISRs in IDT
 *
 * We first remap the interrupt controllers, and then we install
 * the appropriate ISRs to the correct entries in the IDT.\n
 * This is just like installing the exception handlers
 */
static int irq_install(void)
{
	irq_remap();

	idt_set_gate(32, (size_t)irq0, KERNEL_CODE_SELECTOR,
		IDT_FLAG_PRESENT|IDT_FLAG_RING0|IDT_FLAG_32BIT|IDT_FLAG_INTTRAP);
	idt_set_gate(33, (size_t)irq1, KERNEL_CODE_SELECTOR,
		IDT_FLAG_PRESENT|IDT_FLAG_RING0|IDT_FLAG_32BIT|IDT_FLAG_INTTRAP);
	idt_set_gate(34, (size_t)irq2, KERNEL_CODE_SELECTOR,
		IDT_FLAG_PRESENT|IDT_FLAG_RING0|IDT_FLAG_32BIT|IDT_FLAG_INTTRAP);
	idt_set_gate(35, (size_t)irq3, KERNEL_CODE_SELECTOR,
		IDT_FLAG_PRESENT|IDT_FLAG_RING0|IDT_FLAG_32BIT|IDT_FLAG_INTTRAP);
	idt_set_gate(36, (size_t)irq4, KERNEL_CODE_SELECTOR,
		IDT_FLAG_PRESENT|IDT_FLAG_RING0|IDT_FLAG_32BIT|IDT_FLAG_INTTRAP);
	idt_set_gate(37, (size_t)irq5, KERNEL_CODE_SELECTOR,
		IDT_FLAG_PRESENT|IDT_FLAG_RING0|IDT_FLAG_32BIT|IDT_FLAG_INTTRAP);
	idt_set_gate(38, (size_t)irq6, KERNEL_CODE_SELECTOR,
		IDT_FLAG_PRESENT|IDT_FLAG_RING0|IDT_FLAG_32BIT|IDT_FLAG_INTTRAP);
	idt_set_gate(39, (size_t)irq7, KERNEL_CODE_SELECTOR,
		IDT_FLAG_PRESENT|IDT_FLAG_RING0|IDT_FLAG_32BIT|IDT_FLAG_INTTRAP);
	idt_set_gate(40, (size_t)irq8, KERNEL_CODE_SELECTOR,
		IDT_FLAG_PRESENT|IDT_FLAG_RING0|IDT_FLAG_32BIT|IDT_FLAG_INTTRAP);
	idt_set_gate(41, (size_t)irq9, KERNEL_CODE_SELECTOR,
		IDT_FLAG_PRESENT|IDT_FLAG_RING0|IDT_FLAG_32BIT|IDT_FLAG_INTTRAP);
	idt_set_gate(42, (size_t)irq10, KERNEL_CODE_SELECTOR,
		IDT_FLAG_PRESENT|IDT_FLAG_RING0|IDT_FLAG_32BIT|IDT_FLAG_INTTRAP);
	idt_set_gate(43, (size_t)irq11, KERNEL_CODE_SELECTOR,
		IDT_FLAG_PRESENT|IDT_FLAG_RING0|IDT_FLAG_32BIT|IDT_FLAG_INTTRAP);
	idt_set_gate(44, (size_t)irq12, KERNEL_CODE_SELECTOR,
		IDT_FLAG_PRESENT|IDT_FLAG_RING0|IDT_FLAG_32BIT|IDT_FLAG_INTTRAP);
	idt_set_gate(45, (size_t)irq13, KERNEL_CODE_SELECTOR,
		IDT_FLAG_PRESENT|IDT_FLAG_RING0|IDT_FLAG_32BIT|IDT_FLAG_INTTRAP);
	idt_set_gate(46, (size_t)irq14, KERNEL_CODE_SELECTOR,
		IDT_FLAG_PRESENT|IDT_FLAG_RING0|IDT_FLAG_32BIT|IDT_FLAG_INTTRAP);
	idt_set_gate(47, (size_t)irq15, KERNEL_CODE_SELECTOR,
		IDT_FLAG_PRESENT|IDT_FLAG_RING0|IDT_FLAG_32BIT|IDT_FLAG_INTTRAP);
	idt_set_gate(48, (size_t)irq16, KERNEL_CODE_SELECTOR,
		IDT_FLAG_PRESENT|IDT_FLAG_RING0|IDT_FLAG_32BIT|IDT_FLAG_INTTRAP);
	idt_set_gate(49, (size_t)irq17, KERNEL_CODE_SELECTOR,
		IDT_FLAG_PRESENT|IDT_FLAG_RING0|IDT_FLAG_32BIT|IDT_FLAG_INTTRAP);
	idt_set_gate(50, (size_t)irq18, KERNEL_CODE_SELECTOR,
		IDT_FLAG_PRESENT|IDT_FLAG_RING0|IDT_FLAG_32BIT|IDT_FLAG_INTTRAP);
	idt_set_gate(51, (size_t)irq19, KERNEL_CODE_SELECTOR,
		IDT_FLAG_PRESENT|IDT_FLAG_RING0|IDT_FLAG_32BIT|IDT_FLAG_INTTRAP);
	idt_set_gate(52, (size_t)irq20, KERNEL_CODE_SELECTOR,
		IDT_FLAG_PRESENT|IDT_FLAG_RING0|IDT_FLAG_32BIT|IDT_FLAG_INTTRAP);
	idt_set_gate(53, (size_t)irq21, KERNEL_CODE_SELECTOR,
		IDT_FLAG_PRESENT|IDT_FLAG_RING0|IDT_FLAG_32BIT|IDT_FLAG_INTTRAP);
	idt_set_gate(54, (size_t)irq22, KERNEL_CODE_SELECTOR,
		IDT_FLAG_PRESENT|IDT_FLAG_RING0|IDT_FLAG_32BIT|IDT_FLAG_INTTRAP);
	idt_set_gate(55, (size_t)irq23, KERNEL_CODE_SELECTOR,
		IDT_FLAG_PRESENT|IDT_FLAG_RING0|IDT_FLAG_32BIT|IDT_FLAG_INTTRAP);

	// add APIC interrupt handler
	idt_set_gate(123, (size_t)apic_timer, KERNEL_CODE_SELECTOR,
		IDT_FLAG_PRESENT|IDT_FLAG_RING0|IDT_FLAG_32BIT|IDT_FLAG_INTTRAP);
	idt_set_gate(124, (size_t)apic_lint0, KERNEL_CODE_SELECTOR,
		IDT_FLAG_PRESENT|IDT_FLAG_RING0|IDT_FLAG_32BIT|IDT_FLAG_INTTRAP);
	idt_set_gate(125, (size_t)apic_lint1, KERNEL_CODE_SELECTOR,
		IDT_FLAG_PRESENT|IDT_FLAG_RING0|IDT_FLAG_32BIT|IDT_FLAG_INTTRAP);
	idt_set_gate(126, (size_t)apic_error, KERNEL_CODE_SELECTOR,
		IDT_FLAG_PRESENT|IDT_FLAG_RING0|IDT_FLAG_32BIT|IDT_FLAG_INTTRAP);
	idt_set_gate(127, (size_t)apic_svr, KERNEL_CODE_SELECTOR,
		IDT_FLAG_PRESENT|IDT_FLAG_RING0|IDT_FLAG_32BIT|IDT_FLAG_INTTRAP);

	return 0;
}

int irq_init(void)
{
	idt_install();
	isrs_install();
	irq_install();

	return 0;
}

/** @brief Default IRQ handler
 *
 * Each of the IRQ ISRs point to this function, rather than
 * the 'fault_handler' in 'isrs.c'. The IRQ Controllers need
 * to be told when you are done servicing them, so you need
 * to send them an "End of Interrupt" command. If we use the PIC
 * instead of the APIC, we have two 8259 chips: The first one
 * exists at 0x20, the second one exists at 0xA0. If the second
 * controller (an IRQ from 8 to 15) gets an interrupt, you need to
 * acknowledge the interrupt at BOTH controllers, otherwise, you
 * only send an EOI command to the first controller. If you don't send
 * an EOI, it won't raise any more IRQs.
 *
 * Note: If we enabled the APIC, we also disabled the PIC. Afterwards,
 * we get no interrupts between 0 and 15.
 */
size_t** irq_handler(struct state *s)
{
	/* This is a blank function pointer */
	void (*handler) (struct state * s);

	/*
	 * Find out if we have a custom handler to run for this
	 * IRQ and then finally, run it
	 */
	if (BUILTIN_EXPECT(s->int_no < MAX_HANDLERS, 1)) {
		handler = irq_routines[s->int_no];
		if (handler)
			handler(s);
	} else kprintf("Invalid interrupt number %d\n", s->int_no);

	/*
	 * If the IDT entry that was invoked was greater-than-or-equal to 48,
	 * then we use the APIC
	 */
	if (apic_is_enabled() || s->int_no >= 123) {
		apic_eoi();
		goto leave_handler;
	}

	/*
	 * If the IDT entry that was invoked was greater-than-or-equal to 40
	 * and lower than 48 (meaning IRQ8 - 15), then we need to
	 * send an EOI to the slave controller of the PIC
	 */
	if (s->int_no >= 40)
		outportb(0xA0, 0x20);

	/*
	 * In either case, we need to send an EOI to the master
	 * interrupt controller of the PIC, too
	 */
	outportb(0x20, 0x20);

leave_handler:
	// timer interrupt?
	if (s->int_no == 32)
		return scheduler(); // switch to a new task
	else if ((s->int_no >= 32) && (get_highest_priority() > current_task->prio))
		return scheduler();

	return NULL;
}
