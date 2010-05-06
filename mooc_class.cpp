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

#define	REQ_TO_CHAN(a)		OMSPDEF_TO_CHAN(*(OMSP_DEF const*) &(a)->OMSP)
#define	REQ_TO_TYPE(a)		OMSPDEF_TO_TYPE(*(OMSP_DEF const*) &(a)->OMSP)
#define REQ_TO_SUBCODE(req)	((((OMSP_DEF const*)&(req)->OMSP)->chan & 0xf0) >> 4)
#define REQ_TO_453CHAN(req)	(((OMSP_DEF const*)&(req)->OMSP)->chan & 0x3)

// for Subcode 15 only...

#define REQ_TO_453TYPE(req)	((((OMSP_DEF*)&(req)->OMSP)->typ >> 6) & 0x3)
#define REQ_TO_TABLE(req)	(((OMSP_DEF*)&(req)->OMSP)->typ & 0x3f)
#define REQ_TO_START(req)	((req)->misc2 & 0xff)
#define REQ_TO_DELTA(req)	((req)->misc2 >> 12)
#define REQ_TO_TOTAL(req)	(((req)->misc2 >> 8) & 0xf)

#define DATAS(req) (*(unsigned short const*)(req)->data)

// Junk class. Some versions of GCC don't honor the
// constructor/destructor attributes unless there's a C++ global
// object needing to be created/destroyed. This small section creates
// an unused object to make sure these functions are called.

namespace v473 { class Junk {} junk; };

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

typedef bool (V473::Card::*STableReadCallback)(vwpp::Lock const&, uint16_t,
					       uint16_t, uint16_t*, uint16_t);
typedef bool (V473::Card::*STableWriteCallback)(vwpp::Lock const&, uint16_t,
						uint16_t, uint16_t const*,
						uint16_t);

// Reading the simple tables and the maps are done practically the
// same way. This function encapsulates the similarities.

static STATUS readSimpleTable(RS_REQ const* const req, size_t const entrySize,
			      size_t const maxSize, V473::Card* const obj,
			      STableReadCallback mt, uint16_t* const ptr)
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
    return NOERR;
}

static STATUS writeSimpleTable(RS_REQ const* const req, size_t const entrySize,
			       size_t const maxSize, V473::Card* const obj,
			       STableWriteCallback mt,
			       uint16_t const* const ptr)
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
    return NOERR;
}

#define BUMP(l,o,p,s)	{ (l) -= (s); (o) += (s); \
	(p) = (void*)((char*) (p) + (s)); }

static STATUS readVersionDevice(RS_REQ const* const req, void* rep,
				V473::Card* const* const obj)
{
    static size_t const entrySize = 2;
    static size_t const maxSize = 42 * entrySize;
    size_t length = req->ILEN;
    size_t offset = req->OFFSET;

    if (!length || length % entrySize || length > maxSize)
	return ERR_BADLEN;
    if (offset % entrySize || offset > maxSize - entrySize)
	return ERR_BADOFF;
    if (offset + length > maxSize)
	return ERR_BADOFLEN;

    vwpp::Lock lock((*obj)->mutex, 100);

    if (offset == 0) {
	if (!(*obj)->getFirmwareVersion(lock, (uint16_t*) rep))
	    return ERR_MISBOARD;
	BUMP(length, offset, rep, 2);
    }

    if (offset == 2 && length >= 2) {
	if (!(*obj)->getActiveRamp(lock, (uint16_t*) rep))
	    return ERR_MISBOARD;
	BUMP(length, offset, rep, 2);
    }

    if (offset == 4 && length >= 2) {
	if (!(*obj)->getActiveScaleFactor(lock, (uint16_t*) rep))
	    return ERR_MISBOARD;
	BUMP(length, offset, rep, 2);
    }

    if (offset == 6 && length >= 2) {
	size_t const amount = length > 8 ? 8 : length;

	memset(rep, amount, 0);
	BUMP(length, offset, rep, amount);
    }

    if (offset == 14 && length >= 2) {
	if (!(*obj)->getCurrentSegment(lock, (uint16_t*) rep))
	    return ERR_MISBOARD;
	BUMP(length, offset, rep, 2);
    }

    if (offset == 16 && length >= 2) {
	size_t const amount = length > 8 ? 8 : length;

	memset(rep, amount, 0);
	BUMP(length, offset, rep, amount);
    }

    if (offset == 24 && length >= 2) {
	if (!(*obj)->getModuleId(lock, (uint16_t*) rep))
	    return ERR_MISBOARD;
	BUMP(length, offset, rep, 2);
    }

    if (offset == 26 && length >= 2) {
	size_t const amount = length > 42 ? 42 : length;

	memset(rep, amount, 0);
	BUMP(length, offset, rep, amount);
    }

    if (offset == 68 && length >= 2) {
	if (!(*obj)->getCurrentIntLvl(lock, (uint16_t*) rep))
	    return ERR_MISBOARD;
	BUMP(length, offset, rep, 2);
    }

    if (offset == 70 && length >= 2) {
	if (!(*obj)->getLastTclkEvent(lock, (uint16_t*) rep))
	    return ERR_MISBOARD;
	BUMP(length, offset, rep, 2);
    }

    if (offset == 72 && length >= 2) {
	size_t const amount = length > 12 ? 12 : length;

	memset(rep, amount, 0);
	BUMP(length, offset, rep, amount);
    }

    return NOERR;
}

static STATUS devReading(short, RS_REQ const* const req, void* const rep,
			 V473::Card* const* const ivs)
{
    try {
	switch (REQ_TO_SUBCODE(req)) {
	 case 7:
	    return readVersionDevice(req, rep, ivs);

	 case 1:		// G(i) tables. We dont have these, so fake it.
	 case 2:		// F(t) tables.
	 case 3:		// Delay Table
	 case 4:		// Offset Table
	 case 5:
	 case 6:		// Scale Factor Table
	 case 9:		// Frequency Table
	 case 10:		// Phase Table
	 default:
	    return ERR_BADPROP;
	}
	return NOERR;
    }
    catch (std::exception const& e) {
	printf("%s: exception '%s'\n", __func__, e.what());
	return ERR_DEVICEERROR;
    }
}

static STATUS devReadSetting(short, RS_REQ const* const req,
			     void* const rep, V473::Card* const* const ivs)
{
    try {
	switch (REQ_TO_SUBCODE(req)) {
	 case 1:		// G(i) tables. We dont have these, so fake it.
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

		 if (rep)
		     memset(rep, 0, req->ILEN);
	     }
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
		 static STableReadCallback const mt[] = {
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
	     }
	     return NOERR;

	 case 6:		// Scale Factor Table
	    return readSimpleTable(req, 2, 64, *ivs,
				   &V473::Card::getScaleFactors,
				   (uint16_t*) rep);

	 case 7:
	    return readVersionDevice(req, rep, ivs);

	 case 8:
	     {
		 size_t const length = req->ILEN;
		 size_t const offset = req->OFFSET;
		 size_t const chan = REQ_TO_453CHAN(req);

		 if (chan >= 4)
		     return ERR_BADCHN;
		 if (length > sizeof(uint16_t))
		     return ERR_BADLEN;
		 if (offset != 0)
		     return ERR_BADOFF;

		 vwpp::Lock lock((*ivs)->mutex, 100);

		 return (*ivs)->getDAC(lock, chan, (uint16_t*) rep) ?
		     NOERR : ERR_MISBOARD;
	     }
	     break;

	 case 9:		// Frequency Table
	    return readSimpleTable(req, 2, 64, *ivs,
				   &V473::Card::getFrequencies,
				   (uint16_t*) rep);

	 case 10:		// Phase Table
	    return readSimpleTable(req, 2, 64, *ivs,
				   &V473::Card::getPhases,
				   (uint16_t*) rep);

	 case 11:
	    return readSimpleTable(req, 2, 512, *ivs,
				   &V473::Card::getTriggerMap,
				   (uint16_t*) rep);

	 default:
	    return ERR_BADPROP;
	}
	return NOERR;
    }
    catch (std::exception const& e) {
	printf("%s: exception '%s'\n", __func__, e.what());
	return ERR_DEVICEERROR;
    }
}

static STATUS devSetting(short, RS_REQ* req, void*,
			 V473::Card* const* const obj)
{
    try {
	switch (REQ_TO_SUBCODE(req)) {
	 case 1:
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
	     }
	     return NOERR;

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
		 vwpp::Lock lock((*obj)->mutex, 100);

		 if (!(*obj)->setRamp(lock, REQ_TO_453CHAN(req),
				      offset / rampSize + 1,
				      (offset % rampSize) / 4,
				      (uint16_t const*) req->data,
				      length / 2))
		     return ERR_MISBOARD;
	     }
	     break;

	 case 3:		// Delay Table
	    return writeSimpleTable(req, 2, 64, *obj,
				    &V473::Card::setDelays,
				    (uint16_t const*) req->data);

	 case 4:		// Offset Table
	    return writeSimpleTable(req, 2, 64, *obj,
				    &V473::Card::setOffsets,
				    (uint16_t const*) req->data);

	 case 5:
	     {
		 static STableWriteCallback const mt[] = {
		     &V473::Card::setRampMap,
		     &V473::Card::setScaleFactorMap,
		     &V473::Card::setOffsetMap,
		     &V473::Card::setFrequencyMap,
		     &V473::Card::setPhaseMap
		 };
		 static size_t const nTables = sizeof(mt) / sizeof(*mt);
		 static size_t const entrySize = 2;
		 static size_t const tableSize = 32 * entrySize;
		 static size_t const maxSize = nTables * tableSize;

		 size_t length = req->ILEN;
		 size_t offset = req->OFFSET;
		 uint16_t const* ptr = (uint16_t const*) req->data;

		 if (REQ_TO_453CHAN(req) >= 4)
		     return ERR_BADCHN;
		 if (length % entrySize || length > maxSize)
		     return ERR_BADLEN;
		 if (offset % entrySize || offset > maxSize - entrySize)
		     return ERR_BADOFF;
		 if (offset + length > maxSize)
		     return ERR_BADOFLEN;

		 vwpp::Lock lock((*obj)->mutex, 100);

		 for (size_t ii = 0; length > 0 && ii < nTables; ++ii)
		     if (offset < ii * tableSize) {
			 size_t const total(std::min(length, (ii * tableSize) - offset));

			 if (!((*obj)->*mt[ii])(lock, REQ_TO_453CHAN(req),
						(offset % 64) / 2,
						ptr, total / 2))
			     return ERR_MISBOARD;
			 ptr += total / 2;
			 offset += total;
			 length -= total;
		     }
	     }
	     return NOERR;

	 case 6:		// Scale Factor Table
	    return writeSimpleTable(req, 2, 64, *obj,
				    &V473::Card::setScaleFactors,
				    (uint16_t const*) req->data);

	 case 7:
	     {
		 static size_t const entrySize = 2;
		 static size_t const maxSize = 84;
		 size_t const length = req->ILEN;
		 size_t const offset = req->OFFSET;

		 if (length % entrySize || length > maxSize)
		     return ERR_BADLEN;
		 if (offset % entrySize || offset > maxSize - entrySize)
		     return ERR_BADOFF;
		 if (offset + length > maxSize)
		     return ERR_BADOFLEN;
	     }
	     return NOERR;

	 case 8:
	     {
		 size_t const length = req->ILEN;
		 size_t const offset = req->OFFSET;
		 size_t const chan = REQ_TO_453CHAN(req);

		 if (chan >= 4)
		     return ERR_BADCHN;
		 if (length > sizeof(uint16_t))
		     return ERR_BADLEN;
		 if (offset != 0)
		     return ERR_BADOFF;

		 vwpp::Lock lock((*obj)->mutex, 100);

		 return (*obj)->setDAC(lock, chan, DATAS(req)) ?
		     NOERR : ERR_MISBOARD;
	     }
	     break;

	 case 9:		// Frequency Table
	    return writeSimpleTable(req, 2, 64, *obj,
				    &V473::Card::setFrequencies,
				    (uint16_t const*) req->data);

	 case 10:		// Phase Table
	    return writeSimpleTable(req, 2, 64, *obj,
				    &V473::Card::setPhases,
				    (uint16_t const*) req->data);

	 case 11:		// Trigger map
	    return writeSimpleTable(req, 2, 512, *obj,
				    &V473::Card::setTriggerMap,
				    (uint16_t const*) req->data);

	 default:
	    return ERR_BADPROP;
	}
	return NOERR;
    }
    catch (std::exception const& e) {
	printf("%s: exception '%s'\n", __func__, e.what());
	return ERR_DEVICEERROR;
    }
}

static STATUS devBasicControl(short, RS_REQ const* const req, void*,
			      V473::Card* const* const obj)
{
    try {
	bool result = true;

	switch (REQ_TO_SUBCODE(req)) {
	 case 8:
	     {
		 size_t const length = req->ILEN;
		 size_t const offset = req->OFFSET;
		 size_t const chan = REQ_TO_453CHAN(req);

		 if (chan >= 4)
		     return ERR_BADCHN;
		 if (length != sizeof(uint16_t))
		     return ERR_BADLEN;
		 if (offset != 0)
		     return ERR_BADOFF;

		 vwpp::Lock lock((*obj)->mutex, 100);

		 switch (DATAS(req)) {
		  case 1:
		     result = (*obj)->enablePowerSupply(lock, chan, false);
		     break;

		  case 2:
		     result = (*obj)->enablePowerSupply(lock, chan, true);
		     break;

		  case 3:
		     result = (*obj)->resetPowerSupply(lock, chan);
		     break;

		  default:
		     return ERR_WRBASCON;
		 }
	     }
	     break;

	 case 9:
	 case 10:
	     {
		 size_t const length = req->ILEN;
		 size_t const offset = req->OFFSET;
		 size_t const chan = REQ_TO_453CHAN(req);

		 if (chan >= 4)
		     return ERR_BADCHN;
		 if (length > sizeof(uint16_t))
		     return ERR_BADLEN;
		 if (offset != 0)
		     return ERR_BADOFF;

		 vwpp::Lock lock((*obj)->mutex, 100);

		 result = (*obj)->setSineWaveMode(lock, chan, DATAS(req));
	     }
	     break;

	 case 11:
	     {
		 size_t const length = req->ILEN;
		 size_t const offset = req->OFFSET;

		 if (length != sizeof(uint16_t))
		     return ERR_BADLEN;
		 if (offset != 0)
		     return ERR_BADOFF;

		 uint16_t const val = DATAS(req);

		 if (val != 1 && val != 2)
		     return NOERR;

		 vwpp::Lock lock((*obj)->mutex, 100);

		 result = (*obj)->tclkTrigEnable(lock, val == 2);
	     }
	     break;

	 default:
	    return ERR_BADPROP;
	}
	return result ? NOERR : ERR_MISBOARD;
    }
    catch (std::exception const& e) {
	printf("%s: exception '%s'\n", __func__, e.what());
	return ERR_DEVICEERROR;
    }
}

static STATUS devBasicStatus(short, RS_REQ const* const req, void* const rep,
			     V473::Card* const* const obj)
{
    size_t const length = req->ILEN;
    size_t const offset = req->OFFSET;
 
    try {
	switch (REQ_TO_SUBCODE(req)) {
	 case 8:
	     {
		 if (length != 2 * sizeof(uint16_t))
		     return ERR_BADLEN;
		 if (offset != 0)
		     return ERR_BADOFF;

		 ((uint16_t*)rep)[0] = (*obj)->getIrqSource();
		 ((uint16_t*)rep)[1] = 0;
		 return NOERR;
	     }
	     break;

	 case 9:
	 case 10:
	     {
		 size_t const chan = REQ_TO_453CHAN(req);

		 if (chan >= 4)
		     return ERR_BADCHN;
		 if (length > sizeof(uint16_t))
		     return ERR_BADLEN;
		 if (offset != 0)
		     return ERR_BADOFF;

		 vwpp::Lock lock((*obj)->mutex, 100);

		 return (*obj)->getSineWaveMode(lock, chan, (uint16_t*) rep) ?
		     NOERR : ERR_MISBOARD;
	     }
	     break;

	 case 11:
	     {
		 if (length != sizeof(uint16_t))
		     return ERR_BADLEN;
		 if (offset != 0)
		     return ERR_BADOFF;

		 vwpp::Lock lock((*obj)->mutex, 100);
		 bool val;

		 if ((*obj)->getTclkInterruptEnable(lock, &val)) {
		     *(uint16_t*)rep = val;
		     return NOERR;
		 } else
		     return ERR_MISBOARD;
	     }
	     break;

	 default:
	    return ERR_BADPROP;
	}
	return NOERR;
    }
    catch (std::exception const& e) {
	printf("%s: exception '%s'\n", __func__, e.what());
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

    if (NOERR != create_class(cls, 0, 0, 6, sizeof(V473::Card*))) {
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
    if (NOERR != add_class_msg(cls, rPRSET, (PMETHOD) devReadSetting)) {
	printf("Error trying to add the reading-of-the-setting handler.\n");
	return ERROR;
    }
    if (NOERR != add_class_msg(cls, sPRSET, (PMETHOD) devSetting)) {
	printf("Error trying to add the setting handler.\n");
	return ERROR;
    }
    if (NOERR != add_class_msg(cls, rPRBSTS, (PMETHOD) devBasicStatus)) {
	printf("Error trying to add the basic status handler.\n");
	return ERROR;
    }
    if (NOERR != add_class_msg(cls, sPRBCTL, (PMETHOD) devBasicControl)) {
	printf("Error trying to add the basic control handler.\n");
	return ERROR;
    }

    printf("V473 class successfully registered with MOOC.\n");
    return OK;
}
