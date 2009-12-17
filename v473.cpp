// $Id$

#include "v473.h"
#include <errlogLib-2.0.h>
#include <cstdio>
#include <cassert>
#include <algorithm>
#include <vme.h>
#include <sysLib.h>
#include <iv.h>
#include <intLib.h>
#include <taskLib.h>
#include <ppcLib-1.2.h>

extern "C" UINT16 sysIn16(UINT16*);
extern "C" void sysOut16(UINT16*, UINT16);

static void init() __attribute__((constructor));
static void term() __attribute__((destructor));

static HLOG hLog = 0;

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
    logInform1(hLog, "(V473::Card*) %p detected a command error", this);
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
    sysOut16(irqSource, 0x1000);

    uint16_t const sts = sysIn16(irqSource);

    logInform1(hLog, "interrupt fired -- status 0x%04x", sts);
    sysOut16(irqSource, sts);

    if (sts & 0x8000)
	handleCommandErr();
    if (sts & 0x4000)
	handleCalculationErr();
    if (sts & 0x1000)
	handleMissingTCLK();
    if (sts & 0x200)
	handlePSTrackingErr();
    if (sts & 0x10)
	intDone.wakeOne();
    if (sts & 0x8)
	handlePS3Err();
    if (sts & 0x4)
	handlePS2Err();
    if (sts & 0x2)
	handlePS1Err();
    if (sts & 0x1)
	handlePS0Err();

    logInform1(hLog, "leaving int handler -- bits still set: 0x%04x",
	       sts & sysIn16(irqSource));
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
    sysOut16(mailbox, mb);
    sysOut16(count, (uint16_t) n);
    sysOut16(readWrite, 0);

    // Wait up to 40 milliseconds for a response.

    unsigned long const start = ppcTick();
    bool const result = intDone.wait(40);

    logInform1(hLog, "readProperty(): took %ld uS", ticks_to_uS(ppcTick() - start));
    return result;
}

// Sends the mailbox value, the word count and the SET command to the
// hardware. When this function returns, the data buffer will hold the
// return value.

bool Card::setProperty(vwpp::Lock const&, uint16_t const mb, size_t const n)
{
    sysOut16(mailbox, mb);
    sysOut16(count, (uint16_t) n);
    sysOut16(readWrite, 1);

    // Wait up to 40 milliseconds for a response.

    unsigned long const start = ppcTick();
    bool const result = intDone.wait(40);

    logInform1(hLog, "setProperty(): took %ld uS", ticks_to_uS(ppcTick() - start));
    return result;
}

bool Card::readBank(vwpp::Lock const& lock, uint16_t const chan,
		    ChannelProperty const prop, uint16_t const start,
		    uint16_t* const ptr, uint16_t const n)
{
    if (start >= 32)
	throw std::logic_error("interrupt level out of range");

    if (setProperty(lock, GEN_ADDR(chan, prop, start), n)) {
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
    if (start >= 32)
	throw std::logic_error("interrupt level out of range");

    for (uint16_t ii = 0; ii < n; ++ii)
	sysOut16(dataBuffer + ii, ptr[ii]);
    return setProperty(lock, GEN_ADDR(chan, prop, start), n);
}

bool Card::setTriggerMap(vwpp::Lock const& lock, uint16_t const intLvl,
			 uint8_t const events[8], size_t const n)
{
    if (n <= 8) {
	for (size_t ii = 0; ii < 8; ++ii) {
	    sysOut16(dataBuffer, ii < n ? events[ii] : 0x00fe);
	    if (!setProperty(lock, 0x4000 + (intLvl << 3) + ii, 1))
		return false;
	}
	return true;
    } else
	throw std::logic_error("# of TCLK events cannot exceed 8");
}

bool Card::tclkTrigEnable(vwpp::Lock const& lock, bool const en)
{
    sysOut16(dataBuffer, static_cast<uint16_t>(en));
    return setProperty(lock, (ChannelProperty) 0x4200, 1);
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

extern "C" STATUS v473_test(V473::HANDLE hw)
{
    try {
	vwpp::Lock lock(hw->mutex);

	logInform0(hLog, "hardware is locked");

	hw->waveformEnable(lock, 0, false);

	logInform0(hLog, "channel 0 is disabled");

	// Generate sine wave table.

	uint16_t data[128];

	for (unsigned ii = 0; ii < 62; ++ii) {
	    data[ii * 2] = (int16_t) (0x4000 * sin(ii * 6.2830 / 62));
	    data[ii * 2 + 1] = (uint16_t) 105;
	}
	data[124] = 0;
	data[125] = 0;
	hw->setRamp(lock, 0, 1, data, 126);

	logInform0(hLog, "ramp table 1 loaded");

	data[0] = 1;
	hw->setRampMap(lock, 0, 0, data, 1);

	logInform0(hLog, "int level 0 points to ramp 1");

	// Set the scale factor to 1.0.

	data[0] = 128;
	hw->setScaleFactors(lock, 0, 1, data, 1);
	logInform0(hLog, "set scale factor #1 to 1.0");
	data[0] = 1;
	hw->setScaleFactorMap(lock, 0, 0, data, 1);
	logInform0(hLog, "pointed channel 0, interrupt level 0 to scale factor");

	// Set the offset

	data[0] = 0;
	hw->setOffsetMap(lock, 0, 0, data, 1);
	logInform0(hLog, "point to null offset");

	// Trigger interrupt level 0 on $0f events.

	uint8_t event = 0x0f;

	hw->setTriggerMap(lock, 0, &event, 1);
	logInform0(hLog, "Set $0F event to trigger int lvl 0");
	hw->tclkTrigEnable(lock, true);
	logInform0(hLog, "enable triggering from TCLKs");
	hw->waveformEnable(lock, 0, true);
	logInform0(hLog, "enable channel 0");
    }
    catch (std::exception const& e) {
	printf("caught: %s\n", e.what());
	return ERROR;
    }

    return OK;
}

#define M_PI 3.14159265358979323846

typedef float POINT[4];
typedef float const CPOINT[4];
typedef float MATRIX[4][4];
typedef float const CMATRIX[4][4];

static void project(CMATRIX m, CPOINT p, float b[2])
{
    POINT tmp = { 0.0 };

    for (size_t row = 0; row < 4; ++row)
	for (size_t col = 0; col < 4; ++col)
	    tmp[row] = m[row][col] * p[col];
    b[0] = std::min(tmp[0] * 5 / (std::max(tmp[2], 0.f) + 5), 1.f);
    b[1] = std::min(tmp[1] * 5 / (std::max(tmp[2], 0.f) + 5), 1.f);
}

static void product(CMATRIX p, MATRIX m)
{
    MATRIX tmp = { { 0.0 } };

    for (size_t row = 0; row < 4; ++row)
	for (size_t col = 0; col < 4; ++col)
	    for (size_t ii = 0; ii < 4; ++ii)
		for (size_t jj = 0; jj < 4; ++jj)
		    tmp[row][col] += m[ii][jj] * p[jj][ii];
    m = tmp;
}

static void identity(MATRIX m)
{
    m[0][0] = m[1][1] = m[2][2] = m[3][3] = 1.0;
    m[0][1] = m[0][2] = m[0][3] = m[1][0] = m[1][2] = m[1][3] = m[2][0] =
	m[2][1] = m[2][3] = m[3][0] = m[3][1] = m[3][2] = 0.0;
}

static void translate(MATRIX m, float const dx, float const dy,
		      float const dz)
{
    m[3][0] += dx;
    m[3][1] += dy;
    m[3][2] += dz;
}

static void rotate(MATRIX m, float const rx, float const ry, float const rz)
{
    float const rrx = (2.0 * M_PI / 360.0) * rx;
    float const rry = (2.0 * M_PI / 360.0) * ry;
    float const rrz = (2.0 * M_PI / 360.0) * rz;

    float const sx = sin(rrx);
    float const cx = cos(rrx);
    float const sy = sin(rry);
    float const cy = cos(rry);
    float const sz = sin(rrz);
    float const cz = cos(rrz);

    CMATRIX mx = {
	{1., 0., 0., 0.},
	{0., cx, sx, 0.},
	{0., -sx, cx, 0.},
	{0., 0., 0., 1.}
    };

    product(mx, m);

    CMATRIX my = {
	{cy, 0., -sy, 0.},
	{0., 1., 0., 0.},
	{sy, 0., cy, 0.},
	{0., 0., 0., 1.}
    };

    product(my, m);

    CMATRIX mz = {
	{cz, -sz, 0., 0.},
	{sz, cz, 0., 0.},
	{0., 0., 0., 0.},
	{0., 0., 0., 1.}
    };

    product(mz, m);
}

extern "C" STATUS v473_cube(V473::HANDLE hw)
{
    int xa = 0;
    int ramp = 0;

    try {
	{
	    // Setup the hardware

	    uint16_t data;
	    vwpp::Lock lock(hw->mutex);

	    hw->waveformEnable(lock, 0, false);
	    hw->waveformEnable(lock, 1, false);

	    data = 1;
	    hw->setRampMap(lock, 0, 0, &data, 1);
	    hw->setRampMap(lock, 1, 0, &data, 1);
	    data = 2;
	    hw->setRampMap(lock, 0, 1, &data, 2);
	    hw->setRampMap(lock, 1, 1, &data, 2);

	    // Set the scale factor to 1.0.

	    data = 128;
	    hw->setScaleFactors(lock, 0, 1, &data, 1);
	    hw->setScaleFactors(lock, 1, 1, &data, 1);
	    data = 1;
	    hw->setScaleFactorMap(lock, 0, 0, &data, 1);
	    hw->setScaleFactorMap(lock, 1, 0, &data, 1);

	    // Set the offset

	    data = 0;
	    hw->setOffsetMap(lock, 0, 0, &data, 1);
	    hw->setOffsetMap(lock, 1, 0, &data, 1);

	    hw->tclkTrigEnable(lock, true);

	    // Enable channels 0 and 1.

	    hw->waveformEnable(lock, 0, true);
	    hw->waveformEnable(lock, 1, true);
	}

	while (true) {
	    static size_t const path[] = {
		1, 2, 3, 4, 8, 7, 6, 5, 1, 4, 3, 7, 8, 5, 6, 2, 1
	    };

	    // Update rotation

	    if ((xa += 12) >= 360)
		xa %= 360;

	    // Compute transform matrix

	    MATRIX m;

	    identity(m);
	    rotate(m, (float) xa, 0, 0);
	    translate(m, 0., 0., 1.);

	    // Translate each point and save result into ramp table
	    // buffer.

	    uint16_t data[2][2 * sizeof(path) / sizeof(*path)];

	    for (size_t ii = 0; ii < sizeof(path) / sizeof(*path); ++ii) {
		static POINT const point[] = {
		    { -0.5,  0.5, -0.5, 1. },
		    { -0.5,  0.5,  0.5, 1. },
		    {  0.5,  0.5,  0.5, 1. },
		    {  0.5,  0.5, -0.5, 1. },
		    { -0.5, -0.5, -0.5, 1. },
		    { -0.5, -0.5,  0.5, 1. },
		    {  0.5, -0.5,  0.5, 1. },
		    {  0.5, -0.5, -0.5, 1. }
		};
		float b[2];

		project(m, point[path[ii]], b);
		data[0][ii * 2] = uint16_t(b[0] * 32000);
		data[1][ii * 2] = uint16_t(b[1] * 32000);
		data[0][ii * 2 + 1] = data[1][ii * 2 + 1] = 800;
	    }

	    {
		vwpp::Lock lock(hw->mutex);

		hw->setRamp(lock, 0, ramp + 1, data[0], 128);
		hw->setRamp(lock, 1, ramp + 1, data[1], 128);

		// Hand the $0f event to the interrupt assigned to the
		// next ramp.

		uint8_t const event = 0x0f;

		hw->setTriggerMap(lock, ramp, &event, 1);

		// Wait for the ramp to start to play. Then we can
		// switch to updating the other ramp.

		// while (hw->getActiveInterruptLevel(lock) == ramp)
		taskDelay(4);
		ramp = !ramp;
	    }
	}
    }
    catch (std::exception const& e) {
	printf("caught: %s\n", e.what());
	return ERROR;
    }

    return OK;
}
