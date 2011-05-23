// $Id: test_v473.cpp

#include "v473.h"
#include <cmath>
#include <taskLib.h>


STATUS v473_autotest(V473::HANDLE const hw)
{
    try {
	{
	    // Setup the hardware

	    uint16_t data;
	    vwpp::Lock lock(hw->mutex);

	    hw->waveformEnable(lock, 0, false);
	    hw->waveformEnable(lock, 1, false);
	    
	    printf("Hello, world\r\n");


	}

    }
    catch (std::exception const& e) {
	printf("caught: %s\n", e.what());
	return ERROR;
    }

    return OK;
}
