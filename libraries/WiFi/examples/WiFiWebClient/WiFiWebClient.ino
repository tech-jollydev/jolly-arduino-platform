/* WiFiWebClient.ino
 *
 * This example shows how to connect to a website and make
 * HTTP requests.
 *
 * Change ssid and password accordingly to your network.
 * Open the serial monitor to read replies to our requests.
 */

#include <WiFi.h>

//IPAddress server(74,125,232,128);  // numeric IP for Google (no DNS)
char server[] = "www.google.com";    // name address for Google (using DNS)

char ssid[] = "yourNetwork";     //  your network SSID (name)
char pass[] = "secretPassword";  // your network password

unsigned long lastConnectionTime = 0;            // last time you connected to the server, in milliseconds
const unsigned long postingInterval = 10L * 1000L; // delay between updates, in milliseconds

WiFiClient client;

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
      Serial.print("You're connected to the network ");
      printWifiStatus();
    }
  }

}

void loop() {
  if(WiFi.connectionStatus == WL_CONNECTED){
    // read server's reply
    while (client.available() > 0) {
      char c = client.read();
      Serial.write(c);
    }
    
    // if ten seconds have passed since your last connection,
    // then connect again and send data:
    if (millis() - lastConnectionTime > postingInterval) {
      httpRequest();
    }
  }
}


// this method makes a HTTP connection to the server:
void httpRequest() {
  // close any connection before send a new request.
  // This will free the socket on the WiFi shield
  client.stop();
  
  // if there's a successful connection:
  if (client.connect(server, 80)) {
    Serial.println("connecting...");
    // Make a HTTP request:
    client.println("GET /search?q=jolly%20module HTTP/1.1");
    client.println("Host: www.google.com");
    client.println("Connection: close");
    client.println();
    // note the time that the connection was made:
    lastConnectionTime = millis();
  }
  else {
    // if you couldn't make a connection:
    Serial.println("connection failed");
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
