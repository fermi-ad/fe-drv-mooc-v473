// $Id$

#include "v473.h"
#include <errlogLib-2.0.h>
#include <ssmlite-1.0.h>
#include <vme.h>
#include <sysLib.h>
#include <iv.h>
#include <intLib.h>
#include <taskLib.h>
#include <cassert>

extern "C" UINT16 sysIn16(UINT16*);
extern "C" void sysOut16(UINT16*, UINT16);
extern "C" UINT32 sysIn32(UINT32*);
extern "C" void sysOut32(UINT32*, UINT32);

static void init() __attribute__((constructor));
static void term() __attribute__((destructor));

static HLOG hLog = 0;
static uint16_t lastMb = 0;
static uint16_t lastCount = 0;
static uint16_t lastDir = 0;

// Constructor function sets up the logger handle.

static void init()
{
    hLog = logRegister("V473", 0);
}

static void term()
{
    logUnregister(&hLog);
}

using namespace V473;

Card::Card(uint8_t addr, uint8_t intVec) :
    vecNum(intVec)
{
    char* baseAddr;

    if (ERROR == sysBusToLocalAdrs(VME_AM_STD_SUP_DATA,
				   reinterpret_cast<char*>((uint32_t) addr << 16),
				   &baseAddr))
	throw std::runtime_error("illegal A24 VME address");

    // Compute the memory-mapped registers based upon the computed
    // base address.

    dataBuffer = reinterpret_cast<__typeof dataBuffer>(baseAddr);
    mailbox = reinterpret_cast<uint16_t*>(baseAddr + 0x7ffa);
    count = reinterpret_cast<uint16_t*>(baseAddr + 0x7ffc);
    readWrite = reinterpret_cast<uint16_t*>(baseAddr + 0x7ffe);
    resetAddr = reinterpret_cast<uint16_t*>(baseAddr + 0xfffe);
    irqEnable = reinterpret_cast<uint16_t*>(baseAddr + 0x8000);
    irqSource = reinterpret_cast<uint16_t*>(baseAddr + 0x8002);
    irqMask = reinterpret_cast<uint16_t*>(baseAddr + 0x8004);
    irqStatus = reinterpret_cast<uint16_t*>(baseAddr + 0x8006);

    // Now that we think we're configured, let's check to see if we
    // are, indeed, a V473.

    sysOut16(mailbox, 0xff00);
    sysOut16(count, 1);
    sysOut16(readWrite, 0);
    taskDelay(2);

    if (sysIn16(readWrite) != 2 || sysIn16(count) != 1 || sysIn16(dataBuffer) != 473)
	throw std::runtime_error("VME A16 address doesn't refer to V473 hardware");

    sysOut16(mailbox, 0xff01);
    sysOut16(count, 1);
    sysOut16(readWrite, 0);
    taskDelay(2);

    uint16_t const firmware = sysIn16(dataBuffer);

    sysOut16(mailbox, 0xff02);
    sysOut16(count, 1);
    sysOut16(readWrite, 0);
    taskDelay(2);

    uint16_t const fpga = sysIn16(dataBuffer);

    logInform5(hLog, "V473: Found hardware -- addr %p, Firmware v%d.%d, FPGA v%d.%d",
	       dataBuffer, firmware >> 4, firmware & 0xf, fpga >> 4, fpga & 0xf);

    // Now that we know we're a V473, we can attach the interrupt
    // handler.

    if (OK != intConnect(INUM_TO_IVEC((int) intVec),
			 reinterpret_cast<VOIDFUNCPTR>(gblIntHandler),
			 reinterpret_cast<int>(this)))
	throw std::runtime_error("cannot connect V473 hardware to interrupt vector");

    sysIntEnable(1);
    sysOut16(irqStatus, intVec);
    sysOut16(irqSource, 0xffff);
    sysOut16(irqMask, 0xd21f);
}

Card::~Card()
{
    generateInterrupts(false);
    intDisconnect(INUM_TO_IVEC((int) vecNum),
		  reinterpret_cast<VOIDFUNCPTR>(gblIntHandler),
		  reinterpret_cast<int>(this));
}

void Card::gblIntHandler(Card* const ptr)
{
    ptr->intHandler();
}

void Card::handleCommandErr()
{
    logInform4(hLog, "(V473::Card*) %p detected a command error -- dir %d, "
	       "cmd 0x%04x, count %d", this, lastDir, lastMb, lastCount);
}

void Card::handleCalculationErr()
{
    logInform1(hLog, "(V473::Card*) %p detected a calculation error", this);
}

void Card::handleMissingTCLK()
{
    logInform1(hLog, "(V473::Card*) %p detected missing TCLK", this);
}

void Card::handlePSTrackingErr()
{
    logInform1(hLog, "(V473::Card*) %p detected a tracking error", this);
}

void Card::handlePS0Err()
{
    logInform1(hLog, "(V473::Card*) %p detected power supply 0 error", this);
}

void Card::handlePS1Err()
{
    logInform1(hLog, "(V473::Card*) %p detected power supply 1 error", this);
}

void Card::handlePS2Err()
{
    logInform1(hLog, "(V473::Card*) %p detected power supply 2 error", this);
}

void Card::handlePS3Err()
{
    logInform1(hLog, "(V473::Card*) %p detected power supply 3 error", this);
}

uint16_t Card::getActiveInterruptLevel(vwpp::Lock const& lock)
{
    if (readProperty(lock, 0x4210, 1))
	return sysIn16(dataBuffer);
    throw std::runtime_error("cannot read active interrupt level");
}

void Card::intHandler()
{
    ssmBaseAddr->led[0] = Yellow;
    sysOut16(irqSource, 0x1000);

    uint16_t const sts = prevIrqSource = sysIn16(irqSource);

    if (sts & 0x8000) {
	ssmBaseAddr->led[1] = Yellow;
	handleCommandErr();
	ssmBaseAddr->led[1] = Black;
    }
    if (sts & 0x4000) {
	ssmBaseAddr->led[2] = Yellow;
	handleCalculationErr();
	ssmBaseAddr->led[2] = Black;
    }
    if (sts & 0x1000) {
	ssmBaseAddr->led[3] = Yellow;
	handleMissingTCLK();
	ssmBaseAddr->led[3] = Black;
    }
    if (sts & 0x200) {
	ssmBaseAddr->led[4] = Yellow;
	handlePSTrackingErr();
	ssmBaseAddr->led[4] = Black;
    }
    if (sts & 0x10) {
	ssmBaseAddr->led[5] = Yellow;
	intDone.wakeOne();
	ssmBaseAddr->led[5] = Black;
    }
    if (sts & 0x8) {
	ssmBaseAddr->led[6] = Yellow;
	handlePS3Err();
	ssmBaseAddr->led[6] = Black;
    }
    if (sts & 0x4) {
	ssmBaseAddr->led[7] = Yellow;
	handlePS2Err();
	ssmBaseAddr->led[7] = Black;
    }
    if (sts & 0x2) {
	ssmBaseAddr->led[8] = Yellow;
	handlePS1Err();
	ssmBaseAddr->led[8] = Black;
    }
    if (sts & 0x1) {
	ssmBaseAddr->led[9] = Yellow;
	handlePS0Err();
	ssmBaseAddr->led[9] = Black;
    }

    sysOut16(irqSource, sts);
    ssmBaseAddr->led[0] = Black;
}

void Card::generateInterrupts(bool flg)
{
    sysOut16(irqEnable, flg ? 1 : 0);
}

// Sends the mailbox value, the word count and the READ command to the
// hardware. This function assumes the data buffer has been preloaded
// with the appropriate data. Returns true if everything is
// successful.

bool Card::readProperty(vwpp::Lock const&, uint16_t const mb, size_t const n)
{
    sysOut16(mailbox, lastMb = mb);
    sysOut16(count, lastCount = (uint16_t) n);
    sysOut16(readWrite, lastDir = 0);

    // Wait up to 40 milliseconds for a response.

    return intDone.wait(40);
}

// Sends the mailbox value, the word count and the SET command to the
// hardware. When this function returns, the data buffer will hold the
// return value.

bool Card::setProperty(vwpp::Lock const&, uint16_t const mb, size_t const n)
{
    sysOut16(mailbox, lastMb = mb);
    sysOut16(count, lastCount = (uint16_t) n);
    sysOut16(readWrite, lastDir = 1);

    // Wait up to 40 milliseconds for a response.

    return intDone.wait(40);
}

bool Card::readBank(vwpp::Lock const& lock, uint16_t const chan,
		    ChannelProperty const prop, uint16_t const start,
		    uint16_t* const ptr, uint16_t const n)
{
    if (prop == cpTriggerMap) {
	if (start >= 256)
	    throw std::logic_error("interrupt level out of range");
    } else if (start >= 32)
	    throw std::logic_error("interrupt level out of range");

    assert(sysIn16(readWrite) & 2);

    if (readProperty(lock, GEN_ADDR(chan, prop, start), n)) {
	for (uint16_t ii = 0; ii < n; ++ii)
	    ptr[ii] = sysIn16(dataBuffer + ii);
	return true;
    } else
	return false;
}

bool Card::writeBank(vwpp::Lock const& lock, uint16_t const chan,
		     ChannelProperty const prop, uint16_t const start,
		     uint16_t const* const ptr, uint16_t const n)
{
    if (prop == cpTriggerMap) {
	if (start >= 256)
	    throw std::logic_error("interrupt level out of range");
    } else if (start >= 32)
	    throw std::logic_error("interrupt level out of range");

    assert(sysIn16(readWrite) & 2);

    for (uint16_t ii = 0; ii < n; ++ii)
	sysOut16(dataBuffer + ii, ptr[ii]);
    return setProperty(lock, GEN_ADDR(chan, prop, start), n);
}

bool Card::setTriggerMap(vwpp::Lock const& lock, uint16_t const intLvl,
			 uint8_t const events[8], size_t const n)
{
    if (n <= 8) {
	assert(sysIn16(readWrite) & 2);

	for (size_t ii = 0; ii < 8; ++ii) {
	    sysOut16(dataBuffer, ii < n ? events[ii] : 0x00fe);
	    if (!setProperty(lock, 0x4000 + (intLvl << 3) + ii, 1))
		return false;
	}
	return true;
    } else
	throw std::logic_error("# of TCLK events cannot exceed 8");
}

bool Card::getModuleId(vwpp::Lock const& lock, uint16_t* const ptr)
{
    assert(sysIn16(readWrite) & 2);

    if (readProperty(lock, GEN_ADDR(0, cpModuleID), 1)) {
	*ptr = sysIn16(dataBuffer);
	return true;
    } else
	return false;
}

bool Card::getFirmwareVersion(vwpp::Lock const& lock, uint16_t* const ptr)
{
    assert(sysIn16(readWrite) & 2);

    if (readProperty(lock, GEN_ADDR(0, cpFirmwareVersion), 1)) {
	*ptr = sysIn16(dataBuffer);
	return true;
    } else
	return false;
}

bool Card::getActiveRamp(vwpp::Lock const& lock, uint16_t* const ptr)
{
    assert(sysIn16(readWrite) & 2);

    if (readProperty(lock, GEN_ADDR(0, cpActiveRampTable), 1)) {
	*ptr = sysIn16(dataBuffer);
	return true;
    } else
	return false;
}

bool Card::getActiveScaleFactor(vwpp::Lock const& lock, uint16_t* const ptr)
{
    assert(sysIn16(readWrite) & 2);

    if (readProperty(lock, GEN_ADDR(0, cpActiveScaleFactor), 1)) {
	*ptr = sysIn16(dataBuffer);
	return true;
    } else
	return false;
}

bool Card::getCurrentSegment(vwpp::Lock const& lock, uint16_t* ptr)
{
    assert(sysIn16(readWrite) & 2);

    if (readProperty(lock, GEN_ADDR(0, cpActiveRampTableSegment), 1)) {
	*ptr = sysIn16(dataBuffer);
	return true;
    } else
	return false;
}

bool Card::getCurrentIntLvl(vwpp::Lock const& lock, uint16_t* ptr)
{
    assert(sysIn16(readWrite) & 2);

    if (readProperty(lock, GEN_ADDR(0, cpActiveInterruptLevel), 1)) {
	*ptr = sysIn16(dataBuffer);
	return true;
    } else
	return false;
}

bool Card::getLastTclkEvent(vwpp::Lock const& lock, uint16_t* ptr)
{
    assert(sysIn16(readWrite) & 2);

    if (readProperty(lock, GEN_ADDR(0, cpLastTclkEvent), 1)) {
	*ptr = sysIn16(dataBuffer);
	return true;
    } else
	return false;
}

bool Card::getDAC(vwpp::Lock const& lock, uint16_t const chan,
		  uint16_t* const ptr)
{
    assert(sysIn16(readWrite) & 2);

    if (readProperty(lock, GEN_ADDR(chan, cpDACReadWrite), 1)) {
	*ptr = sysIn16(dataBuffer);
	return true;
    } else
	return false;
}

bool Card::getTclkInterruptEnable(vwpp::Lock const& lock, bool* const ptr)
{
    assert(sysIn16(readWrite) & 2);

    if (readProperty(lock, GEN_ADDR(0, cpTclkInterruptEnable), 1)) {
	*ptr = (bool) sysIn16(dataBuffer);
	return true;
    } else
	return false;
}

bool Card::setDAC(vwpp::Lock const& lock, uint16_t const chan,
		  uint16_t const val)
{
    assert(sysIn16(readWrite) & 2);

    sysOut16(dataBuffer, val);
    return setProperty(lock, GEN_ADDR(chan, cpDACReadWrite), 1);
}

bool Card::getSineWaveMode(vwpp::Lock const& lock, uint16_t const chan,
			   uint16_t* const ptr)
{
    assert(sysIn16(readWrite) & 2);

    if (readProperty(lock, GEN_ADDR(chan, cpSineWaveMode), 1)) {
	*ptr = sysIn16(dataBuffer) & 7;
	return true;
    } else
	return false;
}

bool Card::setSineWaveMode(vwpp::Lock const& lock, uint16_t const chan,
			   uint16_t const val)
{
    assert(sysIn16(readWrite) & 2);

    sysOut16(dataBuffer, val & 7);
    return setProperty(lock, GEN_ADDR(chan, cpSineWaveMode), 1);
}

bool Card::tclkTrigEnable(vwpp::Lock const& lock, bool const en)
{
    assert(sysIn16(readWrite) & 2);

    sysOut16(dataBuffer, static_cast<uint16_t>(en));
    return setProperty(lock, cpTclkInterruptEnable, 1);
}

bool Card::enablePowerSupply(vwpp::Lock const& lock, uint16_t const chan,
			     bool const en)
{
    assert(sysIn16(readWrite) & 2);

    sysOut16(dataBuffer, static_cast<uint16_t>(en));
    return setProperty(lock, cpPowerSupplyEnable, 1);
}

bool Card::resetPowerSupply(vwpp::Lock const& lock, uint16_t const chan)
{
    assert(sysIn16(readWrite) & 2);

    sysOut16(dataBuffer, 1);
    if (setProperty(lock, GEN_ADDR(chan, cpPowerSupplyReset), 1)) {
	assert(sysIn16(readWrite) & 2);

	sysOut16(dataBuffer, 0);
	return setProperty(lock, GEN_ADDR(chan, cpPowerSupplyReset), 1);
    } else
	return false;
}

V473::HANDLE v473_create(int addr, int intVec)
{
    try {
	V473::HANDLE const ptr = new Card(addr, intVec);

	ptr->generateInterrupts(true);
	return ptr;
    }
    catch (std::exception const& e) {
	printf("ERROR: %s\n", e.what());
	return 0;
    }
}

STATUS v473_destroy(V473::HANDLE ptr)
{
    delete ptr;
    return OK;
}

STATUS v473_setupInterrupt(V473::HANDLE const ptr, uint8_t const chan,
			   uint8_t const intLvl, uint8_t const ramp,
			   uint8_t const scale, uint8_t const offset,
			   uint8_t const delay, uint8_t const* const event,
			   size_t const n)
{
    if (ptr) {
	try {
	    // Sanity checks.

	    if (chan >= 4) {
		printf("ERROR: channel must be less than 4.\n");
		return ERROR;
	    }

	    if (intLvl >= 32) {
		printf("ERROR: interrupt level must be less than 32.\n");
		return ERROR;
	    }

	    if (ramp >= 16) {
		printf("ERROR: ramp index must be less than 16.\n");
		return ERROR;
	    }

	    if (scale >= 32) {
		printf("ERROR: scale factor index must be less than 32.\n");
		return ERROR;
	    }

	    if (offset >= 32) {
		printf("ERROR: offset index must be less than 32.\n");
		return ERROR;
	    }

	    if (delay >= 32) {
		printf("ERROR: delay index must be less than 32.\n");
		return ERROR;
	    }

	    if (event && n > 8) {
		printf("ERROR: Can only set up to 8 clock events per interrupt.\n");
		return ERROR;
	    }

	    return OK;
	}
	catch (std::exception const& e) {
	    printf("ERROR: %s\n", e.what());
	}
    }
    return ERROR;
}

#include <cmath>

STATUS v473_test(V473::HANDLE const hw, uint8_t const chan)
{
    if (chan >= 4) {
	printf("channel must be less than 4.\n");
	return ERROR;
    }

    try {
	vwpp::Lock lock(hw->mutex);

	logInform0(hLog, "hardware is locked");

	if (!hw->waveformEnable(lock, chan, false))
	    throw std::runtime_error("error disabling waveform");

	logInform0(hLog, "channel 0 is disabled");

	// Generate sine wave table.

	uint16_t data[128];

	for (unsigned ii = 0; ii < 62; ++ii) {
	    data[ii * 2] = (int16_t) (0x4000 * sin(ii * 6.2830 / 62));
	    data[ii * 2 + 1] = (uint16_t) 105;
	}
	data[124] = 0;
	data[125] = 0;
	if (!hw->setRamp(lock, chan, 1, 0, data, 126))
	    throw std::runtime_error("error setting ramp");

	logInform0(hLog, "ramp table 1 loaded");

	data[0] = 1;
	if (!hw->setRampMap(lock, chan, 0, data, 1))
	    throw std::runtime_error("error setting ramp map");

	logInform0(hLog, "int level 0 points to ramp 1");

	// Set the scale factor to 1.0.

	data[0] = 128;
	if (!hw->setScaleFactors(lock, chan, 1, data, 1))
	    throw std::runtime_error("error setting scale factor");
	logInform0(hLog, "set scale factor #1 to 1.0");
	data[0] = 1;
	if (!hw->setScaleFactorMap(lock, chan, 0, data, 1))
	    throw std::runtime_error("error setting scale factor map");
	logInform1(hLog, "pointed channel %d, interrupt level 0 to scale factor", chan);

	// Set the offset

	data[0] = 0;
	if (!hw->setOffsetMap(lock, chan, 0, data, 1))
	    throw std::runtime_error("error setting offset map");
	logInform0(hLog, "point to null offset");

	// Trigger interrupt level 0 on $0f events.

	uint8_t event = 0x0f;

	if (!hw->setTriggerMap(lock, 0, &event, 1))
	    throw std::runtime_error("error setting trigger map");
	logInform0(hLog, "Set $0F event to trigger int lvl 0");
	if (!hw->tclkTrigEnable(lock, true))
	    throw std::runtime_error("error enabling triggers");
	logInform0(hLog, "enable triggering from TCLKs");
	if (!hw->waveformEnable(lock, chan, true))
	    throw std::runtime_error("error enabling waveform");
	logInform0(hLog, "enable channel 0");
    }
    catch (std::exception const& e) {
	printf("caught: %s\n", e.what());
	return ERROR;
    }

    return OK;
}
