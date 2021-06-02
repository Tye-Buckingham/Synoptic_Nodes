/**
  ******************************************************************************
  * @file    readings_node.ino
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
#include "FS.h"

/*-- Global Definitions --*/

#define   MESH_PREFIX     "whateverYouLike"
#define   MESH_PASSWORD   "somethingSneaky"
#define   MESH_PORT       5555

/*-- Global Variables --*/
Scheduler userScheduler;
const int chip_select = D8;
namedMesh  mesh;                /* namedMesh network class, used to interface with the network */
String nodeName = "lobitos";                   /* Name for this specific node */

String readings_path = "/readings.txt";
String errors_path   = "/errors.txt";
String settings_path = "/settings.txt";

/*-- Prototypes --*/

/** @brief Function used to send stored readings as a string to the bridge node when the user requests
 *  @param @param ticket_number the ticket that is currently being sent back to the bridge/root node
 *  @return Void.
 */
void sendReadings(unsigned char ticket_number);


/** @brief Basic function used to test the connections between the root during development
 *  @param Void.
 *  @return Void.
 */
void sendMessage();


/** @brief Appends a string, in this case a log, to the specified file
 *  @param fs a file system pointer used in interfacing with the file
 *  @param path the files path on the storage medium
 *  @param content the string to be appended to the file
 *  @return Void.
 */
void appendFile(fs::FS &fs, const char* path, const char* content);


/** @brief Writes a new file onto the storage device
 *  @param fs a file system pointer used in interfacing with the file
 *  @param path the files path on the storage medium
 *  @return Void.
 */
void writeFile(fs::FS &fs, const char* path);


/** @brief Records a reading to the readings files
 *  @param fs a file system pointer used in interfacing with the file
 *  @param path the files path on the storage medium
 *  @return Void.
 */
void logReading(fs::FS &fs, const char* path);


void logReading(fs::FS &fs, const char* path)
{

   String reading = "\"";
   reading += String(now()) + "\": [" ;
   reading += String(random(0, 250)) + "," + String(random(0, 250)) + "," + String(random(0, 250)) + "]";
   appendFile(SD, readings_path, reading);

}


void appendFile(fs::FS &fs, const char* path, const char* content)
{

  // Dont append with final closing }, this will be done when sending the string (along with the ticket number)
  File file = fs.open(path, FILE_APPEND);
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

void writeFile(fs::FS &fs, const char* path) 
{

  File file = fs.open(path, FILE_WRITE);
  if(!file) {
    Serial.println("File creation failed");
    return;
  }
  if(file.print("")) {
    Serial.println("Start to JSON file " + String(path) + " successfuly made");
  } else {
    Serial.println("Failed to write to file " + String(path));
  }
  file.close();


}


void sendReadings(unsigned char ticket_number)
{

   if (!SD.begin(chip_select)) {
    Serial.println("SD card Initialization failed");
    while (1);
  }
  String readings = "{\"ticket\":" + String(ticket_number)  + ",";
  File readings_file = SD.open(readings_path);
  if(readings_file) {
    while(readings_file.available()) {
      readings_file += readings_file.read();
  }
    readings_file.close();
    readings += "}";
    mesh.sendSingle(root, readings);
  } else {
    Serial.println("Error opening file");
  }
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

  if(!SD.begin()){
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
    if(msg.c_str()[0] == 'T') {
      char* ptr;
      char* temp = (char*)malloc((msg.length())*sizeof(char*));
      msg.remove(0, 1);
      msg.toCharArray(temp, msg.length() + 1);
      time_t new_time = strtoul(temp, &ptr, 10);
      setTime(new_time);
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
      } else if(msg.indexOf("ERRORS") != -1) {

      } else if(msg.indexOf("SETTINGS") != -1) {

      } else {
        String reply = "hello from lobitos, i got " + msg;
        mesh.sendSingle(from, reply); 
      }
    }
    //Serial.print("Now: ");
    //Serial.println(now());
    
  });

}



void loop() 
{
  mesh.update();
}
