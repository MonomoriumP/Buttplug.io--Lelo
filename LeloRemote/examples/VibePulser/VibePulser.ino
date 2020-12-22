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
  for (int i = 0; i < 20; ++i) {
    remote.txMotorPower(128);
    delay(10);
  }
  for (int i = 0; i < 20; ++i) {
    remote.txMotorPower(0);
    delay(10);
  }
}

