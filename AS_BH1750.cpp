/*
 This is a (Arduino) library for the BH1750FVI Digital Light Sensor.
 
 Description:
 http://www.rohm.com/web/global/products/-/product/BH1750FVI
 
 Datasheet:
 http://rohmfs.rohm.com/en/products/databook/datasheet/ic/sensor/light/bh1750fvi-e.pdf
 
 Copyright (c) 2013 Alexander Schulz.  All right reserved.
 
 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.
 
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the GNU
 Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA	 02110-1301	 USA
 */

#include "AS_BH1750.h"

// Debug-Flag
#define BH1750_DEBUG 0

/**
 * Constructor.
 * Allows to change the I2C address of the sensor.
 * Default address: 0x23, alternative address: 0x5C.
 * Constants are defined as: BH1750_DEFAULT_I2CADDR and BH1750_SECOND_I2CADDR.
 * When not specified, the default address is used.
 * To use the alternative address, the sensor pin 'ADR' of the chip must be set to VCC.
 */
AS_BH1750::AS_BH1750(uint8_t address) {
  _address = address;
  _hardwareMode = 255;
}

/**
* Performs the first initialization of the sensor.
 * Possible parameters:
 * - Sensor resolution mode:
 * - RESOLUTION_LOW: Physical sensor mode with 4 lx resolution. Measurement time approx. 16ms. Range 0-54612.
 * - RESOLUTION_NORMAL: Physical sensor mode with 1 lx resolution. Measurement time approx. 120ms. Range 0-54612.
 * - RESOLUTION_HIGH: Physical sensor mode with 0.5 lx resolution. Measurement time approx. 120ms. Range 0-54612.
 * (The measuring ranges can be moved by changing the MTreg.)
 * - RESOLUTION_AUTO_HIGH: The values in the MTreg are automatically adjusted according to the brightness,
 * that a maximum resolution and measuring range are achieved.
 * The measurable values range from 0.11 lx to 100,000 lx.
 * (I do not know how accurate the values are in border areas,
 * especially with high values I have my doubts.
 * However, the values seem to grow largely linearly with the increasing brightness.)
 * Resolution in the lower range approximately 0.13 lx, in the middle 0.5 lx, in the upper approximately 1-2 lx.
 * The measurement times are extended by multiple measurements and measurements
 * the changes from Measurement Time (MTreg) to max. approx. 500 ms.
 *
 * - AutoPowerDown: true = The sensor is placed in the power saving mode after the measurement.
 * The subsequent wake-up is performed automatically, but takes slightly more time.
 *
 * Default values: RESOLUTION_AUTO_HIGH, true
 *
 */
bool AS_BH1750::begin(sensors_resolution_t mode, bool autoPowerDown) {
#if BH1750_DEBUG == 1
  Serial.print("  sensors_resolution_mode (virtual): ");
  Serial.println(mode, DEC);
#endif
  _virtualMode = mode;
  _autoPowerDown = autoPowerDown;
  
  Wire.begin();

  defineMTReg(BH1750_MTREG_DEFAULT); // actually normally unnecessary since standard

  // Determine the hardware mode for the desired Virtual Mode
  switch (_virtualMode) {
  case RESOLUTION_LOW:
    _hardwareMode = autoPowerDown?BH1750_ONE_TIME_LOW_RES_MODE:BH1750_CONTINUOUS_LOW_RES_MODE;
    break;
  case RESOLUTION_NORMAL:
    _hardwareMode = autoPowerDown?BH1750_ONE_TIME_HIGH_RES_MODE:BH1750_CONTINUOUS_HIGH_RES_MODE;
    break;
  case RESOLUTION_HIGH:
    _hardwareMode = autoPowerDown?BH1750_ONE_TIME_HIGH_RES_MODE_2:BH1750_CONTINUOUS_HIGH_RES_MODE_2;
    break;
  case RESOLUTION_AUTO_HIGH:
    _hardwareMode = BH1750_CONTINUOUS_LOW_RES_MODE;
    break;
  default:
    // must not actually happen...
    _hardwareMode = 255;
    break;
  }

#if BH1750_DEBUG == 1
  Serial.print("hardware mode: ");
  Serial.println(_hardwareMode, DEC);
#endif

  if(_hardwareMode==255) {
    return false;
  }

  // Try to activate the selected hardware mode
  if(selectResolutionMode(_hardwareMode)){
#if BH1750_DEBUG == 1
    Serial.print("hardware mode defined successfully");
    Serial.println(_hardwareMode, DEC);
#endif
    return true;
  }

  // Initialization failed
#if BH1750_DEBUG == 1
  Serial.print("failure to aktivate hardware mode ");
  Serial.println(_hardwareMode, DEC);
#endif
  _hardwareMode = 255;
  return false;
}

/**
 * Allow a check to see if a (responsive) BH1750 sensor is present.
 */
bool AS_BH1750::isPresent() {
  // Check I2C Address
  Wire.beginTransmission(_address);
  if(Wire.endTransmission()!=0) {
    return false; 
  }

  // Check device: is it a BH1750
  if(!isInitialized()) {
    // previously inactive, therefore, to test fast one-time mode
    //write8(BH1750_POWER_ON);
    selectResolutionMode(BH1750_ONE_TIME_LOW_RES_MODE);
    _hardwareMode=255;
  } 
  else {
    // if once-mode was active, the sensor must be awakened
    powerOn(); 
  }

  // Check whether values are actually delivered (last mode, auto-PowerDown will be executed)
  return (readLightLevel()>=0);
}

/**
 * Awakens a sensor in power down mode (does not damage the 'wake up' sensor).
 * Works only if the sensor has already been initialized.
 */
void AS_BH1750::powerOn() {
  if(!isInitialized()) {
#if BH1750_DEBUG == 1
    Serial.println("sensor not initialized");
#endif
    return;
  }

  _valueReaded=false;
  //write8(BH1750_POWER_ON); //
  //fDelayPtr(10); // Nötig?
  // Apparently the setting of HardwareMode sufficient also without PowerON command
  selectResolutionMode(_hardwareMode); // activate the last mode
}

/**
 * Sends the sensor to power saving mode.
 * Only works if the sensor has already been initialized.
 */
void AS_BH1750::powerDown() {
  if(!isInitialized()) {
#if BH1750_DEBUG == 1
    Serial.println("sensor not initialized");
#endif
    return;
  }

  write8(BH1750_POWER_DOWN);
}

/**
 * Sends to the sensor a command to select HardwareMode.
 *
 * Parameters:
 * - mode: s.o.
 * - DelayFuncPtr: delay(n) Ability to add your own delay function (e.g., to use sleep mode).
 *
 * Default values: delay()
 */
bool AS_BH1750::selectResolutionMode(uint8_t mode, DelayFuncPtr fDelayPtr) {
#if BH1750_DEBUG == 1
    Serial.print("selectResolutionMode: ");
    Serial.println(mode, DEC);
#endif
  if(!isInitialized()) {
    return false;
#if BH1750_DEBUG == 1
    Serial.println("sensor not initialized");
#endif
  }

  _hardwareMode=mode;
  _valueReaded=false;

  // Check whether a valid mode is present and, in the positive case, activate the desired mode
  switch (mode) {
  case BH1750_CONTINUOUS_HIGH_RES_MODE:
  case BH1750_CONTINUOUS_HIGH_RES_MODE_2:
  case BH1750_CONTINUOUS_LOW_RES_MODE:
  case BH1750_ONE_TIME_HIGH_RES_MODE:
  case BH1750_ONE_TIME_HIGH_RES_MODE_2:
  case BH1750_ONE_TIME_LOW_RES_MODE:
    // Mode
    if(write8(mode)) {
    // Short pause necessary, otherwise the mode is not activated safely
    // (e.g., in the AutoHigh mode, sensor provides alternately over-steered values, such as: 54612.5, 68123.4, 54612.5, 69345.3, ..)
    fDelayPtr(5);
      return true;
    }
    break;
  default:
    // Invalid measurement mode
#if BH1750_DEBUG == 1
    Serial.println("Invalid measurement mode");
#endif
    break;
  }

  return false;
}

/**
 * Returns current measured value for brightness in lux (lx).
 * If the sensor is in power saving mode, it is automatically awakened.
 *
 * If the sensor has not (yet) been initialized (begin), the value -1 is supplied.
 */
float AS_BH1750::readLightLevel(DelayFuncPtr fDelayPtr) {
#if BH1750_DEBUG == 1
    Serial.print("call: readLightLevel. virtualMode: ");
    Serial.println(_virtualMode, DEC);
#endif

  if(!isInitialized()) {
#if BH1750_DEBUG == 1
    Serial.println("sensor not initialized");
#endif
    return -1;
  }

  // ggf. PowerOn  
  if(_autoPowerDown && _valueReaded){
    powerOn();
  }

  // Automatic mode requires special treatment.
  // First, the brightness is read in the LowRes mode,
  // depending on the range (dark, normal, very bright), the values of MTreg are set and
  // the actual measurement is then carried out.
  /*
     The fixed limits may cause a 'jump' in the measurement curve.
   In this case, a continuous adaptation of MTreg in border areas would probably be better.
   For my purposes, however, this is of no importance.
   */
  if(_virtualMode==RESOLUTION_AUTO_HIGH) {
    defineMTReg(BH1750_MTREG_DEFAULT);
    selectResolutionMode(BH1750_CONTINUOUS_LOW_RES_MODE, fDelayPtr);
    fDelayPtr(16); // Reading time in LowResMode
    uint16_t level = readRawLevel();
#if BH1750_DEBUG == 1
    Serial.print("AutoHighMode: check level read: ");
    Serial.println(level, DEC);
#endif
    if(level<10) {
#if BH1750_DEBUG == 1
    Serial.println("level 0: dark");
#endif    
      // Dark, sensitivity to maximum.
      // The value is random. From about 16000 this approach would be possible.
      // I need this accuracy but only in the dark areas (to see when really 'dark').
      defineMTReg(BH1750_MTREG_MAX);
      selectResolutionMode(_autoPowerDown?BH1750_ONE_TIME_HIGH_RES_MODE_2:BH1750_CONTINUOUS_HIGH_RES_MODE_2, fDelayPtr);
      fDelayPtr(120*3.68); // TODO: Check the value
      //fDelayPtr(122);
    }
    else if(level<32767) {
#if BH1750_DEBUG == 1
    Serial.println("level 1: normal");
#endif    
      // Up to this point, the 0.5 lx mode is enough. Normal sensitivity.
      defineMTReg(BH1750_MTREG_DEFAULT);
      selectResolutionMode(_autoPowerDown?BH1750_ONE_TIME_HIGH_RES_MODE_2:BH1750_CONTINUOUS_HIGH_RES_MODE_2, fDelayPtr);
      fDelayPtr(120); // TODO: Check the value
    } 
    else if(level<60000) {
#if BH1750_DEBUG == 1
    Serial.println("level 2: bright");
#endif    
      // high range, 1 lx mode, normal sensitivity. The value of 60000 is more or less random, it simply needs to be a high value, close to the limit.
      defineMTReg(BH1750_MTREG_DEFAULT);
      selectResolutionMode(_autoPowerDown?BH1750_ONE_TIME_HIGH_RES_MODE:BH1750_CONTINUOUS_HIGH_RES_MODE, fDelayPtr);
      fDelayPtr(120); // TODO: Check the value
    }
    else {
#if BH1750_DEBUG == 1
    Serial.println("level 3: very bright");
#endif    
      // very high range, reduce sensitivity
      defineMTReg(32); // Min+1, at the minimum from Doku the sensor (at least my) is crazy: The values are about 1/10 of the expected.
      selectResolutionMode(_autoPowerDown?BH1750_ONE_TIME_HIGH_RES_MODE:BH1750_CONTINUOUS_HIGH_RES_MODE, fDelayPtr);
      fDelayPtr(120); // TODO: Check the value
    }
  } 

  // Hardware read value and convert to Lux.
  uint16_t raw = readRawLevel();
  if(raw==65535) {
    // Value suspiciously high. Check sensor.
    // Check I2C Adresse
    Wire.beginTransmission(_address);
    if(Wire.endTransmission()!=0) {
      return -1; 
    }
  }
  return convertRawValue(raw); 
}

/**
 * Read the raw value of the brightness.
 * Range of values 0-65535.
 */
uint16_t AS_BH1750::readRawLevel(void) {
  uint16_t level;
  Wire.beginTransmission(_address);
  Wire.requestFrom(_address, 2);
#if (ARDUINO >= 100)
  level = Wire.read();
  level <<= 8;
  level |= Wire.read();
#else
  level = Wire.receive();
  level <<= 8;
  level |= Wire.receive();
#endif
  if(Wire.endTransmission()!=0) {
#if BH1750_DEBUG == 1
    Serial.println("I2C read error");
#endif
    return 65535; // Error marker
  }

#if BH1750_DEBUG == 1
  Serial.print("Raw light level: ");
  Serial.println(level);
#endif

  _valueReaded=true;

  return level;
}

/**
 * Convert raw values to lux.
 */
float AS_BH1750::convertRawValue(uint16_t raw) {
  // Conversion
  float flevel = raw/1.2;

#if BH1750_DEBUG == 1
  Serial.print("convert light level (1): ");
  Serial.println(flevel);
#endif

  // Calculate the MTreg influence
  if(_MTreg!=BH1750_MTREG_DEFAULT) { // at 69, the factor is 1
    flevel = flevel * _MTregFactor;
#if BH1750_DEBUG == 1
    Serial.print("convert light level (2): ");
    Serial.println(flevel);
#endif
  }

  // depending on the mode a further conversion is necessary
  switch (_hardwareMode) {
  case BH1750_CONTINUOUS_HIGH_RES_MODE_2:
  case BH1750_ONE_TIME_HIGH_RES_MODE_2:
    flevel = flevel/2;
#if BH1750_DEBUG == 1
    Serial.print("convert light level (3): ");
    Serial.println(flevel);
#endif
    break;
  default:
    // nothing to do
    break;
  }

#if BH1750_DEBUG == 1
  Serial.print("Light level: ");
  Serial.println(flevel);
#endif

  return flevel;
}

/**
 * MTreg. Defines sensor sensitivity.
 * Min.value (BH1750_MTREG_MIN) = 31 (Sensitivity: default * 0.45)
 * Max.value (BH1750_MTREG_MAX) = 254 (Sensitivity: default * 3.68)
 * Default (BH1750_MTREG_DEFAULT) = 69.
 * The sensitivity changes the reading time (higher sensitivity means longer period of time).
 */
void AS_BH1750::defineMTReg(uint8_t val) {
  if(val<BH1750_MTREG_MIN) {
    val = BH1750_MTREG_MIN;
  }
  if(val>BH1750_MTREG_MAX) {
    val = BH1750_MTREG_MAX;
  }
  if(val!=_MTreg) {
    _MTreg = val;
    _MTregFactor = (float)BH1750_MTREG_DEFAULT / _MTreg;

    // Change Measurement time
    // Transmission in two steps: 3 bits and 5 bits, each with a prefix.
    //   High bit: 01000_MT[7,6,5]
    //   Low bit:  011_MT[4,3,2,1,0]
    uint8_t hiByte = val>>5;
    hiByte |= 0b01000000;
#if BH1750_DEBUG == 1
    Serial.print("MGTreg high byte: ");
    Serial.println(hiByte, BIN);
#endif
    write8(hiByte);
    //fDelayPtr(10);
    // Pause necessary?
    uint8_t loByte = val&0b00011111;
    loByte |= 0b01100000;
#if BH1750_DEBUG == 1
    Serial.print("MGTreg low byte: ");
    Serial.println(loByte, BIN);
#endif
    write8(hiByte);
    //fDelayPtr(10);
  }
}

/**
 * Indicates whether the sensor is initialized.
 */
bool AS_BH1750::isInitialized() {
  return _hardwareMode!=255; 
}

/**
 * Write one byte to I2C bus (to the address of the sensor).
 */
bool AS_BH1750::write8(uint8_t d) {
  Wire.beginTransmission(_address);
#if (ARDUINO >= 100)
  Wire.write(d);
#else
  Wire.send(d);
#endif
  return (Wire.endTransmission()==0);
}


