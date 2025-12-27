/*
This project assumes I2C for communicating with the BNO08X.
SPI is untested because frankly, I can't be bothered breaking what is working.
It might be faster though?

My setup uses an Arduino Pro Micro and a transistor-based level shifter.
I've tried a TXS0108E but it either arrived dead or wasn't fit for my purpose.
You *should* level shift unless your board has 3.3V logic.

Performance-wise, this project gets a ~375 Hz update rate; ~360 without FAST_MATH.
The BNO08X is capable of 1000 Hz as set up here, so grab the fastest AVR MCU you can.
Unsure if the deciding factor is FLOPS, IOPS or raw clock speed,
but anything generally more powerful than an ATmega32U4 is gonna be good.

Tested with OpenTrack in numerous games.
There is a bug in OpenTrack [1] causing jitter.

[1] OpenTrack jitter bug discussion
https://github.com/opentrack/opentrack/discussions/2100

[2] CEVA, BNO08X Datasheet
https://www.ceva-ip.com/wp-content/uploads/BNO080_085-Datasheet.pdf
*/

// Settings

const unsigned long I2C_RATE = 400000;
const unsigned long SERIAL_BAUD = 115200;
// Probably should be kept unchanged
const uint16_t IMU_INTERVAL = 1; // ms

// Default address is 0x4B, but it could be set to 0x4A with a solder blob
const uint8_t IMU_ADDRESS = 0x4B;
// const uint8_t IMU_ADDRESS = 0x4A;

// Set to -1 to disable interrupts
const uint8_t PIN_INT = 10;

// Enable FastTrig
// FastTrig trades accuracy for speed
// If you want the extra (marginal) accuracy, comment the next line
#define FAST_MATH

// // // // // // // //

#include <avr/power.h>
#include <Wire.h>

#ifdef FAST_MATH
#include <FastTrig.h>                         // Click here to get the library: http://librarymanager/All#FastTrig
#endif
#include "SparkFun_BNO080_Arduino_Library.h"  // Click here to get the library: http://librarymanager/All#SparkFun_BNO080

// // // // // // // //

typedef struct {
  float Gyro[3];
  float Position[3];
} hatire_attitude_t;

typedef union {
  hatire_attitude_t Attitude;
  char Message[24];
} hatire_container_t;

typedef struct {
  int16_t Begin = 0xAAAA;
  uint16_t Capture = 0;
  hatire_container_t Data;
  int16_t End = 0x5555;
} hatire_t;

// // // // // // // //

const char Version[] = "BNO086 Hatire 1.2";

BNO080 imu;

void hatire_message(uint16_t code, const char Message[24], bool EOL)
{
  hatire_t hatire;
  hatire.Capture = code;
  memset(hatire.Data.Message, 0x00, sizeof(hatire.Data));
  strcpy(hatire.Data.Message, Message);
  if(EOL)
    hatire.Data.Message[23] = 0x0A;

  Serial.write((byte*)&hatire, sizeof(hatire));
}


void setup()
{
  // Turn off everything we don't need
  // Something about ADC
  ADCSRA &= ~bit(ADEN);
  DIDR0 = 0x3F;

  power_spi_disable(); 
  power_adc_disable();
  power_timer1_disable();
  power_timer2_disable();

  Serial.begin(SERIAL_BAUD);

  Wire.begin();
  Wire.setClock(I2C_RATE);
  imu.begin(IMU_ADDRESS, Wire, PIN_INT);

  imu.enableGyroIntegratedRotationVector(IMU_INTERVAL);

  while(!Serial) delay(100);

  hatire_message(2000, Version, true);
  hatire_message(5000, "HAT BEGIN", true);
}

// Quake III Fast Inverse Square Root
float Q_rsqrt(float number)
{
	long i;
	float x2, y;
	const float threehalfs = 1.5F; 

	x2 = number * 0.5F;
	y  = number;
	i  = * (long*) &y;                       // evil floating point bit level hacking
	i  = 0x5f3759df - ( i >> 1 );            // what the fuck?
	y  = * (float*) &i;
	y  = y * (threehalfs - (x2 * y * y));    // 1st iteration

	return y;
}

// This function is assembled and modified out of SparkFun's getYaw, getPitch and getRoll
void getEuler(float &yaw, float &pitch, float &roll)
{
  float dqw = imu.getQuatReal();
	float dqx = imu.getQuatI();
	float dqy = imu.getQuatJ();
	float dqz = imu.getQuatK();

	float norm = Q_rsqrt(dqw*dqw + dqx*dqx + dqy*dqy + dqz*dqz);
	dqx *= norm;
	dqy *= norm;
	dqw *= norm;
	dqz *= norm;

	float ysqr = dqy * dqy;

	// yaw (z-axis rotation)
	float t3 = +2.0 * (dqw * dqz + dqx * dqy);
	float t4 = +1.0 - 2.0 * (ysqr + dqz * dqz);

	// pitch (y-axis rotation)
	float t2 = +2.0 * (dqw * dqy - dqz * dqx);
	t2 = t2 > 1.0 ? 1.0 : t2;
	t2 = t2 < -1.0 ? -1.0 : t2;

	// roll (x-axis rotation)
	float t0 = +2.0 * (dqw * dqx + dqy * dqz);
	float t1 = +1.0 - 2.0 * (dqx * dqx + ysqr);

  // use FastTrig functions if enabled
  #ifdef FAST_MATH
  // iasin() somehow outputs degrees, unsure if it's the right fit
	yaw = atan2Fast(t3, t4) * RAD_TO_DEG;
	pitch = iasin(t2);
	roll = atan2Fast(t0, t1) * RAD_TO_DEG;
  #else
  yaw = atan2(t3, t4) * RAD_TO_DEG;
	pitch = asin(t2) * RAD_TO_DEG;
	roll = atan2(t0, t1) * RAD_TO_DEG;
  #endif
}

void loop()
{
  if (imu.dataAvailable())
  {
    hatire_t hatire;

    getEuler(
      hatire.Data.Attitude.Gyro[0],
      hatire.Data.Attitude.Gyro[1],
      hatire.Data.Attitude.Gyro[2]
    );

    Serial.write((byte*)&hatire, sizeof(hatire_t));

    if (++hatire.Capture > 999)
      hatire.Capture = 0;
  }
}
