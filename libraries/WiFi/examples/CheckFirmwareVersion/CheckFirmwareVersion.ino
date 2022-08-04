/*   CheckFirmwareVersion.ino
 *
 *    This example is useful to know which version of the firmware is currently
 *    running on your board and to recognize if an update is required or not to
 *    ensure a seamless functionality.
 *
 *    To see the behaviour of this sketch you have to upload it on your board 
 *    and open a serial monitor to read the output informations supplied.
 */

#include <WiFi.h>

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

  Serial.println("esp WiFi firmware check.");
  Serial.println();
}

void loop() {
  firmwareCheck();
}

void firmwareCheck(void)
{
  if (WiFi.status() == WL_NO_WIFI_MODULE_COMM){
    /*
       notify the user if the previous sequence of operations to establish
       the communication between the ESP and the main mcu fail.
    */
    Serial.println("Communication with WiFi module not established.");
  }
  else {
    /*
       otherwise we have a correct communication between the two chips
       and we can print out the firmware version
    */
    String fv = WiFi.firmwareVersion();
    Serial.print("Firmware version installed: ");
    Serial.println(fv);

    // Print required firmware version
    Serial.print("Firmware version required : ");
    Serial.println(WIFI_FIRMWARE_REQUIRED);

    // Check if the required version is installed
    Serial.println();
    if (WiFi.checkFirmwareVersion(WIFI_FIRMWARE_REQUIRED)) {
      Serial.println("Check result: PASSED");
    }
    else {
      Serial.println("Check result: NOT PASSED");
      Serial.println(" - The firmware version installed do not match the");
      Serial.println("   version required by the library, you may experience");
      Serial.println("   issues or failures.");
    }
  }

  delay(1000);
}
