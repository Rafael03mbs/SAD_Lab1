#include <Wire.h>

int valueA0 = 0;
int valueA1 = 0;

void setup()
{
  Serial.begin(9600);
  Serial.println("Setup");
  Wire.begin(0x48);
  Wire.onReceive(receiveEvent); 
  Wire.onRequest(requestEvent);
}

void loop()
{
  Serial.print(valueA0);
  Serial.print(",");
  Serial.println(valueA1);
  delay(2000);
}

void receiveEvent(int howMany)
{
  while(Wire.available()) 
  {
    int command = Wire.read(); 
    if (command == 0xAC) {
      valueA0 = analogRead(A0);
      valueA1 = analogRead(A1);
    }
  }
}

void requestEvent(int howMany)
{
  Wire.write(0x40);
  Wire.write(byte(valueA0>>8));
  Wire.write(byte(valueA0));
  Wire.write(byte(valueA1>>8));
  Wire.write(byte(valueA1));
}
