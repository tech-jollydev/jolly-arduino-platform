/* WiFiUdpSendReceiveString.ino
 *
 * This sketch wait for an UDP packet on the localPort specified below.
 * When a packet is received an Acknowledge packet is sent to the client on port remotePort
 *
 * Change ssid and password accordingly to your network. Once connected,
 * open a udp socket on localPort at the given ip address. The ip address
 * will show up on the Serial monitor after connection.
 */

#include <WiFi.h>
#include <WiFiUdp.h>

char ssid[] = "yourNetwork";     //  your network SSID (name)
char pass[] = "secretPassword";  // your network password

unsigned int localPort = 2390;      // local port to listen on

char packetBuffer[256]; //buffer to hold incoming packet
char  ReplyBuffer[] = "acknowledged";       // a string to send back

WiFiUDP Udp;

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
  }
  else{
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
    if(WiFi.begin(ssid, pass) != WL_CONNECTED){
      if(WiFi.status()!= WL_CONNECTED){
	    /*
           if the connection fails due to an error...
        */
        Serial.println("Connection error! Check ssid and password and try again.");
      }
    }
    else{
      /*
         you're now connected, so print out the network data
      */
      Serial.print("You're connected to the ");
      printWifiStatus();
    }
  }
  Serial.println("\nStarting connection to the server...");
  Udp.begin(localPort);
}

void loop() {
    // if there's data available, read a packet
    int packetSize = Udp.parsePacket();
    if (packetSize > 0)
    {
      Serial.print("Received packet of size ");
      Serial.println(packetSize);
      Serial.print("From ");
      IPAddress remoteIp = Udp.remoteIP();
      Serial.print(remoteIp);
      Serial.print(", port ");
      Serial.println(Udp.remotePort());

      // read the packet into packetBufffer
      int len = Udp.read(packetBuffer, 512);
      if (len > 0) packetBuffer[len] = 0;
      Serial.println("Contents:");
      Serial.println(packetBuffer);

      // send a reply, to the IP address and port that sent us the packet we received
      Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
      Udp.write(ReplyBuffer);
      Udp.endPacket();
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