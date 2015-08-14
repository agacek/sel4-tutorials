/*
 * Copyright 2015, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

/*
 * seL4 tutorial part 2: create and run a new thread
 */

/* Include Kconfig variables. */
#include <autoconf.h>

#include <stdio.h>
#include <assert.h>

#include <sel4/sel4.h>

#include <simple/simple.h>
#include <simple-default/simple-default.h>

#include <vka/object.h>

#include <allocman/allocman.h>
#include <allocman/bootstrap.h>
#include <allocman/vka.h>


/* static memory for the allocator to bootstrap with */
#define ALLOCATOR_STATIC_POOL_SIZE ((1 << seL4_PageBits) * 10)
static char allocator_mem_pool[ALLOCATOR_STATIC_POOL_SIZE];

/* stack for the new thread */
#define THREAD_2_STACK_SIZE 512
static uint64_t thread_2_stack[THREAD_2_STACK_SIZE];

/* convenience function */
extern void name_thread(seL4_CPtr tcb, char *name);

/* function to run in the new thread */
void thread_2(void) {
  printf("child: hello world\n");
  abort();
}

int main(void)
{
    /* give us a name: useful for debugging if the thread faults */
    name_thread(seL4_CapInitThreadTCB, "hello-2");

    /* TODO: get boot info: hint seL4_GetBootInfo() */
    seL4_BootInfo *bi = seL4_GetBootInfo();

    /* TODO: init simple: hint simple_default_init_bootinfo() */
    simple_t *simple = malloc(sizeof(simple_t));
    simple_default_init_bootinfo(simple, bi);

    /* TODO: print out bootinfo and other info about simple: hint simple_print() */
    simple_print(simple);

    /* TODO: create an allocator: hint bootstrap_use_current_simple() */
    allocman_t *alloc = bootstrap_use_current_simple(simple, ALLOCATOR_STATIC_POOL_SIZE, allocator_mem_pool);

    /* TODO: create a vka (interface for interacting with the underlying allocator)
     * hint: allocman_make_vka() */
    vka_t *vka = malloc(sizeof(vka_t));
    allocman_make_vka(vka, alloc);

    /* TODO: get our cspace root cnode: hint simple_get_cnode() */
    seL4_CPtr root = simple_get_cnode(simple);

    /* TODO: get our vspace root page diretory: hint simple_get_pd() */
    seL4_CPtr pd = simple_get_pd(simple);

    /* TODO: create a new TCB: hint vka_alloc_tcb() */
    vka_object_t *tcb = malloc(sizeof(vka_object_t));
    vka_alloc_tcb(vka, tcb);

    /* TODO: initialise the new TCB:
     * hint 1: seL4_TCB_Configure()
     * hint 2: use seL4_CapNull for the fault endpoint
     * hint 3: use seL4_NilData for cspace and vspace data
     * hint 4: we don't need an IPC buffer frame or address yet */
    /*
      seL4_TCB_Configure(seL4_TCB service,
                         seL4_Word fault_ep,
			 seL4_Uint8 priority,
			 seL4_CNode cspace_root,
			 seL4_CapData_t cspace_root_data,
			 seL4_CNode vspace_root,
			 seL4_CapData_t vspace_root_data,
			 seL4_Word buffer,
			 seL4_CPtr bufferFrame)
    */
    seL4_TCB_Configure(tcb->cptr,
		       seL4_CapNull,
		       seL4_MaxPrio,
		       root,
		       seL4_NilData,
		       pd,
		       seL4_NilData,
		       0,
		       0);

    /* TODO: give the new thread a name */
    name_thread(tcb->cptr, "Child thread");

    /*
     * set start up registers for the new thread:
     */
    seL4_UserContext *regs = malloc(sizeof(seL4_UserContext));

    /* TODO: set instruction pointer where the thread shoud start running
     * hint: sel4utils_set_instruction_pointer() */
    sel4utils_set_instruction_pointer(regs, (seL4_Word) &thread_2);

    /* TODO: set stack pointer for the new thread.
     * hint 1: sel4utils_set_stack_pointer
     * hint 2: remember the stack grows down */
    sel4utils_set_stack_pointer(regs, (seL4_Word) &thread_2_stack[THREAD_2_STACK_SIZE]);

    /* TODO: actually write the TCB registers.  we write 2 registers:
     * instruction pointer is first, stack pointer is second.
     * hint: seL4_TCB_WriteRegisters() */
    /*
      seL4_TCB_WriteRegisters(seL4_TCB service,
                              seL4_Bool resume_target,
			      seL4_Uint8 arch_flags,
			      seL4_Word count,
			      seL4_UserContext *regs)
    */
    seL4_TCB_WriteRegisters(tcb->cptr, seL4_False, 0, 2, regs);

    /* TODO: start the new thread running: hint seL4_TCB_Resume() */
    seL4_TCB_Resume(tcb->cptr);

    /* we are done, say hello */
    printf("main: hello world\n");

    return 0;
}

