/* ConnectToASavedNetwork.ino

   This example will show you how to use the auto-connect function supplied with the library.
   Thanks to this functionality, once you have successfully connected your board to a network,
   you will not need to re-enter the network informations for further connections (to the same
   network).
   This behaviour is achieved thanks to a memorization structure integrated into the ESP8266's
   firmware that is capable of storing the last 10 networks you have succesfully connected to.
   Once you have called the auto-connect function, it starts a network scan to find out if one
   (or more than one) netowork you have in your memory is reachable or not. If the last one
   connected is among them, than it choses that, otherwise it choses the one with the highest
   RSSI (signal strenght). In the case none of the stored network is reachable it throws an error.

   To see the behaviour of this example you have to upload this sketch on your board and open
   a serial monitor to read the output informations supplied.
*/

#include "WiFi.h"

uint8_t inByte = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("Checking WiFi linkage");

  /*
      begin ESP8266 chip: these functions perform the chip reset and
      initialization to ensure that the communication between ESP8266
      and ATMEGA328P starts in a known fashion.
  */
  WiFi.reset();
  WiFi.init(AP_STA_MODE);

  if (WiFi.status() == WL_NO_WIFI_MODULE_COMM) {
    /*
       notify the user if the previous sequence of operations to establish
       the communication between the ESP and the 328P fail.
    */
    Serial.println("Communication with WiFi module not established.");
  }
  else {
    /*
       otherwise we have a correct communication between the two chips
       and we can start the auto-connect feature
    */
    Serial.println("WiFi module linked!");
    Serial.println("Attempting to connect to a stored network");

    if (WiFi.begin() != WL_CONNECTED) {
      if (WiFi.status() != WL_CONNECTED) {
        /*
        	since none of the memorized network is reachable or the board has never
        	been connected to any network, connect your mobile or PC to the board's
        	access point, using your browser load the webpanel (192.168.240.1) and
        	use it to choose a new network to connect to. You can store up to 10
          networks
        */
        Serial.println("Connection error! None of the memorized network is reachable.");
        Serial.println("Connect your PC or mobile to the board's Access Point and using the webpanel connect it to a network.");
      }
    }
  }
}

void loop() {
  /*
     This sketch shows two more fuctions available within the library:
     - notity events (that help user to keep trace of the connection status)
     - memory erase function (to erase the memorized list of preferred network
     stored into the ESP8266).
  */

  /*
     In the case of a connect event, notify it to the user.
  */
  if (WiFi.status() == WL_CONNECTED && WiFi.notify == true) {
    WiFi.notify = false;
    Serial.print("You're connected to the network ");
    printWifiStatus();
    /*
    	now that your board is correctly connected to a network, send
    	a 'c' ascii character using a serial monitor to call the memory
    	erase function. In this way after the reset the auto-connect function
    	will throw an error.
    */
    Serial.println("Now reset the board to let the auto-connection function make the work for you!");
    Serial.println("Or you can send a 'c' character to erase all the memorized networks.");
  }
  /*
     In the case of a disconnect event, notify it to the user.
  */
  else if (WiFi.status() == WL_DISCONNECTED && WiFi.notify == true) {
    WiFi.notify = false;
    Serial.println("You've been disconnected");
  }

  /*
     the 'c' ascii character sent to the board through the serial monitor
     will call the memory erase function that will erase all the memorized
    network from your board.
  */
  if (Serial.available() > 0) {
    // get incoming byte:
    inByte = Serial.read();
    if (inByte == 'c') {
      WiFi.cancelNetworkListMem();
      Serial.println("Memorized networks list erased... reset the board to check.");
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