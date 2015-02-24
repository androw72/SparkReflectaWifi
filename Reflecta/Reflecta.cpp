#include "Reflecta.h"
#include <Adafruit_CC3000.h>

/*int getFreeRam(void)
{
  extern int  __bss_end;
  extern int  *__brkval;
  
  int free_memory;
  if((int)__brkval == 0) {
    free_memory = ((int)&free_memory) - ((int)&__bss_end);
  }
  else {
    free_memory = ((int)&free_memory) - ((int)__brkval);
  }
  return free_memory;
}
*/ 

namespace reflecta
{
  
  /*
  int getFreeRam(void)
  {
    extern int  __bss_end;
    extern int  *__brkval;

    int free_memory;
    if((int)__brkval == 0) {
      free_memory = ((int)&free_memory) - ((int)&__bss_end);
    }
    else {
      free_memory = ((int)&free_memory) - ((int)__brkval);
    }
    return free_memory;
  } 
  */
  


  void setup(int speed)
  {
       reflectaFrames::setup(speed);
       reflectaFunctions::setup();
       reflectaArduinoCore::setup();
  }
  
  void loop()
  {
    //reflectaFrames::loop(Adafruit_CC3000_ClientRef client);
  }
}

