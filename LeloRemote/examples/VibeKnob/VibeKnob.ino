#include <SPI.h>
#include <LeloRemote.h>

LeloRemote remote;

void setup()
{
  Serial.begin(9600);
  SPI.begin();
  remote.reset();
}

void loop()
{
  const int hysteresis = 4;
  const int deadzone = 5;
  
  // Read potentiometer input, with a little hysteresis
  static int analogValue;
  int sample = analogRead(0);
  if (abs(sample - analogValue) > hysteresis)
    analogValue = sample;

  // Scale analog input from 0 to MAX_POWER
  int power = map(analogValue, 0 + deadzone, 1023 - deadzone, 0, remote.MAX_POWER);
  power = constrain(power, 0, remote.MAX_POWER);

  // Send radio packets at about 100 Hz
  remote.txMotorPower(power);
  delay(10);
}

