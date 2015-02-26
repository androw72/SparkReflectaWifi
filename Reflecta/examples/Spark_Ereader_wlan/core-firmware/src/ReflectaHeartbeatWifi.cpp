/*
ReflectaHeartbeat.cpp - Library for sending timed heartbeats of data over Reflecta to a host
*/

#include "Reflecta.h"

using namespace reflecta;

namespace reflectaHeartbeat
{
  const byte heartbeatStackMax = 8;
  int heartbeatStackTop = -1;
  int8_t heartbeatStack[heartbeatStackMax + 1];

  void push(int8_t b, TCPClient& client)
  {
    if (heartbeatStackTop == heartbeatStackMax)
    {
      reflectaFrames::sendEvent(Error, StackOverflow, client);
    }
    else
    {
      heartbeatStack[++heartbeatStackTop] = b;
    }
  }

  void push16(int16_t w, TCPClient& client)
  {
    if (heartbeatStackTop > heartbeatStackMax - 2)
    {
      reflectaFrames::sendEvent(Error, StackOverflow, client);
    }
    else
    {
      heartbeatStack[++heartbeatStackTop] = (w >> 8);
      heartbeatStack[++heartbeatStackTop] = (w & 0xFF);
    }
  }
  
  void pushf(float f, TCPClient& client)
  {
    if (heartbeatStackTop > heartbeatStackMax - 4)
    {
      reflectaFrames::sendEvent(Error, StackOverflow, client);
    }
    else
    {
      byte* pb = (byte*)&f;
      heartbeatStack[++heartbeatStackTop] = pb[3];
      heartbeatStack[++heartbeatStackTop] = pb[2];
      heartbeatStack[++heartbeatStackTop] = pb[1];
      heartbeatStack[++heartbeatStackTop] = pb[0];
    }
  }
  
  int8_t pop(TCPClient& client)
  {
    if (heartbeatStackTop == -1)
    {
      reflectaFrames::sendEvent(Error, StackUnderflow, client);
      return -1;
    }
    else
    {
      return heartbeatStack[heartbeatStackTop--];
    }
  }

  
  const int MaxFunctions = 8;
  
  // Function table of all functions to call in heartbeat
  bool (*heartbeatFunctions[MaxFunctions])(TCPClient& client);

  // Has the function finished for this heartbeat
  bool functionComplete[MaxFunctions];

  int functionsTop = 0;

  void bind(bool (*function)(TCPClient& client)) {
    heartbeatFunctions[functionsTop++] = function;
    //heartbeatFunctions[0] = function;
    //bool (*heartbeatFunctions_1)() = function;
    //function();
  };
  
  uint32_t microsBetweenFrames = 100000;
  
  void setFrameRate(TCPClient& client) {
    int16_t framesPerSecond = reflectaFunctions::pop16(client);
    microsBetweenFrames = 1000000 / framesPerSecond;
  };
  void setFrameRate() {
    int16_t framesPerSecond = reflectaFunctions::pop16();
    microsBetweenFrames = 1000000 / framesPerSecond;
  };
  
  // Number of times loop is called and we're still waiting for one of our bound data
  // collection functions to finish
  uint16_t collectingLoops = 0;
  
  // Number of times loop is called and we've finished collecting data but the timeout
  // for next heartbeat has not yet expired
  uint16_t idleLoops = 0;
  
  void sendHeartbeat(TCPClient& client) {
    
    push16(idleLoops, client);
    push16(collectingLoops, client);
    push(Heartbeat,client);
      byte heartbeatSize = heartbeatStackTop + 1; 
      byte frame[heartbeatSize];
      
      for (int i = 0; i < heartbeatSize; i++)
      {
        frame[i] = pop(client);
      }
      
      reflectaFrames::sendFrame(frame, heartbeatSize, client);
  };
  
  bool finished = false;
  uint32_t nextHeartbeat = 0;
  void loop(TCPClient& client) {
    
    if (!finished) {
      collectingLoops++;
      finished = true;
      for (int i = 0; i < functionsTop; i++) {
        if (!functionComplete[i]) {
          functionComplete[i] = heartbeatFunctions[i](client);
          if (!functionComplete[i]) finished = false;
        }
      }
    }
    
    if (finished) {
      
      unsigned long currentTime = micros();
      if (currentTime >= nextHeartbeat) {
        
        sendHeartbeat(client);
        
        // Set the micros delay for when the next heartbeat should be sent
        nextHeartbeat = currentTime + microsBetweenFrames;
        
        // Zero out the heartbeat state
        collectingLoops = 0;
        idleLoops = 0;
        
        for (int i = 0; i < MaxFunctions; i++) { 
          functionComplete[i] = false;
          finished = false;
        }
        
      } else {
        idleLoops++;
      };
    }
  };

  //void dummy(bool (*function)()) {};
    
  
  void setup() {
    reflectaFunctions::bind("hart1", setFrameRate);
  };
};
