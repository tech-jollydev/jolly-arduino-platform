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

#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#include <inttypes.h>

#define CMD_FLAG        0
#define REPLY_FLAG      1<<7
#define DATA_FLAG 		0x40

#define NO_SOCKET_AVAIL     255
// Maximum size of a SSID
#define WL_SSID_MAX_LENGTH 32
// Length of passphrase. Valid lengths are 8-63.
#define WL_WPA_KEY_MAX_LENGTH 63
// Length of key in bytes. Valid values are 5 and 13.
#define WL_WEP_KEY_MAX_LENGTH 13
// Size of a MAC-address or BSSID
#define WL_MAC_ADDR_LENGTH 6
// Size of a MAC-address or BSSID
#define WL_IPV4_LENGTH 4
// Maximum size of a SSID list
#define WL_NETWORKS_LIST_MAXNUM	10
// Maxmium number of socket
#define	MAX_SOCK_NUM		4
//Maximum number of attempts to establish wifi connection
#define WL_MAX_ATTEMPT_CONNECTION	100

#define START_CMD   	0xe0
#define DATA_PKT		0xf0
#define END_CMD     	0xee
#define ERR_CMD   		0xef
#define CMD_POS			1		// Position of Command OpCode on SPI stream
#define PARAM_LEN_POS 	2		// Position of Param len on SPI stream


enum {
	NONE				= 0x00,
	ESP_READY			= 0x09,
	CONNECT_OPEN_AP 	= 0x10,
	CONNECT_SECURED_AP	= 0x11,
	SET_KEY_CMD	        = 0x12,
	TEST_CMD	        = 0x13,
	SET_IP_CONFIG_CMD	= 0x14,
	SET_DNS_CONFIG_CMD	= 0x15,
	AUTOCONNECT_TO_STA	= 0x16,
	CANCEL_NETWORK_LIST	= 0x17,

	GET_CONN_STATUS		= 0x20,
	GET_IPADDR_CMD		= 0x21,
	GET_MACADDR_CMD		= 0x22,
	GET_CURR_SSID_CMD	= 0x23,
	GET_CURR_BSSID_CMD	= 0x24,
	GET_CURR_RSSI_CMD	= 0x25,
	GET_CURR_ENCT_CMD	= 0x26,
	SCAN_NETWORKS_RESULT= 0x27,
	START_SERVER_TCP_CMD= 0x28,
	GET_STATE_TCP_CMD   = 0x29,
	DATA_SENT_TCP_CMD	= 0x2A,
    AVAIL_DATA_TCP_CMD	= 0x2B,
    GET_DATA_TCP_CMD	= 0x2C,
    START_CLIENT_TCP_CMD= 0x2D,
    STOP_CLIENT_TCP_CMD = 0x2E,
    GET_CLIENT_STATE_TCP_CMD= 0x2F,
    DISCONNECT_CMD		= 0x30,
	GET_IDX_SSID_CMD	= 0x31,
	GET_IDX_RSSI_CMD	= 0x32,
	GET_IDX_ENCT_CMD	= 0x33,
	GET_HOST_BY_NAME_CMD= 0x35,
	START_SCAN_NETWORKS	= 0x36,
	GET_FW_VERSION_CMD	= 0x37,
	GET_TEST_CMD		= 0x38,
	SEND_DATA_UDP_CMD	= 0x39,
	GET_REMOTE_DATA_CMD = 0x3A,
	GET_HOSTNAME		= 0x3B,
	SET_HOSTNAME		= 0x3C,
	
	TEST_DATA_TXRX		= 0x40,

    // All command with DATA_FLAG 0x40 send a 16bit Len

	SEND_DATA_TCP_CMD		= 0x44,
    GET_DATABUF_TCP_CMD		= 0x45,
    INSERT_DATABUF_CMD		= 0x46,
};


enum wl_tcp_state {
  CLOSED      = 0,
  LISTEN      = 1,
  SYN_SENT    = 2,
  SYN_RCVD    = 3,
  ESTABLISHED = 4,
  FIN_WAIT_1  = 5,
  FIN_WAIT_2  = 6,
  CLOSE_WAIT  = 7,
  CLOSING     = 8,
  LAST_ACK    = 9,
  TIME_WAIT   = 10
};


enum numParams{
    PARAM_NUMS_0,
    PARAM_NUMS_1,
    PARAM_NUMS_2,
    PARAM_NUMS_3,
    PARAM_NUMS_4,
    PARAM_NUMS_5,
    MAX_PARAM_NUMS
};


/*
WL_CONNECTED: assigned when connected to a WiFi network;
WL_IDLE_STATUS: it is a temporary status assigned when WiFi.begin() is called and remains active until the number of attempts expires (resulting in WL_CONNECT_FAILED) or a connection is established (resulting in WL_CONNECTED);
WL_NO_SSID_AVAIL: assigned when no SSID are available;
WL_SCAN_COMPLETED: assigned when the scan networks is completed;
WL_CONNECT_FAILED: assigned when the connection fails for all the attempts;
WL_CONNECTION_LOST: assigned when the connection is lost;
WL_DISCONNECTED: assigned when disconnected from a network;
*/
typedef enum {
  WL_NO_WIFI_MODULE_COMM = 255,
  WL_IDLE_STATUS = 0,
  WL_NO_SSID_AVAIL,
  WL_SCAN_COMPLETED,
  WL_CONNECTED,
  WL_CONNECT_FAILED,
  WL_CONNECTION_LOST,
  WL_DISCONNECTED
} wl_status_t;

/* Encryption modes */
enum wl_enc_type {  /* Values map to 802.11 encryption suites... */
  ENC_TYPE_WEP  = 5,
  ENC_TYPE_TKIP = 2,
  ENC_TYPE_CCMP = 4,
  /* ... except these two, 7 and 8 are reserved in 802.11-2007 */
  ENC_TYPE_NONE = 7,
  ENC_TYPE_AUTO = 8,
  ENC_TYPE_UNKNOW = 255
};

#endif // DEFINITIONS_H
