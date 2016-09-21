#include <OneWire.h>
#include <Nextion.h>

#define PIN_OUTPUT 3

OneWire  ds(10);  // on pin 10 (a 4.7K resistor is necessary)

//Initialize Nextion pages and components
NexText tempDisp = NexText(0, 1, "tempDisp");

NexText setPntDisp = NexText(1, 3, "setPntDisp");
NexButton buttonPlus = NexButton(1, 1, "buttonPlus");
NexButton buttonMinus = NexButton(1, 2, "buttonMinus");
NexSlider setPntSlider = NexSlider(1, 5, "setPntSlider");

NexPage Home = NexPage(0, 0, "home");
NexPage SetPnt = NexPage(1, 0, "setpnt");

int number = 25, dutyCycle = 0, dutySetpoint, Delay;
char buffer[10] = {0};

double Setpoint, Input, Output;

NexTouch *nex_listen_list[] =
{
  &tempDisp,
  &setPntDisp,
  &buttonPlus,
  &buttonMinus,
  &setPntSlider,
  NULL
};

void setup(void) {
  
  pinMode(3, OUTPUT);
  digitalWrite(PIN_OUTPUT, HIGH);
  
  nexInit();
  Serial.begin(9600);
  
  Setpoint = 28.00;
  dutySetpoint = 5000;
  Delay = 250;
  
  /* Register the pop event callback function of the current button1 component. */
  buttonPlus.attachPop(buttonPlusPopCallback);
  buttonMinus.attachPop(buttonMinusPopCallback);
  setPntSlider.attachPop(setPntSliderPopCallback);
  
  dbSerialPrintln("Setup done");
}

void loop(void) {
  nexLoop(nex_listen_list);

  float celsius;
  
  celsius = GetTemp();
  Serial.print("Temperature: ");
  Serial.print(celsius);
  Serial.println();
  
  dtostrf(celsius,4,2,buffer);
  tempDisp.setText(buffer);
  
  dtostrf(Setpoint,3,1,buffer);
  setPntDisp.setText(buffer);
  
  Input=celsius;

  if (dutyCycle > dutySetpoint){
    if (Input > Setpoint) digitalWrite(PIN_OUTPUT, LOW);
    else digitalWrite(PIN_OUTPUT, HIGH);
    dutyCycle = 0;
  }else{
    dutyCycle = dutyCycle + Delay;
  }
  
  delay(Delay);
  
}

void buttonPlusPopCallback(void *ptr){
  dbSerialPrintln("Setpoint Plus PopCallback");

  if (Setpoint < 100) Setpoint += 1;
  dtostrf(Setpoint,3,1,buffer);
  setPntDisp.setText(buffer);
}

void buttonMinusPopCallback(void *ptr){
  dbSerialPrintln("Setpoint Minus PopCallback");

  if (Setpoint > 0) Setpoint -= 1;
  dtostrf(Setpoint,3,1,buffer);
  setPntDisp.setText(buffer);
}

void setPntSliderPopCallback(void *ptr){
  uint32_t number = 0;
  char temp[10] = {0};
    
  setPntSlider.getValue(&number);
  Setpoint = number;
  dtostrf(Setpoint,3,1,buffer);
  setPntDisp.setText(buffer);
}

float GetTemp(){
    byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];
  float celsius;
  
 ds.search(addr);

  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);        // start conversion, with parasite power on at the end
  
  //delay(500);    
  ds.depower();
  
  present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE);         // Read Scratchpad
  
    for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
  }

  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  }
  celsius = (float)raw / 16.0;
  return celsius;
}
