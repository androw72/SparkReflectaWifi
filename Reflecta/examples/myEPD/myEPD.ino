#include <Wire.h>
#include <Servo.h>
#include <Reflecta.h>
#include "EReader.h"


int botton = 0;
bool isPushed = false;

void setup()
{ 
  
  reflectaFrames::setup(9600);
  reflectaFunctions::setup();
  reflectaArduinoCore::setup();
  reflectaHeartbeat::setup();
  
  reflectaFunctions::push16(1);
  reflectaHeartbeat::setFrameRate();
  reflectaHeartbeat::bind(myReadbutton);

  pinMode(UP_PIN, INPUT);
  pinMode(DOWN_PIN, INPUT);
  pinMode(SEL_PIN, INPUT);
  pinMode(MODE_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);


}

bool myReadbutton(){

 if(digitalRead(SEL_PIN)){
    isPushed = true;
    button = 1;
  }
 if(digitalRead(DOWN_PIN)){
    isPushed = true;
    button = 2;

  }
 if(digitalRead(UP_PIN)){
    isPushed = true;
    button = 3;

  }
 if(digitalRead(MODE_PIN)){
    isPushed = true;
    button = 4;

  }

  if(buttonIsPushed){
    reflectaHeartbeat::push(buttonPushed);
    botton = 0;
    isPushed = false;
    return true;
  }
  else
  {
    return false;
  }
}

  reflectaHeartbeat::pushf(1.0);

}

void loop() //Main Loop
{
  reflectaFrames::loop();
  reflectaHeartbeat::loop();
}


