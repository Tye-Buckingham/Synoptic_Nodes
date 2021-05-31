/*
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp-mesh-esp32-esp8266-painlessmesh/
  
  This is a simple example that uses the painlessMesh library: https://github.com/gmag11/painlessMesh/blob/master/examples/basic/basic.ino
*/

#include "painlessMesh.h"
#include "namedMesh.h"

#include <ArduinoJson.h>
#include <SD.h>
#include <SPI.h>

#define   MESH_PREFIX     "whateverYouLike"
#define   MESH_PASSWORD   "somethingSneaky"
#define   MESH_PORT       5555

Scheduler userScheduler; // to control your personal task
namedMesh  mesh;

String nodeName = "lobitos";


// User stub
void sendMessage() ; // Prototype so PlatformIO doesn't complain
void sendReadings(unsigned char ticket_number);


//Task taskSendMessage( TASK_SECOND * 1 , TASK_FOREVER, &sendMessage );

void sendMessage() 
{
  String msg = "Hi from node1";
  msg += mesh.getNodeId();
  String to = "root";
  mesh.sendSingle(to, msg);
  //taskSendMessage.setInterval( random( TASK_SECOND * 1, TASK_SECOND * 5 ));
}

void sendReadings(unsigned char ticket_number)
{

  /* Read from SD card 
  *  Format of string to be parsed :: 
  *  [ticket_number]:[data{timestamp,reading1,reading2,reading3}:{timestamp,reading1,reading2,reading3}]
  */
  /* File file = open("filename"); // each line will have a timestamp with readings in correct format
  *  String readings = String(ticket_number);
  *  while(file.nextline != EOF) {
  *     readings += file.nextline();
  *  }
  */
  String root = "root";
  String readings = String(ticket_number) + ":" + "1622119135383,9.214,3.45453,1.3424:1622119135383,12.354,3.44753,6.37424";
  mesh.sendSingle(root, readings);

}

void newConnectionCallback(uint32_t nodeId) 
{
    Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback() 
{
  Serial.printf("Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset) 
{
    Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(),offset);
}

void setup() 
{
  Serial.begin(115200);

//mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
  mesh.setDebugMsgTypes( ERROR | STARTUP ); 

  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );

  mesh.setName(nodeName); 

  mesh.onReceive([](String &from, String &msg) 
  {
    String reply = "hello from lobitos, i got " + msg;
    mesh.sendSingle(from, reply);
    Serial.printf("Received message by name from: %s, %s\n", from.c_str(), msg.c_str());
  });

}

void sendReadings()
{



}

//void receivedCallback( const uint32_t &from, const String &msg ) {
  //String rep = "High from lobitos";
  //mesh.sendSingle(from, rep);
//}

void loop() 
{
  mesh.update();
}
