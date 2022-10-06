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
// Perform various read/write tests from the VME Bus
//------------------------------------------------------------------------------
int TestVmeBus(V473::HANDLE const hw)
{
    printf("\nVME Bust Test\n");
    
    const uint16_t data_pattern[10] = {0x0000,
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
        sysOut16(dataBuffer+index, data_pattern[index]); // Write to <base address> + 2*index
    }
                                       
    for(size_t index = 0; index < 10; index++)
    {
        receivedData = sysIn16(dataBuffer+index); // Read from <base address> + 2*index
        
        printf("Address 0x%04X:  Wrote 0x%04X, Read 0x%04X", index*2, data_pattern[index], receivedData);
        
        if(receivedData != data_pattern[index])
        {
            printf(" <- FAIL");
            testPass = false;
        }
        
        printf("\n");
    }
                                       
    // Address Bus Walking 1
    printf("\nTesting VME -> Dual Port Address Bus, Walking 1...\n");
    
    for(size_t index = 0; index < 14; index++)
    {
        sysOut16(dataBuffer+(0x0001 << index), 0x5555);
    }
    
    for(size_t index = 0; index < 14; index++)
    {
        receivedData = sysIn16(dataBuffer+(0x0001 << index));
        expectedData = 0x5555;
        
        printf("Address 0x%04X:  Wrote 0x%04X, Read 0x%04X", (0x0002 << index), expectedData, receivedData);
        
        if(receivedData != expectedData)
        {
            printf(" <- FAIL");
            testPass = false;
        }
        
        printf("\n");
        
        sysOut16(dataBuffer+(0x0001 << index), 0xAAAA);
    }
                                       
    for(size_t index = 0; index < 14; index++)
    {
        receivedData = sysIn16(dataBuffer+(0x0001 << index));
        expectedData = 0xAAAA;
        
        printf("Address 0x%04X:  Wrote 0x%04X, Read 0x%04X", (0x0002 << index), expectedData, receivedData);
        
        if(receivedData != expectedData)
        {
            printf(" <- FAIL");
            testPass = false;
        }
        
        printf("\n");
    }
    
    // Address Bus Walking 0
    // Skips address FFFE, because the mailbox mechanism interferes
    printf("\nTesting VME -> Dual Port Address Bus, Walking 0...\n");
    
    for(size_t index = 1; index < 14; index++)
    {
        sysOut16(dataBuffer + (~(0x0001 << index) & 0x00003FFF), 0x5555);
    }
    
    for(size_t index = 1; index < 14; index++)
    {
        receivedData = sysIn16(dataBuffer + (~(0x0001 << index) & 0x00003FFF));
        expectedData = 0x5555;
        
        printf("Address 0x%04X:  Wrote 0x%04X, Read 0x%04X", (~(0x0002 << index) & 0x00007FFE), expectedData, receivedData);
        
        if(receivedData != expectedData)
        {
            printf(" <- FAIL");
            testPass = false;
        }
        
        printf("\n");
        
        sysOut16(dataBuffer + (~(0x0001 << index) & 0x00003FFF), 0xAAAA);
    }
                                       
    for(size_t index = 1; index < 14; index++)
    {
        receivedData = sysIn16(dataBuffer + (~(0x0001 << index) & 0x00003FFF));
        expectedData = 0xAAAA;
        
        printf("Address 0x%04X:  Wrote 0x%04X, Read 0x%04X", (~(0x0002 << index) & 0x00007FFE), expectedData, receivedData);
        
        if(receivedData != expectedData)
        {
            printf(" <- FAIL");
            testPass = false;
        }
        
        printf("\n");
    }
    
    // V473 Diagnostic Read/Write test
    printf("\nTesting Dual Port -> FPGA Data Bus\n");
    
    for(size_t index = 0; index < 10; index++)
    {
        hw->setVmeDataBusDiag(lock, &data_pattern[index]);
        sysOut16(dataBuffer, ~data_pattern[index]); // Overwrite data pattern to verify CPU really processed it
        
        expectedData = data_pattern[index];
        hw->getVmeDataBusDiag(lock, &receivedData, 1);
        
        printf("Wrote 0x%04X, Read 0x%04X", expectedData, receivedData);
        
        if(receivedData != expectedData)
        {
            printf(" <- FAIL");
            testPass = false;
        }
        
        printf("\n");
    }
    
    // V473 Diagnostic Read - Automatic pattern
    printf("\nTesting FPGA Automatic Pattern\n");
    
    hw->setVmeDataBusDiag(lock, &data_pattern[0]); // One write to prime the pattern

    uint16_t receivedDataBuf[11];
    hw->getVmeDataBusDiag(lock, receivedDataBuf, 11);
        
    expectedData = data_pattern[0];
    receivedData = receivedDataBuf[0];
    printf("Wrote 0x%04X, Read 0x%04X", expectedData, receivedData);
        
    if(receivedData != expectedData)
    {
        printf(" <- FAIL");
        testPass = false;
    }
        
    printf("\n");
    
    printf("Reading automatic pattern...\n");
    
    for(size_t index = 0; index < 10; index++)
    {
        expectedData = data_pattern[index];
        receivedData = receivedDataBuf[index+1];
        
        printf("Expected 0x%04X, Read 0x%04X", expectedData, receivedData);
        
        if(receivedData != expectedData)
        {
            printf(" <- FAIL");
            testPass = false;
        }
        
        printf("\n");
    }
    
    if(testPass)
    {
    	printf("\nV473 VME Bus Test PASSED\n");
        return 0;
    }
    else
    {
    	printf("\nV473 VME Bus Test FAILED\n");
        return 1;
    }
                                       
    return 0;
}

//------------------------------------------------------------------------------
// TestDiscIO
//
// Automatically do a discrete I/O checkout of the V473 board
//
// V473 is connected to a loopback box with the following connections:
//  (all channels, inputs, and outputs counted from zero)
//
//  PS0 Enable -> All channels status input 0
//  PS0 Reset  -> All channels status input 1
//  PS1 Enable -> All channels status input 2
//  PS1 Reset  -> All channels status input 3
//  PS2 Enable -> All channels status input 4
//  PS2 Reset  -> All channels status input 5
//  PS3 Enable -> All channels status input 6
//  PS3 Reset  -> All channels status input 7
//------------------------------------------------------------------------------
int TestDiscIO(V473::HANDLE const hw)
{
	uint16_t channel_drive, channel_read;

	const uint16_t ps_stat_mask = 0x24FF;
	uint16_t ps_stat_expected, ps_stat_received;
	
    bool testPass = true;

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
				testPass = false;
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
				testPass = false;
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
				testPass = false;
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
				testPass = false;
			}
        }

	}
	
	if(testPass)
	{
        printf("\nV473 Discrete I/O Test PASSED\n");
        return 0;
	}
	else
	{
        printf("\nV473 Discrete I/O Test FAILED\n");
        return 1;
	}

	return 0;
}

//------------------------------------------------------------------------------
//  Repeatedly set V473 outputs to 0, +10V, and -10V in a loop
//------------------------------------------------------------------------------
int Calibrate(V473::HANDLE const hw)
{
    char kb_input = 0;
    uint16_t channel = 0;
    
    printf("\nV473 Calibration\n");
    printf("Press any key to continue\n");
    
    while(!isalnum(kb_input) && (kb_input != ' ')) // Skips over extra line feeds and other characters
    {
        scanf("%c", &kb_input);
    }
        
    vwpp::Lock lock(hw->mutex);
    hw->tclkTrigEnable(lock, false);
    
// Put the relays in default positions    
	hw->enablePowerSupply(lock, 0, false);
    hw->enablePowerSupply(lock, 1, false);
    hw->enablePowerSupply(lock, 2, false);
    
    while(1)
    {
        printf("\nSetting all DAC Channels to 0V\n");
        for(channel = 0; channel < 4; channel++)
        {
            hw->setDAC(lock, channel, 0);
        }
        printf("Press any key to continue, Q to quit\n");
        
        kb_input = 0;
        while(!isalnum(kb_input) && (kb_input != ' ')) // Skips over extra line feeds and other characters
        {
            scanf("%c", &kb_input);
        }
        
        if((kb_input == (unsigned int) 'q') || (kb_input == (unsigned int) 'Q'))
        {
            return 0;
        }
        
        printf("\nSetting all DAC Channels to +10V\n");
        for(channel = 0; channel < 4; channel++)
        {
            hw->setDAC(lock, channel, 0x7FFF);
        }
        printf("Press any key to continue, Q to quit\n");
        
        kb_input = 0;
        while(!isalnum(kb_input) && (kb_input != ' ')) // Skips over extra line feeds and other characters
        {
            scanf("%c", &kb_input);
        }
        
        if((kb_input == (unsigned int) 'q') || (kb_input == (unsigned int) 'Q'))
        {
            return 0;
        }
        
        printf("\nSetting all DAC Channels to -10V\n");
        for(channel = 0; channel < 4; channel++)
        {
            hw->setDAC(lock, channel, 0x8000);
        }
        printf("Press any key to continue, Q to quit\n");
        
        kb_input = 0;
        while(!isalnum(kb_input) && (kb_input != ' ')) // Skips over extra line feeds and other characters
        {
            scanf("%c", &kb_input);
        }
        
        if((kb_input == (unsigned int) 'q') || (kb_input == (unsigned int) 'Q'))
        {
            return 0;
        }
    }
    
    
    return 0;
}

//------------------------------------------------------------------------------
// Wrap DAC outputs back to ADC inputs and see what we get
//
//  V473 is connected to a loopback box with control relays
//
//  PS0 Enable output low  = ADC 0, ADC 1 inputs grounded
//  PS0 Enable output high = DAC 0 wrapped to ADC 0, DAC 1 wrapped to ADC 1
//  PS1 Enable output low  = ADC 2, ADC 3 inputs grounded
//  PS1 Enable output high = DAC 2 wrapped to ADC 2, DAC 3 wrapped to ADC 3
//  PS2 Enable output low  = All bias inputs open
//  PS2 Enable output high = All bias inputs connected to 2.5V (adds 5.0V to all DAC outputs)
//------------------------------------------------------------------------------
int TestAnalogIO(V473::HANDLE const hw)
{
    bool testPass = true;
    uint16_t channel = 0;
    
    uint16_t ADCvalue = 0;
    short ADCvalue_signed = 0;
    
    const int tolerance_0V =   100;
    const int tolerance_5V =   200;
    const int tolerance_9_5V = 500;
    
    printf("\nTesting V473 Analog I/O\n");

    vwpp::Lock lock(hw->mutex);
    hw->tclkTrigEnable(lock, false);

// Deenergize K1 and K2 to wrap analog outputs to inputs
    hw->enablePowerSupply(lock, 0, true);
    hw->enablePowerSupply(lock, 1, true);

// Energize K3 to set bias input to ground
    hw->enablePowerSupply(lock, 2, false);

// Set all DACs to zero
    for(channel = 0; channel < 4; channel++)
    {
        hw->setDAC(lock, channel, 0);
    }
    taskDelay(30); // Give card a chance to change outputs and update status

// Read ADCs - should all be near zero
	for(channel = 0; channel < 4; channel++)
	{
    	hw->getADC(lock, channel, &ADCvalue);
    	ADCvalue_signed = (short) ADCvalue;
		
		if((ADCvalue_signed < -tolerance_0V) || (ADCvalue_signed > tolerance_0V))
		{
    		printf("\nAnalog Readback Error on channel %i:\n", channel);
    		printf("DAC output wrapped to ADC input, DAC = 0V\n");
    		printf("Expected ADC(raw) = 0, Received %i\n", ADCvalue_signed);
    		testPass = false;
		}
	}

// Set all DACs to 9.5 volts
    for(channel = 0; channel < 4; channel++)
    {
        hw->setDAC(lock, channel, 0x799A);
    }
    taskDelay(30); // Give card a chance to change outputs and update status

// Read ADCs - should all be near zero
	for(channel = 0; channel < 4; channel++)
	{
    	hw->getADC(lock, channel, &ADCvalue);
    	ADCvalue_signed = (short) ADCvalue;
		
		if((ADCvalue_signed < -tolerance_0V) || (ADCvalue_signed > tolerance_0V))
		{
    		printf("\nAnalog Readback Error on channel %i:\n", channel);
    		printf("DAC output wrapped to ADC input, DAC = 9.5V\n");
    		printf("Expected ADC(raw) = 0, Received %i\n", ADCvalue_signed);
    		testPass = false;
		}
	}

// Set all DACs to -9.5 volts
    for(channel = 0; channel < 4; channel++)
    {
        hw->setDAC(lock, channel, 0x8666);
    }
    taskDelay(30); // Give card a chance to change outputs and update status

// Read ADCs - should all be near zero
	for(channel = 0; channel < 4; channel++)
	{
    	hw->getADC(lock, channel, &ADCvalue);
    	ADCvalue_signed = (short) ADCvalue;
		
		if((ADCvalue_signed < -tolerance_0V) || (ADCvalue_signed > tolerance_0V))
		{
    		printf("\nAnalog Readback Error on channel %i:\n", channel);
    		printf("DAC output wrapped to ADC input, DAC = -9.5V\n");
    		printf("Expected ADC(raw) = 0, Received %i\n", ADCvalue_signed);
    		testPass = false;
		}
	}

// Energize K1 and K2 to ground analog inputs
    hw->enablePowerSupply(lock, 0, false);
    hw->enablePowerSupply(lock, 1, false);

// Set all DACs to zero
    for(channel = 0; channel < 4; channel++)
    {
        hw->setDAC(lock, channel, 0);
    }
    taskDelay(30); // Give card a chance to change outputs and update status

// Read ADCs - should all be near zero
	for(channel = 0; channel < 4; channel++)
	{
    	hw->getADC(lock, channel, &ADCvalue);
    	ADCvalue_signed = (short) ADCvalue;
		
		if((ADCvalue_signed < -tolerance_0V) || (ADCvalue_signed > tolerance_0V))
		{
    		printf("\nAnalog Readback Error on channel %i:\n", channel);
    		printf("ADC input grounded, DAC = 0V\n");
    		printf("Expected ADC(raw) = 0, Received %i\n", ADCvalue_signed);
    		testPass = false;
		}
	}

// Set all DACs to 9.5 volts
    for(channel = 0; channel < 4; channel++)
    {
        hw->setDAC(lock, channel, 0x799A);
    }
    taskDelay(30); // Give card a chance to change outputs and update status

// Read ADCs - should all be near 9.5V
	for(channel = 0; channel < 4; channel++)
	{
    	hw->getADC(lock, channel, &ADCvalue);
    	ADCvalue_signed = (short) ADCvalue;
		
		if((ADCvalue_signed < 31130-tolerance_9_5V) || (ADCvalue_signed > 31130+tolerance_9_5V))
		{
    		printf("\nAnalog Readback Error on channel %i:\n", channel);
    		printf("ADC input grounded, DAC = 9.5V\n");
    		printf("Expected ADC(raw) = 31130, Received %i\n", ADCvalue_signed);
    		testPass = false;
		}
	}

// Set all DACs to -9.5 volts
    for(channel = 0; channel < 4; channel++)
    {
        hw->setDAC(lock, channel, 0x8666);
    }
    taskDelay(30); // Give card a chance to change outputs and update status

// Read ADCs - should all be near -9.5V
	for(channel = 0; channel < 4; channel++)
	{
    	hw->getADC(lock, channel, &ADCvalue);
    	ADCvalue_signed = (short) ADCvalue;
		
		if((ADCvalue_signed < -31130-tolerance_9_5V) || (ADCvalue_signed > -31130+tolerance_9_5V))
		{
    		printf("\nAnalog Readback Error on channel %i:\n", channel);
    		printf("ADC input grounded, DAC = -9.5V\n");
    		printf("Expected ADC(raw) = -31130, Received %i\n", ADCvalue_signed);
    		testPass = false;
		}
	}

// Denergize K3 to set bias input to 2.5V
    hw->enablePowerSupply(lock, 2, true);

// Set all DACs to zero
    for(channel = 0; channel < 4; channel++)
    {
        hw->setDAC(lock, channel, 0);
    }
    taskDelay(30); // Give card a chance to change outputs and update status

// Read ADCs - should all be near 5V
	for(channel = 0; channel < 4; channel++)
	{
    	hw->getADC(lock, channel, &ADCvalue);
    	ADCvalue_signed = (short) ADCvalue;
		
		if((ADCvalue_signed < 16334-tolerance_5V) || (ADCvalue_signed > 16434+tolerance_5V))
		{
    		printf("\nAnalog Readback Error on channel %i:\n", channel);
    		printf("ADC input grounded, DAC = 0V, Bias Input = 2.5V\n");
    		printf("Expected ADC(raw) = 16334, Received %i\n", ADCvalue_signed);
    		testPass = false;
		}
	}
	
// Put the relays back to default positions    
	hw->enablePowerSupply(lock, 0, false);
    hw->enablePowerSupply(lock, 1, false);
    hw->enablePowerSupply(lock, 2, false);
    
    if(testPass)
    {
    	printf("\nV473 Analog I/O Test PASSED\n");
        return 0;
    }
    else
    {
    	printf("\nV473 Analog I/O Test FAILED\n");
        return 1;
    }

    return 0;
}

//------------------------------------------------------------------------------
// Set up a unique ramp to play on each channel, $0F trigger
//------------------------------------------------------------------------------
int PlayRamps(V473::HANDLE const hw)
{
	uint16_t data;
	uint16_t channel;
	
	printf("\nSetting up Test Ramps...\n");
	    
    vwpp::Lock lock(hw->mutex);
    hw->tclkTrigEnable(lock, true);
    
    // Put the relays in default positions    
	hw->enablePowerSupply(lock, 0, false);
    hw->enablePowerSupply(lock, 1, false);
    hw->enablePowerSupply(lock, 2, false);

    // Set scale factor (1) in each table to unity (256)
    for(channel = 0; channel < 4; channel++)
    {
        data = 0x0100;
	    hw->setScaleFactors(lock, channel, 0, &data, 1);
    }
    
    // Set scale factor map to select scale factor (1) for Interrupt Level (0)
    for(channel = 0; channel < 4; channel++)
    {
        data = 1;
	    hw->setScaleFactorMap(lock, channel, 0, &data, 1);
    }
    
    // Set ramp map to select ramp (1) for Interrupt Level (0)
    for(channel = 0; channel < 4; channel++)
    {
        data = 1;
	    hw->setRampMap(lock, channel, 0, &data, 1);
    }
    
    // Map TCLK event to Interrupt Level (0)
    uint8_t event = 0x0F;
    hw->setTriggerMap(lock, 0, &event, 1);
    
    uint16_t Buffer[16];
        
    // Send ramp 1 data, channel 0
    Buffer[0*2    ] = (uint16_t)  30000;
    Buffer[0*2 + 1] = (uint16_t)   1000;
    Buffer[1*2    ] = (uint16_t)  30000;
    Buffer[1*2 + 1] = (uint16_t)   1000;
    Buffer[2*2    ] = (uint16_t)      0;
    Buffer[2*2 + 1] = (uint16_t)   1000;
    Buffer[3*2    ] = (uint16_t) -30000;
    Buffer[3*2 + 1] = (uint16_t)   1000;
    Buffer[4*2    ] = (uint16_t) -30000;
    Buffer[4*2 + 1] = (uint16_t)   1000;
    Buffer[5*2    ] = (uint16_t)      0;
    Buffer[5*2 + 1] = (uint16_t)   1000;
    Buffer[6*2    ] = (uint16_t)      0;
    Buffer[6*2 + 1] = (uint16_t)      0;
	hw->setRamp(lock, 0, 1, 0, Buffer, 14);
        
    // Send ramp 1 data, channel 1
    Buffer[0*2    ] = (uint16_t) -30000;
    Buffer[0*2 + 1] = (uint16_t)   1000;
    Buffer[1*2    ] = (uint16_t) -30000;
    Buffer[1*2 + 1] = (uint16_t)   1000;
    Buffer[2*2    ] = (uint16_t)      0;
    Buffer[2*2 + 1] = (uint16_t)   1000;
    Buffer[3*2    ] = (uint16_t)  30000;
    Buffer[3*2 + 1] = (uint16_t)   1000;
    Buffer[4*2    ] = (uint16_t)  30000;
    Buffer[4*2 + 1] = (uint16_t)   1000;
    Buffer[5*2    ] = (uint16_t)      0;
    Buffer[5*2 + 1] = (uint16_t)   1000;
    Buffer[6*2    ] = (uint16_t)      0;
    Buffer[6*2 + 1] = (uint16_t)      0;
	hw->setRamp(lock, 1, 1, 0, Buffer, 14);
        
    // Send ramp 1 data, channel 2
    Buffer[0*2    ] = (uint16_t)  30000;
    Buffer[0*2 + 1] = (uint16_t)   1000;
    Buffer[1*2    ] = (uint16_t)  30000;
    Buffer[1*2 + 1] = (uint16_t)   1000;
    Buffer[2*2    ] = (uint16_t)      0;
    Buffer[2*2 + 1] = (uint16_t)   1000;
    Buffer[3*2    ] = (uint16_t)  30000;
    Buffer[3*2 + 1] = (uint16_t)   1000;
    Buffer[4*2    ] = (uint16_t)  30000;
    Buffer[4*2 + 1] = (uint16_t)   1000;
    Buffer[5*2    ] = (uint16_t)      0;
    Buffer[5*2 + 1] = (uint16_t)   1000;
    Buffer[6*2    ] = (uint16_t)      0;
    Buffer[6*2 + 1] = (uint16_t)      0;
	hw->setRamp(lock, 2, 1, 0, Buffer, 14);
        
    // Send ramp 1 data, channel 3
    Buffer[0*2    ] = (uint16_t) -30000;
    Buffer[0*2 + 1] = (uint16_t)   1000;
    Buffer[1*2    ] = (uint16_t) -30000;
    Buffer[1*2 + 1] = (uint16_t)   1000;
    Buffer[2*2    ] = (uint16_t)      0;
    Buffer[2*2 + 1] = (uint16_t)   1000;
    Buffer[3*2    ] = (uint16_t) -30000;
    Buffer[3*2 + 1] = (uint16_t)   1000;
    Buffer[4*2    ] = (uint16_t) -30000;
    Buffer[4*2 + 1] = (uint16_t)   1000;
    Buffer[5*2    ] = (uint16_t)      0;
    Buffer[5*2 + 1] = (uint16_t)   1000;
    Buffer[6*2    ] = (uint16_t)      0;
    Buffer[6*2 + 1] = (uint16_t)      0;
	hw->setRamp(lock, 3, 1, 0, Buffer, 14);
	
	printf("Ramp setup complete\n");
	
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
            printf("  2:  Discrete I/O Test\n");
            printf("  3:  Calibration\n");
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
    	            TestDiscIO(hw);
    	            break;
    	        
    	        case '3':
    	            Calibrate(hw);
    	            break;
    	        
    	        case '4':
    	            TestAnalogIO(hw);
    	            break;
    	        
    	        case '5':
    	            PlayRamps(hw);
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
