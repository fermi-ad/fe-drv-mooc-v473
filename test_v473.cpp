// $Id$

#include "v473.h"
#include <cmath>
#include <taskLib.h>
#include <cstdio>
#include <iostream>
#include <string>
#include <ctype.h>

extern "C" UINT16 sysIn16(UINT16*);
extern "C" void sysOut16(UINT16*, UINT16);

//------------------------------------------------------------------------------
// TestDiscIO
//
// Automatically do a discrete I/O checkout of the C473 board
//------------------------------------------------------------------------------
int TestDiscIO(V473::HANDLE const hw)
{
	uint16_t channel_drive, channel_read;

	const uint16_t ps_stat_mask = 0x24FF;
	uint16_t ps_stat_expected, ps_stat_received;
	
	printf("\nTesting V473 Discrete I/O\n");

    vwpp::Lock lock(hw->mutex);

// Put discrete outputs in a known state
    printf("Setting all PS Enable/Reset outputs to 0...\n");

	for(channel_drive = 0; channel_drive < 4; channel_drive++)
	{
        hw->enablePowerSupply(lock, channel_drive, false);
	}
    taskDelay(10); // Give card a chance to change outputs and update status

// Toggle PS enable outputs
	for(channel_drive = 0; channel_drive < 4; channel_drive++)
	{
		for(channel_read = 0; channel_read < 4; channel_read++)
		{
    		hw->getPowerSupplyStatus(lock, channel_read, &ps_stat_received);

			ps_stat_received &= ps_stat_mask;
			ps_stat_expected = 0x00FF;

			if(ps_stat_received != ps_stat_expected)
			{
    			printf("\nDiscIO Error, Reading Channel %i\n", channel_read);
				printf("Expected 0x%04X\n", ps_stat_expected);
				printf("Received 0x%04X\n", ps_stat_received);
				return -1;
			}
		}
		
		printf("Setting Channel %i PS Enable to 1...\n", channel_drive);

        hw->enablePowerSupply(lock, channel_drive, true);
        taskDelay(10); // Give card a chance to change outputs and update status

		for(channel_read = 0; channel_read < 4; channel_read++)
		{
			hw->getPowerSupplyStatus(lock, channel_read, &ps_stat_received);

			ps_stat_received &= ps_stat_mask;
            
			if(channel_drive == channel_read)
            {
    			ps_stat_expected = 0x04FF & ~(0x0001 << (2*channel_drive));
            }
            else
            {
    			ps_stat_expected = 0x00FF & ~(0x0001 << (2*channel_drive));
            }

			if(ps_stat_received != ps_stat_expected)
			{
				printf("\nDiscIO Error, Reading Channel %i\n", channel_read);
				printf("Expected 0x%04X\n", ps_stat_expected);
				printf("Received 0x%04X\n", ps_stat_received);
				return -1;
			}
		}

		printf("Setting Channel %i PS Enable to 0...\n", channel_drive);
        hw->enablePowerSupply(lock, channel_drive, false);
        taskDelay(10); // Give card a chance to change outputs and update status
	}

// Toggle PS reset outputs
	for(channel_drive = 0; channel_drive < 4; channel_drive++)
	{
		printf("Sending 1-sec pulse to Channel %i PS Reset...\n", channel_drive);
        hw->resetPowerSupply(lock, channel_drive);

// The Reset output is run by the processor's 1sec interrupt cycle
//  It may take up to a second to see the output change.
        hw->getPowerSupplyStatus(lock, channel_drive, &ps_stat_received);
        
        while((ps_stat_received & 0x2000) != 0x2000) // Sniff the PS Reset status bit
        {
            hw->getPowerSupplyStatus(lock, channel_drive, &ps_stat_received);
        }
        
        taskDelay(10); // Give card a chance to change outputs and update status
        
        for(channel_read = 0; channel_read < 4; channel_read++)
        {
            hw->getPowerSupplyStatus(lock, channel_read, &ps_stat_received);
			ps_stat_received &= ps_stat_mask;
            
            if(channel_drive == channel_read)
            {
                ps_stat_expected = 0x20FF & ~(0x0002 << (2*channel_drive));
            }
            else
            {
                ps_stat_expected = 0x00FF & ~(0x0002 << (2*channel_drive));
            }

			if(ps_stat_received != ps_stat_expected)
			{
				printf("\nDiscIO Error, Reading Channel %i\n", channel_read);
				printf("Expected 0x%04X\n", ps_stat_expected);
				printf("Received 0x%04X\n", ps_stat_received);
				return -1;
			}
        }

// Wait for the Reset output to turn off
        hw->getPowerSupplyStatus(lock, channel_drive, &ps_stat_received);
        
        while((ps_stat_received & 0x2000) != 0x0000) // Sniff the PS Reset status bit
        {
            hw->getPowerSupplyStatus(lock, channel_drive, &ps_stat_received);
        }
        
        taskDelay(10); // Give card a chance to change outputs and update status
        
        for(channel_read = 0; channel_read < 4; channel_read++)
        {
            hw->getPowerSupplyStatus(lock, channel_read, &ps_stat_received);
			ps_stat_received &= ps_stat_mask;
            
            ps_stat_expected = 0x00FF;

			if(ps_stat_received != ps_stat_expected)
			{
				printf("\nDiscIO Error, Reading Channel %i\n", channel_read);
				printf("Expected 0x%04X\n", ps_stat_expected);
				printf("Received 0x%04X\n", ps_stat_received);
				return -1;
			}
        }

	}
	
	printf("\nDiscrete I/O Test PASSED\n");

	return 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int TestVmeBus(V473::HANDLE const hw)
{
    printf("\nVME Bust Test\n");
    
    const uint16_t test_pattern[10] = {0x0000,
                                       0xFFFF,
                                       0x00FF,
                                       0xFF00,
                                       0x0F0F,
                                       0xF0F0,
                                       0x3333,
                                       0xCCCC,
                                       0x5555,
                                       0xAAAA};
    
    uint16_t expectedData, receivedData;
    bool testPass = true;
    
    vwpp::Lock lock(hw->mutex);
    uint16_t* dataBuffer = hw->getDataBuffer(lock);
    
    // Data Bus Bit Independence
    printf("\nTesting VME -> Dual Port Data Bus...\n");
    
    for(size_t index = 0; index < 10; index++)
    {
        sysOut16(dataBuffer+index, test_pattern[index]);
    }
                                       
    for(size_t index = 0; index < 10; index++)
    {
        receivedData = sysIn16(dataBuffer+index);
        
        printf("Address 0x%04X:  Wrote 0x%04X, Read 0x%04X", index, test_pattern[index], receivedData);
        
        if(receivedData != test_pattern[index])
        {
            printf(" <- FAIL");
            testPass = false;
        }
        
        printf("\n");
    }
                                       
    // Address Bus Bit Independence
    printf("\nTesting VME -> Dual Port Address Bus...\n");
    
    sysOut16(dataBuffer+0x7FFA, 0xFFFF); // Load MB Addr reg with an invalid value to keep from stepping on anything
    
    for(size_t index = 0; index < 10; index++)
    {
        sysOut16(dataBuffer+(test_pattern[index] & 0x7FFE), 0x5555);
    }
    
    for(size_t index = 0; index < 10; index++)
    {
        receivedData = sysIn16(dataBuffer+(test_pattern[index] & 0x7FFE));
        expectedData = 0x5555;
        
        printf("Address 0x%04X:  Wrote 0x%04X, Read 0x%04X", (test_pattern[index] & 0x7FFE), expectedData, receivedData);
        
        if(receivedData != expectedData)
        {
            printf(" <- FAIL");
            testPass = false;
        }
        
        printf("\n");
        
        sysOut16(dataBuffer+(0x0001 << index), 0xAAAA);
    }
                                       
    for(size_t index = 0; index < 10; index++)
    {
        receivedData = sysIn16(dataBuffer+(test_pattern[index] & 0x7FFE));
        expectedData = 0xAAAA;
        
        printf("Address 0x%04X:  Wrote 0x%04X, Read 0x%04X", (test_pattern[index] & 0x7FFE), expectedData, receivedData);
        
        if(receivedData != expectedData)
        {
            printf(" <- FAIL");
            testPass = false;
        }
        
        printf("\n");
    }
                                       
    return 0;
}

STATUS v473_autotest(V473::HANDLE const hw)
{
    try {
	{
	    printf("V473 Test:\n");
	    printf("Detecting Card...\n");
	    
	    {
    	    uint16_t temp;
    	    
    	    vwpp::Lock lock(hw->mutex);
	        
    	    hw->getModuleId(lock, &temp);
	        printf("Module ID = %i\n", temp);
	        
    	    hw->getFirmwareVersion(lock, &temp);
	        printf("Firmware Version = %04X\n", temp);
	        
    	    hw->getFpgaVersion(lock, &temp);
	        printf("FPGA Version = %04X\n", temp);
        }
        
        char test_num = 0;
        
        while(test_num != 'q')
        {
            printf("\n");
            printf("Select Procedure:\n");
            printf("  1:  VME Bus Test\n");
            printf("  2:  Calibration\n");
            printf("  3:  Discrete I/O Test\n");
            printf("  4:  Analog I/O Test\n");
            printf("  5:  Play Ramps\n");
            printf("  Q:  Quit this program\n");
            
            test_num = 0;
            
            while(!isalnum(test_num)) // Skips over extra line feeds and other characters
            {
                scanf("%c", &test_num);
            }
            
	        switch(test_num)
	        {
    	        case '1':
    	            TestVmeBus(hw);
    	            break;
    	        
    	        case '2':
    	            break;
    	        
    	        case '3':
    	            TestDiscIO(hw);
    	            break;
    	        
    	        case '4':
    	            break;
    	        
    	        case '5':
    	            break;
    	        
    	        case 'Q':
    	            test_num = 'q';
    	            break;
    	        
    	        default:
    	            break;
	        }
	        
        }
        


	}

    }
    catch (std::exception const& e) {
	printf("caught: %s\n", e.what());
	return ERROR;
    }

    return OK;
}
