/**
  ******************************************************************************
  * @file    bridge_node.ino
  * @author  T. Buckingham
  * @brief   Sketch file for the bridge node of the mesh network using HTTP requests
  *           with a ticket system [see readings_node.ino] for 
  *
  *          The file contains ::
  *           + Functions for querying nodes and handling user requests
	*						+ Testing html to be used without a client program
  *
  * FOR MORE DETAILS ON LIBRARIES OR EXAMPLES USED PLEASE SEE THE README
  *
  ******************************************************************************
  */

/*-- Includes --*/

#include "IPAddress.h"         // IP address handling
#include "painlessMesh.h"      // Mesh network header
#include "namedMesh.h"         // Mesh network with names implemented - see Nodes GitHub version for any changes
#include <map>                 // C++ map
#include <ArduinoJson.h>       // JSON parsing and encoding

#ifdef ESP8266
#include "Hash.h"
#include <ESPAsyncTCP.h>
#else
#include <AsyncTCP.h>           // Async handling header
#endif
#include <ESPAsyncWebServer.h>  // Web server header

/*-- Global Definitions --*/

#define   MESH_PREFIX     "whateverYouLike"
#define   MESH_PASSWORD   "somethingSneaky"
#define   MESH_PORT       5555

#define   STATION_SSID     "----"
#define   STATION_PASSWORD "----"

#define HOSTNAME "HTTP_BRIDGE"

/*-- Global Variables --*/
       
String from_node;                  /* What node replied [may no longer be use in later in development] */
String node_reply;              /* What the node replied  [may no longer be use in later in development] */
std::map<unsigned char, String> client_requests;  /* Maps the ticket number to the reply */
namedMesh  mesh;                      /* namedMesh network class, used to interface with the network */
String nodeName = "root";                             /* Name for this specific node */
unsigned char current_ticket = 0;               /* Current ticket number to give to requests */
std::map<IPAddress, String> client_connections;           /* USer connections map */

const char* client_username = "admin";
const char* client_password = "admin";

/*-- Prototypes --*/

//void receivedCallback( const uint32_t &from, const String &msg );
IPAddress getlocalIP();
AsyncWebServer server(80);
IPAddress myIP(0,0,0,0);
IPAddress myAPIP(0,0,0,0);

void setup() 
{
  // Serial and mesh init
  Serial.begin(115200);
  mesh.setDebugMsgTypes( ERROR | STARTUP | CONNECTION );  // set before init() so that you can see startup messages
  mesh.init( MESH_PREFIX, MESH_PASSWORD, MESH_PORT, WIFI_AP_STA, 6 );
  mesh.setName(nodeName);
 

  //mesh.onReceive(&receivedCallback);
  mesh.onReceive([](String &from, String &msg) { // add node reply to the map
      from_node  = from; 
      node_reply = msg;
      char buffer[4];
      int i = 0;
      while(msg.c_str()[i] != ':') {
        buffer[i] = msg.c_str()[i];
        i++;
      }
      buffer[i] = '\0';
      client_requests[(unsigned char)sscanf(buffer, "%d", &i)] = msg;
      Serial.printf("bridge: Received from %u msg=%s\n", from, msg.c_str());
  });

  mesh.stationManual(STATION_SSID, STATION_PASSWORD); // connect to network outside of mesh
  mesh.setHostname(HOSTNAME);

  mesh.setRoot(true); // bridge should be root
  mesh.setContainsRoot(true);

  myAPIP = IPAddress(mesh.getAPIP());
  Serial.println("My AP IP is " + myAPIP.toString());

  server.on("/request", HTTP_GET, [](AsyncWebServerRequest *request) 
  {
    if(!request->authenticate(client_username, client_password))
      return request->requestAuthentication();
    String empty = "";
    client_connections.insert({request->client()->remoteIP(), empty});
    //request->send(200, "text/html", "<form>Text to Broadcast<br><input type='text' name='NODE'><br><br><input type='text' name='BROADCAST'><br><br><input type='submit' value='Submit'><br><br><p></p></form>");
    if (request->hasArg("BROADCAST")){
      current_ticket++;
      String msg = String(current_ticket);
      String node = request->arg("NODE");
      msg += ":";
      msg += request->arg("BROADCAST");
      mesh.sendSingle(node, msg);
      //t1.setInterval(random( TASK_SECOND * 1, TASK_SECOND * 5 ));
      
      //request->send(200, "text/html", "<form>Text to Broadcast<br><input type='text' name='NODE'><br><br><input type='text' name='BROADCAST'><br><br><input type='submit' value='Submit'><br><br><p>"+ String(current_ticket) +"</p></form>");
      request->send(200, "text/plain", String(current_ticket));
      client_requests.insert({current_ticket, empty});
      Serial.println(node_reply);
    } else {
      request->send(404, "text/plain", "Invalid");
    }
    //}
  });

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    request->redirect("/request");
  });

  server.on("/check", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    if(!request->authenticate(client_username, client_password)) {
      request->redirect("/request");
    }
    String tickets = "";
    for(auto const& pr : client_requests) {
      tickets += String(pr.first);
      tickets += ":";
    }
    request->send(200, "text/plain", tickets);
  });

  server.on("/get-request", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    if(!request->authenticate(client_username, client_password))
      request->redirect("/request");
    //request->send(200, "text/html", "<form>Text to Broadcast<br><input type='text' name='TICKET'><br><br><input type='submit' value='Submit'></form>");
    if (request->hasArg("TICKET")){
      unsigned char client_request = (unsigned char) request->arg("TICKET").toInt();
      auto search = client_requests.find(client_request);
       if(search != client_requests.end()) {
         if(client_requests[client_request] != "") {
            request->send(200, "text/plain", client_requests[client_request]);
         } else {
            request->send(200, "text/plain", "request not ready");
         }
       } else {
         request->send(200, "text/plain", "ticket not found");
       }
      client_requests.erase(client_request);
     }
  });

  server.on("/nodes", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    if(!request->authenticate(client_username, client_password))
      request->redirect("/request");
      auto nodes = mesh.getNodeList(true);
      String str;
      for (auto &&id : nodes) {
        str += "ID : " + String(id) + String(", Name : ");
        for (auto && pr : mesh.getnameMap()) {
                if (id == (pr.first)) {
                    str += pr.second;
                    str += "\n";
                } else {
                    str = " ";
                }
            }
      }
      request->send(200, "text/plain", str.c_str());
  });
  server.begin();

}

void loop() 
{
  mesh.update();
  if(myIP != getlocalIP()){
    myIP = getlocalIP();
    Serial.println("My IP is " + myIP.toString());
  }
}

//void receivedCallback( const uint32_t &from, const String &msg ) 
//{
//  from_node  = from; 
//  node_reply = msg;
//  Serial.printf("bridge: Received from %u msg=%s\n", from, msg.c_str());
//}

IPAddress getlocalIP() 
{
  return IPAddress(mesh.getStationIP());
}