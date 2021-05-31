//************************************************************
// building on the painlessMesh server example to create a node system
// that transfer data readings as a string, with one node acting as the bridge (this file)
// in the form of a HTTP server. using a ticket system to handle multiple requests.
// https://gitlab.com/painlessMesh/painlessMesh/wikis/bridge-between-mesh-and-another-network
// for more details about my version
// https://gitlab.com/Assassynv__V/painlessMesh
// and for more details about the AsyncWebserver library
// https://github.com/me-no-dev/ESPAsyncWebServer
//************************************************************

#include "IPAddress.h"
#include "painlessMesh.h"
#include "namedMesh.h"

#ifdef ESP8266
#include "Hash.h"
#include <ESPAsyncTCP.h>
#else
#include <AsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>

#define   MESH_PREFIX     "whateverYouLike"
#define   MESH_PASSWORD   "somethingSneaky"
#define   MESH_PORT       5555

#define   STATION_SSID     "*****"
#define   STATION_PASSWORD "*****"

#define HOSTNAME "HTTP_BRIDGE"
int returned = 0;
String from_node;
String node_reply;
std::map<unsigned char, String> client_requests;

//void receivedCallback( const uint32_t &from, const String &msg );
IPAddress getlocalIP();

namedMesh  mesh;

String nodeName = "root";
unsigned char current_ticket = 0;

AsyncWebServer server(80);
IPAddress myIP(0,0,0,0);
IPAddress myAPIP(0,0,0,0);

void setup() 
{
  Serial.begin(115200);

  mesh.setDebugMsgTypes( ERROR | STARTUP | CONNECTION );  // set before init() so that you can see startup messages

  mesh.init( MESH_PREFIX, MESH_PASSWORD, MESH_PORT, WIFI_AP_STA, 6 );

  mesh.setName(nodeName);
 

  //mesh.onReceive(&receivedCallback);
  mesh.onReceive([](String &from, String &msg) {
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

  mesh.stationManual(STATION_SSID, STATION_PASSWORD);
  mesh.setHostname(HOSTNAME);

  mesh.setRoot(true);
  mesh.setContainsRoot(true);

  myAPIP = IPAddress(mesh.getAPIP());
  Serial.println("My AP IP is " + myAPIP.toString());

  server.on("/request", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    request->send(200, "text/html", "<form>Text to Broadcast<br><input type='text' name='NODE'><br><br><input type='text' name='BROADCAST'><br><br><input type='submit' value='Submit'></form>");
    if (request->hasArg("BROADCAST")){
      String msg = request->arg("BROADCAST");
      String node = request->arg("NODE");
      mesh.sendSingle(node, msg);
      //t1.setInterval(random( TASK_SECOND * 1, TASK_SECOND * 5 ));
      int period = 3000;
      unsigned long time_now = 0;
      time_now = millis();
      while(millis() < time_now + period){
        //wait approx. [period] ms
      }
      Serial.println(node_reply);
      current_ticket++;
      request->send(200, "text/plain", String(current_ticket));
      String empty = "";
      client_requests.insert({current_ticket, empty});
    } else {
      request->send(404, "text/plain", "Invalid");
    }
    //}
  });

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    request->redirect("/request");
  });

  server.on("/get-request", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    request->send(200, "text/html", "<form>Text to Broadcast<br><input type='text' name='TICKET'><br><br><input type='submit' value='Submit'></form>");
     if (request->hasArg("TICKET")){
      unsigned char client_request = (unsigned char) request->arg("TICKET").toInt();
      request->send(200, "text/plain", client_requests[client_request]);
      client_requests.erase(client_request);
     }
  });

  server.on("/nodes", HTTP_GET, [](AsyncWebServerRequest *request)
  {
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