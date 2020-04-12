/*
  Steps:
  1. Connect to the access point.
  2. Point your web browser to http://192.168.4.1
*/

#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <WebSocketsServer.h>

const char *indexPage =
#include "index.html"
	; // ignore the error. it is working.


// Set these to your desired credentials.
const char *ssid = "TP-Link_D15F";
const char *password = "69459191";

WiFiServer server(80);
WebSocketsServer webSocket(81);

bool ready = true;
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) { // When a WebSocket message is received
  switch (type) {
    case WStype_DISCONNECTED:             // if the websocket is disconnected
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED: {              // if a new websocket connection is established
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
      }
      break;
    case WStype_TEXT:                     // if new text data is received
        Serial.printf("[%u] get Text: %s \n", num, payload);
        ready = true; // "ok" received. 

    case WStype_PONG:
        ready = true; // "ok" received. 
  }
}

void startWebSocket() { // Start a WebSocket server
  webSocket.begin();                          // start the websocket server
  webSocket.onEvent(webSocketEvent);          // if there's an incomming websocket message, go to function 'webSocketEvent'
  
  Serial.println("WebSocket server started.");
}

void setup() {

  Serial.begin(115200);
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
}

void loop() {
  
  if (ready){
    String msg = "";
    //if (Serial.available()){
    //  msg = Serial.readString();	//read Serial       
    //}
    ready = false; // client is not ready to receive more data until we receive an "ok" through the websocket.        
    webSocket.broadcastTXT("{\"y\":"+String(analogRead(34))+", \"x\":" + String(millis())+", \"msg\": \""+ msg+"\"}"); // send serial data over websocket.
  }
  String msg = "";
  webSocket.broadcastPing(msg);
  webSocket.loop();
  WiFiClient client = server.available();   // listen for incoming clients
  
  if (client) {                             // if you get a client,
    Serial.println("New Client.");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      webSocket.loop();
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

