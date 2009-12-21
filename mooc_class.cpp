// $Id$

#include <cstdio>
#include <mooc++-4.0.h>
#include "v473.h"

// Local prototypes...

static void term(void) __attribute__((destructor));

static void term(void)
{
    printf("ERROR: the V473 module is not designed to be unloaded! This\n"
	   "       system is now unstable -- a reboot is recommended!");
}

// Creates the MOOC class for the V473. Instances of V473::Card
// objects can be attached to instances of this MOOC class.

STATUS v473_create_mooc_class(uint8_t cls)
{
    if (cls < 16) {
	printf("MOOC class codes need to be 16, or greater.\n");
	return ERROR;
    }

    if (NOERR != create_class(cls, 0, 0, 1, sizeof(V473::Card*)))
	return ERROR;
    if (NOERR != name_class(cls, "V473"))
	return ERROR;
    if (NOERR != add_class_msg(cls, Init, (PMETHOD) objInit))
	return NOERR;

    return OK;
}
