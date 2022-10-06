# fe-drv-mooc-v473

The V473 is a VME-based programmable ramp controller capable of
generating four semi-independent, time-based analog outputs. These
outputs are updated at a 100 kHz rate. The V473 also contains digital
control capabilities to turn on, turn off, and reset four power
supplies. It can also return eight status bits, a ramp enable bit, and
a power supply enable bit from each of the regulator supplies.

	Online info: http://www-bd.fnal.gov/issues/wiki/V473

# Using this Driver

v1.6 is built against MOOC v4.6.

Our development environment now supports quite a few combinations of
hardware and software platforms. Be sure to download from the
appropriate area.

| BSP  | VxWorks | File |
| ---- | ------- | ---- |
| mv2400 | 6.4 | vxworks_boot/v6.4/module/mv2400/v473-1.6.out |
| mv2401 | 6.4 | vxworks_boot/v6.4/module/mv2401/v473-1.6.out |
| mv2434 | 6.4 | vxworks_boot/v6.4/module/mv2434/v473-1.6.out |
| mv5500 | 6.4 | vxworks_boot/v6.4/module/mv5500/v473-1.6.out |
| mv2400 | 6.7 | vxworks_boot/v6.7/module/mv2400/v473-1.6.out |
| mv2401 | 6.7 | vxworks_boot/v6.7/module/mv2401/v473-1.6.out |
| mv2434 | 6.7 | vxworks_boot/v6.7/module/mv2434/v473-1.6.out |
| mv5500 | 6.7 | vxworks_boot/v6.7/module/mv5500/v473-1.6.out |

## Loading the Driver

The latest version of the V473 driver is 1.5, which has been built
against MOOC 4.2. The proper loading sequence for the V473 driver in
your VxWorks startup script is:

    ld (1,1,"vxworks_boot/VER/module/BSP/mooc-4.2.out");
    ld (1,1,"vxworks_boot/VER/module/BSP/v473-1.4.out");

Replace `VER` with your system's version of VxWorks (v6.4, or
v6.7). Replace `BSP` with your system's BSP (mv2400, mv2401, mv2434, or
mv5500.)

### Connecting to MOOC

The first step is to register the driver with MOOC. In this example,
we associate the ramp card driver with MOOC Class ID 16. If your
front-end already is using this class ID, specify a different one.

    v473_create_mooc_class(16);

The next step is to create instances of the driver. This example shows
two instances being created, which indicates that two V473s are
installed in the crate.

    v473_create_mooc_instance(16, 0, 0x90);
    v473_create_mooc_instance(17, 1, 0x91);

The first argument is the MOOC OID to associate with the instance.
This OID value will be the value used in the second word of the SSDN
when creating database entries.

The V473 has 8 DIP switches to adjust its base address in A32
space. Each card needs to be assigned a different address. The second
argument indicates the value of the DIP switches, which helps the
driver locate the ramp card hardware.

The last argument is the interrupt vector to use. It, too, must be
unique across all ramp cards.

## DABBEL Template

This is the template used to create V473 devices. In the following
`dabbel` source, the following strings should be replaced with text
appropriate for your installation:

| String in Source | Replaced By |
| ---------------- | ----------- |
| p:bbbbb | base name for device |
| nnnnnn | ACNET node name for front-end |
| oo | hexadecimal value of OID |

This example does not specify a reading property for the first
device. Typically, that property is created to point to an ADC in a
different piece of hardware (a CAMAC 190/290, or an HRM channel, for
instance.)

The V473 has four ramp generators in it. This dabbel file only defines
devices for one generator. The clock device, however, (the device with
the 'C' suffix) is shared among all channels. It can only be defined
once, but it will be referred to in all four channels' family device
(the device with the 'Z' suffix.)

```
ADD p:bbbbb  ( "bbbbb Current", nnnnnn,, 27FFFFFE, 0, , , "" )
AAMASK ("MCR+MCRCrewChief")
SSDNHX PRBCTL (0000/00oo/0000/0010)
PRO PRBCTL ( 2, 2, 60)
SSDNHX PRBSTS (0000/00oo/0000/0010)
PRO PRBSTS ( 2, 2, 60)
PDB PRBSTS ( 47, 00, 00000100, 00000001, 00000088, 00000000, 2,
             042A022E, 0454022E, 0657022E, 00000000)
PRO EXTEXT ( 1, 14, 2, "No     ", 4, "Yes    ", "Supply Tracking Error   ",
             1, 13, 2, "Off    ", 4, "On     ", "Power Supply Reset      ",
             1, 12, 2, "No     ", 2, "Yes    ", "Ramp Active             ",
             1, 11, 4, "Inact  ", 2, "Active ", "Interlock Input         ",
             1, 10, 4, "Disable", 2, "Enable ", "Power Supply            ",
             1, 9, 2, "No     ", 4, "Yes    ", "Overflow                ",
             1, 8, 4, "No     ", 2, "Yes    ", "Ramp Enabled            ",
             1, 7, 4, "Fault  ", 2, "OK     ", "Tracking Error Fault....",
             1, 6, 4, "Fault  ", 2, "OK     ", "DC Over Current Fault...",
             1, 5, 4, "Fault  ", 2, "OK     ", "Ground Fault............",
             1, 4, 4, "Fault  ", 2, "OK     ", "RMS Over Current Fault..",
             1, 3, 4, "Fault  ", 2, "OK     ", "Low Input Voltage Fault.",
             1, 2, 4, "Fault  ", 2, "OK     ", "External Permit Fault...",
             1, 1, 4, "Fault  ", 2, "OK     ", "Over Temp Fault.........",
             1, 0, 4, "Fault  ", 2, "OK     ", "Trip Summation Fault....")
SSDNHX PRSET  (0000/00oo/0000/0010)
PRO PRSET  ( 4, 3840, 60)
PDB PRSET  ( "Volt", "Amps", 2, 6, 2, 0, 0, 0, 4, 1)
PRO PRDCTL ( 00000003, 0, "RESET", "Reset",
             00000002, 1, "ON", "On",
             00000001, 2, "OFF", "Off")
!
ADD p:bbbbbB ( "Bulk Param Device", nnnnnn,, 27FFFFFE, 0, , , "" )
AAMASK ("MCR+MCRCrewChief")
SSDNHX PRBCTL (0000/00oo/0000/0080)
PRO PRBCTL ( 2, 2, 60)
SSDNHX PRBSTS (0000/00oo/0000/0080)
PRO PRBSTS ( 4, 4, 60)
PDB PRBSTS ( 13, 00, 00000100, 000000FF, 00000000, 00000000, 4,
             0444022E, 0454022E, 00000000, 00000000)
PRO EXTEXT ( 1, 14, 2, "No     ", 4, "Yes    ", "Supply Tracking Error   ",
             1, 13, 2, "Off    ", 4, "On     ", "Power Supply Reset      ",
             1, 12, 2, "No     ", 2, "Yes    ", "Ramp Active             ",
             1, 11, 4, "Inact  ", 2, "Active ", "Interlock Input         ",
             1, 10, 4, "Disable", 2, "Enable ", "Power Supply            ",
             1, 9, 2, "No     ", 4, "Yes    ", "Overflow                ",
             1, 8, 4, "No     ", 2, "Yes    ", "Ramp Enabled            ")
SSDNHX PRSET  (0000/00oo/0000/0080)
PRO PRSET  ( 2, 2, 60)
PRO PRDCTL ( 00000003, 0, "RESET", "Reset",
             00000002, 1, "ON", "On",
             00000001, 2, "OFF", "Off")
!
ADD p:bbbbbL ( "Delay Table", nnnnnn, , 27FFFFFE, 0, , , "" )
AAMASK ("MCR+MCRCrewChief")
PRO EXTEXT ( 1, 14, 2, "No     ", 4, "Yes    ", "Supply Tracking Error   ",
             1, 13, 2, "Off    ", 4, "On     ", "Power Supply Reset      ",
             1, 12, 2, "No     ", 2, "Yes    ", "Ramp Active             ",
             1, 11, 4, "Inact  ", 2, "Active ", "Interlock Input         ",
             1, 10, 4, "Disable", 2, "Enable ", "Power Supply            ",
             1, 9, 2, "No     ", 4, "Yes    ", "Overflow                ",
             1, 8, 4, "No     ", 2, "Yes    ", "Ramp Enabled            ")
SSDNHX PRSET  (0000/00oo/0000/0030)
PRO PRSET  ( 2, 64, 60)
PDB PRSET  ( "raw ", "uSec", 20, 40, 2, 0, 0, 0, 1, 1, 0, 30, 255, 1)
PRO PRDCTL ( 00000003, 0, "RESET", "Reset",
             00000002, 1, "ON", "On",
             00000001, 2, "OFF", "Off")
!
ADD p:bbbbbM ( "Map Table               ", nnnnnn,         , 27FFFFFE,
     0, , , "" )
AAMASK ("MCR+MCRCrewChief")
PRO EXTEXT ( 1, 14, 2, "No     ", 4, "Yes    ", "Supply Tracking Error   ",
             1, 13, 2, "Off    ", 4, "On     ", "Power Supply Reset      ",
             1, 12, 2, "No     ", 2, "Yes    ", "Ramp Active             ",
             1, 11, 4, "Inact  ", 2, "Active ", "Interlock Input         ",
             1, 10, 4, "Disable", 2, "Enable ", "Power Supply            ",
             1, 9, 2, "No     ", 4, "Yes    ", "Overflow                ",
             1, 8, 4, "No     ", 2, "Yes    ", "Ramp Enabled            ")
SSDNHX PRREAD (0000/00oo/0000/0050)
PRO PRREAD ( 2, 88, 60)
PDB PRREAD ( "    ", "    ", 20, 0, 2, 0, 0, 0)
SSDNHX PRSET  (0000/00oo/0000/0050)
PRO PRSET  ( 2, 320, 60)
PRO PRDCTL ( 00000003, 0, "RESET", "Reset",
             00000002, 1, "ON", "On",
             00000001, 2, "OFF", "Off")
!
ADD p:bbbbbO ( "Offset Table            ", nnnnnn,         , 27FFFFFE,
     0, , , "" )
AAMASK ("MCR+MCRCrewChief")
PRO EXTEXT ( 1, 14, 2, "No     ", 4, "Yes    ", "Supply Tracking Error   ",
             1, 13, 2, "Off    ", 4, "On     ", "Power Supply Reset      ",
             1, 12, 2, "No     ", 2, "Yes    ", "Ramp Active             ",
             1, 11, 4, "Inact  ", 2, "Active ", "Interlock Input         ",
             1, 10, 4, "Disable", 2, "Enable ", "Power Supply            ",
             1, 9, 2, "No     ", 4, "Yes    ", "Overflow                ",
             1, 8, 4, "No     ", 2, "Yes    ", "Ramp Enabled            ")
SSDNHX PRSET  (0000/00oo/0000/0040)
PRO PRSET  ( 2, 64, 60)
PDB PRSET  ( "Volt", "Amps", 2, 6, 2, 0, 0, 0, 3, 1)
PRO PRDCTL ( 00000003, 0, "RESET", "Reset",
             00000002, 1, "ON", "On",
             00000001, 2, "OFF", "Off")
!
ADD p:bbbbbP ( "Phase Table             ", nnnnnn,         , 27FFFFFE,
     0, , , "" )
AAMASK ("MCR+MCRCrewChief")
SSDNHX PRBCTL (0000/00oo/0000/00A0)
PRO PRBCTL ( 2, 2, 60)
SSDNHX PRBSTS (0000/00oo/0000/00A0)
PRO PRBSTS ( 2, 2, 60)
PDB PRBSTS ( 13, 00, 00000100, 000000FF, 00000000, 00000000, 2,
             0444022E, 0454022E, 00000000, 00000000)
PRO EXTEXT ( 1, 14, 2, "No     ", 4, "Yes    ", "Supply Tracking Error   ",
             1, 13, 2, "Off    ", 4, "On     ", "Power Supply Reset      ",
             1, 12, 2, "No     ", 2, "Yes    ", "Ramp Active             ",
             1, 11, 4, "Inact  ", 2, "Active ", "Interlock Input         ",
             1, 10, 4, "Disable", 2, "Enable ", "Power Supply            ",
             1, 9, 2, "No     ", 4, "Yes    ", "Overflow                ",
             1, 8, 4, "No     ", 2, "Yes    ", "Ramp Enabled            ")
SSDNHX PRSET  (0000/00oo/0000/00A0)
PRO PRSET  ( 2, 64, 60)
PDB PRSET  ( "raw ", "Deg ", 2, 6, 2, 0, 0, 0, 18, 1)
PRO PRDCTL ( 00000003, 0, "RESET", "Reset",
             00000002, 1, "ON", "On",
             00000001, 2, "OFF", "Off")
!
ADD p:bbbbbQ ( "Chi Freq Table          ", nnnnnn,         , 27FFFFFE,
     0, , , "" )
AAMASK ("MCR+MCRCrewChief")
SSDNHX PRBCTL (0000/00oo/0000/0090)
PRO PRBCTL ( 2, 2, 60)
SSDNHX PRBSTS (0000/00oo/0000/0090)
PRO PRBSTS ( 2, 2, 60)
PDB PRBSTS ( 13, 00, 00000100, 000000FF, 00000000, 00000000, 2,
             0444022E, 0454022E, 00000000, 00000000)
PRO EXTEXT ( 1, 14, 2, "No     ", 4, "Yes    ", "Supply Tracking Error   ",
             1, 13, 2, "Off    ", 4, "On     ", "Power Supply Reset      ",
             1, 12, 2, "No     ", 2, "Yes    ", "Ramp Active             ",
             1, 11, 4, "Inact  ", 2, "Active ", "Interlock Input         ",
             1, 10, 4, "Disable", 2, "Enable ", "Power Supply            ",
             1, 9, 2, "No     ", 4, "Yes    ", "Overflow                ",
             1, 8, 4, "No     ", 2, "Yes    ", "Ramp Enabled            ")
SSDNHX PRSET  (0000/00oo/0000/0090)
PRO PRSET  ( 2, 64, 60)
PDB PRSET  ( "raw ", "KHz ", 2, 6, 2, 0, 0, 0, 5, 1)
PRO PRDCTL ( 00000003, 0, "RESET", "Reset",
             00000002, 1, "ON", "On",
             00000001, 2, "OFF", "Off")
!
ADD p:bbbbbS ( "Scale Factor Table      ", nnnnnn,         , 27FFFFFE,
     0, , , "" )
AAMASK ("MCR+MCRCrewChief")
PRO EXTEXT ( 1, 14, 2, "No     ", 4, "Yes    ", "Supply Tracking Error   ",
             1, 13, 2, "Off    ", 4, "On     ", "Power Supply Reset      ",
             1, 12, 2, "No     ", 2, "Yes    ", "Ramp Active             ",
             1, 11, 4, "Inact  ", 2, "Active ", "Interlock Input         ",
             1, 10, 4, "Disable", 2, "Enable ", "Power Supply            ",
             1, 9, 2, "No     ", 4, "Yes    ", "Overflow                ",
             1, 8, 4, "No     ", 2, "Yes    ", "Ramp Enabled            ")
SSDNHX PRSET  (0000/00oo/0000/0060)
PRO PRSET  ( 2, 62, 60)
PDB PRSET  ( "    ", "    ", 40, 0, 2, 0, 0, 0)
PRO PRDCTL ( 00000003, 0, "RESET", "Reset",
             00000002, 1, "ON", "On",
             00000001, 2, "OFF", "Off")
!
ADD p:bbbbbT ( "F(t) Table              ", nnnnnn,         , 27FFFFFE,
     0, , , "" )
AAMASK ("MCR+MCRCrewChief")
PRO EXTEXT ( 1, 14, 2, "No     ", 4, "Yes    ", "Supply Tracking Error   ",
             1, 13, 2, "Off    ", 4, "On     ", "Power Supply Reset      ",
             1, 12, 2, "No     ", 2, "Yes    ", "Ramp Active             ",
             1, 11, 4, "Inact  ", 2, "Active ", "Interlock Input         ",
             1, 10, 4, "Disable", 2, "Enable ", "Power Supply            ",
             1, 9, 2, "No     ", 4, "Yes    ", "Overflow                ",
             1, 8, 4, "No     ", 2, "Yes    ", "Ramp Enabled            ")
SSDNHX PRSET  (0000/00oo/0000/0020)
PRO PRSET  ( 4, 3840, 60)
PDB PRSET  ( "Volt", "Amps", 2, 6, 2, 0, 0, 0, 4, 1)
PRO PRDCTL ( 00000003, 0, "RESET", "Reset",
             00000002, 1, "ON", "On",
             00000001, 2, "OFF", "Off")
!
ADD p:bbbbbU ( "SW Version Table        ", nnnnnn,         , 27FFFFFE,
     0, , , "" )
AAMASK ("MCR+MCRCrewChief")
PRO EXTEXT ( 1, 14, 2, "No     ", 4, "Yes    ", "Supply Tracking Error   ",
             1, 13, 2, "Off    ", 4, "On     ", "Power Supply Reset      ",
             1, 12, 2, "No     ", 2, "Yes    ", "Ramp Active             ",
             1, 11, 4, "Inact  ", 2, "Active ", "Interlock Input         ",
             1, 10, 4, "Disable", 2, "Enable ", "Power Supply            ",
             1, 9, 2, "No     ", 4, "Yes    ", "Overflow                ",
             1, 8, 4, "No     ", 2, "Yes    ", "Ramp Enabled            ")
SSDNHX PRSET  (0000/00oo/0000/0070)
PRO PRSET  ( 2, 84, 60)
PRO PRDCTL ( 00000003, 0, "RESET", "Reset",
             00000002, 1, "ON", "On",
             00000001, 2, "OFF", "Off")
!
ADD p:bbbbC  ( "Clock Events            ", nnnnnn,         , 27FFFFFE,
     0, , , "" )
AAMASK ("MCR+MCRCrewChief")
SSDNHX PRBCTL (0000/00oo/0000/00B0)
PRO PRBCTL ( 2, 2, 60)
SSDNHX PRBSTS (0000/00oo/0000/00B0)
PRO PRBSTS ( 2, 2, 60)
PDB PRBSTS ( 11, 00, 00000001, 00000000, 00000000, 00000000, 2,
             0444022E, 00000000, 00000000, 00000000)
PRO EXTEXT ( 1, 0, 4, "No     ", 2, "Yes    ", "Clock Interrupts Enabled")
SSDNHX PRSET  (0000/00oo/0000/00B0)
PRO PRSET  ( 2, 512, 60)
PRO PRDCTL ( 00000003, 0, "RESET", "Reset",
             00000002, 1, "ON", "On",
             00000001, 2, "OFF", "Off")
!
ADD p:bbbbbZ ( "Family Device           ", LOCAL ,         , 07FFFFFE,
     0, , , "" )
AAMASK ("MCR+MCRCrewChief")
PRO PRFMLY ( p:bbbbb , p:bbbbbM, p:bbbbbS, p:bbbbbT, p:bbbbbU, p:bbbbbO,
             p:bbbbbL, p:bbbbbB, p:bbbbC , p:bbbbbQ, p:bbbbbP)
```
