/*
  Steps:
  1. Connect to the access point.
  2. Point your web browser to http://192.168.4.1
*/
#define WEBSOCKETS_TCP_TIMEOUT (100)
#define WIFI_CLIENT_SELECT_TIMEOUT_US (10000)
#define WIFI_CLIENT_MAX_WRITE_RETRY   (1)
#define DEBUG_WEBSOCKETS
#define DEBUG_ESP_PORT Serial

#include <ctype.h>
#include <ArduinoJson.h>
#include <ESP32_Servo.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <WebSocketsServer.h>
#include <string.h>


const char *indexPage =
#include "index.html"
	; // ignore the error. it is working.


// Set these to your desired credentials.
const char *ssid = "TP-Link_D15F";
const char *password = "69459191";

WiFiServer server(80);
WebSocketsServer webSocket(81);
bool ready = true;


SemaphoreHandle_t sem;
TaskHandle_t Task1;
uint8_t *payload_current = NULL;

DynamicJsonDocument doc(1024);
JsonObject obj;
Servo yaw;
int t0 = millis();

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) { // When a WebSocket message is received
  switch (type) {
    case WStype_DISCONNECTED:             // if the websocket is disconnected
      Serial.printf("[%u] Disconnected!\n", num);
    case WStype_CONNECTED: {              // if a new websocket connection is established
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
      }
    case WStype_TEXT:  {
        // if new text data is received
        Serial.printf("[%u] get Text: %s \n", num, payload);
        //ready = true; // "ok" received. 
        if (xSemaphoreTake(sem, 20)){
          payload_current = payload;
          
        }else{
          Serial.println("sem timeout");
        }
        xSemaphoreGive(sem);
        
      }                   

  }
}


void startWebSocket() { // Start a WebSocket server
  webSocket.begin();                          // start the websocket server
  webSocket.onEvent(webSocketEvent);          // if there's an incomming websocket message, go to function 'webSocketEvent'
  
  Serial.println("WebSocket server started.");
}



void Task1code( void * pvParameters ){


  Servo yaw;
  yaw.attach(2);
  DynamicJsonDocument doc(1024);
  for (;;){  //create an infinate loop

    xSemaphoreTake(sem, portMAX_DELAY);
    deserializeJson(doc, payload_current);
    Serial.println("semahphore taken");
    
    xSemaphoreGive(sem);

    JsonObject obj = doc.as<JsonObject>();
    
    if (obj.containsKey("yaw") && obj["yaw"] != ""){
      
      int angle = obj["yaw"];
      Serial.println(String(angle));
      yaw.write(angle);
      
    }
    
    
    delay(20); // prevent the idle task watchdog from triggering
  }
}

void setup() {
  
  Serial.begin(115200);



  sem = xSemaphoreCreateBinary();
  xSemaphoreGive(sem);
  xTaskCreatePinnedToCore(
                    Task1code,   /* Task function. */
                    "Task1",     /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    2,           /* priority of the task */
                    // when priority was set to 100 then network scan would not complete
                    &Task1,      /* Task handle to keep track of created task */
                    0);          /* pin task to core 0 */



  Serial.println();
  Serial.println("Connecting to WiFi");
  
  // You can remove the password parameter if you want the AP to be open.
  WiFi.begin(ssid, password);
  
  delay(500);
  if (WiFi.status() == WL_CONNECTED) {
    // Print local IP address and start web server
    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }else{
  Serial.println("Unable to connect to WiFi. Configuring access point...");
  WiFi.softAP(ssid, password);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  }
  server.begin();
  startWebSocket();
 // WiFi.setTxPower(WIFI_POWER_19_5dBm);
  server.setTimeout(1);
  webSocket.enableHeartbeat(100,100,200);
}






void loop() {
  
  

  if (millis()-t0 > 100){
    t0 = millis();
    String msg = "";
    //if (Serial.available()){
    //  msg = Serial.readString();	//read Serial       
    //}
    ready = false; // client is not ready to receive more data until we receive an "ok" through the websocket.      
   //  Serial.println("tx.");  
    webSocket.broadcastTXT("{\"y\":"+String(analogRead(34))+", \"x\":" + String(millis())+", \"msg\": \""+ msg+"\"}"); // send serial data over websocket.
    
  }
  //Serial.println("Clients: "+String(webSocket.connectedClients()));
  String msg = "";
 // Serial.println("ping.");
  //webSocket.broadcastPing(msg);
  
  // Serial.println("rx");
  webSocket.loop();

  
  WiFiClient client = server.available();   // listen for incoming clients

  if (client) {                             // if you get a client,
    Serial.println("New Client.");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        if (c == '\n') {                    // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            //client.println("Connection: Keep-Alive");
            client.println();

            client.print(indexPage);
            

            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          } else {    // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }

        // Check to see if the client request was "GET /H"
        if (currentLine.endsWith("GET /H")) {
        }
      }
    }
    // close the connection:
    client.stop();
    Serial.println("Client Disconnected.");
  }
}

