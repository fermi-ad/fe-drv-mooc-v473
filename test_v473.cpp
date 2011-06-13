// $Id$

#include "v473.h"
#include <cmath>
#include <taskLib.h>
#include <cstdio>

//------------------------------------------------------------------------------
// DoTestDiscIO
//
// Automatically do a discrete I/O checkout of the C473 board
//------------------------------------------------------------------------------
int DoTestDiscIO(V473::HANDLE const hw)
{
	uint16_t channel_drive, channel_read;

	const int ps_stat_mask = 0x28FF;
	uint16_t ps_stat_expected, ps_stat_received;
	
	printf("\nTesting V473 Discrete I/O\n");

    vwpp::Lock lock(hw->mutex);

// Put discrete outputs in a known state
	for(channel_drive = 0; channel_drive < 4; channel_drive++)
	{
        hw->enablePowerSupply(lock, channel_drive, false);
	}
    taskDelay(2); // Give card a chance to change outputs and update status

// Toggle PS enable outputs
	for(channel_drive = 0; channel_drive < 4; channel_drive++)
	{
		for(channel_read = 0; channel_read < 4; channel_read++)
		{
			hw->getPowerSupplyStatus(lock, channel_read, &ps_stat_received);

			ps_stat_received &= ps_stat_mask;
			ps_stat_expected = 0x0000;

			if(ps_stat_received != ps_stat_expected)
			{
				printf("\nDiscIO Error, Reading Channel %d\n", channel_read);
				printf("Expected 0x%4X\n", ps_stat_expected);
				printf("Received 0x%4X\n", ps_stat_received);
				return -1;
			}
		}

        hw->enablePowerSupply(lock, channel_drive, true);
        taskDelay(2); // Give card a chance to change outputs and update status

		for(channel_read = 0; channel_read < 4; channel_read++)
		{
			hw->getPowerSupplyStatus(lock, channel_read, &ps_stat_received);

			ps_stat_received &= ps_stat_mask;
			ps_stat_expected = 0x0000 | (0x0001 << (2*channel_drive));

			if(ps_stat_received != ps_stat_expected)
			{
				printf("\nDiscIO Error, Reading Channel %d\n", channel_read);
				printf("Expected 0x%4X\n", ps_stat_expected);
				printf("Received 0x%4X\n", ps_stat_received);
				return -1;
			}
		}

        hw->enablePowerSupply(lock, channel_drive, false);
        taskDelay(2); // Give card a chance to change outputs and update status
	}

// Toggle PS reset outputs
	for(channel_drive = 0; channel_drive < 4; channel_drive++)
	{
        hw->resetPowerSupply(lock, channel_drive);

// The Reset output is run by the processor's 1sec interrupt cycle
//  It may take up to a second to see the output change.
        hw->getPowerSupplyStatus(lock, channel_drive, &ps_stat_received);
        
        while((ps_stat_received & 0x2000) != 0x2000) // Sniff the PS Reset status bit
        {
            hw->getPowerSupplyStatus(lock, channel_drive, &ps_stat_received);
        }
        
        taskDelay(2); // Give card a chance to change outputs and update status
        
        for(channel_read = 0; channel_read < 4; channel_read++)
        {
            hw->getPowerSupplyStatus(lock, channel_read, &ps_stat_received);
			ps_stat_received &= ps_stat_mask;
            
            if(channel_drive == channel_read)
            {
                ps_stat_expected = 0x2055 | (0x0002 << (2*channel_drive));
            }
            else
            {
                ps_stat_expected = 0x0055 | (0x0002 << (2*channel_drive));
            }

			if(ps_stat_received != ps_stat_expected)
			{
				printf("\nDiscIO Error, Reading Channel %d\n", channel_read);
				printf("Expected 0x%4X\n", ps_stat_expected);
				printf("Received 0x%4X\n", ps_stat_received);
				return -1;
			}
        }

// Wait for the Reset output to turn off
        hw->getPowerSupplyStatus(lock, channel_drive, &ps_stat_received);
        
        while((ps_stat_received & 0x2000) != 0x0000) // Sniff the PS Reset status bit
        {
            hw->getPowerSupplyStatus(lock, channel_drive, &ps_stat_received);
        }
        
        taskDelay(2); // Give card a chance to change outputs and update status
        
        for(channel_read = 0; channel_read < 4; channel_read++)
        {
            hw->getPowerSupplyStatus(lock, channel_read, &ps_stat_received);
			ps_stat_received &= ps_stat_mask;
            
            ps_stat_expected = 0x0000;

			if(ps_stat_received != ps_stat_expected)
			{
				printf("\nDiscIO Error, Reading Channel %d\n", channel_read);
				printf("Expected 0x%4X\n", ps_stat_expected);
				printf("Received 0x%4X\n", ps_stat_received);
				return -1;
			}
        }

	}
	
	printf("\nDiscrete I/O Test PASSED\n");

	return 0;
}

STATUS v473_autotest(V473::HANDLE const hw, uint8_t const test_num)
{
    try {
	{
	    // Setup the hardware


	    printf("Hello, world!!!!!\n");
	    
	    printf("Resetting V473...\r\r");
	    
	    {
	        vwpp::Lock lock(hw->mutex);
            hw->reset(lock);
	    }
	    
	    switch(test_num)
	    {
    	    case 0:
    	        DoTestDiscIO(hw);
    	        break;
    	        
    	    case 1:
    	        break;
	    }


	}

    }
    catch (std::exception const& e) {
	printf("caught: %s\n", e.what());
	return ERROR;
    }

    return OK;
}
