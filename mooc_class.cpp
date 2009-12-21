// $Id$

#include <cstdio>
#include <mooc++-4.0.h>

// Local prototypes...

static void register_class(void) __attribute__((constructor));
static void term(void) __attribute__((destructor));

static void register_class(void)
{
    printf("V473 module loaded.\n");
}

static void term(void)
{
    printf("ERROR: the V473 module is not designed to be unloaded! This\n"
	   "       system is now unstable -- a reboot is recommended!");
}
