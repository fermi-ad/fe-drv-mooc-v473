// $Id$

#include "v473.h"
#include <cmath>
#include <taskLib.h>
#include <cstdio>

STATUS v473_autotest(V473::HANDLE const hw, uint8_t const test_num)
{
    try {
	{
	    // Setup the hardware

	    uint16_t data;
	    vwpp::Lock lock(hw->mutex);

	    hw->waveformEnable(lock, 0, false);
	    hw->waveformEnable(lock, 1, false);
	    
	    printf("Hello, world!!!!!r\n");


	}

    }
    catch (std::exception const& e) {
	printf("caught: %s\n", e.what());
	return ERROR;
    }

    return OK;
}
