const int VRx = 1;
const int VRy = 0;


void setup() {
  // put your setup code here, to run once:
  pinMode(VRx, ANALOG);
  pinMode(VRy, ANALOG);
  Serial.begin(115200);
  analogSetAttenuation(ADC_11db);
}

void loop() {
  // put your main code here, to run repeatedly:
  uint32_t vrx = analogReadMilliVolts(VRx);
  uint32_t vry = analogReadMilliVolts(VRy);
  Serial.printf("%u %u\n", vrx, vry);
  delay(200);
}
