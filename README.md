# Arduino core for the Jolly's AVR ATMEGA328PB

### Contents
  - [Documentation](#documentation)
  - [Issue/Bug report template](#issuebug-report-template)

### Documentation

**Getting Started**
Jolly is a new electronic module for makers and Arduino UNO lovers. 
The module has integrated Wi-Fi, to infinitely extend the potential of your projects into the new world of IoT.
You only need to remove the ATMEGA328P microcontroller, replace it with the Jolly module and you will have an IoT board with Wi-Fi.
To date, all the existing projects are mutually compatible.

**Installing (Windows, Linux and macOS)**
To install the Jolly AVR platform in the Arduino IDE it is necessary to follow the following steps:
  - select the Preferences tab in the file menu
  - add the following two links to the **Additional Boards Manager URLs**
    https://tech-jollydev.github.io/package_jolly_index.json
    https://espressif.github.io/arduino-esp32/package_esp32_index.json
  - select the board menu in the tools menu and finally select the boards manager option
  - by typing **jolly** in the search bar the **Jolly AVR Boards** platform should appears
  - now the platform can be installed

**Reload the ESP8285 default firmware**
A pre-compiled version of the default ESP8285 firmware is available under the path /firmwares/jolly_esp/
The firmware is composed by two files: WiFiManager.bin and WiFiManager_spiff.bin
In case the firmware needs to be updated for any reason, there is a specific semi-automatic procedure to follow:
  - in Arduino IDE select the tools menu
  - choose ESP8285 for the bootloader option as shown in the picture below![](/bootimg.png)
  - press the boot button located on the Jolly module. While holding it pressed, press the reset button on the Arduino UNO for 1 second, then release it. Keep holding the boot button for 1 second longer, then release it ![](/boot_sequence.png) Now the Jolly is entered in the boot mode for the ESP8285
  - finally click on **burn bootloader** option and the procedure will start automatically

**Hint**: to keep track of the update process, enable both the **compilation** and **upload** radio button in the **show verbose output during:** option located in the file menu Preferences tab of the Arduino IDE

**Troubleshooting**

### Issue/Bug report template
Before reporting an issue, make sure you've searched for similar one that was already created.
Finally, and only for severe issues, write an e-mail to tech@jolly-dev.com
