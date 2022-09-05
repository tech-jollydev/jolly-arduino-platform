/*
This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef WiFi_h
#define WiFi_h

#define WIFI_FIRMWARE_REQUIRED "1.0.0"

#include <inttypes.h>

#include "utility/definitions.h"

#if defined(__AVR_ATmega328P__) && defined(ESP_CH_UART)
#include "utility/uart/SC16IS750.h"
#endif

#include "IPAddress.h"
#include "WiFiClient.h"
#include "WiFiServer.h"
#include "utility/definitions.h"

#define GENERAL_TIMEOUT		10000
#define MAX_HOSTNAME_LEN	32

/*  -----------------------------------------------------------------
* Structure to memorize network names and relative string len.
*/
typedef struct{
	int32_t rssi;
	uint8_t enc;
	uint8_t SSID_len;
	char SSID_name[33];
}networkParams_s;

typedef enum
{
	UNCONNECTED = -1,
	AP_MODE = 0,
	STA_MODE,
	AP_STA_MODE,
} teConnectionMode;

typedef struct __attribute__((__packed__))
{
	uint8_t cmdType;
	uint8_t cmd;
	uint32_t totalLen;
	uint32_t receivedLen;
	bool endReceived;
} tsDataPacket;

typedef struct __attribute__((__packed__))
{
	uint8_t cmdType;
	uint8_t cmd;
	uint8_t totalLen;
	uint8_t nParam;
	uint8_t* dataPtr;
} tsNewCmd;

class WiFiClass
{
	private:
	volatile static tsDataPacket dataPkt;
	volatile static tsNewCmd cmdPkt;
	
	public:
	static uint8_t hostname[MAX_HOSTNAME_LEN];
	static uint8_t _state[MAX_SOCK_NUM];
	static uint16_t _server_port[MAX_SOCK_NUM];
	static uint16_t _client_data[MAX_SOCK_NUM];

	static bool gotResponse;
	static uint8_t responseType;
	static uint32_t dataLen;
	static uint8_t* data;
	static wl_status_t connectionStatus;
	static bool notify;
	static int32_t pktLen;

	WiFiClass();
	
	static void handleEvents(void);
	static void init();
	static void init(teConnectionMode connectionMode = AP_STA_MODE);
	static uint16_t getAvailableData();
	static void cancelNetworkListMem();
	/*
	* Get the first socket available
	*/
	static uint8_t getSocket();

	/*
	* Get firmware version
	*/
	static char* firmwareVersion();

	/*
	* Check the firmware version required
	*/
	bool checkFirmwareVersion(String fw_required);

	wl_status_t begin();
	
	/* Start Wifi connection for OPEN networks
	*
	* param ssid: Pointer to the SSID string.
	*/
	wl_status_t begin(char* ssid);

	/* Start Wifi connection with WEP encryption.
	* Configure a key into the device. The key type (WEP-40, WEP-104)
	* is determined by the size of the key (5 bytes for WEP-40, 13 bytes for WEP-104).
	*
	* param ssid: Pointer to the SSID string.
	* param key_idx: The key index to set. Valid values are 0-3.
	* param key: Key input buffer.
	*/
	wl_status_t begin(char* ssid, uint8_t key_idx, const char* key);

	/* Start Wifi connection with passphrase
	* the most secure supported mode will be automatically selected
	*
	* param ssid: Pointer to the SSID string.
	* param passphrase: Passphrase. Valid characters in a passphrase
	*        must be between ASCII 32-126 (decimal).
	*/
	wl_status_t begin(char* ssid, const char *passphrase);

	/* Change Ip configuration settings disabling the dhcp client
	*
	* param local_ip: 	Static ip configuration
	*/
	void config(IPAddress local_ip);

	/* Change Ip configuration settings disabling the dhcp client
	*
	* param local_ip: 	Static ip configuration
	* param dns_server:     IP configuration for DNS server 1
	*/
	void config(IPAddress local_ip, IPAddress dns_server);

	/* Change Ip configuration settings disabling the dhcp client
	*
	* param local_ip: 	Static ip configuration
	* param dns_server:     IP configuration for DNS server 1
	* param gateway : 	Static gateway configuration
	*/
	void config(IPAddress local_ip, IPAddress dns_server, IPAddress gateway);

	/* Change Ip configuration settings disabling the dhcp client
	*
	* param local_ip: 	Static ip configuration
	* param dns_server:     IP configuration for DNS server 1
	* param gateway: 	Static gateway configuration
	* param subnet:		Static Subnet mask
	*/
	void config(IPAddress local_ip, IPAddress dns_server, IPAddress gateway, IPAddress subnet);

	/* Change DNS Ip configuration
	*
	* param dns_server1: ip configuration for DNS server 1
	*/
	void setDNS(IPAddress dns_server1);

	/* Change DNS Ip configuration
	*
	* param dns_server1: ip configuration for DNS server 1
	* param dns_server2: ip configuration for DNS server 2
	*
	*/
	void setDNS(IPAddress dns_server1, IPAddress dns_server2);

	/*
	* Disconnect from the network
	*
	* return: one value of wl_status_t enum
	*/
	int disconnect(void);

	/*
	* Get the interface MAC address.
	*
	* return: pointer to uint8_t array with length WL_MAC_ADDR_LENGTH
	*/
	void macAddress(uint8_t* mac);

	/*
	* Get the interface IP address.
	*
	* return: Ip address value
	*/
	IPAddress localIP();

	/*
	* Get the interface subnet mask address.
	*
	* return: subnet mask address value
	*/
	IPAddress subnetMask();

	/*
	* Get the gateway ip address.
	*
	* return: gateway ip address value
	*/
	IPAddress gatewayIP();

	/*
	* Return the current SSID associated with the network
	*
	* return: ssid string
	*/
	char* SSID();

	/*
	* Return the current BSSID associated with the network.
	* It is the MAC address of the Access Point
	*
	* return: pointer to uint8_t array with length WL_MAC_ADDR_LENGTH
	*/
	bool BSSID(uint8_t* bssid);

	/*
	* Return the current RSSI /Received Signal Strength in dBm)
	* associated with the network
	*
	* return: signed value
	*/
	int32_t RSSI();

	/*
	* Return the Encryption Type associated with the network
	*
	* return: one value of wl_enc_type enum
	*/
	uint8_t	encryptionType();

	/*
	* Start scan WiFi networks available
	*
	* return: Number of discovered networks
	*/
	int8_t scanNetworks();
	
	char* getHostname();
	bool setHostname(char* name);
	
	/*
	*
	*/
	uint8_t getScannedNetwork(uint8_t netNum, char *ssid, int32_t& rssi, uint8_t& enc);

	/*
	* Return the SSID discovered during the network scan.
	*
	* param networkItem: specify from which network item want to get the information
	*
	* return: ssid string of the specified item on the networks scanned list
	*/
	char*	SSID(uint8_t networkItem);

	/*
	* Return the encryption type of the networks discovered during the scanNetworks
	*
	* param networkItem: specify from which network item want to get the information
	*
	* return: encryption type (enum wl_enc_type) of the specified item on the networks scanned list
	*/
	uint8_t	encryptionType(uint8_t networkItem);

	/*
	* Return the RSSI of the networks discovered during the scanNetworks
	*
	* param networkItem: specify from which network item want to get the information
	*
	* return: signed value of RSSI of the specified item on the networks scanned list
	*/
	int32_t RSSI(uint8_t networkItem);

	/*
	* Return Connection status.
	*
	* return: one of the value defined in wl_status_t
	*/
	wl_status_t status();
	
	void reset();
	void off();
	void on();

	/*
	* Resolve the given hostname to an IP address.
	* param aHostname: Name to be resolved
	* param aResult: IPAddress structure to store the returned IP address
	* result: 1 if aIPAddrString was successfully converted to an IP address,
	*          else error code
	*/
	int hostByName(const char* aHostname, IPAddress& aResult);

	/*
	* Disable default web server at port 80 (pin control feature will be unavailable).
	*/
	void disableWebPanel();

	friend class WiFiClient;
	friend class WiFiServer;
	friend void wifiDrvCB(void);
	
};

extern WiFiClass WiFi;

#endif
