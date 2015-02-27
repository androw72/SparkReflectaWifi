/*
ReflectaFramesSerial.cpp - Library for sending frames of information from a Microcontroller to a PC over a serial port.
*/

#include "sd-card-library.h"
#include "Reflecta.h"
//#include <Adafruit_CC3000.h>
//#include "utility/debug.h"
//#include "utility/socket.h"
#include <SPI.h>


using namespace reflecta;

namespace reflectaFrames
{
// SLIP (http://www.ietf.org/rfc/rfc1055.txt) protocol special character definitions
// Used to find end of frame when using a streaming communications protocol
enum EscapeCharacters {
End           = 0xC0,
  Escape        = 0xDB,
  EscapedEnd    = 0xDC,
  EscapedEscape = 0xDD
  };

enum ReadState {
FrameStart,
  ReadingFrame,
  FrameEnded,
  FrameInvalid
  };

// Checksum for the incoming frame, calculated byte by byte using XOR.  Compared against the checksum byte
// which is stored in the last byte of the frame.
byte readChecksum = 0;

// Checksum for the outgoing frame, calculated byte by byte using XOR.  Added to the payload as the last byte of the frame. 
  byte writeChecksum = 0;
  
  // Sequence number of the incoming frames.  Compared against the sequence number at the beginning of the incoming frame
  //  to detect out of sequence frames which would point towards lost data or corrupted data.
  byte readSequence = 0;
  
  // Sequence number of the outgoing frames.
  byte writeSequence = 0;
  
  // protocol parser escape state -- set when the ESC character is detected so the next character will be de-escaped
  bool escaped = false;
  
  // protocol parser state
  int state = FrameStart;
 
 
  frameBufferAllocationFunction frameBufferAllocationCallback = NULL;
  frameReceivedFunction frameReceivedCallback = NULL;


  void setFrameReceivedCallback(frameReceivedFunction frameReceived)
  {
    frameReceivedCallback = frameReceived;
  }
  
  void setBufferAllocationCallback(frameBufferAllocationFunction frameBufferAllocation)
  {
    frameBufferAllocationCallback = frameBufferAllocation;
  }
  
  void writeEscaped(byte b, TCPClient& client)
  {
    switch(b)
    {
      case End:
        client.write(Escape);
        client.write(EscapedEnd);
        break;
      case Escape:
        client.write(Escape);
        client.write(EscapedEscape);
        break;
      default:
        client.write(b);
        break;
    }
    writeChecksum ^= b;
  }
  
  byte sendFrame(byte* frame, byte frameLength )
  {
    /*writeChecksum = 0;
    writeEscaped(writeSequence, client);
    for (byte index = 0; index < frameLength; index++)
    {
      writeEscaped(frame[index], client);
    }
    writeEscaped(writeChecksum, client);
    client.write(End);

    // On Teensies, use the extended send_now to perform an undelayed send
    #ifdef USBserial_h_
    client.send_now();
    #endif
    
    return writeSequence++;
    */
    Serial.println("reflectaframesWifi.cpp: sendFrame old!");
  }
  byte sendFrame(byte* frame, byte frameLength,  TCPClient& client)
  {
    
    writeChecksum = 0;
    writeEscaped(writeSequence, client);
    for (byte index = 0; index < frameLength; index++)
    {
      writeEscaped(frame[index], client);
    }
    writeEscaped(writeChecksum, client);
    client.write(End);

    // On Teensies, use the extended send_now to perform an undelayed send
    #ifdef USBserial_h_
    client.send_now();
    #endif
    
    return writeSequence++;
    
    Serial.println("reflectaframesWifi.cpp: sendFrame");
  }
  
  void sendEvent(FunctionId type, byte code)
  {
    /*
      byte buffer[2];
    buffer[0] = type;
    buffer[1] = code;
    sendFrame(buffer, 2);
    */
    Serial.println(F("reflectaframesWifi.cpp: sendEvent old!"));
  }
  void sendEvent(FunctionId type, byte code, TCPClient& client)
  {
    byte buffer[2];
    buffer[0] = type;
    buffer[1] = code;
    Serial.println("reflectaframesWifi.cpp: sendEvent");
    sendFrame(buffer, 2, client);
  }
  
  void sendMessage(String message, TCPClient& client)
  {
    byte bufferLength = message.length() + 3;
    byte buffer[bufferLength];
    
    buffer[0] = Message;
    buffer[1] = message.length();
    message.getBytes(buffer + 2, bufferLength - 2);
    
    // Strip off the trailing '\0' that Arduino String.getBytes insists on postpending
    sendFrame(buffer, bufferLength - 1, client);
  }
  void sendMessage(String message)
  {
    Serial.print(message);
    /*
      byte bufferLength = message.length() + 3;
    byte buffer[bufferLength];
    
    buffer[0] = Message;
    buffer[1] = message.length();
    message.getBytes(buffer + 2, bufferLength - 2);
    
    // Strip off the trailing '\0' that Arduino String.getBytes insists on postpending
    sendFrame(buffer, bufferLength - 1);
    */
  }
  
  int readUnescaped(byte &b, TCPClient& client)
  {
    b = client.read();
    
    if (escaped)
    {
      switch (b)
      {
        case EscapedEnd:
          b = End;
          break;
        case EscapedEscape:
          b = Escape;
          break;
        default:
          sendEvent(Warning, UnexpectedEscape, client);
          state = FrameInvalid;
          break;
      }
      escaped = false;
      readChecksum ^= b;
    }
    else
    {
      if (b == Escape)
      {
        escaped = true;
        return 0; // read escaped value on next pass
      }
      if (b == End)
      {
        switch (state)
        {
          case FrameInvalid:
            readChecksum = 0;
            state = FrameStart;
            break;
          case ReadingFrame:
            state = FrameEnded;
            break;
          default:
            sendEvent(Warning, UnexpectedEnd, client);
            state = FrameInvalid;
            break;
        }
      }
      else
      {
        readChecksum ^= b;
      }
    }
    
    return 1;
  }
  
  const byte frameBufferSourceLength = 32;
  byte* frameBufferSource = NULL;
  
  // Default frame buffer allocator for when caller does not set one.
  byte frameBufferAllocation(byte** frameBuffer)
  {
    *frameBuffer = frameBufferSource;
    return frameBufferSourceLength;
  }
  
   
  void reset()
  {
    readSequence = 0;
    writeSequence = 0;
    //anbr
    Serial.print(F("anbr: reset \n"));
    delay(1000);
    Serial.flush();
  }
  
  bool displayConnectionDetails(void);

 void setup(int speed)
  {
    if (frameBufferAllocationCallback == NULL)
      {
	frameBufferSource = (byte*)malloc(frameBufferSourceLength);
	frameBufferAllocationCallback = frameBufferAllocation;
      }
    
    Serial.begin(speed);

    Serial.flush();

   

}


  byte* frameBuffer;
  byte frameBufferLength;
  byte frameIndex = 0;
  
byte sequence;

uint32_t lastFrameReceived;

void loop(TCPClient& client)
{
       byte b;

      while (client.available())
  {
    //Serial.println(F("reflectaFramesWifi.cpp: client.availeble"));
    if (readUnescaped(b, client))
    {
     switch (state)
     {
      case FrameInvalid:
      Serial.print(F("FramesWifi: frame invalid \n"));
      delay(1000);
      break;
      
      case FrameStart:
      sequence = b;
      if (readSequence++ != sequence)
      {
      // Only send an out of sequence warning if the time between frames is < 10 seconds
      // This is because we have no 'port opened/port closed' API on Arduino to tell when
      // a connection has been physically reset by the host
      if (lastFrameReceived - millis() < 10000) {
	sendMessage("Expected " + String(readSequence - 1, HEX) + " received " + String(sequence, HEX), client );
      sendEvent(Warning, OutOfSequence, client);
      }
      
      readSequence = sequence + 1;
      }
      
      frameBufferLength = frameBufferAllocationCallback(&frameBuffer);
      frameIndex = 0; // Reset the buffer pointer to beginning
      readChecksum = sequence;
      state = ReadingFrame;
      break;
      
      case ReadingFrame:
      if (frameIndex == frameBufferLength)
      {
	sendEvent(Error, BufferOverflow, client);
      state = FrameInvalid;
      readChecksum = 0;
      }
      else
      {
      frameBuffer[frameIndex++] = b;
      }
      break;
      
      case FrameEnded:
	lastFrameReceived = millis();
	if (readChecksum == 0) // zero expected because finally XOR'd with itself
	  {
	    // TODO: add a MessageReceived callback too
	    
	    if (frameReceivedCallback != NULL)
	      {
		frameReceivedCallback(sequence, frameIndex - 1, frameBuffer, client);
	      }
	  }
	else
	  {
	    sendEvent(Warning, CrcMismatch, client);
	    state = FrameInvalid;
	    readChecksum = 0;
	  }
	state = FrameStart;
	break;
      }
      }
}}
    
}


