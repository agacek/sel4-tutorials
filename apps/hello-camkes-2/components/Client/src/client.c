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
 * CAmkES tutorial part 2: events and dataports
 */

#include <stdio.h>
#include <string.h>

#include <camkes/dataport.h>

/* generated header for our component */
#include <camkes.h>

/* strings to send across to the other component */
char *s_arr[NUM_STRINGS] = { "hello", "world", "how", "are", "you?" };

/* run the control thread */
int run(void) {
    printf("%s: Starting the client\n", get_instance_name());

    /* TODO: copy strings to an untyped dataport */
    char *buffer = (char *) untyped;
    buffer[0] = NUM_STRINGS;
    buffer++;
    for (int i = 0; i < NUM_STRINGS; i++) {
      int size = strlen(s_arr[i]) + 1;
      memcpy(buffer, s_arr[i], size);
      buffer += size;
    }

    /* TODO: emit event to signal that the data is available */
    data_ready_emit();

    /* TODO: wait to get event back signalling that reply data is avaialble */
    reply_ready_wait();

    /* TODO: read the reply data from a typed dataport */
    for (int i = 0; i < strs->n; i++) {
      printf("Client got back: %s\n", strs->str[i]);
    }

    /* TODO: send the data over again, this time using two dataports, one typed
     * dataport containing dataport pointers, pointing to data in the 
     * second, untyped, dataport. */
    buffer = (char *) untyped;
    ptrs->n = NUM_STRINGS;
    for (int i = 0; i < NUM_STRINGS; i++) {
      strcpy(buffer, s_arr[i]);
      ptrs->ptr[i] = dataport_wrap_ptr(buffer);
      buffer += 100;
    }

    /* TODO: emit event to signal that the data is available */
    data_ready_emit();

    /* TODO: wait to get an event back signalling that data has been read */
    reply_ready_wait();

    /* TODO: test the read and write permissions on the dataport.  
     * When we try to write to a read-only dataport, we will get a VM fault. */
    printf("Doing bad write\n");
    strs->n = 0;

    return 0;
}
