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
 * CAmkES tutorial part 1: components with RPC. Server part.
 */


#include <stdio.h>

/* generated header for our component */
#include <camkes.h>

/* TODO: implement the RPC function. */
void hello_say_hi(void) {
  printf("Echo server: hi\n");
}
