/**
  ******************************************************************************
  * @file    logging_node.ino
  * @author  T. Buckingham
  * @brief   Sketch file for children nodes that record and send readings to the 
  *           user via the bridge [see bridge_node.ino]
  *
  *          The file contains ::
  *           + Functions for handling requests and responding to the bridge
	*						+ Node specific details such as its string name     
  *
  * FOR MORE DETAILS ON LIBRARIES OR EXAMPLES USED PLEASE SEE THE README
  *
  ******************************************************************************
  */

/*-- Includes --*/

#include "painlessMesh.h"   // Mesh network header
#include "namedMesh.h"      // Mesh network with names implemented - see Nodes GitHub version for any changes

#include <SD.h>             // SD Storage header
#include <SPI.h>            // Serial Peripheral Interface
#include <TimeLib.h>        // Arduino time library
#include <FS.h>             // Arduino File System libbrary

/*-- Global Definitions --*/

#define   MESH_PREFIX     "whateverYouLike"
#define   MESH_PASSWORD   "somethingSneaky"
#define   MESH_PORT       5555

/*-- Global Variables --*/
Scheduler userScheduler;
const int chip_select = D8;                /* Used for interfacing with the SD card */
namedMesh  mesh;                /* namedMesh network class, used to interface with the network */
String nodeName = "lobitos";                   /* Name for this specific node */

String readings_path = "/readings.txt";
String errors_path   = "/errors.txt";
String settings_path = "/settings.txt";

String readings_archive_path = "/readings_archive.txt";
String errors_archive_path   = "/errors.txt";
String settings_archive_path = "/settings.txt";

unsigned char minutes_interval = 1;           /* Time interval to record data logs */
time_t prev_time;                    /* Determine  if enough time has passed to record a data log */

/*-- Prototypes --*/

/** 
*   @brief Function used to send stored readings as a string to the bridge node when the user requests
 *  @param ticket_number the ticket that is currently being sent back to the bridge/root node
 *  @return Void.
 */
void sendReadings(unsigned char ticket_number);


/** 
*   @brief Function used to send stored readings as a string to the bridge node when the user requests
 *          then stores those readings in an archive file
 *  @param ticket_number the ticket that is currently being sent back to the bridge/root node
 *  @return Void.
 */
void sendReadingsArchive(unsigned char ticket_number);


/** 
 *  @brief Basic function used to test the connections between the root during development
 *  @param Void.
 *  @return Void.
 */
void sendMessage();


/** 
*   @brief Appends a string, in this case a log, to the specified file
 *  @param path the files path on the storage medium
 *  @param content the string to be appended to the file
 *  @return Void.
 */
void appendFile(String path, String content);


/** 
 *  @brief Writes a new file onto the storage device
 *  @param content the content to be saved to the file
 *  @param path the files path on the storage medium
 *  @return Void.
 */
void writeFile(String path, String content);


/** 
 *  @brief Records a reading to the readings files
 *  @param path the files path on the storage medium
 *  @return Void.
 */
void logReading(String path);


void logReading(String path)
{

   // "[time_stamp]:[data1, data2, data3],"
   String reading = "\"";
   Serial.println(reading);
   reading += String(now());
   Serial.println(reading);
   reading += "\": [" ;
   Serial.println(reading);
   // Turbidity, TDS, pressure, flow rate, pH, Temp
   reading += String(random(1, 1000)) + "," + String(random(300, 2000)) + "," + String(random(20, 8) + "," + random(1, 6)) + "," + String(random(0, 14)) + "," + String(random(20, 30)) + "],";
   Serial.println(reading);
   appendFile(readings_path, reading);

}


void appendFile(String path, String content)
{

  // Dont append with final closing }, this will be done when sending the string (along with the ticket number)
  /* Desired format [ignoring new lines]
  "1622119135383": [ // timestamp
    48.75608, // readings...
    2.302038,
    48.75608 //...
  ]
  */
  Serial.println(path + ":" + content);
  File file = SD.open(path, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file " + String(path) + " for appending");
        return;
    }
    if(file.print(content)){
        Serial.println("Message appended");
    } else {
        Serial.println("Append failed");
    }
    file.close();


}

void writeFile(String path, String content) 
{
 
  File file = SD.open(path, FILE_WRITE);
  if(!file) {
    Serial.println("File creation failed");
    return;
  }
  if(file.print(content)) {  // Write empty string to test writing to the file
    Serial.println("Start to JSON file " + String(path) + " successfuly made");
  } else {
    Serial.println("Failed to write to file " + String(path));
  }
  file.close();

}


void sendReadingsArchive(unsigned char ticket_number)
{

  if (!SD.begin(chip_select)) {
    Serial.println("SD card Initialization failed");
    while (1);
  }
  char ch;
  String root = "root";
  String readings = "{\"ticket\":" + String(ticket_number)  + ",";
  File readings_file = SD.open(readings_path);
  if(readings_file) {
    while(readings_file.available()) {
      ch = readings_file.read();
      readings += String(ch);
    }
    readings_file.close();
    readings.remove(readings.length()- 1); // removes final trailing ','
    readings += "}"; // may have trailing ',' - may need to remove before adding the '}' for parsing
    mesh.sendSingle(root, readings);
    Serial.println("Message sent: ");
    Serial.println(readings);
  } else {
    Serial.println("Error opening file");
  }
  readings_file.close();
  writeFile(readings_archive_path, readings); // write old readings to archive file
  SD.remove(readings_path); // delete old file
  writeFile(readings_path, ""); // make new readings file
  
}

void sendReadings(unsigned char ticket_number)
{

/* Desired format [ignoring new lines]
{
  "ticket": 5, // ticket number
  "1622119135383": [ // time stamp
    48.75608, // readings...
    2.302038,
    48.75608  // ...
  ],
  "1622119135383": [ // time stamp
    48.75608, // readings...
    2.302038,
    48.75608  // ...
  ], etc...
}
  */
  if (!SD.begin(chip_select)) {
    Serial.println("SD card Initialization failed");
    while (1);
  }
  char ch;
  String root = "root";
  String readings = "{\"ticket\":" + String(ticket_number)  + ",";
  File readings_file = SD.open(readings_path);
  if(readings_file) {
    while(readings_file.available()) {
      ch = readings_file.read();
      readings += String(ch);
    }

    readings_file.close();
    readings.remove(readings.length()- 1); // removes final trailing ','
    readings += "}"; // may have trailing ',' - may need to remove before adding the '}' for parsing
    mesh.sendSingle(root, readings);
    Serial.println("Message sent: ");
    Serial.println(readings);
  } else {
    Serial.println("Error opening file");
  }
  readings_file.close();
  //String root = "root";
  //String readings = "{\"ticket\": " +String(ticket_number) + "," + "\"1622119135383\":[9.214,3.45453,1.3424],\"1622119135383\"[12.354,3.44753,6.37424]}";
  
}

void sendMessage() 
{
  String msg = "Hi from node1";
  msg += mesh.getNodeId();
  String to = "root";
  mesh.sendSingle(to, msg);
}

void setup() 
{
  Serial.begin(115200);

  if(!SD.begin(chip_select)){
      Serial.println("Card Mount Failed");
      return;
  }

//mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
  mesh.setDebugMsgTypes( ERROR | STARTUP ); 

  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );

  mesh.setName(nodeName); 
  //mesh.onReceive(&receivedCallback);

  mesh.onReceive([](String &from, String &msg) 
  {
    //Serial.printf("Received message by name from: %s, %s\n", from.c_str(), msg.c_str());
    Serial.println(msg);
    if(msg.c_str()[0] == 'T') {
      char* ptr;
      char* temp = (char*)malloc((msg.length())*sizeof(char*));
      msg.remove(0, 1);
      msg.toCharArray(temp, msg.length() + 1);
      time_t new_time = strtoul(temp, &ptr, 10);
      setTime(new_time);
      prev_time = now();
      free(temp);
    } else if(msg.c_str()[0] == 'I') { 
      char* ptr;
      char* temp = (char*)malloc((msg.length())*sizeof(char*));
      msg.remove(0, 1);
      msg.toCharArray(temp, msg.length() + 1);
      unsigned long new_interval = strtoul(temp, &ptr, 10);
      minutes_interval = (unsigned char) new_interval;
      Serial.println("Interval changed to: " + String(minutes_interval));
      free(temp);
    } else {
      // ticket_num:message - message can be READINGS, ERRORS, SETTINGS
      char buffer[4];
      char* str;
      unsigned char i = 0;
      unsigned char j = 0;
      while(msg.c_str()[i] != ':') { // ticket number
          buffer[i] = msg.c_str()[i];
          i++;
      }
      buffer[i] = '\0';
      int ticket_num = 0;
      sscanf(buffer, "%d", &ticket_num);
      if(msg.indexOf("READINGS") != -1) {
        sendReadings((unsigned char)ticket_num);
      } else if(msg.indexOf("READ_ARCHIVE")) {
        sendReadingsArchive((unsigned char)ticket_num);
      } else if(msg.indexOf("ERRORS") != -1) {

      } else if(msg.indexOf("SETTINGS") != -1) {

      } else {
        String reply = "hello from lobitos, i got " + msg;
        mesh.sendSingle(from, reply); 
      }
    }
  });
  setTime(now());
}


void loop() 
{
  if(timeStatus() == timeSet || timeStatus() == timeNeedsSync) {
    if(now() >= prev_time + (minutes_interval * 60)){
      Serial.println("Logging reading");
      logReading(readings_path);
      prev_time = now();
    }
  } else {
    Serial.println(timeStatus() + ":" + String(now()));
  }
  mesh.update();
}
