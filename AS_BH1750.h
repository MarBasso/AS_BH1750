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

#ifndef AS_BH1750_h
#define AS_BH1750_h

#if (ARDUINO >= 100)
#include <Arduino.h>
#else
#include <WProgram.h>
#endif
#include "Wire.h"

// Possible I2C addresses
#define BH1750_DEFAULT_I2CADDR 0x23
#define BH1750_SECOND_I2CADDR 0x5C

// MTreg values
// Default
#define BH1750_MTREG_DEFAULT 69
// Sensitivity : default = 0.45
#define BH1750_MTREG_MIN 31
// Sensitivity : default = 3.68
#define BH1750_MTREG_MAX 254

// Hardware Modes
// No active state
#define BH1750_POWER_DOWN 0x00

// Wating for measurment command
#define BH1750_POWER_ON 0x01

// Reset data register value - not accepted in POWER_DOWN mode
#define BH1750_RESET 0x07

// Start measurement at 1lx resolution. Measurement time is approx 120ms.
#define BH1750_CONTINUOUS_HIGH_RES_MODE  0x10

// Start measurement at 0.5lx resolution. Measurement time is approx 120ms.
#define BH1750_CONTINUOUS_HIGH_RES_MODE_2  0x11

// Start measurement at 4lx resolution. Measurement time is approx 16ms.
#define BH1750_CONTINUOUS_LOW_RES_MODE  0x13

// Start measurement at 1lx resolution. Measurement time is approx 120ms.
// Device is automatically set to Power Down after measurement.
#define BH1750_ONE_TIME_HIGH_RES_MODE  0x20

// Start measurement at 0.5lx resolution. Measurement time is approx 120ms.
// Device is automatically set to Power Down after measurement.
#define BH1750_ONE_TIME_HIGH_RES_MODE_2  0x21

// Start measurement at 1lx resolution. Measurement time is approx 120ms.
// Device is automatically set to Power Down after measurement.
#define BH1750_ONE_TIME_LOW_RES_MODE  0x23

/** Virtual Modes */
typedef enum
{
  RESOLUTION_LOW         = (1), /** 4lx resolution. Measurement time is approx 16ms. */  
  RESOLUTION_NORMAL      = (2), /** 1lx resolution. Measurement time is approx 120ms. */
  RESOLUTION_HIGH        = (3), /** 0,5lx resolution. Measurement time is approx 120ms. */
  RESOLUTION_AUTO_HIGH   = (99) /** 0,11-1lx resolution. Measurement time is above 250ms. */
  }  
  sensors_resolution_t;

typedef void (*DelayFuncPtr)(unsigned long);
typedef unsigned long (*TimeFuncPtr)(void);

/**
 * BH1750 driver class.
 */
class AS_BH1750 {
public:
  /**
   * Constructor.
   * Allows to change the I2C address of the sensor.
   * Default address: 0x23, alternative address: 0x5C.
   * Constants are defined as: BH1750_DEFAULT_I2CADDR and BH1750_SECOND_I2CADDR.
   * When not specified, the default address is used.
   * To use the alternative address, the sensor pin 'ADR' of the chip must be set to VCC.
   */
  AS_BH1750(uint8_t address = BH1750_DEFAULT_I2CADDR);

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
   * Default values: RESOLUTION_AUTO_HIGH, true, delay()
   *
   */
  bool begin(sensors_resolution_t mode = RESOLUTION_AUTO_HIGH, bool autoPowerDown = true);

  /**
   * Allow a check to see if a (responsive) BH1750 sensor is present.
   */
  bool isPresent(void);

  /**
   * Returns current measured value for brightness in lux (lx).
   * If the sensor is in low-power mode, it is automatically awakened.
   *
   * If the sensor has not (yet) been initialized (begin), the value -1 is supplied.
   *
   * Possible parameters:
   *
   * - DelayFuncPtr: delay (n) Ability to add your own delay function (e.g., to use sleep mode).
   *
   * Default values: delay ()
   *
   */
  float readLightLevel(DelayFuncPtr fDelayPtr = &delay);

  /**
   * Sends the sensor to power saving mode.
   * Only works if the sensor has already been initialized.
   */
  void powerDown(void);

private:
  int _address;
  uint8_t _hardwareMode;

  uint8_t _MTreg;
  float _MTregFactor;

  sensors_resolution_t _virtualMode;
  bool _autoPowerDown;

  bool _valueReaded;

  bool selectResolutionMode(uint8_t mode, DelayFuncPtr fDelayPtr = &delay);
  void defineMTReg(uint8_t val);
  void powerOn(void);
  void reset(void);
  uint16_t readRawLevel(void);
  float convertRawValue(uint16_t raw);
  bool isInitialized();
  bool write8(uint8_t data);
};

#endif


