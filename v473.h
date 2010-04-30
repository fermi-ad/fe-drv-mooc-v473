// $Id$

#include <vxWorks.h>
#include <stdexcept>
#include <vwpp-0.1.h>

namespace V473 {

    class Card {
	uint8_t const vecNum;

	vwpp::Event intDone;

	uint16_t* dataBuffer;
	uint16_t* mailbox;
	uint16_t* count;
	uint16_t* readWrite;
	uint16_t* resetAddr;

	uint16_t* irqEnable;
	uint16_t* irqSource;
	uint16_t* irqMask;
	uint16_t* irqStatus;

	uint16_t prevIrqSource;

	// No copying!

	Card();
	Card(Card const&);
	Card& operator=(Card const&);

	static uint16_t* xlatAddr(uint8_t, uint32_t);

	enum ChannelProperty {
	    cpRampTable, cpRampMap = 0x800, cpScaleFactorMap = 0x840,
	    cpScaleFactors = 0x860, cpOffsetMap = 0x880, cpOffsets = 0x8a0,
	    cpDelays = 0x8e0, cpFrequencyMap = 0x900, cpFrequencies = 0x920,
	    cpPhaseMap = 0x940, cpPhases = 0x960, cpWaveformEnable = 0xa00,
	    cpSineWaveMode = 0xa01, cpPowerSupplyEnable = 0xa02,
	    cpPowerSupplyReset = 0xa03, cpDACReadWrite = 0xa04,
	    cpIncDecDAC = 0xa05, cpPSTrackingTol = 0xa10,
	    cpReadADC = 0xa11, cpPSStatus = 0xa20, cpPSStatusNom = 0xa21,
	    cpPSStatusMask = 0xa22, cpPSStatusErr = 0xa23,
	    cpActiveRampTable = 0xa30, cpActiveScaleFactor = 0xa31,
	    cpActiveOffset = 0xa32, cpActiveRampTableSegment = 0xa33,
	    cpTimeRemaining = 0xa34, cpActiveSineWaveFreq = 0xa35,
	    cpActiveSiveWavePhase = 0xa36, cpFinalSineSaveFreq = 0xa37,
	    cpFinalSineWavePhase = 0xa38, cpCalcOverflow = 0xa39,
	    cpTriggerMap = 0x4000, cpTclkInterruptEnable = 0x4200,
	    cpActiveInterruptLevel = 0x4210, cpLastTclkEvent = 0x4211,
	    cpModuleID = 0xff00, cpFirmwareVersion = 0xff01
	};

	enum SineMode {
	    smOff = 0, smFixed = 1, smSweep = 3, smFixedLoop = 5,
	    smSweepLoop = 7
	};

	// Most addresses in the V473 memory map have the same bit
	// layout, so this function computes the address for a
	// channel's property with an optional interrupt level.

	uint16_t GEN_ADDR(uint16_t const chan, ChannelProperty const prop,
			  uint16_t const intLvl = 0) const
	{
	    if (chan >= 4)
		throw std::logic_error("channel out of range");
	    if (prop == cpTriggerMap) {
		if (intLvl >= 256)
		    throw std::logic_error("interrupt level out of range");
	    } else if (intLvl >= 32)
		throw std::logic_error("interrupt level out of range");
	    return 0x1000 * chan + prop + intLvl;
	}

	// These functions set up the transaction registers.

	bool readProperty(vwpp::Lock const&, uint16_t, size_t);
	bool setProperty(vwpp::Lock const&, uint16_t, size_t);

	// Many properties in the V473 are in banks of 32 values.
	// These functions grab any subset of a bank of values. If the
	// range is invalid, a logic_error exception will be thrown.

	bool readBank(vwpp::Lock const&, uint16_t, ChannelProperty, uint16_t,
		      uint16_t*, uint16_t);
	bool writeBank(vwpp::Lock const&, uint16_t, ChannelProperty, uint16_t,
		       uint16_t const*, uint16_t);

	static void gblIntHandler(Card*);

	void intHandler();

     protected:
	virtual void handleCommandErr();
	virtual void handleCalculationErr();
	virtual void handleMissingTCLK();
	virtual void handlePSTrackingErr();
	virtual void handlePS0Err();
	virtual void handlePS1Err();
	virtual void handlePS2Err();
	virtual void handlePS3Err();

     public:
	vwpp::Mutex mutex;

	Card(uint8_t, uint8_t);
	virtual ~Card();

	void reset() { *resetAddr = 0; }
	void generateInterrupts(bool);

	uint16_t getActiveInterruptLevel(vwpp::Lock const&);

	bool getDelays(vwpp::Lock const& lock, uint16_t const chan,
		       uint16_t const intLvl, uint16_t* const ptr,
		       uint16_t const n)
	{
	    return readBank(lock, chan, cpDelays, intLvl, ptr, n);
	}

	bool getFrequencies(vwpp::Lock const& lock, uint16_t const chan,
			    uint16_t const intLvl, uint16_t* const ptr,
			    uint16_t const n)
	{
	    return readBank(lock, chan, cpFrequencies, intLvl, ptr, n);
	}

	bool getFrequencyMap(vwpp::Lock const& lock, uint16_t const chan,
			     uint16_t const intLvl, uint16_t* const ptr,
			     uint16_t const n)
	{
	    return readBank(lock, chan, cpFrequencyMap, intLvl, ptr, n);
	}

	bool getOffsetMap(vwpp::Lock const& lock, uint16_t const chan,
			  uint16_t const intLvl, uint16_t* const ptr,
			  uint16_t const n)
	{
	    return readBank(lock, chan, cpOffsetMap, intLvl, ptr, n);
	}

	bool getOffsets(vwpp::Lock const& lock, uint16_t const chan,
			uint16_t const intLvl, uint16_t* const ptr,
			uint16_t const n)
	{
	    return readBank(lock, chan, cpOffsets, intLvl, ptr, n);
	}

	bool getPhaseMap(vwpp::Lock const& lock, uint16_t const chan,
			 uint16_t const intLvl, uint16_t* const ptr,
			 uint16_t const n)
	{
	    return readBank(lock, chan, cpPhaseMap, intLvl, ptr, n);
	}

	bool getPhases(vwpp::Lock const& lock, uint16_t const chan,
		       uint16_t const intLvl, uint16_t* const ptr,
		       uint16_t const n)
	{
	    return readBank(lock, chan, cpPhases, intLvl, ptr, n);
	}

	bool getRamp(vwpp::Lock const& lock, uint16_t const chan,
		     uint16_t const ramp, uint16_t const offset,
		     uint16_t* const ptr, uint16_t const n)
	{
	    if (offset >= 64)
		throw std::logic_error("offset should be less than 64");

	    return readBank(lock, chan, ChannelProperty(ramp << 7),
			    2 * offset, ptr, n);
	}

	bool getRampMap(vwpp::Lock const& lock, uint16_t const chan,
			uint16_t const intLvl, uint16_t* const ptr,
			uint16_t const n)
	{
	    return readBank(lock, chan, cpRampMap, intLvl, ptr, n);
	}

	bool getScaleFactorMap(vwpp::Lock const& lock, uint16_t const chan,
			       uint16_t const intLvl, uint16_t* const ptr,
			       uint16_t const n)
	{
	    return readBank(lock, chan, cpScaleFactorMap, intLvl, ptr, n);
	}

	bool getScaleFactors(vwpp::Lock const& lock, uint16_t const chan,
			     uint16_t const intLvl, uint16_t* const ptr,
			     uint16_t const n)
	{
	    return readBank(lock, chan, cpScaleFactors, intLvl, ptr, n);
	}

	bool getTriggerMap(vwpp::Lock const& lock, uint16_t,
			     uint16_t const intLvl, uint16_t* const ptr,
			     uint16_t const n)
	{
	    return readBank(lock, 0, cpTriggerMap, intLvl, ptr, n);
	}

	uint16_t getIrqSource() const { return prevIrqSource; }

	bool getModuleId(vwpp::Lock const&, uint16_t*);
	bool getFirmwareVersion(vwpp::Lock const&, uint16_t*);
	bool getActiveRamp(vwpp::Lock const&, uint16_t*);
	bool getActiveScaleFactor(vwpp::Lock const&, uint16_t*);
	bool getCurrentSegment(vwpp::Lock const&, uint16_t*);
	bool getCurrentIntLvl(vwpp::Lock const&, uint16_t*);
	bool getLastTclkEvent(vwpp::Lock const&, uint16_t*);

	bool getDAC(vwpp::Lock const&, uint16_t, uint16_t*);
	bool setDAC(vwpp::Lock const&, uint16_t, uint16_t);

	bool getSineWaveMode(vwpp::Lock const&, uint16_t, uint16_t*);
	bool setSineWaveMode(vwpp::Lock const&, uint16_t, uint16_t);
	bool setTclkInterruptEnable(vwpp::Lock const&, bool);
	bool setDelays(vwpp::Lock const& lock, uint16_t const chan,
		       uint16_t const intLvl, uint16_t const* const ptr,
		       uint16_t const n)
	{
	    return writeBank(lock, chan, cpDelays, intLvl, ptr, n);
	}

	bool setFrequencies(vwpp::Lock const& lock, uint16_t const chan,
			    uint16_t const intLvl, uint16_t const* const ptr,
			    uint16_t const n)
	{
	    return writeBank(lock, chan, cpFrequencies, intLvl, ptr, n);
	}

	bool setFrequencyMap(vwpp::Lock const& lock, uint16_t const chan,
			     uint16_t const intLvl, uint16_t const* const ptr,
			     uint16_t const n)
	{
	    return writeBank(lock, chan, cpFrequencyMap, intLvl, ptr, n);
	}

	bool setOffsetMap(vwpp::Lock const& lock, uint16_t const chan,
			  uint16_t const intLvl, uint16_t const* const ptr,
			  uint16_t const n)
	{
	    return writeBank(lock, chan, cpOffsetMap, intLvl, ptr, n);
	}

	bool setOffsets(vwpp::Lock const& lock, uint16_t const chan,
			uint16_t const intLvl, uint16_t const* const ptr,
			uint16_t const n)
	{
	    return writeBank(lock, chan, cpOffsets, intLvl, ptr, n);
	}

	bool setPhaseMap(vwpp::Lock const& lock, uint16_t const chan,
			 uint16_t const intLvl, uint16_t const* const ptr,
			 uint16_t const n)
	{
	    return writeBank(lock, chan, cpPhaseMap, intLvl, ptr, n);
	}

	bool setPhases(vwpp::Lock const& lock, uint16_t const chan,
		       uint16_t const intLvl, uint16_t const* const ptr,
		       uint16_t const n)
	{
	    return writeBank(lock, chan, cpPhases, intLvl, ptr, n);
	}

	bool setRamp(vwpp::Lock const& lock, uint16_t const chan,
		     uint16_t const ramp, uint16_t const offset,
		     uint16_t const* const ptr, uint16_t const n)
	{
	    if (offset >= 64)
		throw std::logic_error("offset should be less than 64");
	    return writeBank(lock, chan, ChannelProperty(ramp << 7),
			     2 * offset, ptr, n);
	}

	bool setRampMap(vwpp::Lock const& lock, uint16_t const chan,
			uint16_t const intLvl, uint16_t const* const ptr,
			uint16_t const n)
	{
	    return writeBank(lock, chan, cpRampMap, intLvl, ptr, n);
	}

	bool setScaleFactorMap(vwpp::Lock const& lock, uint16_t const chan,
			       uint16_t const intLvl, uint16_t const* const ptr,
			       uint16_t const n)
	{
	    return writeBank(lock, chan, cpScaleFactorMap, intLvl, ptr, n);
	}

	bool setTriggerMap(vwpp::Lock const& lock, uint16_t,
			   uint16_t const intLvl, uint16_t const* const ptr,
			   uint16_t const n)
	{
	    return writeBank(lock, 0, cpTriggerMap, intLvl, ptr, n);
	}

	bool setScaleFactors(vwpp::Lock const& lock, uint16_t const chan,
			     uint16_t const intLvl, uint16_t const* const ptr,
			     uint16_t const n)
	{
	    return writeBank(lock, chan, cpScaleFactors, intLvl, ptr, n);
	}

	bool sineWaveMode(vwpp::Lock const& lock, uint16_t const chan,
			  SineMode const mode)
	{
	    uint16_t const tmp = static_cast<uint16_t>(mode);

	    return writeBank(lock, chan, cpSineWaveMode, 0, &tmp, 1);
	}

	bool waveformEnable(vwpp::Lock const& lock, uint16_t const chan,
			    bool const en)
	{
	    uint16_t const tmp = static_cast<uint16_t>(en);

	    return writeBank(lock, chan, cpWaveformEnable, 0, &tmp, 1);
	}

	bool tclkTrigEnable(vwpp::Lock const&, bool);
	bool setTriggerMap(vwpp::Lock const&, uint16_t,
			   uint8_t const[8], size_t);
    };

    typedef Card* HANDLE;
};

extern "C" {
    V473::HANDLE v473_create(int, int);
    STATUS v473_create_mooc_class(uint8_t);
    STATUS v473_create_mooc_instance(unsigned short, uint8_t, uint8_t);
    STATUS v473_cube(V473::HANDLE);
    STATUS v473_destroy(V473::HANDLE);
    STATUS v473_setupInterrupt(int, int, int, int, int, int, int, int, int);
    STATUS v473_test(V473::HANDLE, uint8_t);
};

// Local Variables:
// mode:c++
// End:
