a (Arduino) library for the BH1750FVI Digital Light Sensor.

( http://s6z.de/joomla3/index.php/arduino/sensoren/15-umgenungslichtsensor-bh1750 )

Features:

- Supports both possible I2C addresses of the sensor: standard address: 0x23, alternative address: 0x5C. There are corresponding constants defined: BH1750_DEFAULT_I2CADDR and BH1750_SECOND_I2CADDR.

- All hardware resolution modes:

	RESOLUTION_LOW: Physical sensor mode with 4 lx resolution. Measurement time approx. 16ms. Range 0-54612.
	RESOLUTION_NORMAL: Physical sensor mode with 1 lx resolution. Measurement time approx. 120ms. Range 0-54612.
	RESOLUTION_HIGH: Physical sensor mode with 0.5 lx resolution. Measurement time approx. 120ms. Range 0-54612.

- Virtual Mode:

	RESOLUTION_AUTO_HIGH: Depending on the brightness, the values in the MTreg ('Measurement Time' register) are automatically adjusted in such a way that a maximum resolution and measuring range are achieved. The measurable values start from 0.11 lx and go to over 100000 lx. (I do not know how exactly the values are in the boundary regions, especially in the case of high values, I have my doubts, but the values seem to grow largely linearly with the increasing brightness.) Resolution in the lower range approx 0.5 lx, in the upper about 1-2 lx. The measurement times are extended by multiple measurements and the changes from Measurement Time (MTreg) to max. approx. 500 ms.
Method for changing 'Measurement Time' registers. This can affect sensitivity.

- Auto power down: The sensor is placed in the power saving mode after the measurement. The subsequent wake-up is possibly carried out automatically, but takes a little more time.

Default values: Mode = RESOLUTION_AUTO_HIGH, AutoPowerDown = true
