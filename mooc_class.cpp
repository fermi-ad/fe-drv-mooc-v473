// $Id$

#include <cstdio>
#include <memory>
#include <mooc++-4.0.h>
#include "v473.h"

// Local prototypes...

static void term(void) __attribute__((destructor));

// This function gets called when trying to unload the module under
// VxWorks. Since we don't support it, let the user know she screwed
// up the machine.

static void term(void)
{
    printf("ERROR: the V473 module is not designed to be unloaded! This\n"
	   "       system is now unstable -- a reboot is recommended!");
}

// This function initializes an instance of the MOOC V473 class.

static STATUS objInit(short const oid, void const*, void const*,
		      void* ivs)
{
    return OK;
}

STATUS v473_create_mooc_instance(unsigned short const oid,
				 uint8_t const addr,
				 uint8_t const intVec)
{
    try {
	short const cls = find_class("V473");

	if (cls == -1)
	    throw std::runtime_error("V473 class is not registered with MOOC");

	std::auto_ptr<V473::Card> ptr(v473_create(addr, intVec));

	if (create_instance(oid, cls, ptr.get(), "V473") != NOERR)
	    throw std::runtime_error("problem creating an instance");

	ptr.release();
	return OK;
    }
    catch (std::exception const& e) {
	printf("ERROR: %s\n", e.what());
	return ERROR;
    }
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
