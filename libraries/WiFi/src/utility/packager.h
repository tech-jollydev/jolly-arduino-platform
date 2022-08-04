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

#ifndef PACKAGER_H
#define PACKAGER_H

#include <inttypes.h>
#include "utility/definitions.h"
#include "IPAddress.h"

// Key index length
#define KEY_IDX_LEN     1
// firmware version string length
#define WL_FW_VER_LENGTH 6


typedef struct __attribute__((__packed__))
{
	uint8_t cmdType;
	uint8_t cmd;
	uint32_t totalLen;
	uint8_t sockNum;
	uint16_t* dataPtr;
} tsNewData;

typedef enum eProtMode {TCP_MODE, UDP_MODE}tProtMode;

class Packager
{
private:
	// settings of requested network
	static char 	_networkSsid[WL_NETWORKS_LIST_MAXNUM][WL_SSID_MAX_LENGTH];
	
	// firmware version string in the format a.b.c
	static char 	fwVersion[WL_FW_VER_LENGTH];

	// settings of current selected network
	static char 	_ssid[WL_SSID_MAX_LENGTH];

	/*
	 * Get network Data information
	 */
    static bool getNetworkData();
    static bool getHostByName(const char* aHostname);

    /*
     * Get remote Data information on UDP socket
     */
    static bool getRemoteData(uint8_t sock);

public:
	
	static bool getHostname();
	static bool setHostname(const char* name);
	
	static bool wifiAutoConnect();
	static void wifiCancelNetList();
	
    /*
     * Set the desired network which the connection manager should try to
     * connect to.
     *
     * The ssid of the desired network should be specified.
     *
     * param ssid: The ssid of the desired network.
     * param ssid_len: Lenght of ssid string.
     * return: WL_SUCCESS or WL_FAILURE
	 */
    static bool wifiSetNetwork(char* ssid, uint8_t ssid_len);

    /* Start Wifi connection with passphrase
     * the most secure supported mode will be automatically selected
     *
     * param ssid: Pointer to the SSID string.
     * param ssid_len: Lenght of ssid string.
     * param passphrase: Passphrase. Valid characters in a passphrase
     *        must be between ASCII 32-126 (decimal).
     * param len: Lenght of passphrase string.
     * return: WL_SUCCESS or WL_FAILURE
     */
    static bool wifiSetPassphrase(char* ssid, uint8_t ssid_len, const char *passphrase, const uint8_t len);

    /* Start Wifi connection with WEP encryption.
     * Configure a key into the device. The key type (WEP-40, WEP-104)
     * is determined by the size of the key (5 bytes for WEP-40, 13 bytes for WEP-104).
     *
     * param ssid: Pointer to the SSID string.
     * param ssid_len: Lenght of ssid string.
     * param key_idx: The key index to set. Valid values are 0-3.
     * param key: Key input buffer.
     * param len: Lenght of key string.
     * return: WL_SUCCESS or WL_FAILURE
     */
    static bool wifiSetKey(char* ssid, uint8_t ssid_len, uint8_t key_idx, const void *key, const uint8_t len);

    /* Set ip configuration disabling dhcp client
        *
        * param validParams: set the number of parameters that we want to change
        * 					 i.e. validParams = 1 means that we'll change only ip address
        * 					 	  validParams = 3 means that we'll change ip address, gateway and netmask
        * param local_ip: 	Static ip configuration
        * param gateway: 	Static gateway configuration
        * param subnet: 	Static subnet mask configuration
        */
    static bool config(uint8_t validParams, uint32_t local_ip, uint32_t gateway, uint32_t subnet);

    /* Set DNS ip configuration
           *
           * param validParams: set the number of parameters that we want to change
           * 					 i.e. validParams = 1 means that we'll change only dns_server1
           * 					 	  validParams = 2 means that we'll change dns_server1 and dns_server2
           * param dns_server1: Static DNS server1 configuration
           * param dns_server2: Static DNS server2 configuration
           */
    static bool setDNS(uint8_t validParams, uint32_t dns_server1, uint32_t dns_server2);

    /*
     * Disconnect from the network
     *
     * return: WL_SUCCESS or WL_FAILURE
     */
    static bool disconnect();

    /*
     * Disconnect from the network
     *
     * return: one value of wl_status_t enum
     */
    static bool getConnectionStatus();

    /*
     * Get the interface MAC address.
     *
     * return: pointer to uint8_t array with length WL_MAC_ADDR_LENGTH
     */
    static bool getMacAddress();

    /*
     * Return the current SSID associated with the network
     *
     * return: ssid string
     */
    static bool getCurrentSSID();

    /*
     * Return the current BSSID associated with the network.
     * It is the MAC address of the Access Point
     *
     * return: pointer to uint8_t array with length WL_MAC_ADDR_LENGTH
     */
    static bool getCurrentBSSID();

    /*
     * Return the current RSSI /Received Signal Strength in dBm)
     * associated with the network
     *
     * return: signed value
     */
    static bool getCurrentRSSI();

    /*
     * Return the Encryption Type associated with the network
     *
     * return: one value of wl_enc_type enum
     */
    static bool getCurrentEncryptionType();

    /*
     * Start scan WiFi networks available
     *
     * return: Number of discovered networks
     */
    static bool startScanNetworks();

    /*
     * Get the networks available
     *
     * return: Number of discovered networks
     */
	static bool getScanNetworks(uint8_t which);

    /*
     * Return the SSID discovered during the network scan.
     *
     * param networkItem: specify from which network item want to get the information
	 *
     * return: ssid string of the specified item on the networks scanned list
     */
    static char* getSSIDNetoworks(uint8_t networkItem);

    /*
     * Return the RSSI of the networks discovered during the scanNetworks
     *
     * param networkItem: specify from which network item want to get the information
	 *
     * return: signed value of RSSI of the specified item on the networks scanned list
     */
    static bool getRSSINetoworks(uint8_t networkItem);

    /*
     * Return the encryption type of the networks discovered during the scanNetworks
     *
     * param networkItem: specify from which network item want to get the information
	 *
     * return: encryption type (enum wl_enc_type) of the specified item on the networks scanned list
     */
    static bool getEncTypeNetowrks(uint8_t networkItem);

    /*
     * Get the firmware version
     * result: version as string with this format a.b.c
     */
    static bool getFwVersion();

	static bool startServer(uint16_t port, uint8_t sock, uint8_t protMode=TCP_MODE);

	static bool startClient(uint32_t ipAddress, uint16_t port, uint8_t sock, uint8_t protMode=TCP_MODE);

	static bool stopClient(uint8_t sock);
                                                                                  
    static bool getServerState(uint8_t sock);

    static bool getClientState(uint8_t sock);

	static bool getAvailable(uint8_t sock);

    static bool getData(uint8_t sock, uint8_t peek = 0);

    static bool getDataBuf(uint8_t sock, uint16_t len = 25);

    static bool sendData(uint8_t sock, const uint8_t *data, uint16_t len);

    static bool sendUdpData(uint8_t sock);

    friend class WiFiUDP;
	friend class WiFiClass;
};

#endif // PACKAGER_H