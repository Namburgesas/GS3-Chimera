//***********************************************************************************
// Calculate and return pump speed per pull mode
//***********************************************************************************

unsigned setPumpPWMbyMode(unsigned profileIndex, unsigned long pullTimer, unsigned pumpPWM, float currentPressure, boolean preInfusion, unsigned sumFlowProfile)
{
	unsigned currentPotValue = analogRead(CONTROL_POT);
	double PIDSetValue;
	switch(g_pullMode)
	{
		case MANUAL_PULL:	
#ifdef OTTO_HTWF 
			// For Otto Controls HTWF-1A12A22A Hall Effect 0-5V paddle control
			if (currentPotValue < 520)
				pumpPWM = constrain(map(currentPotValue, 100, 520, 0, FLBThresholdPWM), pumpMinPWM, FLBThresholdPWM);
			else
				pumpPWM = constrain(map(currentPotValue, 520, 930, FLBThresholdPWM, pumpMaxPWM), FLBThresholdPWM, pumpMaxPWM);
#else
			pumpPWM = (unsigned)((float)currentPotValue * pumpMaxPWM / 1024); 
#endif
			break;

		case AUTO_PWM_PROFILE_PULL:
			pumpPWM = (unsigned) 400 / 255 * ((g_PWMProfile[profileIndex]+ (g_PWMProfile[profileIndex+1] 
						- g_PWMProfile[profileIndex])*((double)(pullTimer % 500)/500.0)));
			break;

		case AUTO_PRESSURE_PROFILE_PULL:
			PIDSetValue = (double)g_pressureProfile[profileIndex] * 12.0 / 100.0 + (double)(g_pressureProfile[profileIndex + 1] -
				g_pressureProfile[profileIndex]) * 12.0 / 100.0 * ((double)(pullTimer % 500) /500.0) ; 
			pumpPWM = executePID_P(PIDSetValue, pumpPWM, currentPressure);
			break;

		case AUTO_FLOW_PROFILE_PULL:
			PIDSetValue = (double)sumFlowProfile + (double)(g_flowProfile[profileIndex + 1] >> 1 -
				g_flowProfile[profileIndex] >> 1) * ((double)(pullTimer % 500) /500.0) ;
			pumpPWM = executePID_F(PIDSetValue, pumpPWM, (double)g_flowPulseCount);
			break;

		case SLAYER_LIKE_PULL:
			pumpPWM = slayerMainPWM; 
			break;	

		case SLAYER_LIKE_PI_PRESSURE_PROFILE:	
			PIDSetValue = (double)currentPotValue / 90.0;    // 0-~11Bar
			pumpPWM = executePID_P(PIDSetValue, pumpPWM, currentPressure); 
			if (pullTimer < 1000 || currentPressure < unionThreshold)                                                                         // override during PI
				pumpPWM = slayerMainPWM;
			break;
			
		case FLOW_PRESSURE_PROFILE:
			// Centers the "detente" For Otto Controls HTWF-1A12A22A Hall Effect 0-5V paddle control
			if (preInfusion)
			{
				PIDSetValue = constrain(map(currentPotValue, 100, 520, PID_MIN_FLOW, PID_MAX_FLOW), PID_MIN_FLOW, PID_MAX_FLOW);
				pumpPWM = executePID_F(PIDSetValue, pumpPWM, flowRate(preInfusion)); // Closed loop PID based
				//pumpPWM = constrain(map(currentPotValue, 100, 520, 0, 220), 0, 220); // Open loop control 
			}
			else
			{
				if (currentPotValue > 520)
					PIDSetValue = (double)(((currentPotValue - 520.0) / 410.0) * (PID_MAX_PRESSURE - PID_MIN_PRESSURE) + PID_MIN_PRESSURE); //4.0 - ~10bar: 
				else 
					PIDSetValue = PID_MIN_PRESSURE;
				pumpPWM = executePID_P(PIDSetValue, pumpPWM, currentPressure);
			}
			break;
	}
	// Action Time - operate pump & operate FLB solenoid
	md.setM1Speed(constrain(pumpPWM, pumpMinPWM, pumpMaxPWM));
	return pumpPWM;
}



//*******************************************************************************
// Calculate FLB cutoff for each pull mode and activate/deactivate solenoid
//*******************************************************************************

boolean setFlowLimitBypass(unsigned pumpPWM, int profileIndex, boolean preInfusion, float currentPressure)
{
	if (preInfusion)
	{
		if (profileIndex % 2 == 0) //flash the LED
			ledColor('g');
		else
			ledColor('x');

		switch(g_pullMode)    //let's check if PI should continue....
		{
			case MANUAL_PULL:
			case AUTO_PWM_PROFILE_PULL:
			case AUTO_PRESSURE_PROFILE_PULL:
			case AUTO_FLOW_PROFILE_PULL:
				if (pumpPWM > FLBThresholdPWM + 1)
					preInfusion = false;
				break;

			case SLAYER_LIKE_PULL:
				if (profileIndex > slayerPIPeriod << 1)
					preInfusion = false;
				break;	

			case SLAYER_LIKE_PI_PRESSURE_PROFILE:
				if(profileIndex > 2 && currentPressure > unionThreshold)
					preInfusion = false;
				break;
				
			case FLOW_PRESSURE_PROFILE:
				if(profileIndex > 2 && currentPressure > unionThreshold)
					preInfusion = false;
				break;
		}
	}
	if (!preInfusion) 
		ledColor('g');
	setFLB(!preInfusion);
	return preInfusion;	
}

void initFlowLimitBypass()
{
	pinMode(FLOW_LIMIT_BYPASS, OUTPUT);
	setFLB(false);
}

void flushCycle()	
{
	Serial.println("Flush activated...");
	detachInterrupt(digitalPinToInterrupt(GROUP_SOLENOID));
	g_newPull = false;
	unsigned long flushTimer = millis();
	unsigned long flushPauseTimer = flushTimer;
	while ((millis() - flushPauseTimer) < 6000 && !Serial2.available()) //Pause is about 6 seconds...
	{
		while (digitalRead(GROUP_SOLENOID) == LOW && !Serial2.available())
		{
			setFLB(true); // turn on FLB
			md.setM1Speed(constrain(flushPWM, pumpMinPWM, pumpMaxPWM));
			flushPauseTimer = millis();
			if (((millis() - flushTimer))/250 % 2 == 0)
				ledColor('g');
			else
				ledColor('y');
		}
		md.setM1Speed(0); //Shut down pump motor
		setFLB(false);
		if (((millis() - flushTimer))/250 % 2 == 0)
			ledColor('r');
		else
			ledColor('y');
	}
	resetSystem();
}

void setFLB (boolean enable)
{
#ifdef INVERT_FLB_OUTPUT 
			digitalWrite(FLOW_LIMIT_BYPASS, !enable); // switch FLB solenoid
#else
			digitalWrite(FLOW_LIMIT_BYPASS, enable); 
#endif
}

void ledColor (char color)
{
	if (color == 'r')
	{
		digitalWrite(GREEN_LED, LOW);
		digitalWrite(RED_LED, HIGH);
	}
	else if (color == 'g')
	{
		digitalWrite(RED_LED, LOW);
		digitalWrite(GREEN_LED, HIGH);
	}
	else if (color == 'y')
	{
		digitalWrite(RED_LED, HIGH);
		digitalWrite(GREEN_LED, HIGH);
	}
	else
	{
		digitalWrite(RED_LED, LOW);
		digitalWrite(GREEN_LED, LOW);
	}
}