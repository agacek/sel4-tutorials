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
 * seL4 tutorial part 3: IPC between 2 threads
 */

/* Include Kconfig variables. */
#include <autoconf.h>

#include <stdio.h>
#include <assert.h>

#include <sel4/sel4.h>
#include <sel4/types_gen.h>

#include <simple/simple.h>
#include <simple-default/simple-default.h>

#include <vka/object.h>
#include <vka/object_capops.h>

#include <allocman/allocman.h>
#include <allocman/bootstrap.h>
#include <allocman/vka.h>

#include <vspace/vspace.h>

#include <sel4utils/vspace.h>
#include <sel4utils/mapping.h>


/* constants */
#define IPCBUF_FRAME_SIZE_BITS 12 // use a 4K frame for the IPC buffer
#define IPCBUF_VADDR 0x7000000 // arbitrary (but free) address for IPC buffer

#define EP_BADGE 0x61 // arbitrary (but unique) number for a badge
#define MSG_DATA 0x6161 // arbitrary data to send

/* global environment variables */
seL4_BootInfo *info;
simple_t simple;
vka_t vka;
allocman_t *allocman;
vka_object_t ep_object;

/* static memory for the allocator to bootstrap with */
#define ALLOCATOR_STATIC_POOL_SIZE ((1 << seL4_PageBits) * 10)
UNUSED static char allocator_mem_pool[ALLOCATOR_STATIC_POOL_SIZE];

/* stack for the new thread */
#define THREAD_2_STACK_SIZE 512
static uint64_t thread_2_stack[THREAD_2_STACK_SIZE];

/* convenience function */
extern void name_thread(seL4_CPtr tcb, char *name);

/* function to run in the new thread */
void thread_2(void) {
    seL4_Word sender_badge;
    UNUSED seL4_MessageInfo_t tag;
    seL4_Word msg;

    printf("thread_2: hallo wereld\n");

    /* TODO: wait for a message to come in over the endpoint
     * hint: seL4_Wait */
    seL4_Wait(ep_object.cptr, &sender_badge);

    /* TODO: get the message stored in the first message register
     * hint: seL$_GetMR() */
    msg = seL4_GetMR(0);
    printf("thread_2: got a message %#x from %#x\n", msg, sender_badge);

    /* modify the message */ 
    msg = ~msg;

    /* TODO: copy the modified message back into the message register
     * hint: seL4_SetMR() */
    seL4_SetMR(0, msg);

    /* TODO: send the message back: hint seL4_ReplyWait() */
    seL4_MessageInfo_t msg_info = seL4_MessageInfo_new(0, 0, 0, 0);
    seL4_ReplyWait(ep_object.cptr, msg_info, 0);
}

int main(void)
{
    int error;

    /* give us a name: useful for debugging if the thread faults */
    name_thread(seL4_CapInitThreadTCB, "hello-3");

    /* get boot info */
    info = seL4_GetBootInfo();

    /* init simple */
    simple_default_init_bootinfo(&simple, info);

    /* print out bootinfo and other info about simple */
    simple_print(&simple);

    /* create an allocator */
    allocman = bootstrap_use_current_simple(&simple, ALLOCATOR_STATIC_POOL_SIZE, allocator_mem_pool);
    assert(allocman);

    /* create a vka (interface for interacting with the underlying allocator) */
    allocman_make_vka(&vka, allocman);

    /* get our cspace root cnode */
    seL4_CPtr cspace_cap;
    cspace_cap = simple_get_cnode(&simple);

    /* get our vspace root page directory */
    seL4_CPtr pd_cap;
    pd_cap = simple_get_pd(&simple);

    /* create a new TCB */
    vka_object_t tcb_object = {0};
    error = vka_alloc_tcb(&vka, &tcb_object);
    assert(error == 0);

    /*
     * create and map an ipc buffer:
     */

    /* TODO: get a frame cap for the ipc buffer: hint vka_alloc_frame() */
    vka_object_t ipc_frame_object = {0};
    vka_alloc_frame(&vka, IPCBUF_FRAME_SIZE_BITS, &ipc_frame_object);

    /*
     * TODO: map the frame into the vspace at ipc_buffer_vaddr.
     * To do this we first try to map it in to the root page directory.
     * If there is already a page table mapped in the appropriate slot in the
     * page diretory where we can insert this frame, then this will succeed.
     * Otherwise we first need to create a page table, and map it in to
     * the page directory, before we can map the frame in.
     * hint 1: seL4_ARCH_Page_Map() 
     * hint 1b: seL4_AllRights seL4_ARCH_Default_VMAttributes
     * hint 2: vka_alloc_page_table()
     * hint 3: seL4_ARCH_PageTable_Map()
     */
    /*
      seL4_IA32_Page_Map(seL4_IA32_Page service,
                         seL4_IA32_PageDirectory pd,
			 seL4_Word vaddr,
			 seL4_CapRights rights,
			 seL4_IA32_VMAttributes attr)
    */
    error = seL4_IA32_Page_Map(ipc_frame_object.cptr,
			       pd_cap,
			       IPCBUF_VADDR,
			       seL4_AllRights,
			       seL4_IA32_Default_VMAttributes);

    if (error != 0) {
      vka_object_t pt_object = {0};
      vka_alloc_page_table(&vka, &pt_object);

      /*
	seL4_IA32_PageTable_Map(seL4_IA32_PageTable service,
	                        seL4_IA32_PageDirectory pd,
				seL4_Word vaddr,
				seL4_IA32_VMAttributes attr)
      */
      error = seL4_IA32_PageTable_Map(pt_object.cptr,
				      pd_cap,
				      IPCBUF_VADDR,
				      seL4_IA32_Default_VMAttributes);
      assert (error == 0);

      error = seL4_IA32_Page_Map(ipc_frame_object.cptr,
				 pd_cap,
				 IPCBUF_VADDR,
				 seL4_AllRights,
				 seL4_IA32_Default_VMAttributes);
      assert (error == 0);
    }

    /* set the IPC buffer's virtual address in a field of the IPC buffer */
    seL4_IPCBuffer *ipcbuf = (seL4_IPCBuffer*) IPCBUF_VADDR;
    ipcbuf->userData = IPCBUF_VADDR;

    /* TODO: create an endpoint: hint vka_alloc_endpoint() */
    vka_alloc_endpoint(&vka, &ep_object);

    /* TODO: make a badged copy of it in our cspace.
     * This copy will be used to send an IPC message to the original cap
     * hint 1: vka_mint_object()
     * hint 2: seL4_AllRights()
     * hint 3: seL4_CapData_Badge_new()
     */
    /*
      vka_mint_object(vka_t *vka, vka_object_t *object, cspacepath_t *result, 
        seL4_CapRights rights, seL4_CapData_t badge)
    */
    cspacepath_t path;
    vka_mint_object(&vka, &ep_object, &path, seL4_AllRights, seL4_CapData_Badge_new(EP_BADGE));

    /* initialise the new TCB */
    error = seL4_TCB_Configure(tcb_object.cptr, seL4_CapNull, seL4_MaxPrio,
        cspace_cap, seL4_NilData, pd_cap, seL4_NilData,
        IPCBUF_VADDR, ipc_frame_object.cptr);
    assert(error == 0);

    /* give the new thread a name */
    name_thread(tcb_object.cptr, "hello-3: thread_2");

    /* set start up registers for the new thread */
    seL4_UserContext regs = {0};
    size_t regs_size = sizeof(seL4_UserContext) / sizeof(seL4_Word);

    /* set instruction pointer where the thread shoud start running */
    sel4utils_set_instruction_pointer(&regs, (seL4_Word)thread_2);

    /* check that stack is aligned correctly */
    uintptr_t thread_2_stack_top = (uintptr_t)thread_2_stack + sizeof(thread_2_stack);
    assert(thread_2_stack_top % (sizeof(seL4_Word) * 2) == 0);

    /* set stack pointer for the new thread. remember the stack grows down */
    sel4utils_set_stack_pointer(&regs, thread_2_stack_top);

    /* set the gs register for thread local storage */
    regs.gs = IPCBUF_GDT_SELECTOR;

    /* actually write the TCB registers. */
    error = seL4_TCB_WriteRegisters(tcb_object.cptr, 0, 0, regs_size, &regs);
    assert(error == 0);

    /* start the new thread running */
    error = seL4_TCB_Resume(tcb_object.cptr);
    assert(error == 0);

    /* we are done, say hello */
    printf("main: hello world\n");

    /*
     * now send a message to the new thread, and wait for a reply
     */

    /* TODO: set the data to send. We send it in the first message register
     * hint 1: seL4_MessageInfo_new()
     * hint 2: seL4_SetMR() */
    seL4_SetMR(0, MSG_DATA);
    seL4_MessageInfo_t msg_info = seL4_MessageInfo_new(0, 0, 0, 0);

    /* TODO: send and wait for a reply: hint seL4_Call() */
    msg_info = seL4_Call(ep_object.cptr, msg_info);

    /* TODO: get the reply message: hint seL4_GetMR() */
    seL4_Word msg = seL4_GetMR(0);

    printf("main: got a reply: %#x\n", msg);
    abort();

    return 0;
}

