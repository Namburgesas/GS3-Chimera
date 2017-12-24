//*********************************************************************
// EEPROM management fuctions
//
// Parameter data structure
// Address	Data				Length
//  0		FLBThreshold		byte
//	1		debounceCount		byte
//	2		pumpMinPWM			byte
//	3		pumpMaxPWM			byte
//	4		mlPerFlowMeterPulse
//	8		unionThreshold
//	12		Kpp					double (4 bytes)
//	16		Kpi					double (4 bytes)
//	20		Kpd					double (4 bytes)
//	24		Kfp					double (4 bytes)
//	28		Kfi					double (4 bytes)
//	32		Kfd					double (4 bytes)
//	36		layerPIFlowRate		byte
//	37		slayerMainPWM		byte
//	38		slayerPIPeriod		byte
//
// 	Profile data structure
//	97		Size of PWMprofile	byte
//	98		Size of Pressure p.	byte
//	99		Size of Flow prof.	byte
//	100-301	PWM Profile			bytes (2 per second)
//	302-503	Pressure Profile	bytes (2 per second)
//	504-705	Flow Profile		bytes (2 per second)
//
//  Assaf 
//  39      slayerMaxPWM		byte
//
//
//*********************************************************************

// Store PWM, pressure and flow Profiles in EEPROM
void storeProfilesinEEPROM()
{
  EEPROM.update(97, (byte)sizeof(g_PWMProfile)); // Always 201
  EEPROM.update(98, (byte)sizeof(g_pressureProfile)); //Always 201
  EEPROM.update(99, (byte)sizeof(g_flowProfile)); // Always 201

  int shiftAddr = 100; // Start writing arrays  at 10th byte
  int shiftPP = shiftAddr + sizeof(g_PWMProfile);
  int shiftFP = shiftPP + sizeof(g_pressureProfile);

  // Write PWM Profile 
  for (int addr = 0; addr < sizeof(g_PWMProfile); addr++)
  {
    EEPROM.update(addr + shiftAddr, g_PWMProfile[addr]);
    EEPROM.update(addr + shiftPP, g_pressureProfile[addr]);
    EEPROM.update(addr + shiftFP, g_flowProfile[addr]);
  }
}

// Retrieve PWM, pressure and flow Profiles from EEPROM
void readProfilesfromEEPROM()
{
  int PWMProfileSize = EEPROM.read(97);
  int pressureProfileSize = EEPROM.read(98);
  int flowProfileSize = EEPROM.read(99);

  int shiftAddr = 100; // Start reading arrays at 10th byte (should match similar decleration at storeProfilesinEEPROM()
  int shiftPP = shiftAddr + PWMProfileSize;
  int shiftFP = shiftPP + pressureProfileSize;
  
  for (int addr = 0; addr < PWMProfileSize; addr++) 
  {
    g_PWMProfile[addr] = EEPROM.read(addr + shiftAddr);
    g_pressureProfile[addr] = EEPROM.read(addr + shiftPP);
    g_flowProfile[addr] = EEPROM.read(addr + shiftFP);
  } 
}

// Retrieve software parameters from EEPROM
void readSWParametersfromEEPROM()
{
  FLBThreshold = EEPROM.read(0);
  debounceCount = EEPROM.read(1);
  pumpMinPWM = EEPROM.read(2);
  pumpMaxPWM = EEPROM.read(3);
  EEPROM.get(4, mlPerFlowMeterPulse);
  EEPROM.get(8, unionThreshold);
  EEPROM.get(12, Kpp);
  EEPROM.get(16, Kpi);
  EEPROM.get(20, Kpd);
  EEPROM.get(24, Kfp);
  EEPROM.get(28, Kfi);
  EEPROM.get(32, Kfd);
} 

// Update software parameters in EEPROM
void writeSWParameterstoEEPROM()
{
  EEPROM.update(0, FLBThreshold);
  EEPROM.update(1, debounceCount);
  EEPROM.update(2, pumpMinPWM);
  EEPROM.update(3, pumpMaxPWM);
  EEPROM.put(4, (float)mlPerFlowMeterPulse);
  EEPROM.put(8, (float)unionThreshold);
  EEPROM.put(12, Kpp);
  EEPROM.put(16, Kpi);
  EEPROM.put(20, Kpd);
  EEPROM.put(24, Kfp);
  EEPROM.put(28, Kfi);
  EEPROM.put(32, Kfd);
} 

// Update Slayer parameters to EEPROM

void readSlayerParametersfromEEPROM()
{
  slayerPIFlowRate = EEPROM.read(36);
  slayerMainPWM = EEPROM.read(37);
  slayerPIPeriod = EEPROM.read(38);
} 

void writeSlayerParameterstoEEPROM()
{
  EEPROM.update(36, slayerPIFlowRate);
  EEPROM.update(37, slayerMainPWM);
  EEPROM.update(38, slayerPIPeriod);
  EEPROM.update(39, slayerMaxPWM);
} 