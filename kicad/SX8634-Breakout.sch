EESchema Schematic File Version 4
EELAYER 26 0
EELAYER END
$Descr User 9449 7087
encoding utf-8
Sheet 1 1
Title "WROOM32 Compute PCB"
Date "2018-12-29"
Rev "2"
Comp "Manuvr, Inc"
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L Connector:Conn_01x01_Male P1
U 1 1 5DF45F11
P 4500 2950
F 0 "P1" H 4500 3050 50  0000 C CNN
F 1 "CONN_01X01" V 4600 2950 50  0001 C CNN
F 2 "kicad:ManuvrLogo" H 4700 2850 50  0000 C CNN
F 3 "" H 4500 2950 50  0000 C CNN
	1    4500 2950
	-1   0    0    1   
$EndComp
$Comp
L power:GND #PWR01
U 1 1 59110AC7
P 4250 3050
F 0 "#PWR01" H 4250 2800 50  0001 C CNN
F 1 "GND" H 4250 2900 50  0000 C CNN
F 2 "" H 4250 3050 50  0000 C CNN
F 3 "" H 4250 3050 50  0000 C CNN
	1    4250 3050
	1    0    0    -1  
$EndComp
Text Label 4100 2950 2    39   ~ 0
GND
Wire Wire Line
	4250 2950 4300 2950
Wire Wire Line
	4250 3050 4250 2950
Wire Wire Line
	4100 2950 4250 2950
Connection ~ 4250 2950
$Comp
L Device:C_Small C1
U 1 1 55688A19
P 1300 2450
F 0 "C1" H 1300 2550 40  0000 L CNN
F 1 "1nF" H 1306 2365 40  0000 L CNN
F 2 "Capacitor_SMD:C_0603_1608Metric" V 1250 2600 30  0001 C CNN
F 3 "~" H 1300 2450 60  0000 C CNN
F 4 "CGB3B1X7R1A105K055AC" V -200 -10600 39  0001 C CNN "Manu_Number"
F 5 "TDK Corporation" V -200 -10600 60  0001 C CNN "Manu_Name"
F 6 "445-13228-1-ND" V 1300 -10750 39  0001 C CNN "Digikey_Number"
	1    1300 2450
	0    1    1    0   
$EndComp
Wire Wire Line
	1550 2450 1400 2450
Wire Wire Line
	1550 2600 1150 2600
Wire Wire Line
	1150 2600 1150 2450
Wire Wire Line
	1150 2450 1200 2450
$Comp
L IansParts:CAPARRAY_4 CARY1
U 1 1 5D2BB127
P 3000 3400
F 0 "CARY1" H 3000 2800 60  0000 C CNN
F 1 "100nF" H 3000 3950 39  0000 C CNN
F 2 "CapArray_CKCL44" H 3000 3400 60  0001 C CNN
F 3 "" H 3000 3400 60  0000 C CNN
F 4 "CKCL44X5R0J104M085AA" H 3000 3400 60  0001 C CNN "Manu_Number"
F 5 "TDK Corporation" H 3000 3400 60  0001 C CNN "Manu_Name"
F 6 "445-1838-1-ND" H 3000 3400 60  0001 C CNN "Digikey_Number"
	1    3000 3400
	0    -1   1    0   
$EndComp
Wire Wire Line
	2150 3000 2150 3050
Wire Wire Line
	2150 3050 2250 3050
Wire Wire Line
	2450 3050 2450 3000
Wire Wire Line
	2250 3000 2250 3050
Connection ~ 2250 3050
Wire Wire Line
	2250 3050 2350 3050
Wire Wire Line
	2350 3000 2350 3050
Connection ~ 2350 3050
Wire Wire Line
	2350 3050 2450 3050
Text Label 1600 3150 2    39   ~ 0
VCC
Text Label 1600 3750 2    39   ~ 0
GND
Wire Wire Line
	1750 3150 1750 3000
Wire Wire Line
	2650 2600 2600 2600
$Comp
L IansParts:IANS_CAP_POLARIZED C2
U 1 1 5D371A72
P 1750 3500
F 0 "C2" V 1650 3400 39  0000 C CNN
F 1 "1uF" V 1800 3400 39  0000 C CNN
F 2 "Capacitor_SMD:C_0805_2012Metric" H 1750 3500 60  0001 C CNN
F 3 "" H 1750 3500 60  0000 C CNN
F 4 "F950J107MPAAQ2" V 1750 3500 60  0001 C CNN "Manu_Number"
F 5 "AVX Corporation" V 1750 3500 60  0001 C CNN "Manu_Name"
F 6 "478-8393-1-ND" V 1750 3500 60  0001 C CNN "Digikey_Number"
	1    1750 3500
	1    0    0    -1  
$EndComp
Text Label 2600 1250 0    39   ~ 0
I2C_SDA
Text Label 2600 1150 0    39   ~ 0
I2C_SCL
Text Label 2600 1350 0    39   ~ 0
~Touch_IRQ
Text Label 2600 1450 0    39   ~ 0
~Touch_Reset
$Comp
L Connector:Conn_01x12_Male J1
U 1 1 5D42BD1A
P 5050 1500
F 0 "J1" V 5000 950 50  0000 C CNN
F 1 "Conn_01x12_Male" V 5000 1500 50  0000 C CNN
F 2 "woose-tracker:Cap-Touch_pattern_compact" H 5050 1500 50  0001 C CNN
F 3 "~" H 5050 1500 50  0001 C CNN
	1    5050 1500
	1    0    0    -1  
$EndComp
Wire Wire Line
	1500 1150 1550 1150
Wire Wire Line
	1500 1250 1550 1250
Wire Wire Line
	1500 1350 1550 1350
Wire Wire Line
	1500 1450 1550 1450
Wire Wire Line
	1500 1550 1550 1550
Wire Wire Line
	1500 1650 1550 1650
Wire Wire Line
	1500 1750 1550 1750
Wire Wire Line
	1500 1850 1550 1850
Wire Wire Line
	1500 1950 1550 1950
Wire Wire Line
	1500 2050 1550 2050
Wire Wire Line
	1500 2150 1550 2150
Wire Wire Line
	1500 2250 1550 2250
Text Label 2500 3050 0    39   ~ 0
GND
Wire Wire Line
	2500 3050 2450 3050
Connection ~ 2450 3050
Wire Wire Line
	1600 3750 1750 3750
Wire Wire Line
	1750 3600 1750 3750
Connection ~ 1750 3750
Wire Wire Line
	1750 3250 1750 3150
Connection ~ 1750 3150
Text Label 2600 1600 0    39   ~ 0
GPIO0
Text Label 2600 1700 0    39   ~ 0
GPIO1
Text Label 2600 1800 0    39   ~ 0
GPIO2
Text Label 2600 1900 0    39   ~ 0
GPIO3
Text Label 2600 2000 0    39   ~ 0
GPIO4
Text Label 2600 2100 0    39   ~ 0
GPIO5
Text Label 2600 2200 0    39   ~ 0
GPIO6
Text Label 2600 2300 0    39   ~ 0
GPIO7
Text Label 3900 1750 2    39   ~ 0
GPIO0
Text Label 3900 1850 2    39   ~ 0
GPIO1
Text Label 3900 1950 2    39   ~ 0
GPIO2
Text Label 3900 2050 2    39   ~ 0
GPIO3
Text Label 3900 2150 2    39   ~ 0
GPIO4
Text Label 3900 2250 2    39   ~ 0
GPIO5
Text Label 3900 2350 2    39   ~ 0
GPIO6
Text Label 3900 2450 2    39   ~ 0
GPIO7
Text Label 3900 1200 2    39   ~ 0
I2C_SDA
Text Label 3900 1100 2    39   ~ 0
I2C_SCL
Text Label 3900 1300 2    39   ~ 0
~Touch_IRQ
Text Label 3900 1600 2    39   ~ 0
~Touch_Reset
Text Label 3900 1500 2    39   ~ 0
GND
Text Label 3900 1400 2    39   ~ 0
VCC
Wire Wire Line
	1600 3150 1750 3150
Text Label 3050 3100 2    39   ~ 0
VCC
Wire Wire Line
	2900 3150 2900 3100
Wire Wire Line
	2900 3100 3150 3100
Wire Wire Line
	3150 3100 3150 3150
Wire Wire Line
	1750 3750 2650 3750
Wire Wire Line
	2600 2500 3400 2500
Wire Wire Line
	3400 2500 3400 3150
Wire Wire Line
	2650 3150 2650 2600
Wire Wire Line
	2650 3650 2650 3750
Connection ~ 2650 3750
Wire Wire Line
	3400 3750 3400 3650
Wire Wire Line
	2650 3750 2900 3750
Wire Wire Line
	3150 3650 3150 3750
Connection ~ 3150 3750
Wire Wire Line
	3150 3750 3400 3750
Wire Wire Line
	2900 3650 2900 3750
Connection ~ 2900 3750
Wire Wire Line
	2900 3750 3150 3750
$Comp
L Connector_Generic:Conn_01x06 J2
U 1 1 5D564CCC
P 4100 1300
F 0 "J2" H 4180 1292 50  0000 L CNN
F 1 "Conn_01x06" H 4180 1201 50  0000 L CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x06_P2.54mm_Vertical_SMD_Pin1Left" H 4100 1300 50  0001 C CNN
F 3 "~" H 4100 1300 50  0001 C CNN
	1    4100 1300
	1    0    0    -1  
$EndComp
$Comp
L Connector_Generic:Conn_01x08 J3
U 1 1 5D564EC2
P 4100 2050
F 0 "J3" H 4180 2042 50  0000 L CNN
F 1 "Conn_01x08" H 4180 1951 50  0000 L CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x08_P2.54mm_Vertical_SMD_Pin1Left" H 4100 2050 50  0001 C CNN
F 3 "~" H 4100 2050 50  0001 C CNN
	1    4100 2050
	1    0    0    -1  
$EndComp
Text Label 5250 1500 0    39   ~ 0
cap0
Text Label 5250 1000 0    39   ~ 0
cap1
Text Label 5250 1400 0    39   ~ 0
cap2
Text Label 5250 1300 0    39   ~ 0
cap3
Text Label 5250 1100 0    39   ~ 0
cap4
Text Label 5250 1200 0    39   ~ 0
cap5
Text Label 5250 2100 0    39   ~ 0
cap6
Text Label 5250 2000 0    39   ~ 0
cap7
Text Label 5250 1900 0    39   ~ 0
cap8
Text Label 5250 1800 0    39   ~ 0
cap9
Text Label 5250 1700 0    39   ~ 0
cap10
Text Label 5250 1600 0    39   ~ 0
cap11
Text Label 1500 1150 2    39   ~ 0
cap0
Text Label 1500 1250 2    39   ~ 0
cap1
Text Label 1500 1350 2    39   ~ 0
cap2
Text Label 1500 1450 2    39   ~ 0
cap3
Text Label 1500 1550 2    39   ~ 0
cap4
Text Label 1500 1650 2    39   ~ 0
cap5
Text Label 1500 1750 2    39   ~ 0
cap6
Text Label 1500 1850 2    39   ~ 0
cap7
Text Label 1500 1950 2    39   ~ 0
cap8
Text Label 1500 2050 2    39   ~ 0
cap9
Text Label 1500 2150 2    39   ~ 0
cap10
Text Label 1500 2250 2    39   ~ 0
cap11
$Comp
L SX8634-Breakout:SX8634 U1
U 1 1 5D2B5070
P 2050 1950
F 0 "U1" H 2075 2941 39  0000 C CNN
F 1 "SX8634" H 2075 2866 39  0000 C CNN
F 2 "Package_DFN_QFN:QFN-32-1EP_5x5mm_P0.5mm_EP3.65x3.65mm" H 2100 2150 39  0001 C CNN
F 3 "" H 2100 2150 39  0001 C CNN
	1    2050 1950
	1    0    0    -1  
$EndComp
$EndSCHEMATC
