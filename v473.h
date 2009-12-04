// $Id$

#include <vxWorks.h>
#include <vwpp-0.1.h>

namespace V473 {

    class Card {
	vwpp::Mutex mutex;
	vwpp::Event intDone;

	uint16_t *dataBuffer;
	uint16_t* mailbox;
	uint16_t* count;
	uint16_t* readWrite;
	uint16_t* reset;

	uint16_t* irqEnable;
	uint16_t* irqSource;
	uint16_t* irqMask;
	uint16_t* irqStatus;
	// No copying!

	Card();
	Card(Card const&);
	Card& operator=(Card const&);

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
	    cpFinalSineWavePhase = 0xa38, cpCalcOverflow = 0xa39
	};

	bool readProperty(vwpp::Lock const&, uint16_t, size_t);
	bool setProperty(vwpp::Lock const&, uint16_t, size_t);

     public:
	Card(uint8_t, uint8_t);
	~Card();
    };

    typedef Card* HANDLE;
};

extern "C" {
    V473::HANDLE v473_create(uint8_t, uint8_t);
    STATUS v473_destroy(V473::HANDLE);
};

// Local Variables:
// mode:c++
// End:
