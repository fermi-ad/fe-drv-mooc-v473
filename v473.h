#include <vxWorks.h>
#include <stdexcept>
#include <vwpp-3.0.h>
#include <mooc++-4.6.h>

namespace V473 {

    class Card {
	vwpp::v3_0::Mutex mutex;

     public:
	typedef vwpp::v3_0::Mutex::PMLock<Card, &Card::mutex> LockType;

	// Create a class that wraps a `size_t` type to represent a
	// channel number. If an instance can be created, it will hold
	// a valid value.

	class Channel {
	    size_t const value;

	    Channel();

	 public:
	    Channel(size_t const v) :
		value(v < 4 ? v : throw int16_t(ERR_BADCHN))
	    {}

	    operator size_t() const { return value; }
	};

     private:
	uint8_t const vecNum;

	vwpp::v3_0::Event<> intDone;
	bool volatile lastCmdOkay;

	uint16_t* dataBuffer;
	uint16_t* mailbox;
	uint16_t* count;
	uint16_t* readWrite;
	uint16_t* resetAddr;

	uint16_t* irqEnable;
	uint16_t* irqSource;
	uint16_t* irqMask;
	uint16_t* irqStatus;
	uint16_t* activeIrqSource;

	// No copying!

	Card();
	Card(Card const&);
	Card& operator=(Card const&);

	static uint16_t* xlatAddr(uint8_t, uint32_t);

	enum ChannelProperty {
	    cpRampTable,
	    cpRampMap = 0x800,
	    cpScaleFactorMap = 0x840,
	    cpScaleFactors = 0x860,
	    cpOffsetMap = 0x880,
	    cpOffsets = 0x8a0,
	    cpDelays = 0x8e0,
	    cpFrequencyMap = 0x900,
	    cpFrequencies = 0x920,
	    cpPhaseMap = 0x940,
	    cpPhases = 0x960,
	    cpWaveformEnable = 0xa00,
	    cpSineWaveMode = 0xa01,
	    cpPowerSupplyEnable = 0xa02,
	    cpPowerSupplyReset = 0xa03,
	    cpDACReadWrite = 0xa04,
	    cpIncDecDAC = 0xa05,
	    cpDACUpdateRate = 0xa06,
	    cpPSTrackingTol = 0xa10,
	    cpReadADC = 0xa11,
	    cpPSStatus = 0xa20,
	    cpPSStatusNom = 0xa21,
	    cpPSStatusMask = 0xa22,
	    cpPSStatusErr = 0xa23,
	    cpActiveRampTable = 0xa30,
	    cpActiveScaleFactor = 0xa31,
	    cpActiveOffset = 0xa32,
	    cpActiveRampTableSegment = 0xa33,
	    cpTimeRemaining = 0xa34,
	    cpActiveSineWaveFreq = 0xa35,
	    cpActiveSiveWavePhase = 0xa36,
	    cpFinalSineSaveFreq = 0xa37,
	    cpFinalSineWavePhase = 0xa38,
	    cpCalcOverflow = 0xa39,
	    cpTriggerMap = 0x4000,
	    cpTclkInterruptEnable = 0x4200,
	    cpActiveInterruptLevel = 0x4210,
	    cpLastTclkEvent = 0x4211,
	    cpInterruptCounter = 0x4220,
	    cpVmeDataBusDiag = 0x4484,
	    cpModuleID = 0xff00,
	    cpFirmwareVersion = 0xff01,
	    cpFpgaVersion = 0xff02
	};

	enum SineMode {
	    smOff = 0, smFixed = 1, smSweep = 3, smFixedLoop = 5,
	    smSweepLoop = 7
	};

	// Create a class that wraps a `size_t` type to represent an
	// interrupt level. If an instance can be created, it will hold
	// a valid value.

	class IntLevel {
	    size_t const value;
	    ChannelProperty const pvalue;

	    IntLevel();

	 public:
	    IntLevel(size_t const v, ChannelProperty const& p) :
		value(v < 32 || (p == cpTriggerMap && v < 256) ?
		      v : throw int16_t(ERR_BADSLOT)),
		pvalue(p)
	    {}

	    size_t level() const { return value; }
	    size_t prop() const { return pvalue; }
	};

	// Most addresses in the V473 memory map have the same bit
	// layout, so this function computes the address for a
	// channel's property with an optional interrupt level.

	uint16_t GEN_ADDR(Channel const& chan, IntLevel const& intLvl) const
	{
	    return 0x1000 * chan + intLvl.prop() + intLvl.level();
	}

	uint16_t GEN_ADDR(Channel const& chan,
			  ChannelProperty const& prop) const
	{
	    IntLevel const intLvl(0, prop);

	    return 0x1000 * chan + intLvl.prop() + intLvl.level();
	}

	// These functions set up the transaction registers.

	bool readProperty(LockType const&, uint16_t, size_t);
	bool setProperty(LockType const&, uint16_t, size_t);

	// Many properties in the V473 are in banks of 32 values.
	// These functions grab any subset of a bank of values. If the
	// range is invalid, a logic_error exception will be thrown.

	bool readBank(LockType const&, Channel const&, ChannelProperty,
		      uint16_t, uint16_t*, uint16_t);
	bool writeBank(LockType const&, Channel const&, ChannelProperty,
		       uint16_t, uint16_t const*, uint16_t);

	static void gblIntHandler(Card*);

	void intHandler();

	bool detect(LockType const&);

     protected:
	virtual void handleCalculationErr();
	virtual void handleMissingTCLK();
	virtual void handlePSTrackingErr();
	virtual void handlePS0Err();
	virtual void handlePS1Err();
	virtual void handlePS2Err();
	virtual void handlePS3Err();

     public:
	Card(uint8_t, uint8_t);
	virtual ~Card();

	void reset(LockType const&);
	void generateInterrupts(bool);

	uint16_t getActiveInterruptLevel(LockType const&);

	bool getIntCounters(LockType const& lock, uint16_t const start,
			    uint16_t* const ptr, uint16_t const n)
	{
	    return readBank(lock, 0, cpInterruptCounter, start, ptr, n);
	}

	bool getDelays(LockType const& lock, Channel const& chan,
		       uint16_t const intLvl, uint16_t* const ptr,
		       uint16_t const n)
	{
	    return readBank(lock, chan, cpDelays, intLvl, ptr, n);
	}

	bool getFrequencies(LockType const& lock, Channel const& chan,
			    uint16_t const intLvl, uint16_t* const ptr,
			    uint16_t const n)
	{
	    return readBank(lock, chan, cpFrequencies, intLvl, ptr, n);
	}

	bool getFrequencyMap(LockType const& lock, Channel const& chan,
			     uint16_t const intLvl, uint16_t* const ptr,
			     uint16_t const n)
	{
	    return readBank(lock, chan, cpFrequencyMap, intLvl, ptr, n);
	}

	bool getOffsetMap(LockType const& lock, Channel const& chan,
			  uint16_t const intLvl, uint16_t* const ptr,
			  uint16_t const n)
	{
	    return readBank(lock, chan, cpOffsetMap, intLvl, ptr, n);
	}

	bool getOffsets(LockType const& lock, Channel const& chan,
			uint16_t const intLvl, uint16_t* const ptr,
			uint16_t const n)
	{
	    return readBank(lock, chan, cpOffsets, intLvl, ptr, n);
	}

	bool getPhaseMap(LockType const& lock, Channel const& chan,
			 uint16_t const intLvl, uint16_t* const ptr,
			 uint16_t const n)
	{
	    return readBank(lock, chan, cpPhaseMap, intLvl, ptr, n);
	}

	bool getPhases(LockType const& lock, Channel const& chan,
		       uint16_t const intLvl, uint16_t* const ptr,
		       uint16_t const n)
	{
	    return readBank(lock, chan, cpPhases, intLvl, ptr, n);
	}

	bool getRamp(LockType const& lock, Channel const& chan,
		     uint16_t const ramp, uint16_t const offset,
		     uint16_t* const ptr, uint16_t const n)
	{
	    if (offset >= 64)
		throw std::logic_error("offset should be less than 64");

	    return readBank(lock, chan, ChannelProperty(ramp << 7),
			    2 * offset, ptr, n);
	}

	bool getRampMap(LockType const& lock, Channel const& chan,
			uint16_t const intLvl, uint16_t* const ptr,
			uint16_t const n)
	{
	    return readBank(lock, chan, cpRampMap, intLvl, ptr, n);
	}

	bool getScaleFactorMap(LockType const& lock, Channel const& chan,
			       uint16_t const intLvl, uint16_t* const ptr,
			       uint16_t const n)
	{
	    return readBank(lock, chan, cpScaleFactorMap, intLvl, ptr, n);
	}

	bool getScaleFactors(LockType const& lock, Channel const& chan,
			     uint16_t const intLvl, uint16_t* const ptr,
			     uint16_t const n)
	{
	    return readBank(lock, chan, cpScaleFactors, intLvl + 1, ptr, n);
	}

	bool getTriggerMap(LockType const& lock, Channel const&,
			   uint16_t const intLvl, uint16_t* const ptr,
			   uint16_t const n)
	{
	    return readBank(lock, 0, cpTriggerMap, intLvl, ptr, n);
	}

	bool getVmeDataBusDiag(LockType const& lock,
			       uint16_t* const ptr, uint16_t const n);

	bool setVmeDataBusDiag(LockType const& lock,
			       uint16_t const* const ptr);

	uint16_t getIrqSource() const;

	uint16_t* getDataBuffer(LockType const&) { return dataBuffer; }

	bool getModuleId(LockType const&, uint16_t*);
	bool getFirmwareVersion(LockType const&, uint16_t*);
	bool getFpgaVersion(LockType const&, uint16_t*);
	bool getActiveRamp(LockType const&, uint16_t*);
	bool getActiveScaleFactor(LockType const&, uint16_t*);
	bool getCurrentSegment(LockType const&, uint16_t*);
	bool getCurrentIntLvl(LockType const&, uint16_t*);
	bool getLastTclkEvent(LockType const&, uint16_t*);
	bool getPowerSupplyStatus(LockType const&, uint16_t, uint16_t*);
	bool getTclkInterruptEnable(LockType const&, bool*);
	bool getDAC(LockType const&, uint16_t, uint16_t*);
	bool getDiagCounters(LockType const&, uint16_t, uint16_t, uint16_t*);
	bool setDAC(LockType const&, uint16_t, uint16_t);
	bool getADC(LockType const&, uint16_t, uint16_t*);

	bool enablePowerSupply(LockType const&, uint16_t, bool);
	bool resetPowerSupply(LockType const&, uint16_t);

	bool getDACUpdateRate(LockType const&, uint16_t, uint16_t*);
	bool setDACUpdateRate(LockType const&, uint16_t, uint16_t);

	bool getSineWaveMode(LockType const&, uint16_t, uint16_t*);
	bool setSineWaveMode(LockType const&, uint16_t, uint16_t);
	bool setDelays(LockType const& lock, Channel const& chan,
		       uint16_t const intLvl, uint16_t const* const ptr,
		       uint16_t const n)
	{
	    return writeBank(lock, chan, cpDelays, intLvl, ptr, n);
	}

	bool setFrequencies(LockType const& lock, Channel const& chan,
			    uint16_t const intLvl, uint16_t const* const ptr,
			    uint16_t const n)
	{
	    return writeBank(lock, chan, cpFrequencies, intLvl, ptr, n);
	}

	bool setFrequencyMap(LockType const& lock, Channel const& chan,
			     uint16_t const intLvl, uint16_t const* const ptr,
			     uint16_t const n)
	{
	    return writeBank(lock, chan, cpFrequencyMap, intLvl, ptr, n);
	}

	bool setOffsetMap(LockType const& lock, Channel const& chan,
			  uint16_t const intLvl, uint16_t const* const ptr,
			  uint16_t const n)
	{
	    return writeBank(lock, chan, cpOffsetMap, intLvl, ptr, n);
	}

	bool setOffsets(LockType const& lock, Channel const& chan,
			uint16_t const intLvl, uint16_t const* const ptr,
			uint16_t const n)
	{
	    return writeBank(lock, chan, cpOffsets, intLvl, ptr, n);
	}

	bool setPhaseMap(LockType const& lock, Channel const& chan,
			 uint16_t const intLvl, uint16_t const* const ptr,
			 uint16_t const n)
	{
	    return writeBank(lock, chan, cpPhaseMap, intLvl, ptr, n);
	}

	bool setPhases(LockType const& lock, Channel const& chan,
		       uint16_t const intLvl, uint16_t const* const ptr,
		       uint16_t const n)
	{
	    return writeBank(lock, chan, cpPhases, intLvl, ptr, n);
	}

	bool setRamp(LockType const& lock, Channel const& chan,
		     uint16_t const ramp, uint16_t const offset,
		     uint16_t const* const ptr, uint16_t const n)
	{
	    if (offset >= 64)
		throw std::logic_error("offset should be less than 64");
	    return writeBank(lock, chan, ChannelProperty(ramp << 7),
			     2 * offset, ptr, n);
	}

	bool setRampMap(LockType const& lock, Channel const& chan,
			uint16_t const intLvl, uint16_t const* const ptr,
			uint16_t const n)
	{
	    return writeBank(lock, chan, cpRampMap, intLvl, ptr, n);
	}

	bool setScaleFactorMap(LockType const& lock, Channel const& chan,
			       uint16_t const intLvl, uint16_t const* const ptr,
			       uint16_t const n)
	{
	    return writeBank(lock, chan, cpScaleFactorMap, intLvl, ptr, n);
	}

	bool setTriggerMap(LockType const& lock, Channel const&,
			   uint16_t const intLvl, uint16_t const* const ptr,
			   uint16_t const n)
	{
	    return writeBank(lock, 0, cpTriggerMap, intLvl, ptr, n);
	}

	bool setScaleFactors(LockType const& lock, Channel const& chan,
			     uint16_t const intLvl, uint16_t const* const ptr,
			     uint16_t const n)
	{
	    return writeBank(lock, chan, cpScaleFactors, intLvl + 1, ptr, n);
	}

	bool sineWaveMode(LockType const& lock, Channel const& chan,
			  SineMode const mode)
	{
	    uint16_t const tmp = static_cast<uint16_t>(mode);

	    return writeBank(lock, chan, cpSineWaveMode, 0, &tmp, 1);
	}

	bool waveformEnable(LockType const& lock, Channel const& chan,
			    bool const en)
	{
	    uint16_t const tmp = static_cast<uint16_t>(en);

	    return writeBank(lock, chan, cpWaveformEnable, 0, &tmp, 1);
	}

	bool tclkTrigEnable(LockType const&, bool);
	bool setTriggerMap(LockType const&, uint16_t,
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
    STATUS v473_autotest(V473::HANDLE);
};

// Local Variables:
// mode:c++
// End:
