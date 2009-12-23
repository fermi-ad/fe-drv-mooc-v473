// $Id$

#include <vxWorks.h>
#include <cstdio>
#include <memory>
#include <mooc++-4.1.h>
#include "v473.h"

typedef unsigned char chan_t;
typedef unsigned char type_t;

#define OMSPDEF_TO_CHAN(a)	((chan_t) (a).chan)
#define OMSPDEF_TO_TYPE(a)	((type_t) (a).typ)

#define	REQ_TO_CHAN(a)	OMSPDEF_TO_CHAN(*(OMSP_DEF const*) &(a)->OMSP)
#define	REQ_TO_TYPE(a)	OMSPDEF_TO_TYPE(*(OMSP_DEF const*) &(a)->OMSP)
#define REQ_TO_SUBCODE(req)   ((((OMSP_DEF const*)&(req)->OMSP)->chan & 0xf0) >> 4)
#define REQ_TO_453CHAN(req)   (((OMSP_DEF const*)&(req)->OMSP)->chan & 0x3)
// for Subcode 15 only...
#define REQ_TO_453TYPE(req)  ((((OMSP_DEF*)&(req)->OMSP)->typ >> 6) & 0x3)
#define REQ_TO_TABLE(req) (((OMSP_DEF*)&(req)->OMSP)->typ & 0x3f)
#define REQ_TO_START(req) ((req)->misc2 & 0xff)
#define REQ_TO_DELTA(req) ((req)->misc2 >> 12)
#define REQ_TO_TOTAL(req) (((req)->misc2 >> 8) & 0xf)

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

static STATUS objInit(short const oid, V473::Card* const ptr, void const*,
		      V473::Card** const ivs)
{
    *ivs = ptr;
    return OK;
}

typedef bool (V473::Card::*STableCallback)(vwpp::Lock const&, uint16_t,
					   uint16_t, uint16_t*, uint16_t);

// Reading the simple tables and the maps are done practically the
// same way. This function encapsulates the similarities.

static STATUS readSimpleTable(RS_REQ const* req, size_t const entrySize,
			      size_t const maxSize, V473::Card* const obj,
			      STableCallback mt, uint16_t* const ptr)
{
    if (REQ_TO_453CHAN(req) >= 4)
	return ERR_BADCHN;
    if (req->ILEN % entrySize || req->ILEN > maxSize)
	return ERR_BADLEN;
    if (req->OFFSET % entrySize || req->OFFSET > maxSize - entrySize)
	return ERR_BADOFF;
    if (req->OFFSET + req->ILEN > maxSize)
	return ERR_BADOFLEN;

    vwpp::Lock lock(obj->mutex, 100);

    if (!(obj->*mt)(lock, REQ_TO_453CHAN(req), req->OFFSET / entrySize,
		    ptr, req->ILEN / entrySize))
	return ERR_MISBOARD;
    return OK;
}

static STATUS devReading(short const cls, RS_REQ const* const req,
			 void* const rep, V473::Card* const* const ivs)
{
    try {
	switch (REQ_TO_SUBCODE(req)) {
	 case 1:		// G(i) tables. We dont have these, so fake it.
	    if (rep)
		memset(rep, 0, req->ILEN);
	    break;

	 case 2:		// F(t) tables.
	     {
		 static size_t const entrySize = 4;
		 static size_t const maxSize = 15 * 64 * entrySize;
		 size_t const length = req->ILEN;
		 size_t const offset = req->OFFSET;

		 if (REQ_TO_453CHAN(req) >= 4)
		     return ERR_BADCHN;
		 if (length % entrySize || length > maxSize)
		     return ERR_BADLEN;
		 if (offset % entrySize || offset > maxSize - entrySize)
		     return ERR_BADOFF;
		 if (offset + length > maxSize)
		     return ERR_BADOFLEN;

		 static size_t const rampSize = 64 * entrySize;
		 vwpp::Lock lock((*ivs)->mutex, 100);

		 if (!(*ivs)->getRamp(lock, REQ_TO_453CHAN(req),
				      offset / rampSize + 1,
				      (offset % rampSize) / 4,
				      (uint16_t*) rep, length / 2))
		     return ERR_MISBOARD;
	     }
	     break;

	 case 3:		// Delay Table
	    return readSimpleTable(req, 2, 64, *ivs,
				   &V473::Card::getDelays,
				   (uint16_t*) rep);

	 case 4:		// Offset Table
	    return readSimpleTable(req, 2, 64, *ivs,
				   &V473::Card::getOffsets,
				   (uint16_t*) rep);

	 case 5:
	     {
		 static STableCallback const mt[] = {
		     &V473::Card::getRampMap,
		     &V473::Card::getScaleFactorMap,
		     &V473::Card::getOffsetMap,
		     &V473::Card::getFrequencyMap,
		     &V473::Card::getPhaseMap
		 };
		 static size_t const nTables = sizeof(mt) / sizeof(*mt);
		 static size_t const entrySize = 2;
		 static size_t const tableSize = 32 * entrySize;
		 static size_t const maxSize = nTables * tableSize;

		 size_t length = req->ILEN;
		 size_t offset = req->OFFSET;
		 uint16_t* ptr = (uint16_t*) rep;

		 if (REQ_TO_453CHAN(req) >= 4)
		     return ERR_BADCHN;
		 if (length % entrySize || length > maxSize)
		     return ERR_BADLEN;
		 if (offset % entrySize || offset > maxSize - entrySize)
		     return ERR_BADOFF;
		 if (offset + length > maxSize)
		     return ERR_BADOFLEN;

		 vwpp::Lock lock((*ivs)->mutex, 100);

		 for (size_t ii = 0; length > 0 && ii < nTables; ++ii)
		     if (offset < ii * tableSize) {
			 size_t const total(std::min(length, (ii * tableSize) - offset));

			 if (!((*ivs)->*mt[ii])(lock, REQ_TO_453CHAN(req),
						(offset % 64) / 2,
						ptr, total / 2))
			     return ERR_MISBOARD;
			 ptr += total / 2;
			 offset += total;
			 length -= total;
		     }
		 return NOERR;
	     }

	 case 6:		// Scale Factor Table
	    return readSimpleTable(req, 2, 64, *ivs,
				   &V473::Card::getScaleFactors,
				   (uint16_t*) rep);

	 case 9:		// Frequency Table
	    return readSimpleTable(req, 2, 64, *ivs,
				   &V473::Card::getFrequencies,
				   (uint16_t*) rep);

	 case 10:		// Phase Table
	    return readSimpleTable(req, 2, 64, *ivs,
				   &V473::Card::getPhases,
				   (uint16_t*) rep);

	 default:
	    return ERR_BADPROP;
	}
	return OK;
    }
    catch (std::exception const& e) {
	printf("%s: exception '%s'\n", __func__, e.what())
	return ERR_DEVICEERROR;
    }
}

// Creates an instance of the MOOC V473 class.

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
	instance_is_reentrant(oid);
	printf("New instance of V473 created. Underlying object @ %p.\n", ptr.release());
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

    if (NOERR != create_class(cls, 0, 0, 3, sizeof(V473::Card*))) {
	printf("Error returned from create_class()!\n");
	return ERROR;
    }
    if (NOERR != name_class(cls, "V473")) {
	printf("Error trying to name the class.\n");
	return ERROR;
    }
    if (NOERR != add_class_msg(cls, Init, (PMETHOD) objInit)) {
	printf("Error trying to add the Init handler.\n");
	return ERROR;
    }
    if (NOERR != add_class_msg(cls, rPRREAD, (PMETHOD) devReading)) {
	printf("Error trying to add the reading handler.\n");
	return ERROR;
    }
    if (NOERR != add_class_msg(cls, rPRSET, (PMETHOD) devReading)) {
	printf("Error trying to add the reading-of-the-setting handler.\n");
	return ERROR;
    }

    printf("V473 class successfully registered with MOOC.\n");
    return OK;
}
