/* WiFiChatServer.ino
 * 
 * A simple server that distributes any incoming messages to all
 * connected clients. You can see the client's input in the serial monitor as well.
 *
 * Change ssid and password accordingly to your network. Once connected,
 * open a telnet client at the given ip address. The ip address
 * will show up on the Serial monitor after connection.
 */



#include <WiFi.h>

// server will listen on port 23
WiFiServer server(23);

char ssid[] = "yourNetwork";     //  your network SSID (name)
char pass[] = "secretPassword";  // your network password

boolean alreadyConnected = false; // whether or not the client was connected previously


void setup() {
  Serial.begin(115200);
  Serial.println("Checking WiFi linkage");

  /*
	  begin ESP8266 chip: these functions perform the chip reset and
	  initialization to ensure that the communication between ESP8266
	  and the main mcu starts in a known fashion.
  */
  WiFi.reset();
  WiFi.init(AP_STA_MODE);

  if (WiFi.status() == WL_NO_WIFI_MODULE_COMM) {
  	/*
       notify the user if the previous sequence of operations to establish
       the communication between the ESP and the main mcu fail.
    */
    Serial.println("Communication with WiFi module not established.");
	while(1);
  }
  else {
    /*
       otherwise we have a correct communication between the two chips
       and we can connect to a preferred network
    */
    Serial.println("\nWiFi module linked!");
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);

    /*
       this is one of the way (suppling SSID and PWD) that can be used to
       establish a connection to a network - other ways are showed in other
       examples, please check them out.
    */
    if (WiFi.begin(ssid, pass) != WL_CONNECTED) {
      if (WiFi.status() != WL_CONNECTED) {
        /*
           if the connection fails due to an error...
        */
        Serial.println("Connection error! Check ssid and password and try again.");
      }
    }
    else {
      /*
         you're now connected, so print out the network data
      */
      Serial.println("You're connected to the network ");
      printWifiStatus();
    }
    server.begin();
  }
}

void loop() {
  // listen for clients
  WiFiClient client = server.available();


  // when the client sends the first byte, say hello:
  while (client.connected()) {
    if (!alreadyConnected) {
      // clear out the input buffer:
      client.flush();
      Serial.println("We have a new client");
      client.println("Hello, client!");
      alreadyConnected = true;
    }

    if (client.available() > 0) {
      // read the bytes incoming from the client:
      char thisChar = client.read();
      // echo the bytes back to the client:
      server.write(thisChar);
      // echo the bytes to the server as well:
      Serial.write(thisChar);
    }
  }
}

void printWifiStatus()
{
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}
