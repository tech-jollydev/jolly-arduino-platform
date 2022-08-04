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

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <WiFi.h>

#include "Arduino.h"
#include "utility/spi/spi_drv.h"

#include "utility/packager.h"


#define _DEBUG_

extern "C" {
	#include "utility/definitions.h"
}

// Array of data to cache the information related to the networks discovered
char Packager::_networkSsid[][WL_SSID_MAX_LENGTH] = {{"1"},{"2"},{"3"},{"4"},{"5"}};

// Cached values of retrieved data
char Packager::_ssid[] = {0};

// Firmware version
char Packager::fwVersion[] = {0};


// ----------------------------------------------------------------- 
bool Packager::getNetworkData()
{
	uint8_t _dummy = DUMMY_DATA;

	commDrv.sendCmd(GET_IPADDR_CMD, PARAM_NUMS_1);
	return commDrv.sendParam(&_dummy, sizeof(_dummy), LAST_PARAM);
}

bool Packager::getRemoteData(uint8_t sock)
{
	commDrv.sendCmd(GET_REMOTE_DATA_CMD, PARAM_NUMS_1);
	return commDrv.sendParam(&sock, sizeof(sock), LAST_PARAM);
}

// -----------------------------------------------------------------
bool Packager::wifiAutoConnect()
{
	return commDrv.sendCmd(AUTOCONNECT_TO_STA, PARAM_NUMS_0);
}

void Packager::wifiCancelNetList()
{
	commDrv.sendCmd(CANCEL_NETWORK_LIST, PARAM_NUMS_0);
}

// -----------------------------------------------------------------
bool Packager::wifiSetNetwork(char* ssid, uint8_t ssid_len)
{
	commDrv.sendCmd(CONNECT_OPEN_AP, PARAM_NUMS_1);
	return commDrv.sendParam((uint8_t*)ssid, ssid_len, LAST_PARAM);
}

// ----------------------------------------------------------------- ok
bool Packager::wifiSetPassphrase(char* ssid, uint8_t ssid_len, const char *passphrase, const uint8_t len)
{
	commDrv.sendCmd(CONNECT_SECURED_AP, PARAM_NUMS_2);
	commDrv.sendParam((uint8_t*)ssid, ssid_len, NO_LAST_PARAM);
	return commDrv.sendParam((uint8_t*)passphrase, len, LAST_PARAM);
}

// -----------------------------------------------------------------
bool Packager::wifiSetKey(char* ssid, uint8_t ssid_len, uint8_t key_idx, const void *key, const uint8_t len)
{
	commDrv.sendCmd(SET_KEY_CMD, PARAM_NUMS_3);
	commDrv.sendParam((uint8_t*)ssid, ssid_len, NO_LAST_PARAM);
	commDrv.sendParam(&key_idx, KEY_IDX_LEN, NO_LAST_PARAM);
	return commDrv.sendParam((uint8_t*)key, len, LAST_PARAM);
}

// -----------------------------------------------------------------
bool Packager::config(uint8_t validParams, uint32_t local_ip, uint32_t gateway, uint32_t subnet)
{
	commDrv.sendCmd(SET_IP_CONFIG_CMD, PARAM_NUMS_4);
	commDrv.sendParam((uint8_t*)&validParams, 1, NO_LAST_PARAM);
	commDrv.sendParam((uint8_t*)&local_ip, 4, NO_LAST_PARAM);
	commDrv.sendParam((uint8_t*)&gateway, 4, NO_LAST_PARAM);
	return commDrv.sendParam((uint8_t*)&subnet, 4, LAST_PARAM);
}

// -----------------------------------------------------------------
bool Packager::setDNS(uint8_t validParams, uint32_t dns_server1, uint32_t dns_server2)
{
	commDrv.sendCmd(SET_DNS_CONFIG_CMD, PARAM_NUMS_3);
	commDrv.sendParam((uint8_t*)&validParams, 1, NO_LAST_PARAM);
	commDrv.sendParam((uint8_t*)&dns_server1, 4, NO_LAST_PARAM);
	return commDrv.sendParam((uint8_t*)&dns_server2, 4, LAST_PARAM);
}

// -----------------------------------------------------------------
bool Packager::disconnect()
{
	uint8_t _dummy = DUMMY_DATA;
	
	commDrv.sendCmd(DISCONNECT_CMD, PARAM_NUMS_1);
	return commDrv.sendParam(&_dummy, 1, LAST_PARAM);
}

// ----------------------------------------------------------------- ok
bool Packager::getConnectionStatus()
{
	return commDrv.sendCmd(GET_CONN_STATUS, PARAM_NUMS_0);
}

// -----------------------------------------------------------------
bool Packager::getHostname()
{
	return commDrv.sendCmd(GET_HOSTNAME, PARAM_NUMS_0);
}

// -----------------------------------------------------------------
bool Packager::setHostname(const char* name)
{
	commDrv.sendCmd(SET_HOSTNAME, PARAM_NUMS_1);
	return commDrv.sendParam((uint8_t*)name, strlen(name), LAST_PARAM);
}

// ----------------------------------------------------------------- ok
bool Packager::getMacAddress()
{
	uint8_t _dummy = DUMMY_DATA;
	
	commDrv.sendCmd(GET_MACADDR_CMD, PARAM_NUMS_1);
	return commDrv.sendParam(&_dummy, 1, LAST_PARAM);
}

// ----------------------------------------------------------------- ok
bool Packager::getCurrentSSID()
{
	uint8_t _dummy = DUMMY_DATA;
	
	commDrv.sendCmd(GET_CURR_SSID_CMD, PARAM_NUMS_1);
	return commDrv.sendParam(&_dummy, 1, LAST_PARAM);
}

// ----------------------------------------------------------------- ok
bool Packager::getCurrentBSSID()
{
	uint8_t _dummy = DUMMY_DATA;

	commDrv.sendCmd(GET_CURR_BSSID_CMD, PARAM_NUMS_1);
	return commDrv.sendParam(&_dummy, 1, LAST_PARAM);
}

// ----------------------------------------------------------------- ok
bool Packager::getCurrentRSSI()
{
	uint8_t _dummy = DUMMY_DATA;
	
	commDrv.sendCmd(GET_CURR_RSSI_CMD, PARAM_NUMS_1);
	return commDrv.sendParam(&_dummy, 1, LAST_PARAM);
}

// ----------------------------------------------------------------- ok
bool Packager::getCurrentEncryptionType()
{
	uint8_t _dummy = DUMMY_DATA;
	
	commDrv.sendCmd(GET_CURR_ENCT_CMD, PARAM_NUMS_1);
	return commDrv.sendParam(&_dummy, 1, LAST_PARAM);
}

// -----------------------------------------------------------------
bool Packager::startScanNetworks()
{
	return commDrv.sendCmd(START_SCAN_NETWORKS, PARAM_NUMS_0);
}

// -----------------------------------------------------------------
bool Packager::getScanNetworks(uint8_t which)
{
	commDrv.sendCmd(SCAN_NETWORKS_RESULT, PARAM_NUMS_1);
	return commDrv.sendParam(&which, 1, LAST_PARAM);
}

// -----------------------------------------------------------------
char* Packager::getSSIDNetoworks(uint8_t networkItem)
{
	if (networkItem >= WL_NETWORKS_LIST_MAXNUM)
	return NULL;

	return _networkSsid[networkItem];
}

// -----------------------------------------------------------------
bool Packager::getEncTypeNetowrks(uint8_t networkItem)
{
	if (networkItem >= WL_NETWORKS_LIST_MAXNUM)
	return false;
	
	commDrv.sendCmd(GET_IDX_ENCT_CMD, PARAM_NUMS_1);
	return commDrv.sendParam(&networkItem, 1, LAST_PARAM);
}

// -----------------------------------------------------------------
bool Packager::getRSSINetoworks(uint8_t networkItem)
{
	if (networkItem >= WL_NETWORKS_LIST_MAXNUM)
	return false;
	
	commDrv.sendCmd(GET_IDX_RSSI_CMD, PARAM_NUMS_1);
	return commDrv.sendParam(&networkItem, 1, LAST_PARAM);
}

// -----------------------------------------------------------------
bool Packager::getHostByName(const char* aHostname)
{
	commDrv.sendCmd(GET_HOST_BY_NAME_CMD, PARAM_NUMS_1);
	return commDrv.sendParam((uint8_t*)aHostname, strlen(aHostname), LAST_PARAM);
}

// -----------------------------------------------------------------
bool Packager::getFwVersion()
{
	return commDrv.sendCmd(GET_FW_VERSION_CMD, PARAM_NUMS_0);
}

// -----------------------------------------------------------------
bool Packager::startServer(uint16_t port, uint8_t sock, uint8_t protMode)
{
    commDrv.sendCmd(START_SERVER_TCP_CMD, PARAM_NUMS_3);
    commDrv.sendParam(port);
    commDrv.sendParam(&sock, 1);
    return commDrv.sendParam(&protMode, 1, LAST_PARAM);
}

// -----------------------------------------------------------------
bool Packager::startClient(uint32_t ipAddress, uint16_t port, uint8_t sock, uint8_t protMode)
{
    commDrv.sendCmd(START_CLIENT_TCP_CMD, PARAM_NUMS_4);
    commDrv.sendParam((uint8_t*)&ipAddress, sizeof(ipAddress));
    commDrv.sendParam(port);
    commDrv.sendParam(&sock, 1);
    return commDrv.sendParam(&protMode, 1, LAST_PARAM);
}

// -----------------------------------------------------------------
bool Packager::stopClient(uint8_t sock)
{
    commDrv.sendCmd(STOP_CLIENT_TCP_CMD, PARAM_NUMS_1);
    return commDrv.sendParam(&sock, 1, LAST_PARAM);
}


// -----------------------------------------------------------------
bool Packager::getServerState(uint8_t sock)
{
    commDrv.sendCmd(GET_STATE_TCP_CMD, PARAM_NUMS_1);
    return commDrv.sendParam(&sock, sizeof(sock), LAST_PARAM);
}

// -----------------------------------------------------------------
bool Packager::getClientState(uint8_t sock)
{
    commDrv.sendCmd(GET_CLIENT_STATE_TCP_CMD, PARAM_NUMS_1);
    return commDrv.sendParam(&sock, sizeof(sock), LAST_PARAM);
}

// -----------------------------------------------------------------
bool Packager::getAvailable(uint8_t sock){
	commDrv.sendCmd(AVAIL_DATA_TCP_CMD, PARAM_NUMS_1);
	return commDrv.sendParam(&sock, sizeof(sock), LAST_PARAM);
}


// -----------------------------------------------------------------
bool Packager::getData(uint8_t sock, uint8_t peek)
{
    commDrv.sendCmd(GET_DATA_TCP_CMD, PARAM_NUMS_2);
    commDrv.sendParam(&sock, sizeof(sock));
    return commDrv.sendParam(peek, LAST_PARAM);
}

// -----------------------------------------------------------------
bool Packager::getDataBuf(uint8_t sock, uint16_t _dataLen)
{
	bool ret;
	
    commDrv.sendDataPkt(GET_DATABUF_TCP_CMD, PARAM_NUMS_2);
	commDrv.sendParam(&sock, sizeof(sock));
	ret = commDrv.sendParam(_dataLen, LAST_PARAM, true);
	if(_dataLen > 25 && ret == true){
		commDrv.multiRead = true;
	}
		
	return ret;
}

// -----------------------------------------------------------------
bool Packager::sendUdpData(uint8_t sock)
{
    commDrv.sendCmd(SEND_DATA_UDP_CMD, PARAM_NUMS_1);
    return commDrv.sendParam(&sock, sizeof(sock), LAST_PARAM);
}

// -----------------------------------------------------------------
bool Packager::sendData(uint8_t sock, const uint8_t *data, uint16_t len)
{
	tsNewData dataPkt;
	dataPkt.cmdType = DATA_PKT;
	dataPkt.cmd = SEND_DATA_TCP_CMD;
	// data len + header + footer
	dataPkt.totalLen = (uint32_t)(len + 8);
	dataPkt.sockNum = sock;
	dataPkt.dataPtr = (uint16_t*)data;

	return commDrv.writeServerData((uint8_t*)&dataPkt, dataPkt.totalLen);	
}