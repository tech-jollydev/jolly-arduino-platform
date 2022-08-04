/* WiFiRestServer.ino
 *
 * This example shows how to access digital and analog pins
 * of the board through REST calls. It demonstrates how you
 * can create your own API when using REST style calls through
 * the browser.
 *
 * Possible commands created in this shetch:
 *
 * "/jolly/digital/9"     -> digitalRead(9)
 * "/jolly/digital/9/1"   -> digitalWrite(9, HIGH)
 * "/jolly/analog/2/123"  -> analogWrite(2, 123)
 * "/jolly/analog/2"      -> analogRead(2)
 * "/jolly/mode/9/input"  -> pinMode(9, INPUT)
 * "/jolly/mode/9/output" -> pinMode(9, OUTPUT)
 *
 * Connect to the board's web panel through a browser (type the
 * board's ip if the board is already connected to a network or
 * connect your computer to the board's SSID and type the default
 * address 192.168.240.1 in order to access the web panel). Open
 * the tab "Wi-Fi Pin Control" to send commands to the board by
 * pressing the related buttons.
 * Alternatively you can directly send the REST command by typing
 * on a browser the board's address and the desired command.
 * e.g. http://yourIpAddress:8080/jolly/digital/9/1
 */


#include "WiFi.h"

// server will listen on port 8080
WiFiServer server(8080);
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
       the communication between the ESP and the 328P fail.
    */
    Serial.println("Communication with WiFi module not established.");
  }
  else {
	/*
       otherwise we have a correct communication between the two chips
       and we can connect to a preferred network
    */
    Serial.println("WiFi module linked!");

    Serial.println("Attempting to connect to a stored network");

	/*
       In this case we are trying to connect to a stored network.
	   Look at ConnectToASavedNetwork example to have further information.
    */
    if (WiFi.begin() != WL_CONNECTED) {

      if (WiFi.status() != WL_CONNECTED) {
		/*
           if the connection fails due to an error...
        */
        Serial.println("Connection error! Check ssid and password and try again.");
      }
    }
    server.begin();
  }


}

void loop() {
  // listen for incoming clients
  WiFiClient client = server.available();
  while (client.connected()) {
    // Process request
    process(client);
    // Close connection and free resources.
    delay(1);
    client.stop();
  }

  /*
     In the case of a connect event, notify it to the user.
  */
  if (WiFi.connectionStatus == WL_CONNECTED && WiFi.notify == true) {
    WiFi.notify = false;
    Serial.print("You're connected to the network ");
    printWifiStatus();
  }
  /*
     In the case of a disconnect event, notify it to the user.
  */
  else if (WiFi.connectionStatus == WL_DISCONNECTED && WiFi.notify == true) {
    WiFi.notify = false;
    Serial.println("You've been disconnected");
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

bool listen_service(WiFiClient client, String service)
{
  //check service
  String currentLine = "";
  while (client.connected()) {
    if (client.available() > 0) {
      char c = client.read();
      currentLine += c;
      if (c == '\n') {
        client.println("HTTP/1.1 200 OK");
        client.println();
        return 0;
      }
      else if (currentLine.endsWith(service + "/")) {
        return 1;
      }
    }
  }
}

// -----------------------------------------------------------------
void digitalCommand(WiFiClient client) {
  int pin, value;

  // Read pin number
  String pinNumber;
  char c = client.read();
  // If the next character is a '/' it means we have an URL
  // with a value like: "/digital/9/1"
  // If the next character is a ' ' it means we have an URL
  // with a value like: "/digital/9"

  while (c != ' ' && c != '/') {
    pinNumber += c;
    c = client.read();
  }
  pin = pinNumber.toInt();

  if (c == '/') {
    value = client.parseInt();
    //value = 1;
    digitalWrite(pin, value);

    // flush input before replying - needed by some browser
    client.flush();
    // Send feedback to client
    client.print("HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\n\r\nPin D" + String(pin) + " set to: " + String(value));
  }
  else {
    value = digitalRead(pin);

    // flush input before replying - needed by some browser
    client.flush();
    // Send feedback to client
    client.print("HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\n\r\nPin D" + String(pin) + " reads: " + String(value));
  }
}

// -----------------------------------------------------------------
void analogCommand(WiFiClient client) {
  int pin, value;
  // Read pin number
  String pinNumber;

  char c = client.read();

  // If the next character is a '/' it means we have an URL
  // with a value like: "/analog/5/120"
  // If the next character is a ' ' it means we have an URL
  // with a value like: "/analog/13"
  while (c != ' ' && c != '/') {
    pinNumber += c;
    c = client.read();
  }
  pin = pinNumber.toInt();
  // If the next character is a '/' it means we have an URL
  // with a value like: "/analog/5/120"
  if (c == '/') {
    // Read value and execute command
    String analogValue;
    char c = client.read();
    while (c != ' ' && c != '/') {
      analogValue += c;
      c = client.read();
    }
    value = analogValue.toInt();
    analogWrite(pin, value);

    // flush input before replying - needed by some browser
    client.flush();
    // Send feedback to client
    client.print("HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\n\r\nPin A" + String(pin) + " set to: " + String(value));
  }
  else {
    // Read analog pin
    value = analogRead(pin);

    // flush input before replying - needed by some browser
    client.flush();
    // Send feedback to client
    client.print("HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\n\r\nPin A" + String(pin) + " reads: " + String(value));
  }
}

// -----------------------------------------------------------------
void modeCommand(WiFiClient client) {
  int pin;
  char feedback[100];

  // Read pin number
  String pinNumber;
  char c = client.read();
  while (c != ' ' && c != '/') {
    pinNumber += c;
    c = client.read();
  }
  pin = pinNumber.toInt();
  // If the next character is not a '/' we have a malformed URL
  if (c != '/') {
    // flush input before replying - needed by some browser
    client.flush();
    client.print("HTTP/1.1 200 OK\r\n\r\nError!\r\n\r\n");

    return;
  }

  String mode = client.readStringUntil(' ');

  if (mode == "input") {
    pinMode(pin, INPUT);

    // flush input before replying - needed by some browser
    client.flush();
    // Send feedback to client
    client.print("HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\n\r\nPin D" + String(pin) + " configured as INPUT!");

    return;
  }

  if (mode == "output") {
    pinMode(pin, OUTPUT);

    // flush input before replying - needed by some browser
    client.flush();
    // Send feedback to client
    client.print("HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\n\r\nPin D" + String(pin) + " configured as OUTPUT!\r\n");
    return;
  }

  // flush input before replying - needed by some browser
  client.flush();
  client.print(String("HTTP/1.1 200 OK\r\n\r\nError!\r\nInvalid mode" + mode + "\r\n"));
}

// -----------------------------------------------------------------
void process(WiFiClient client)
{
  // read the command//
  if (listen_service(client, "jolly")) {
    String command = client.readStringUntil('/');

    if (command == "mode") {
      modeCommand(client);
    }
    else if (command == "digital") {
      digitalCommand(client);
    }
    else if (command == "analog") {
      analogCommand(client);
    }
    else {
      // flush input before replying - needed by some browser
      client.flush();
      client.print(String("HTTP/1.1 200 OK\r\n\r\nError!\r\nUnknown command : " + command + "\r\n\r\n"));
    }
  }
}
