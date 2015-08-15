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
 * seL4 tutorial part 4: create a new process and IPC with it
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

#include <vspace/vspace.h>

#include <sel4utils/vspace.h>
#include <sel4utils/mapping.h>
#include <sel4utils/process.h>

/* constants */
#define EP_BADGE 0x61 // arbitrary (but unique) number for a badge
#define MSG_DATA 0x6161 // arbitrary data to send

#define APP_PRIORITY seL4_MaxPrio
#define APP_IMAGE_NAME "hello-4-app"

/* global environment variables */
seL4_BootInfo *info;
simple_t simple;
vka_t vka;
allocman_t *allocman;
vspace_t vspace;

/* static memory for the allocator to bootstrap with */
#define ALLOCATOR_STATIC_POOL_SIZE ((1 << seL4_PageBits) * 10)
static char allocator_mem_pool[ALLOCATOR_STATIC_POOL_SIZE];

/* dimensions of virtual memory for the allocator to use */
#define ALLOCATOR_VIRTUAL_POOL_SIZE ((1 << seL4_PageBits) * 100)

/* static memory for virtual memory bootstrapping */
static sel4utils_alloc_data_t data;

/* stack for the new thread */
#define THREAD_2_STACK_SIZE 4096
UNUSED static int thread_2_stack[THREAD_2_STACK_SIZE];

/* convenience function */
extern void name_thread(seL4_CPtr tcb, char *name);

int main(void)
{
    int error;

    /* give us a name: useful for debugging if the thread faults */
    name_thread(seL4_CapInitThreadTCB, "hello-4");

    /* get boot info */
    info = seL4_GetBootInfo();

    /* init simple */
    simple_default_init_bootinfo(&simple, info);

    /* print out bootinfo and other info about simple */
    simple_print(&simple);

    /* create an allocator */
    allocman = bootstrap_use_current_simple(&simple, ALLOCATOR_STATIC_POOL_SIZE,
        allocator_mem_pool);
    assert(allocman);

    /* create a vka (interface for interacting with the underlying allocator) */
    allocman_make_vka(&vka, allocman);

    /* TODO: create a vspace object to manage our vspace 
     * hint: sel4utils_bootstrap_vspace_with_bootinfo_leaky() */
    /*
      sel4utils_bootstrap_vspace_with_bootinfo_leaky(
          vspace_t *vspace,
	  sel4utils_alloc_data_t *data,
	  seL4_CPtr page_directory,
	  vka_t *vka,
	  seL4_BootInfo *info)
    */
    error = sel4utils_bootstrap_vspace_with_bootinfo_leaky(&vspace,
							   &data,
							   simple_get_pd(&simple),
							   &vka,
							   info);
    assert(error == 0);

    /* fill the allocator with virtual memory */
    void *vaddr;
    UNUSED reservation_t virtual_reservation;
    virtual_reservation = vspace_reserve_range(&vspace,
        ALLOCATOR_VIRTUAL_POOL_SIZE, seL4_AllRights, 1, &vaddr);
    assert(virtual_reservation.res);
    bootstrap_configure_virtual_pool(allocman, vaddr,
        ALLOCATOR_VIRTUAL_POOL_SIZE, simple_get_pd(&simple));

    /* TODO: use sel4utils to make a new process
     * hint: sel4utils_configure_process() */
    /*
      sel4utils_configure_process(sel4utils_process_t *process,
                                  vka_t *vka,
				  vspace_t *vspace,
				  uint8_t priority,
				  char *image_name)
    */
    sel4utils_process_t process;
    error = sel4utils_configure_process(&process,
					&vka,
					&vspace,
					APP_PRIORITY,
					APP_IMAGE_NAME);
    assert(error == 0);

    /* TODO: give the new process's thread a name */
    name_thread(process.thread.tcb.cptr, "hello-4-app-process");

    /* create an endpoint */
    vka_object_t ep_object = {0};
    error = vka_alloc_endpoint(&vka, &ep_object);
    assert(error == 0);

    /* TODO: make a badged enpoint in the new process's cspace.
     * This copy will be used to send an IPC to the original cap
     * hint 1: vka_cspace_make_path()
     * hint 2: sel4utils_mint_cap_to_process()
     * hint 3: seL4_CapData_Badge_new()
     */
    cspacepath_t path;
    vka_cspace_make_path(&vka, ep_object.cptr, &path);
    seL4_CPtr ep_badged = sel4utils_mint_cap_to_process(&process,
							path,
							seL4_AllRights,
							seL4_CapData_Badge_new(EP_BADGE));
    assert(ep_badged);

    /* TODO: spawn the process: hint sel4utils_spawn_process_v() */
    /*
      sel4utils_spawn_process_v(sel4utils_process_t *process,
                                vka_t *vka,
				vspace_t *vspace,
				int argc,
				char *argv[],
				int resume)
    */
    error = sel4utils_spawn_process_v(&process, &vka, &vspace, 0, 0, 1);
    assert(error == 0);

    /* we are done, say hello*/
    printf("main: hello world\n");

    /*
     * now wait for a message from the new process, then send a reply back
     */

    seL4_Word sender_badge;
    seL4_MessageInfo_t tag;
    seL4_Word msg;

    /* TODO: wait for a message: hint seL4_Wait() */
    seL4_Wait(ep_object.cptr, &sender_badge);

    /* get the message stored in the first message register */
    msg = seL4_GetMR(0);
    printf("main: got a message %#x from %#x\n", msg, sender_badge);

    /* modify the message */
    seL4_SetMR(0, ~msg);

    /* TODO: and send it back: hint seL4_ReplyWait() */
    tag = seL4_MessageInfo_new(0, 0, 0, 1);
    seL4_ReplyWait(ep_object.cptr, tag, 0);

    return 0;
}

