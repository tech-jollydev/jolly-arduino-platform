/*   ConnectNoEncryption.ino
*
*    This example is useful to show you how to connect to an open (no encryption)
*    wifi network.
*    The only thing you have to supply to the sketch is the SSID (name) of the network 
*    you want to connect to.
*
*    To see the behaviour of this example you have to ensure that the network you
*    want to connect to is "reachable", up and running. Then upload 
*    this sketch on your board and open a serial monitor to read the output 
*    informations supplied.
*/

#include <WiFi.h>

char ssid[] = "yourNetwork";     //  your network SSID (name)

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
    if(WiFi.begin(ssid) != WL_CONNECTED){
      if(WiFi.status()!= WL_CONNECTED){
	    /*
           if the connection fails due to an error...
        */
        Serial.println("Connection error! Check ssid and password and try again.");
      }
    }
    else{
      /*
           if the connection fails due to an error...
      */
      Serial.print("You're connected to the ");
      printCurrentNet();
      printWifiData();
    }
  }
}

void loop() {
 /*
     each two seconds print out the network data.
  */
  delay(2000);
  printCurrentNet();
  printWifiData();
}

void printMacAddress(void)
{
  // the MAC address of your esp module
  byte mac[6];

  // print your MAC address:
  WiFi.macAddress(mac);
  Serial.print("MAC: ");
  Serial.print(mac[5], HEX);
  Serial.print(":");
  Serial.print(mac[4], HEX);
  Serial.print(":");
  Serial.print(mac[3], HEX);
  Serial.print(":");
  Serial.print(mac[2], HEX);
  Serial.print(":");
  Serial.print(mac[1], HEX);
  Serial.print(":");
  Serial.println(mac[0], HEX);
}

void printEncryptionType(int thisType) {
  // read the encryption type and print out the name:
  switch (thisType) {
    case ENC_TYPE_WEP:
    Serial.println("WEP");
    break;
    case ENC_TYPE_TKIP:
    Serial.println("WPA");
    break;
    case ENC_TYPE_CCMP:
    Serial.println("WPA2");
    break;
    case ENC_TYPE_NONE:
    Serial.println("None");
    break;
    case ENC_TYPE_AUTO:
    Serial.println("Auto");
    break;
    case ENC_TYPE_UNKNOW:
    Serial.println("Unknown");
    break;
  }
}

/*
  the BSSID is the MAC address of the wireless access point (WAP)
  generated by combining the 24 bit Organization Unique Identifier
  (the manufacturer's identity) and the manufacturer's assigned
  24-bit identifier for the radio chipset in the WAP.
*/
void printBSSID(){
  uint8_t bssid[6];
  WiFi.BSSID(bssid);
  
  Serial.print("BSSID: ");
  Serial.print(bssid[5], HEX);
  Serial.print(":");
  Serial.print(bssid[4], HEX);
  Serial.print(":");
  Serial.print(bssid[3], HEX);
  Serial.print(":");
  Serial.print(bssid[2], HEX);
  Serial.print(":");
  Serial.print(bssid[1], HEX);
  Serial.print(":");
  Serial.println(bssid[0], HEX);
  
}

void printWifiData()
{
  /*
     print out the IP Address the gateway assigned to
     this board during the connection precedure and print
     the ESP MAC Address and the gateway BSSID.
  */
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  printMacAddress();  
  printBSSID();
  
  Serial.println("");
}

void printCurrentNet()
{
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}
