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

#include <WiFi.h>
#include "utility/packager.h"
#include "utility/spi/spi_drv.h"

// XXX: don't make assumptions about the value of MAX_SOCK_NUM.
uint8_t		WiFiClass::hostname[MAX_HOSTNAME_LEN] {0};
uint8_t 	WiFiClass::_state[MAX_SOCK_NUM] = { 0, 0, 0, 0 };
uint16_t 	WiFiClass::_server_port[MAX_SOCK_NUM] = { 0, 0, 0, 0 };
uint16_t	WiFiClass::_client_data[MAX_SOCK_NUM] = { 0, 0, 0, 0};

bool WiFiClass::gotResponse = false;
uint8_t WiFiClass::responseType = 0x00;
uint32_t WiFiClass::dataLen = 0x0000;
uint8_t* WiFiClass::data = NULL;
wl_status_t WiFiClass::connectionStatus = WL_NO_WIFI_MODULE_COMM;
bool WiFiClass::notify = false;
volatile tsDataPacket WiFiClass::dataPkt;
volatile tsNewCmd WiFiClass::cmdPkt;
int32_t WiFiClass::pktLen;


/* -----------------------------------------------------------------
* static callback that handles the data coming from the SPI driver
* calling the packet decoder to fire the right callback group
*/
static void wifiDrvCB(void)
{
	volatile uint8_t ret = 0;
	
	// read the data out from the SPI interface and put them into a buffer
	ret = commDrv.readDataISR(commDrv._rxBuf);
	// decode the 32 byte long packet
	if(ret == 0)
		return;

	WiFiClass::gotResponse = false;
	WiFiClass::responseType = NONE;

	uint8_t cmd = commDrv._rxBuf[1] - 0x80;

	// check asynch events at first
	if(commDrv._rxBuf[0] == START_CMD){
		if(cmd == CONNECT_SECURED_AP || cmd == CONNECT_OPEN_AP || cmd == DISCONNECT_CMD){
			WiFiClass::gotResponse = true;
			WiFiClass::responseType = cmd;
			WiFiClass::connectionStatus = (wl_status_t)commDrv._rxBuf[5];
			WiFiClass::notify = true;
			if(commDrv.multiRead) // manually trigger an ISR to keep read going if an asynchronous event occurred when multiRead is set
				commDrv._interruptReq = true;
			return 0;
		}
	}

	// check multipacket continuation
	if(WiFiClass::dataPkt.totalLen > 0) {
		WiFiClass::cmdPkt.dataPtr = commDrv._rxBuf;
		WiFiClass::cmdPkt.cmdType = WiFiClass::dataPkt.cmdType;
		WiFiClass::cmdPkt.cmd = WiFiClass::dataPkt.cmd;
		
		for(uint8_t i=0; i<SPI_BUF_LEN; i++){
			if(commDrv._rxBuf[i] == END_CMD){
				commDrv.rxIndex = i;
				if(WiFiClass::dataPkt.receivedLen + commDrv.rxIndex == WiFiClass::dataPkt.totalLen - 1){
					commDrv._disableDevice();
					commDrv.multiRead = false;
					WiFiClass::dataPkt.endReceived = true;
					commDrv.payloadSize = commDrv.rxIndex;
					WiFiClass::dataPkt.receivedLen = commDrv.rxIndex = WiFiClass::dataPkt.totalLen = 0;
					break;
				}
			}
		}
		if(WiFiClass::dataPkt.endReceived == false) {
			commDrv.payloadSize = SPI_BUF_LEN;
			WiFiClass::dataPkt.receivedLen += SPI_BUF_LEN;
			if(WiFiClass::dataPkt.receivedLen >= WiFiClass::dataPkt.totalLen){
				commDrv._disableDevice();
				commDrv.multiRead = false;
				WiFiClass::dataPkt.endReceived = true;
				WiFiClass::dataPkt.receivedLen = commDrv.rxIndex = WiFiClass::dataPkt.totalLen = 0;
			}
		}

		WiFiClass::gotResponse = true;
		WiFiClass::responseType = WiFiClass::dataPkt.cmd;
		WiFiClass::data = (uint8_t*)(commDrv._rxBuf);
		WiFiClass::dataLen = commDrv.payloadSize;
		return;
	}

	// check multipacket start
	else if(commDrv._rxBuf[0] == DATA_PKT) {
		// cast the buffer to the packet structure
		memset((tsDataPacket*)&WiFiClass::dataPkt, 0, sizeof(tsDataPacket));
		memcpy((tsDataPacket*)&WiFiClass::dataPkt, commDrv._rxBuf, sizeof(tsDataPacket));
		WiFiClass::dataPkt.cmd -= 0x80;

		// 32 byte - (cmdType (1 byte) + cmd (1 byte) + totalLen (4 byte)) = 32 - 6 = 26
		WiFiClass::dataPkt.receivedLen = SPI_BUF_LEN;
		WiFiClass::dataPkt.endReceived = false;
		
		WiFiClass::cmdPkt.dataPtr = (uint8_t*)(commDrv._rxBuf + 6);
		WiFiClass::pktLen = (int32_t)(commDrv._rxBuf[2] + (commDrv._rxBuf[3] << 8) + (commDrv._rxBuf[4] << 16) + (commDrv._rxBuf[5] << 24));
		WiFiClass::dataPkt.totalLen = WiFiClass::pktLen + 7; // 1 - start, 1 - cmdType, 4 - len, 1 - endCmd

		WiFiClass::dataLen = SPI_BUF_LEN - 6; // do not consider END_CMD

		if(WiFiClass::dataPkt.totalLen <= SPI_BUF_LEN){ // data are contained in a single packet control the packet footer.
			bool ret = 0;
			if(commDrv._rxBuf[WiFiClass::dataPkt.totalLen - 1] != END_CMD){
				ret = 1;
			}
			WiFiClass::dataLen = WiFiClass::dataPkt.totalLen - 7;
			WiFiClass::dataPkt.totalLen = 0;
			WiFiClass::dataPkt.receivedLen = 0;
			WiFiClass::dataPkt.endReceived = true;
			
			if(ret){
				return;
			}
		}

		WiFiClass::gotResponse = true;
		WiFiClass::responseType = WiFiClass::dataPkt.cmd;
		WiFiClass::data = (uint8_t*)(WiFiClass::cmdPkt.dataPtr);
		return;
	}

	// check single packet
	else if (commDrv._rxBuf[0] == START_CMD) {
		// cast the buffer to the packet structure
		memset((tsNewCmd*)&WiFiClass::cmdPkt, 0, sizeof(tsNewCmd));
		memcpy((tsNewCmd*)&WiFiClass::cmdPkt, commDrv._rxBuf, sizeof(tsNewCmd));
		// remove the REPLY_FLAG
		WiFiClass::cmdPkt.cmd -= 0x80;
		WiFiClass::cmdPkt.dataPtr = (uint8_t*)(commDrv._rxBuf + 4);

		switch(WiFiClass::cmdPkt.cmd){
			case ESP_READY:
				commDrv.espStatus = esp_idle;
			break;
			case GET_CONN_STATUS:
				// check the number of parameters of the command
				if(WiFiClass::cmdPkt.nParam == PARAM_NUMS_1 && commDrv._rxBuf[WiFiClass::cmdPkt.totalLen - 1] == END_CMD){
					WiFiClass::gotResponse = true;
					WiFiClass::responseType = WiFiClass::cmdPkt.cmd;
					// extract the data from the data pointer
					WiFiClass::data = (uint8_t*)(WiFiClass::cmdPkt.dataPtr + 1);
					WiFiClass::connectionStatus = (wl_status_t)(WiFiClass::data[0]);
				}
			break;
			case GET_HOSTNAME:
			case SET_HOSTNAME:
			case GET_MACADDR_CMD:
			case GET_FW_VERSION_CMD:
				if(WiFiClass::cmdPkt.nParam == PARAM_NUMS_1 && commDrv._rxBuf[WiFiClass::cmdPkt.totalLen - 1] == END_CMD){
					WiFiClass::gotResponse = true;
					WiFiClass::responseType = WiFiClass::cmdPkt.cmd;
					// extract the data from the data pointer
					WiFiClass::data = (uint8_t*)(WiFiClass::cmdPkt.dataPtr);
				}
			break;
			case START_SCAN_NETWORKS:{
                // check the parameter len
				uint8_t paramLen = (WiFiClass::cmdPkt.dataPtr[0] > WL_NETWORKS_LIST_MAXNUM) ? WL_NETWORKS_LIST_MAXNUM : WiFiClass::cmdPkt.dataPtr[0];
                if (paramLen == PARAM_NUMS_1 && commDrv._rxBuf[WiFiClass::cmdPkt.totalLen - 1] == END_CMD) {
					// set gotResponse flag to true to signal that esp replied us
                    WiFiClass::gotResponse = true;
                    WiFiClass::responseType = WiFiClass::cmdPkt.cmd;
                    // extract the data from the data pointer
                    WiFiClass::data = (uint8_t*)(WiFiClass::cmdPkt.dataPtr + 1);
					WiFiClass::dataLen = paramLen;
				}
			}break;
			case CONNECT_SECURED_AP:
			case CONNECT_OPEN_AP:
			case DISCONNECT_CMD:
			case SET_KEY_CMD:
				if(WiFiClass::cmdPkt.nParam == PARAM_NUMS_1 && commDrv._rxBuf[WiFiClass::cmdPkt.totalLen - 1] == END_CMD){
					// set gotResponse flag to true to signal that esp replied us
					WiFiClass::gotResponse = true;
					WiFiClass::responseType = WiFiClass::cmdPkt.cmd;
					// extract the data from the data pointer
					WiFiClass::data = (uint8_t*)(WiFiClass::cmdPkt.dataPtr + 1);
					WiFiClass::connectionStatus = (wl_status_t)(WiFiClass::cmdPkt.dataPtr[1]);
					WiFiClass::notify = true;
				}
			break;
			case GET_CURR_SSID_CMD:
				if(WiFiClass::cmdPkt.nParam == PARAM_NUMS_1){
					// set gotResponse flag to true to signal that esp replied us
					WiFiClass::gotResponse = true;
					WiFiClass::responseType = WiFiClass::cmdPkt.cmd;
					if(WiFiClass::cmdPkt.totalLen > SPI_BUF_LEN){
						delayMicroseconds(100);
						commDrv.readDataISR(commDrv._rxBuf + SPI_BUF_LEN);
					}
					// extract the data from the data pointer
					WiFiClass::data = (uint8_t*)(WiFiClass::cmdPkt.dataPtr);
				}
			break;
			case START_CLIENT_TCP_CMD:
			case GET_HOST_BY_NAME_CMD:
			case START_SERVER_TCP_CMD:
			case STOP_CLIENT_TCP_CMD:
			case GET_CURR_BSSID_CMD:
			case GET_STATE_TCP_CMD:
			case GET_CURR_ENCT_CMD:
			case GET_CURR_RSSI_CMD:
			case SEND_DATA_UDP_CMD:
			case GET_IDX_RSSI_CMD:
			case GET_IDX_ENCT_CMD:
			case GET_DATA_TCP_CMD:
				if(WiFiClass::cmdPkt.nParam == PARAM_NUMS_1 && commDrv._rxBuf[WiFiClass::cmdPkt.totalLen - 1] == END_CMD){
					WiFiClass::gotResponse = true;
					WiFiClass::responseType = WiFiClass::cmdPkt.cmd;
					// extract the data from the data pointer
					WiFiClass::data = (uint8_t*)(WiFiClass::cmdPkt.dataPtr + 1);
					WiFiClass::dataLen = WiFiClass::cmdPkt.dataPtr[0];
				}
			break;
			case GET_REMOTE_DATA_CMD:
				if((WiFiClass::cmdPkt.nParam == PARAM_NUMS_1 || WiFiClass::cmdPkt.nParam == PARAM_NUMS_2) && commDrv._rxBuf[WiFiClass::cmdPkt.totalLen - 1] == END_CMD){
					WiFiClass::gotResponse = true;
					WiFiClass::responseType = WiFiClass::cmdPkt.cmd;
					// extract the data from the data pointer
					WiFiClass::data = (uint8_t*)(WiFiClass::cmdPkt.dataPtr + 1);
					WiFiClass::dataLen = WiFiClass::cmdPkt.dataPtr[0];
				}
			break;
			case GET_IPADDR_CMD:
				if(WiFiClass::cmdPkt.nParam == PARAM_NUMS_3 && commDrv._rxBuf[WiFiClass::cmdPkt.totalLen - 1] == END_CMD){
					WiFiClass::gotResponse = true;
					WiFiClass::responseType = WiFiClass::cmdPkt.cmd;
					// extract the data from the data pointer
					WiFiClass::data = (uint8_t*)(WiFiClass::cmdPkt.dataPtr);
				}
			break;
			case GET_CLIENT_STATE_TCP_CMD:
				if(WiFiClass::cmdPkt.nParam == PARAM_NUMS_2 && commDrv._rxBuf[WiFiClass::cmdPkt.totalLen - 1] == END_CMD){
					WiFiClass::gotResponse = true;
					WiFiClass::responseType = WiFiClass::cmdPkt.cmd;
					// dataPtr[1] contains socket number, dataPtr[3] contains status of the client attached to that socket
					WiFiClass::_state[WiFiClass::cmdPkt.dataPtr[1]] = WiFiClass::cmdPkt.dataPtr[3];
					WiFiClass::data = (uint8_t*)(WiFiClass::cmdPkt.dataPtr + 1);
				}
			break;
			case AVAIL_DATA_TCP_CMD:
				if(WiFiClass::cmdPkt.nParam == PARAM_NUMS_2 && commDrv._rxBuf[WiFiClass::cmdPkt.totalLen - 1] == END_CMD){
					WiFiClass::gotResponse = true;
					WiFiClass::responseType = WiFiClass::cmdPkt.cmd;
					WiFiClass::data = (uint8_t*)(WiFiClass::cmdPkt.dataPtr);
				}
			break;
		}
	}

	WiFiClass::cmdPkt.cmdType = 0;
	WiFiClass::cmdPkt.cmd = 0;
	WiFiClass::cmdPkt.dataPtr = NULL;
	WiFiClass::cmdPkt.nParam = 0;
	WiFiClass::cmdPkt.totalLen = 0;
}


/* -----------------------------------------------------------------
*
*/
WiFiClass::WiFiClass()
{
	connectionStatus = WL_NO_WIFI_MODULE_COMM;
}

/* -----------------------------------------------------------------
* handles the WiFi driver event to see if an ISR has been detected
*/
void WiFiClass::handleEvents(void)
{
	commDrv.handleSPIEvents();
}

/* -----------------------------------------------------------------
* Initializes the wifi class receiving the relative callback and 
* registering it
*/
void WiFiClass::init(teConnectionMode connectionMode)
{
	delay(100);
	
	(void) connectionMode;

	commDrv.begin();
	commDrv.registerCB(wifiDrvCB);
	
	uint8_t ret = WL_NO_WIFI_MODULE_COMM;
	
	// TODO check the connection status
	bool ESPConnected = commDrv.establishESPConnection();
	
	gotResponse = false;
	responseType = NONE;
	
	uint32_t start = millis() + 1000;
	
	while(start > millis()){
		handleEvents();
		if(gotResponse && responseType == GET_CONN_STATUS){
			ret = *data;
			if(ret != WL_NO_WIFI_MODULE_COMM)
				return;
		}
	}
}

/*
*
*/
uint16_t WiFiClass::getAvailableData()
{
	gotResponse = false;
	responseType = NONE;
	
	if(dataPkt.totalLen == 0){
		handleEvents();
	}
	else if(dataPkt.totalLen > dataPkt.receivedLen) {
		wifiDrvCB();
	}
	else{
		dataPkt.cmdType = 0;
		dataPkt.cmd = NONE;
		dataPkt.totalLen = 0;
		dataPkt.receivedLen = 0;
		dataPkt.endReceived = false;
	}
	if(gotResponse && responseType == GET_DATABUF_TCP_CMD)
	return dataLen;
	return 0;
}

// -----------------------------------------------------------------
char* WiFiClass::getHostname()
{
	handleEvents();
	
	gotResponse = false;
	responseType = NONE;
	
	if(!Packager::getHostname()){ // packet has not been sent. Maybe an interrupt occurred in the meantime
		// launch interrupt management function, then try to send request again
		handleEvents();
		if(!Packager::getHostname()) // exit if another error occurs
			return NULL;
	}

	// Poll response for the next 10 seconds
	uint32_t start = millis();
	while(((millis() - start) < GENERAL_TIMEOUT)){
		handleEvents();
		if(gotResponse && responseType == GET_HOSTNAME){
			uint8_t len = data[0];
			memset(hostname, 0, MAX_HOSTNAME_LEN);
			memcpy(hostname, (void*)&data[1], len);
			return (char *)hostname;
		}
	}
	return NULL;
}

// -----------------------------------------------------------------
bool WiFiClass::setHostname(char* name)
{
	handleEvents();
	
	gotResponse = false;
	responseType = NONE;
	
	if(!Packager::setHostname(name)){ // packet has not been sent. Maybe an interrupt occurred in the meantime
		// launch interrupt management function, then try to send request again
		handleEvents();
		if(!Packager::setHostname(name)) // exit if another error occurs
			return false;
	}

	// Poll response for the next 10 seconds
	uint32_t start = millis();
	while(((millis() - start) < GENERAL_TIMEOUT)){
		handleEvents();
		if(gotResponse && responseType == SET_HOSTNAME){
			return(data[0]);
		}
	}
	return false;
}

// -----------------------------------------------------------------
uint8_t WiFiClass::getSocket()
{
	for (uint8_t i = 0; i < MAX_SOCK_NUM; ++i) {
		if (WiFiClass::_server_port[i] == 0) {
			return i;
		}
	}
	return NO_SOCKET_AVAIL;
}

// -----------------------------------------------------------------
char* WiFiClass::firmwareVersion()
{
	handleEvents();
	
	gotResponse = false;
	responseType = NONE;
	
	if(!Packager::getFwVersion()){ // packet has not been sent. Maybe an interrupt occurred in the meantime
		// launch interrupt management function, then try to send request again
		handleEvents();
		if(!Packager::getFwVersion()) // exit if another error occurs
			return NULL;
	}

	// Poll response for the next 10 seconds
	uint32_t start = millis();
	while(((millis() - start) < GENERAL_TIMEOUT)){
		handleEvents();
		if(gotResponse && responseType == GET_FW_VERSION_CMD){
			uint8_t len = data[0];
			memcpy(Packager::fwVersion, (void*)&data[1], len);
			return (char *)Packager::fwVersion;
		}
	}
	return NULL;
}

// -----------------------------------------------------------------
bool WiFiClass::checkFirmwareVersion(String fw_required)
{
	String fw_version = firmwareVersion();
	int idx = fw_version.indexOf('.');
	int last_idx = fw_version.lastIndexOf('.');
	//fw_version as int value
	int fw_version_tmp = (fw_version.substring(0,idx)+fw_version.substring(idx+1,last_idx)+fw_version.substring(last_idx+1)).toInt();
	idx = fw_required.indexOf('.');
	last_idx = fw_required.lastIndexOf('.');
	//fw_required as int value
	int fw_required_tmp = (fw_required.substring(0,idx)+fw_required.substring(idx+1,last_idx)+fw_required.substring(last_idx+1)).toInt();
	if(fw_version_tmp >= fw_required_tmp)
	return true;
	else
	return false;
}

// -----------------------------------------------------------------
wl_status_t WiFiClass::begin()
{
	uint8_t status = WL_CONNECT_FAILED;
	handleEvents();
	
	gotResponse = false;
	responseType = NONE;
	
	if(!Packager::wifiAutoConnect()){ // packet has not been sent. Maybe an interrupt occurred in the meantime
		// launch interrupt management function, then try to send request again
		handleEvents();
		if(!Packager::wifiAutoConnect()) // exit if another error occurs
		return (wl_status_t)status;
	}

	// Poll response for the next 10 seconds. If nothing happens return WL_CONNECT_FAILED
	uint32_t start = millis();
	while(((millis() - start) < GENERAL_TIMEOUT)){
		handleEvents();
		if(gotResponse && (responseType == CONNECT_OPEN_AP || responseType == CONNECT_SECURED_AP)){
			status = data[0];
			break;
		}
	}

	return (wl_status_t)status;

}

// -----------------------------------------------------------------
wl_status_t WiFiClass::begin(char* ssid)
{
	uint8_t status = WL_CONNECT_FAILED;
	handleEvents();
	
	gotResponse = false;
	responseType = NONE;
	
	if(!Packager::wifiSetNetwork(ssid, strlen(ssid))){ // packet has not been sent. Maybe an interrupt occurred in the meantime
		// launch interrupt management function, then try to send request again
		handleEvents();
		if(!Packager::wifiSetNetwork(ssid, strlen(ssid))) // exit if another error occurs
		return (wl_status_t)status;
	}	

	// Poll response for the next 10 seconds. If nothing happens return WL_CONNECT_FAILED
	uint32_t start = millis();
	while(((millis() - start) < GENERAL_TIMEOUT)){
		handleEvents();
		if(gotResponse && responseType == CONNECT_OPEN_AP){
			status = data[0];
			break;
		}
	}

	return (wl_status_t)status;
}

// -----------------------------------------------------------------
wl_status_t WiFiClass::begin(char* ssid, uint8_t key_idx, const char *key)
{
	uint8_t status = WL_CONNECT_FAILED;
	handleEvents();
	
	gotResponse = false;
	responseType = NONE;
	
	if(!Packager::wifiSetKey(ssid, strlen(ssid), key_idx, key, strlen(key))){ // packet has not been sent. Maybe an interrupt occurred in the meantime
		// launch interrupt management function, then try to send request again
		handleEvents();
		if(!Packager::wifiSetKey(ssid, strlen(ssid), key_idx, key, strlen(key))) // exit if another error occurs
		return (wl_status_t)status;
	}

	// Poll response for the next 10 seconds. If nothing happens return WL_CONNECT_FAILED
	uint32_t start = millis();
	while(((millis() - start) < GENERAL_TIMEOUT)){
		handleEvents();
		if(gotResponse && responseType == SET_KEY_CMD){
			status = data[0];
			break;
		}
	}

	return (wl_status_t)status;
}

// ----------------------------------------------------------------- ok
wl_status_t WiFiClass::begin(char* ssid, const char *passphrase)
{
	uint8_t status = WL_CONNECT_FAILED;
	handleEvents();
	
	gotResponse = false;
	responseType = NONE;

	if(!Packager::wifiSetPassphrase(ssid, strlen(ssid), passphrase, strlen(passphrase))){ // packet has not been sent. Maybe an interrupt occurred in the meantime
		// launch interrupt management function, then try to send request again
		handleEvents();
		if(!Packager::wifiSetPassphrase(ssid, strlen(ssid), passphrase, strlen(passphrase))) // exit if another error occurs
			return (wl_status_t)status;
	}

	// Poll response for the next 10 seconds. If nothing happens return WL_CONNECT_FAILED
	uint32_t start = millis();
	while(((millis() - start) < GENERAL_TIMEOUT)){
		handleEvents();
		if(gotResponse && responseType == CONNECT_SECURED_AP){
			status = data[0];
			break;
		}
	}

	return (wl_status_t)status;
}

void WiFiClass::config(IPAddress local_ip)
{
	Packager::config(1, (uint32_t)local_ip, 0, 0);
}

void WiFiClass::config(IPAddress local_ip, IPAddress dns_server)
{
	Packager::config(1, (uint32_t)local_ip, 0, 0);
	Packager::setDNS(1, (uint32_t)dns_server, 0);
}

void WiFiClass::config(IPAddress local_ip, IPAddress dns_server, IPAddress gateway)
{
	Packager::config(2, (uint32_t)local_ip, (uint32_t)gateway, 0);
	Packager::setDNS(1, (uint32_t)dns_server, 0);
}

void WiFiClass::config(IPAddress local_ip, IPAddress dns_server, IPAddress gateway, IPAddress subnet)
{
	Packager::config(3, (uint32_t)local_ip, (uint32_t)gateway, (uint32_t)subnet);
	Packager::setDNS(1, (uint32_t)dns_server, 0);
}

void WiFiClass::setDNS(IPAddress dns_server1)
{
	Packager::setDNS(1, (uint32_t)dns_server1, 0);
}

void WiFiClass::setDNS(IPAddress dns_server1, IPAddress dns_server2)
{
	Packager::setDNS(2, (uint32_t)dns_server1, (uint32_t)dns_server2);
}

int WiFiClass::disconnect()
{
	handleEvents();

	gotResponse = false;
	responseType = NONE;

	if(!Packager::disconnect()){ // packet has not been sent. Maybe an interrupt occurred in the meantime
		// launch interrupt management function, then try to send request again
		handleEvents();
		if(!Packager::disconnect()) // exit if another error occurs
		return -1;
	}

	// Poll response for the next 10 seconds
	uint32_t start = millis();
	while(((millis() - start) < GENERAL_TIMEOUT)){
		handleEvents();
		if(gotResponse && responseType == DISCONNECT_CMD){
			return (int)data[0];
		}
	}

	return -1;
}

// ----------------------------------------------------------------- ok
void WiFiClass::macAddress(uint8_t* mac)
{
	handleEvents();
	
	gotResponse = false;
	responseType = NONE;

	if(!Packager::getMacAddress()){ // packet has not been sent. Maybe an interrupt occurred in the meantime
		// launch interrupt management function, then try to send request again
		handleEvents();
		if(!Packager::getMacAddress()) // exit if another error occurs
		return;
	}

	// Poll response for the next 10 seconds
	uint32_t start = millis();
	while(((millis() - start) < GENERAL_TIMEOUT)){
		handleEvents();
		if(gotResponse && responseType == GET_MACADDR_CMD){
			uint8_t len = data[0];
			memcpy(mac, (void*)&data[1], len);
			break;
		}
	}
}

// ----------------------------------------------------------------- ok
IPAddress WiFiClass::localIP()
{
	handleEvents();
	uint8_t addr[4] = {0};
	
	gotResponse = false;
	responseType = NONE;

	if(!Packager::getNetworkData()){ // packet has not been sent. Maybe an interrupt occurred in the meantime
		// launch interrupt management function, then try to send request again
		handleEvents();
		if(!Packager::getNetworkData()) // exit if another error occurs
			return IPAddress(addr);
	}

	// Poll response for the next 10 seconds
	uint32_t start = millis();
	while(((millis() - start) < GENERAL_TIMEOUT)){
		handleEvents();
		if(gotResponse && responseType == GET_IPADDR_CMD){
			// local ip is the first of the three parameters returned with GET_IPADDR_CMD command
			memcpy((uint8_t*)&addr, (uint8_t*)&data[1], data[0]);
			
			break;
		}
	}
	
	IPAddress ret(addr);
	
	return ret;
}

// -----------------------------------------------------------------
IPAddress WiFiClass::subnetMask()
{
	uint8_t mask[4] = {0};
	handleEvents();
	
	gotResponse = false;
	responseType = NONE;

	Packager::getNetworkData();

	// Poll response for the next 10 seconds
	uint32_t start = millis();
	while(((millis() - start) < GENERAL_TIMEOUT)){
		handleEvents();
		if(gotResponse && responseType == GET_IPADDR_CMD){
			// subnet mask is the second of the three parameters returned with GET_IPADDR_CMD command
			uint8_t firstDataLen = data[0];
			dataLen = data[firstDataLen + 1];
			memcpy((uint8_t*)&mask, (void*)&data[firstDataLen + 2], dataLen);
			break;
		}
	}

	IPAddress ret(mask);

	return ret;
}

// ----------------------------------------------------------------- ok
IPAddress WiFiClass::gatewayIP()
{
	handleEvents();
	uint8_t gateway[4] = {0};
	
	gotResponse = false;
	responseType = NONE;

	Packager::getNetworkData();

	// Poll response for the next 10 seconds
	uint32_t start = millis();
	while(((millis() - start) < GENERAL_TIMEOUT)){
		handleEvents();
		if(gotResponse && responseType == GET_IPADDR_CMD){
			// gateway ip is the second of the three parameters returned with GET_IPADDR_CMD command
			// skip first two data in order to retrieve the third.
			uint8_t secondDataLen = data[0] + data[data[0] + 1];
			dataLen = data[secondDataLen + 1];
			memcpy((uint8_t*)&gateway, (void*)&data[secondDataLen + 2], dataLen);
			break;
		}
	}
	IPAddress ret(gateway);

	return ret;
}

// ----------------------------------------------------------------- ok
char* WiFiClass::SSID()
{
	handleEvents();
	
	gotResponse = false;
	responseType = NONE;

	if(!Packager::getCurrentSSID()){ // packet has not been sent. Maybe an interrupt occurred in the meantime
		// launch interrupt management function, then try to send request again
		handleEvents();
		if(!Packager::getCurrentSSID()) // exit if another error occurs
		return NULL;
	}

	// Poll response for the next 10 seconds
	uint32_t start = millis();
	while(((millis() - start) < GENERAL_TIMEOUT)){
		handleEvents();
		if(gotResponse && responseType == GET_CURR_SSID_CMD){
			uint8_t len = data[0];
			memset(Packager::_ssid, 0, WL_SSID_MAX_LENGTH);
			memcpy(Packager::_ssid, (void*)&data[1], len);
			return (char *)Packager::_ssid;
		}
	}
	return NULL;
}

// ----------------------------------------------------------------- ok
bool WiFiClass::BSSID(uint8_t* bssid)
{
	handleEvents();
	
	gotResponse = false;
	responseType = NONE;

	if(!Packager::getCurrentBSSID()){ // packet has not been sent. Maybe an interrupt occurred in the meantime
		// launch interrupt management function, then try to send request again
		handleEvents();
		if(!Packager::getCurrentBSSID()) // exit if another error occurs
			return false;
	}

	// Poll response for the next 10 seconds
	uint32_t start = millis();
	while(((millis() - start) < GENERAL_TIMEOUT)){
		handleEvents();
		if(gotResponse && responseType == GET_CURR_BSSID_CMD){
			memcpy(bssid, (void *)data, WL_MAC_ADDR_LENGTH);
			return true;
		}
	}

	return false;
}

// ----------------------------------------------------------------- ok
int32_t WiFiClass::RSSI()
{
	handleEvents();
	int32_t rssi = 0;
	
	gotResponse = false;
	responseType = NONE;

	if(!Packager::getCurrentRSSI()){ // packet has not been sent. Maybe an interrupt occurred in the meantime
		// launch interrupt management function, then try to send request again
		handleEvents();
		if(!Packager::getCurrentRSSI()) // exit if another error occurs
			return rssi;
	}

	// Poll response for the next 10 seconds
	uint32_t start = millis();
	while(((millis() - start) < GENERAL_TIMEOUT)){
		handleEvents();
		if(gotResponse && responseType == GET_CURR_RSSI_CMD){
			// copy 4 consecutive byte in the response to get the resulting int32_t
			memcpy(&rssi, (void *)data, 4);
			break;
		}
	}

	return rssi;
}

// ----------------------------------------------------------------- ok
uint8_t WiFiClass::encryptionType()
{
	handleEvents();
	uint8_t enc = 0;
	
	gotResponse = false;
	responseType = NONE;

	if(!Packager::getCurrentEncryptionType()){ // packet has not been sent. Maybe an interrupt occurred in the meantime
		// launch interrupt management function, then try to send request again
		handleEvents();
		if(!Packager::getCurrentEncryptionType()) // exit if another error occurs
			return enc;
	}
	
	// Poll response for the next 10 seconds
	uint32_t start = millis();
	while(((millis() - start) < GENERAL_TIMEOUT)){
		handleEvents();
		if(gotResponse && responseType == GET_CURR_ENCT_CMD){
			enc = *data;
			break;
		}
	}

	return enc;
}

// ----------------------------------------------------------------- ok
int8_t WiFiClass::scanNetworks()
{
	handleEvents();
	uint8_t networksNumber = 0;
	
	gotResponse = false;
	responseType = NONE;

	if(!Packager::startScanNetworks()){ // packet has not been sent. Maybe an interrupt occurred in the meantime
		// launch interrupt management function, then try to send request again
		handleEvents();
		if(!Packager::startScanNetworks()) // exit if another error occurs
			return networksNumber;
	}
	
	// Poll response for the next 10 seconds. If nothing happens return
	uint32_t start = millis();
	while(((millis() - start) < GENERAL_TIMEOUT)){
		handleEvents();
		if(gotResponse && responseType == START_SCAN_NETWORKS){
			networksNumber = *data;
			break;
		}
	}
	
	return networksNumber;
}

// ----------------------------------------------------------------- ok
uint8_t WiFiClass::getScannedNetwork(uint8_t netNum, char *ssid, int32_t& rssi, uint8_t& enc)
{
	handleEvents();
	
	gotResponse = false;
	responseType = NONE;

	if(!Packager::getScanNetworks(netNum)){ // packet has not been sent. Maybe an interrupt occurred in the meantime
		// launch interrupt management function, then try to send request again
		handleEvents();
		if(!Packager::getScanNetworks(netNum)) // exit if another error occurs
			return 0;
	}
	
	// Poll response for the next 10 seconds. If nothing happens return
	uint32_t start = millis();
	while(((millis() - start) < GENERAL_TIMEOUT)){
		handleEvents();
		if(gotResponse && responseType == SCAN_NETWORKS_RESULT){
			uint16_t receivedBytes;
			uint8_t chunkSize = 0;
			int32_t totalLen = pktLen;	//payload size
			uint8_t skipSize = 14;
			
			if(totalLen < 26){
				receivedBytes = totalLen;
			}
			else{
				receivedBytes = 26;
				skipSize = 13;
			}
			
			uint8_t len = data[6];
			uint8_t cpyLen = SPI_BUF_LEN - skipSize;
			uint8_t which = data[0];
			
			rssi = (int32_t)(data[1] + (data[2] << 8) + (data[3] << 16) + (data[4] << 24));
			enc = data[5];
			memset(ssid, 0, 33);
			if(len < SPI_BUF_LEN - skipSize)
				cpyLen = len;

			memcpy(ssid, (uint8_t*)(data + 7), cpyLen);

			if(len > SPI_BUF_LEN - skipSize){
				start = millis();
				gotResponse = false;
				responseType = NONE;
				while(((millis() - start) < GENERAL_TIMEOUT)){
					handleEvents();
					if(gotResponse && responseType == SCAN_NETWORKS_RESULT){
						memcpy((uint8_t*)(ssid + cpyLen), (uint8_t*)(data), commDrv.payloadSize);
						return 1;
					}
				}				
			}
			else
				return 1;
		}
	}
	return 0;
}

char* WiFiClass::SSID(uint8_t networkItem)
{
	return Packager::getSSIDNetoworks(networkItem);
}

int32_t WiFiClass::RSSI(uint8_t networkItem)
{
	handleEvents();
	int32_t rssi = 0;
	
	gotResponse = false;
	responseType = NONE;

	if(!Packager::getRSSINetoworks(networkItem)){ // packet has not been sent. Maybe an interrupt occurred in the meantime
		// launch interrupt management function, then try to send request again
		handleEvents();
		if(!Packager::getRSSINetoworks(networkItem)) // exit if another error occurs
			return rssi;
	}

	// Poll response for the next 10 seconds
	uint32_t start = millis();
	while(((millis() - start) < GENERAL_TIMEOUT)){
		handleEvents();
		if(gotResponse && responseType == GET_IDX_RSSI_CMD){
			// copy 4 consecutive byte in the response to get the resulting int32_t
			memcpy(&rssi, (void *)data, 4);
			break;
		}
	}

		return rssi;
}

uint8_t WiFiClass::encryptionType(uint8_t networkItem)
{
	handleEvents();
	uint8_t enc = ENC_TYPE_UNKNOW;
	
	gotResponse = false;
	responseType = NONE;

	if(!Packager::getEncTypeNetowrks(networkItem)){ // packet has not been sent. Maybe an interrupt occurred in the meantime
		// launch interrupt management function, then try to send request again
		handleEvents();
		if(!Packager::getEncTypeNetowrks(networkItem)) // exit if another error occurs
			return enc;
	}

	// Poll response for the next 10 seconds
	uint32_t start = millis();
	while(((millis() - start) < GENERAL_TIMEOUT)){
		handleEvents();
		if(gotResponse && responseType == GET_IDX_ENCT_CMD){
			enc = *data;
			break;
		}
	}
	return enc;
}

wl_status_t WiFiClass::status()
{
	gotResponse = false;
	responseType = NONE;
	
	handleEvents();
	
	if(connectionStatus == WL_NO_WIFI_MODULE_COMM){	
		if(!Packager::getConnectionStatus()){ // packet has not been sent. Maybe an interrupt occurred in the meantime
			// launch interrupt management function, then try to send request again
			handleEvents();
			if(!Packager::getConnectionStatus()) // exit if another error occurs
				return (wl_status_t)connectionStatus;
		}
		
		uint32_t start = millis();
		while(((millis() - start) < GENERAL_TIMEOUT)){
			handleEvents();
			if(gotResponse && responseType == GET_CONN_STATUS){
				connectionStatus = (wl_status_t)(*data);
				return connectionStatus;
			}
		}
	}

	return connectionStatus;
}

/*
*
*/
int WiFiClass::hostByName(const char* aHostname, IPAddress& aResult)
{
	uint8_t  _ipAddr[WL_IPV4_LENGTH];
	IPAddress dummy(0xFF,0xFF,0xFF,0xFF);
	int result = 0;
	handleEvents();
	
	gotResponse = false;
	responseType = NONE;

	if(!Packager::getHostByName(aHostname)){ // packet has not been sent. Maybe an interrupt occurred in the meantime
		// launch interrupt management function, then try to send request again
		handleEvents();
		if(!Packager::getHostByName(aHostname)) // exit if another error occurs
			return result;
	}

	// Poll response for the next 10 seconds. If nothing happens return 0
	uint32_t start = millis();
	while(((millis() - start) < GENERAL_TIMEOUT)){
		handleEvents();
		if(gotResponse && responseType == GET_HOST_BY_NAME_CMD){
			if(dataLen == 1) // we got an error, return
				return 0;

			memcpy((void*)_ipAddr, (void*)data, dataLen);
			aResult = _ipAddr;
			result = (aResult != dummy);
			break;
		}
	}

	return result;
}


void WiFiClass::disableWebPanel()
{
	handleEvents();

	gotResponse = false;
	responseType = NONE;

	if(!Packager::disableWebPanel()){ // packet has not been sent. Maybe an interrupt occurred in the meantime
		// launch interrupt management function, then try to send request again
		handleEvents();
		if(!Packager::disableWebPanel()) // exit if another error occurs
			return;
	}
}

void WiFiClass::reset()
{
	commDrv.reset();
}

void WiFiClass::off()
{
	commDrv.off();
}

void WiFiClass::on()
{
	commDrv.on();
}

void WiFiClass::cancelNetworkListMem()
{
	Packager::wifiCancelNetList();	
}


WiFiClass WiFi;