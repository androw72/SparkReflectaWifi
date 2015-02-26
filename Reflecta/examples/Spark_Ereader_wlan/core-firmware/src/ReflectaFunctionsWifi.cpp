/*
ReflectaFunctions.cpp - Library for binding functions to a virtual function table
*/

#include "Reflecta.h"

using namespace reflecta;



namespace reflectaFunctions
{
  // Index of next unused function in the function table (vtable)
  byte openFunctionIndex = 4;



  // Function table that relates function id -> function
  void (*vtable[26])(TCPClient& client);



  // An interface is a well known group of functions.  Function id 0 == QueryInterface
  // which allows a client to determine which functions an Arduino supports.

  // Maximum number of interfaces supported
  const byte maximumInterfaces = 2;

  // Number of interfaces defined
  byte indexOfInterfaces = 0;

  // Interface Id takes the form of CCCCIV
  //    CCCC is Company Id
  //    I is the Interface Id for the Company Id
  //    V is the Version Id for the Interface Id



  String interfaceIds[maximumInterfaces];


  //}; /*

  // Interface starting function id, id of the first function in the interface
  //   in the vtable
  byte interfaceStart[maximumInterfaces];





  // Is this interface already defined?
  bool knownInterface(String interfaceId)
  {
    for(int index = 0; index < indexOfInterfaces; index++)
    {
      if (interfaceIds[index] == interfaceId)
      {
        return true;
      }
    }

    return false;
  }



  // Bind a function to the vtable so it can be remotely invoked.
  //   returns the function id of the binding in the vtable
  //   Note: You don't generally use the return value, the client uses
  //   QueryInterface (e.g. function id 0) to determine the function id
  //   remotely.
  byte bind(String interfaceId, void (*function)(TCPClient& client))
  {
    if (!knownInterface(interfaceId))
    {
      interfaceIds[indexOfInterfaces] = interfaceId;
      interfaceStart[indexOfInterfaces++] = openFunctionIndex;
    }

    if (vtable[openFunctionIndex] == NULL)
    {
      vtable[openFunctionIndex] = function;
    }
    else
    {
      //reflectaFrames::sendEvent(Error, FunctionConflict, client);
      Serial.print("ReflectaFunctionsWifi.cpp: bind - error: function conflict");
    }

    return openFunctionIndex++;
  }



  byte callerSequence;
  
  // Send a response frame from a function invoke.  Used when the function automatically returns
  // data to the caller.
  void sendResponse(byte parameterLength, byte* parameters, TCPClient &client )
  {
    byte frame[3 + parameterLength];

    frame[0] = Response;
    frame[1] = callerSequence;
    frame[2] = parameterLength;
    memcpy(frame + 3, parameters, parameterLength);

    reflectaFrames::sendFrame(frame, 3 + parameterLength, client);
  }

  // Invoke the function, private method called by frameReceived
  void run(byte i, TCPClient &client)
  {
    if (vtable[i] != NULL)
    {
      vtable[i](client);      
    }
    else
    {
      reflectaFrames::sendEvent(Error, FunctionNotFound, client);
    }
  }

  const byte parameterStackMax = 28;
  int parameterStackTop = -1;
  int8_t parameterStack[parameterStackMax + 1];

  void push(int8_t b, TCPClient &client)
  {
    if (parameterStackTop == parameterStackMax)
    {
      reflectaFrames::sendEvent(Error, StackOverflow, client);
    }
    else
    {
      parameterStack[++parameterStackTop] = b;
    }
  }
 void push(int8_t b)
  {
    if (parameterStackTop == parameterStackMax)
    {
      //reflectaFrames::sendEvent(Error, StackOverflow, client);
      Serial.print(F("ReflectaFunctionsWifi.cpp: Error: StackOverFlow"));

    }
    else
    {
      parameterStack[++parameterStackTop] = b;
    }
  }

  void push16(int16_t w, TCPClient& client)
  {
    if (parameterStackTop > parameterStackMax - 2)
    {
      reflectaFrames::sendEvent(Error, StackOverflow, client);
      //Serial.print(F("ReflectaFunctionsWifi.cpp: Error: StackOverFlow"));
    }
    else
    {
      parameterStackTop += 2;
      *((int16_t*)(parameterStack + parameterStackTop - 1)) = w;
    }
  }
  void push16(int16_t w)
  {
    if (parameterStackTop > parameterStackMax - 2)
    {
      //reflectaFrames::sendEvent(Error, StackOverflow, client);
      Serial.print(F("ReflectaFunctionsWifi.cpp: Error: StackOverFlow"));
    }
    else
    {
      parameterStackTop += 2;
      *((int16_t*)(parameterStack + parameterStackTop - 1)) = w;
    }
  }

  int8_t pop(TCPClient& client)
  {
    if (parameterStackTop == -1)
    {
      reflectaFrames::sendEvent(Error, StackUnderflow, client);
      return -1;
    }
    else
    {
      return parameterStack[parameterStackTop--];
    }
  }
  
  int16_t pop16(TCPClient& client)
  {
    if (parameterStackTop == -1 || parameterStackTop == 0)
    {
      reflectaFrames::sendEvent(Error, StackUnderflow, client);
      return -1;
    }
    else
    {
      parameterStackTop -= 2;
      return *(int16_t*)(parameterStack + parameterStackTop + 1);
    }
  }

 int16_t pop16()
  {
    if (parameterStackTop == -1 || parameterStackTop == 0)
    {
      //reflectaFrames::sendEvent(Error, StackUnderflow, client);
      Serial.print(F("reflectafunctionswifi.cpp: stackunderflow"));
      return -1;
    }
    else
    {
      parameterStackTop -= 2;
      return *(int16_t*)(parameterStack + parameterStackTop + 1);
    }
  }
  
  // Request a response frame from data that is on the parameterStack.  Used to retrieve
  // a count of 'n' data bytes that were push on the parameterStack from a previous
  // invocation.  The count of bytes to be returned is determined by popping a byte off
  // the stack so it's expected that 'PushArray 1 ResponseCount' is called first. 
  void sendResponseCount(TCPClient& client)
  {
    int8_t count = pop(client);
    byte size = 3 + count;
    
    byte frame[size];
    
    frame[0] = Response;
    frame[1] = callerSequence;
    frame[2] = count;
    for (int i = 0; i < count; i++)
    {
      int8_t value = pop(client);
      frame[3 + i] = value;
    }

    reflectaFrames::sendFrame(frame, size, client);
  }

  // Request a response frame of one byte data that is on the parameterStack.  Used to
  // retrieve data pushed on the parameterStack from a previous function invocation.
  void sendResponse(TCPClient& client)
  {
    push(1, client);
    sendResponseCount(client);
  }
  
  // Execution pointer for Reflecta Functions.  To be used by functions that
  // change the order of instruction execution in the incoming frame.  Note:
  // if you are not implementing your own 'scripting language', you shouldn't
  // be using this.
  byte* execution;
  
  // Top of the frame marker to be used when modifying the execution pointer.
  // Generally speaking execution should not go beyond frameTop.  When
  // execution == frameTop, the Reflecta Functions frameReceived execution loop
  // stops. 
  byte* frameTop;  



  void pushArray(TCPClient& client)
  {
    // Pull off array length
    if (execution == frameTop) reflectaFrames::sendEvent(Error, FrameTooSmall,client);
    byte length = *execution++;
    
    // Push array data onto parameter stack as bytes, reversed
    for (int i = length - 1; i > -1; i--) // Do not include the length when pushing, just the data
    {
      push(*(execution + i), client);
    }

    // Increment the execution pointer past the data array, being careful not to exceed the frame size
    for (int i = 0; i < length; i++)
    {
      execution++;
      if (execution > frameTop)
      {
        reflectaFrames::sendEvent(Error, FrameTooSmall, client);
        break;
      }
    }
  }
  
  // Private function hooked to reflectaFrames to inspect incoming frames and
  //   Turn them into function calls.
  void frameReceived(byte sequence, byte frameLength, byte* frame, TCPClient &client)
  {    
    execution = frame; // Set the execution pointer to the start of the frame
    callerSequence = sequence;
    frameTop = frame + frameLength;

    while (execution != frameTop) {
      run(*execution++, client);
    }
  }

  // queryInterface is called by invoking function.  It returns a response
  // packet containing the interface id and start index of each registered
  // interface
  void queryInterface(TCPClient &client) {
    
    const int interfaceIdLength = 5;
    
    for(int index = 0; index < indexOfInterfaces; index++) {
      for (int stringIndex = interfaceIdLength - 1; stringIndex > -1; stringIndex--) { 
        push(interfaceIds[index][stringIndex], client);
      }
      push(interfaceStart[index], client);
    }
    
    // each interface contributes 1 payload byte for startIndex and 'n' bytes for the interfaceId string
    push((interfaceIdLength + 1) * indexOfInterfaces, client);
    sendResponseCount(client);
  }

  /*
    void reset() {
    parameterStackTop = -1;
    //anbr
    reflectaFrames::reset();
    }
  */
  
  
  void setup()
  {
    
    // Zero out the vtable function pointers
    
    memset(vtable, NULL, 26);
   
     
    // Bind the QueryInterface function in the vtable
    // Do this manually as we don't want to set a matching Interface
    
    vtable[PushArray] = pushArray;
    vtable[QueryInterface] = queryInterface;
    vtable[SendResponse] = sendResponse;
    vtable[SendResponseCount] = sendResponseCount;
    //vtable[Reset] = reset;
    // Anbr
    
    //vtable[TestAnbr] = testAnbr;
    // TODO: block out FRAMES_ERROR, FRAMES_MESSAGE, and FUNCTIONS_RESPONSE too
    // Hook the incoming frames and turn them into function calls 
    
    //anbr
    reflectaFrames::setFrameReceivedCallback(frameReceived);
    
    
  }
};

//  */


