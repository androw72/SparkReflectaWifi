/*
  SD card test 
   
 This example shows how use the utility libraries on which the'
 SD library is based in order to get info about your SD card.
 Very useful for testing a card when you're not sure whether its working or not.
  
 The circuit:
  * SD card attached to SPI bus as follows:
  Refer to "libraries/SdFat/Sd2Card_config.h" 

 created  28 Mar 2011
 by Limor Fried 
 modified 16 Mar 2011
 by Tom Igoe
 modified for Maple(STM32 micros)/libmaple
 17 Mar 2012
 by dinau
 */
 // include the SD library:

#include "application.h"
#include "sd-card-library.h"
#include "ereader.h"
#include <Reflecta.h>

// set up variables using the SD utility library functions:
Sd2Card card;
SdVolume volume;
SdFile root;

//EReader ereader;

// SOFTWARE SPI pin configuration - modify as required
// The default pins are the same as HARDWARE SPI
const uint8_t chipSelect = A2;    // Also used for HARDWARE SPI setup
const uint8_t mosiPin = A5;
const uint8_t misoPin = A4;
const uint8_t clockPin = A3;


int sd_file_control(String command);
TCPServer server = TCPServer(8080); 
TCPClient client;

SYSTEM_MODE(MANUAL);

bool heartbeat_dummy(TCPClient & client) {
    return true;
    
}

int button = 0;
bool isPushed = false;
bool released = true;
bool heartBeatDone = false;

bool myReadbutton(TCPClient & client){
  
    //check the spark claud
    if(Spark.connected()){
        Spark.process();        
    } else
        Spark.connect();
    
  if(isPushed){
    reflectaHeartbeat::push(button, client);
    button=0;
    isPushed = false;
    heartBeatDone = true;
    return true;
  } else {
    reflectaHeartbeat::push(0, client);
    return true;
  } 
}
 
void poll_button(){
    int pin_d5 = digitalRead(D5);
    if(released){
        if(!pin_d5){
            isPushed = true;
            released = false;
            button = 1;
            Serial.println("isPushed");
        }
    } else {
        if(pin_d5 && heartBeatDone){
            isPushed = false;
            released = true;
            heartBeatDone = false;
            Serial.println("released");
        }
            
    }
    
}
           
                
void setup()
{
    
    //input pin for button
    pinMode(D5, INPUT_PULLUP);
    
    //start wlan
    WiFi.on();
    WiFi.connect();
    // start listening for clients
    server.begin();
 
    //Serial.begin(115200);
    reflectaFrames::setup(115200);
    reflectaFunctions::setup();
    reflectaArduinoCore::setup();
    reflectaHeartbeat::setup();
    reflectaFunctions::push16(1);
    reflectaHeartbeat::setFrameRate();
    reflectaHeartbeat::bind(myReadbutton);
    
    //start spark claud
    //Spark.connect();
  

  //Register our Spark function here

  Spark.function("sd_file", sd_file_control);
  

  //while (!Serial.available());
  
  //Serial.print("\nEreader...\n");
  ereader.setup(EPD_2_7);
  //ereader.display_wif("/IMAGES/LENA.WIF", 0, 0);
  ereader.display_wif("/IMAGES/WYOLUM.WIF", 0, 0);
  ereader.show();
  //ereader.sleep(4000);
  
  
  Serial.print("\nmy_test: nInitializing SD card...");
  
  // we'll use the initialization code from the utility libraries
  // since we're just testing if the card is working!
  // Initialize HARDWARE SPI with user defined chipSelect
  if (!card.init(SPI_HALF_SPEED, chipSelect)) {

  // Initialize SOFTWARE SPI (uncomment and comment out above line to use)
//  if (!card.init(mosiPin, misoPin, clockPin, chipSelect)) {
    Serial.println("initialization failed. Things to check:");
    Serial.println("* is a card is inserted?");
    Serial.println("* Is your wiring correct?");
    Serial.println("* did you change the chipSelect pin to match your shield or module?");
    return;
  } else {
   Serial.println("Wiring is correct and a card is present."); 
  }

  // print the type of card
  Serial.print("\nCard type: ");
  switch(card.type()) {
    case SD_CARD_TYPE_SD1:
      Serial.println("SD1");
      break;
    case SD_CARD_TYPE_SD2:
      Serial.println("SD2");
      break;
    case SD_CARD_TYPE_SDHC:
      Serial.println("SDHC");
      break;
    default:
      Serial.println("Unknown");
  }

  // Now we will try to open the 'volume'/'partition' - it should be FAT16 or FAT32
  if (!volume.init(card)) {
    Serial.println("Could not find FAT16/FAT32 partition.\nMake sure you've formatted the card");
    return;
  }

  // print the type and size of the first FAT-type volume
  uint32_t volumesize;
  Serial.print("\nVolume type is FAT");
  Serial.println(volume.fatType(), DEC);
  Serial.println();
  
  volumesize = volume.blocksPerCluster();    // clusters are collections of blocks
  volumesize *= volume.clusterCount();       // we'll have a lot of clusters
  volumesize *= 512;                         // SD card blocks are always 512 bytes
  Serial.print("Volume size (bytes): ");
  Serial.println(volumesize);
  Serial.print("Volume size (Kbytes): ");
  volumesize /= 1024;
  Serial.println(volumesize);
  Serial.print("Volume size (Mbytes): ");
  volumesize /= 1024;
  Serial.println(volumesize);

  Serial.println("\nFiles found on the card (name, date and size in bytes): ");
  root.openRoot(volume);
  
  // list all files in the card with date and size
  root.ls(LS_R | LS_DATE | LS_SIZE);
}

bool connected = false;
void loop(void) {
     
    if (client.connected()) {
        if (!connected) {
            Serial.print("spark_wifi: client connected \n");
            connected = true;
        }
        // if data from a client decode  the frames
        while (client.available()) {
            //server.write(client.read());
            reflectaFrames::loop(client); 
        }
        reflectaHeartbeat::loop(client);
    } else {
        // if no client is yet connected, check for a new connection
        if(client = server.available()){
            connected = false;
        }
    }   
    //poll buttons
    poll_button();
    

      
}
  


// This function gets called whenever there is a matching API request
// the command string format is l<led number>,<state>
// for example: l1,HIGH or l1,LOW
//              l2,HIGH or l2,LOW

int sd_file_control(String command)
{
        char * cmd_str = (char *)command.c_str();
      //Serial.print("\nRegistred function call...: %s", cmd_str);
        Serial.print("\nRegistred function call: sd_file_control");
      
        ereader.display_wif(cmd_str, 0, 0);
        ereader.show();    
        ereader.EPD.end();
      
}


