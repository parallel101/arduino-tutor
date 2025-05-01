#include <Wire.h>

void setup() {
  Serial.begin (9600);
  pinMode(LED_BUILTIN, OUTPUT);
}  // end of setup

void loop() {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);
  Serial.println ();
  Serial.println ("I2C scanner. Scanning ...");
  byte count = 0;
 
  Wire.begin();
  for (byte i = 1; i < 120; i++)
  {
    Wire.beginTransmission (i);
    if (Wire.endTransmission () == 0)
      {
      Serial.print ("Found address: ");
      Serial.print (i, DEC);
      Serial.print (" (0x");
      Serial.print (i, HEX);
      Serial.println (")");
      count++;
      }
  } // end of for loop

  Serial.println ("Done.");
  Serial.print ("Found ");
  Serial.print (count, DEC);
  Serial.println (" device(s).");
  digitalWrite(LED_BUILTIN, LOW);
}