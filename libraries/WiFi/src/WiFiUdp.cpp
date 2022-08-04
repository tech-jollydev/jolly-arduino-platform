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

extern "C" {
  #include "utility/definitions.h"
}
#include <string.h>
#include "utility/packager.h"

#include <WiFi.h>
#include "WiFiUdp.h"
#include "utility/spi/spi_drv.h"

uint16_t _espSockDataSzUdp = 0;
uint8_t _internalBufUdp[25];
uint8_t _internalBufPtrUdp = 0;
uint16_t _internalBufSzUdp = 0;


/* Constructor */
WiFiUDP::WiFiUDP() : _sock(NO_SOCKET_AVAIL) {}

/* Start WiFiUDP socket, listening at local port PORT */
uint8_t WiFiUDP::begin(uint16_t port) {

	uint8_t sock = WiFiClass::getSocket();
	if (sock != NO_SOCKET_AVAIL)
	{
		if(!Packager::startServer(port, sock, UDP_MODE)){ // packet has not been sent. Maybe an interrupt occurred in the meantime
			// launch interrupt management function, then try to send request again
			WiFiClass::handleEvents();
			if(!Packager::startServer(port, sock, UDP_MODE)) // exit if another error occurs
			return 0;
		}

		// Poll response for the next 10 seconds
		uint32_t start = millis();
		while(((millis() - start) < GENERAL_TIMEOUT)){
			WiFiClass::handleEvents();
			if(WiFiClass::gotResponse && WiFiClass::responseType == START_SERVER_TCP_CMD){
				if(WiFiClass::data[0] != 0){
					WiFiClass::_server_port[sock] = port;
					_sock = sock;
					_port = port;
					return 1;
				}
			}
		}
	}
	return 0;

}

/* return number of bytes available in the current packet*/
int WiFiUDP::available() {
	WiFiClass::handleEvents();

	if(_sock != NO_SOCKET_AVAIL){
		if(_internalBufSzUdp > 0)
			return _internalBufSzUdp;

		if(_espSockDataSzUdp > 0)
			return _espSockDataSzUdp;

		WiFiClass::gotResponse = false;
		WiFiClass::responseType = NONE;

		if(!Packager::getAvailable(_sock)){ // packet has not been sent. Maybe an interrupt occurred in the meantime
			// launch interrupt management function, then try to send request again
			WiFiClass::handleEvents();
			if(!Packager::getAvailable(_sock)) // exit if another error occurs
			return -1;
		}

		// Poll flags until we got a response or timeout occurs
		uint32_t start = millis();
		while(((millis() - start) < GENERAL_TIMEOUT)){
			WiFiClass::handleEvents();
			if(WiFiClass::gotResponse && WiFiClass::responseType == AVAIL_DATA_TCP_CMD){
				// copy int value
				memcpy((uint8_t*)&WiFiClass::_client_data[_sock], &WiFiClass::data[3], 2);
				_espSockDataSzUdp = WiFiClass::_client_data[_sock];
				return _espSockDataSzUdp;
			}
		}
	}
	return -1;
}

/* Release any resources being used by this WiFiUDP instance */
void WiFiUDP::stop(){
	if (_sock == NO_SOCKET_AVAIL)
		return;

	WiFiClass::handleEvents();
	 	
	WiFiClass::gotResponse = false;
	WiFiClass::responseType = NONE;

	if(!Packager::stopClient(_sock)){ // packet has not been sent. Maybe an interrupt occurred in the meantime
		// launch interrupt management function, then try to send request again
		WiFiClass::handleEvents();
		if(!Packager::stopClient(_sock)) // exit if another error occurs
		return;
	}

	// Poll flags until we got a response or timeout occurs
	uint32_t start = millis();
	while(((millis() - start) < GENERAL_TIMEOUT)){
		WiFiClass::handleEvents();
		if(WiFiClass::gotResponse && WiFiClass::responseType == STOP_CLIENT_TCP_CMD){
			break;
		}
	}

	WiFiClass::_state[_sock] = CLOSED;
	_espSockDataSzUdp = _internalBufPtrUdp = _internalBufSzUdp = 0;
	_sock = NO_SOCKET_AVAIL;
}

int WiFiUDP::beginPacket(const char *host, uint16_t port)
{
	// Look up the host first
	IPAddress remote_addr;
	if (WiFi.hostByName(host, remote_addr)) {
		return beginPacket(remote_addr, port);
	}
	return 0;
}

int WiFiUDP::beginPacket(IPAddress ip, uint16_t port)
{
  if (_sock == NO_SOCKET_AVAIL)
	  _sock = WiFiClass::getSocket();
  if (_sock != NO_SOCKET_AVAIL)
  {
	WiFiClass::handleEvents();
	  		
	WiFiClass::gotResponse = false;
	WiFiClass::responseType = NONE;

	if(!Packager::startClient(uint32_t(ip), port, _sock, UDP_MODE)){ // packet has not been sent. Maybe an interrupt occurred in the meantime
		// launch interrupt management function, then try to send request again
		WiFiClass::handleEvents();
		if(!Packager::startClient(uint32_t(ip), port, _sock, UDP_MODE)) // exit if another error occurs
		return 0;
	}

	// Poll flags until we got a response or timeout occurs
	uint32_t start = millis();
	while(((millis() - start) < GENERAL_TIMEOUT)){
		WiFiClass::handleEvents();
		if(WiFiClass::gotResponse && WiFiClass::responseType == START_CLIENT_TCP_CMD){
			if(!WiFiClass::data[0]) // error while starting client
				return 0;
			break;
		}
	}
	WiFiClass::_state[_sock] = ESTABLISHED;
	return 1;
  }
  return 0;
}

int WiFiUDP::endPacket()
{
	WiFiClass::handleEvents();
		
	WiFiClass::gotResponse = false;
	WiFiClass::responseType = NONE;

	if(!Packager::sendUdpData(_sock)){ // packet has not been sent. Maybe an interrupt occurred in the meantime
		// launch interrupt management function, then try to send request again
		WiFiClass::handleEvents();
		if(!Packager::sendUdpData(_sock)) // exit if another error occurs
		return 0;
	}

	// Poll flags until we got a response or timeout occurs
	uint32_t start = millis();
	while(((millis() - start) < GENERAL_TIMEOUT)){
		WiFiClass::handleEvents();
		if(WiFiClass::gotResponse && WiFiClass::responseType == SEND_DATA_UDP_CMD){
			if(WiFiClass::data[0] == 1){
				//reset data available
				_internalBufSzUdp = 0;
				_espSockDataSzUdp = 0;
				return 1;
			}
			break;
		}
	}
	return 0;
}

size_t WiFiUDP::write(uint8_t byte)
{
  return write(&byte, 1);
}

size_t WiFiUDP::write(const uint8_t *buffer, size_t size)
{

	WiFiClass::handleEvents();
	
	if(!Packager::sendData(_sock, buffer, size)) { // packet has not been sent. Maybe an interrupt occurred in the meantime
		// launch interrupt management function, then try to send request again
		WiFiClass::handleEvents();
		if(!Packager::sendData(_sock, buffer, size)) // exit if another error occurs
		return 0;
	}
	return size;
}

int WiFiUDP::parsePacket()
{
	return available();
}

int WiFiUDP::read()
{
	if(_internalBufSzUdp > 0){
		uint8_t ret = _internalBufUdp[_internalBufPtrUdp++];
		_internalBufSzUdp--;
		
		return ret;
	}
	
	WiFiClass::handleEvents();
	
	WiFiClass::gotResponse = false;
	WiFiClass::responseType = NONE;

	uint16_t sz = min(_espSockDataSzUdp, 25);
	
	if(!Packager::getDataBuf(_sock, sz)){ // packet has not been sent. Maybe an interrupt occurred in the meantime
		// launch interrupt management function, then try to send request again
		WiFiClass::handleEvents();
		if(!Packager::getDataBuf(_sock, sz)) // exit if another error occurs
			return -1;
	}
	
	// Poll flags until we got a response or timeout occurs
	uint32_t start = millis();
	while(((millis() - start) < GENERAL_TIMEOUT)){
		WiFiClass::handleEvents();
		if(WiFiClass::gotResponse && WiFiClass::responseType == GET_DATABUF_TCP_CMD){
			_internalBufPtrUdp = 0;
			_internalBufSzUdp = (int16_t)WiFiClass::pktLen;
			memset(_internalBufUdp, 0, 25);
			if(_internalBufSzUdp <= 0){
				_espSockDataSzUdp = 0;
				_internalBufSzUdp = 0;
				return -1;
			}
			memcpy(_internalBufUdp, (uint8_t *)WiFiClass::data, _internalBufSzUdp);
				
			uint8_t ret = _internalBufUdp[_internalBufPtrUdp++];
			_espSockDataSzUdp -= _internalBufSzUdp;
			_internalBufSzUdp--;
				
			return ret;

		}
	}
	
	return -1;
}

int WiFiUDP::read(unsigned char* buffer, size_t len)
{
	WiFiClass::handleEvents();
	
	WiFiClass::gotResponse = false;
	WiFiClass::responseType = NONE;

	
	if(!Packager::getDataBuf(_sock, len)){ // packet has not been sent. Maybe an interrupt occurred in the meantime
		// launch interrupt management function, then try to send request again
		WiFiClass::handleEvents();
		if(!Packager::getDataBuf(_sock, len)) // exit if another error occurs
		return -1;
	}
	
	// Poll flags until we got a response or timeout occurs
	uint32_t start = millis();
	while(((millis() - start) < GENERAL_TIMEOUT)){
		WiFiClass::handleEvents();
		if(WiFiClass::gotResponse && WiFiClass::responseType == GET_DATABUF_TCP_CMD){

			uint16_t receivedBytes;
			uint8_t chunkSize = 0;
			uint32_t totalLen = WiFiClass::pktLen;

			if(totalLen <= 0){ // No data was read. Maybe an error occurred. Restore available flag and exit
				WiFiClass::_client_data[_sock] = 0;
				_espSockDataSzUdp = 0;
				return -1;
			}

			if(totalLen < 26)
				receivedBytes = totalLen;
			else
				receivedBytes = 26;

			memcpy(buffer, (uint8_t *)WiFiClass::data, receivedBytes);

			while(receivedBytes < totalLen && (millis() - start) < 15000){ // wait 15 seconds at maximum for receiving the whole message. Otherwise consider that an error occurred
				chunkSize = WiFiClass::getAvailableData();
				if(chunkSize > 0){
					memcpy(&buffer[receivedBytes], (void *)WiFiClass::data, chunkSize);
					receivedBytes += chunkSize;
				}
			}
			WiFiClass::_client_data[_sock] -= receivedBytes;
			_espSockDataSzUdp -= receivedBytes;
			return receivedBytes;
		}
	}
	commDrv.multiRead = false;
	WiFiClass::_client_data[_sock] = 0;
	_espSockDataSzUdp = 0;
	return -1;
}

int WiFiUDP::peek()
{
	if(_internalBufSzUdp > 0){
		uint8_t ret = _internalBufUdp[_internalBufPtrUdp];
		return ret;
	}

	WiFiClass::handleEvents();

	WiFiClass::gotResponse = false;
	WiFiClass::responseType = NONE;

	uint8_t sz = min(_espSockDataSzUdp, 25);
	
	if(!Packager::getDataBuf(_sock, sz)){ // packet has not been sent. Maybe an interrupt occurred in the meantime
		// launch interrupt management function, then try to send request again
		WiFiClass::handleEvents();
		if(!Packager::getDataBuf(_sock, sz)) // exit if another error occurs
		return -1;
	}

	// Poll flags until we got a response or timeout occurs
	uint32_t start = millis();
	while(((millis() - start) < GENERAL_TIMEOUT)){
		WiFiClass::handleEvents();
		if(WiFiClass::gotResponse && WiFiClass::responseType == GET_DATABUF_TCP_CMD){
			_internalBufPtrUdp = 0;
			_internalBufSzUdp = (int16_t)WiFiClass::dataLen;
			memset(_internalBufUdp, 0, 25);
			if(_internalBufSzUdp <= 0){
				_espSockDataSzUdp = 0;
				_internalBufSzUdp = 0;
				return -1;
			}
			memcpy(_internalBufUdp, (uint8_t *)WiFiClass::data, _internalBufSzUdp);
			
			uint8_t ret = _internalBufUdp[_internalBufPtrUdp];
			_espSockDataSzUdp -= _internalBufSzUdp;
			
			return ret;

		}
	}
	
	return -1;
}

void WiFiUDP::flush()
{
  while (available())
    read();
}

IPAddress  WiFiUDP::remoteIP()
{
	uint8_t _remoteIp[4] = {0};

	WiFiClass::handleEvents();

	WiFiClass::gotResponse = false;
	WiFiClass::responseType = NONE;

	if(!Packager::getRemoteData(_sock)){ // packet has not been sent. Maybe an interrupt occurred in the meantime
		// launch interrupt management function, then try to send request again
		WiFiClass::handleEvents();
		if(!Packager::getRemoteData(_sock)){ // exit if another error occurs
			IPAddress ip(_remoteIp);
			return ip;
		}
	}

	// Poll flags until we got a response or timeout occurs
	uint32_t start = millis();
	while(((millis() - start) < GENERAL_TIMEOUT)){
		WiFiClass::handleEvents();
		if(WiFiClass::gotResponse && WiFiClass::responseType == GET_REMOTE_DATA_CMD){
			if(WiFiClass::dataLen == 4){ // parse received ip address
				memcpy(_remoteIp, (void*)WiFiClass::data, 4);
			}
			break;
		}
	}
	IPAddress ip(_remoteIp);
	return ip;
}

uint16_t  WiFiUDP::remotePort()
{
	uint16_t port = 0;

	WiFiClass::handleEvents();

	WiFiClass::gotResponse = false;
	WiFiClass::responseType = NONE;

	if(!Packager::getRemoteData(_sock)){ // packet has not been sent. Maybe an interrupt occurred in the meantime
		// launch interrupt management function, then try to send request again
		WiFiClass::handleEvents();
		if(!Packager::getRemoteData(_sock)) // exit if another error occurs
			return port;
	}

	// Poll flags until we got a response or timeout occurs
	uint32_t start = millis();
	while(((millis() - start) < GENERAL_TIMEOUT)){
		WiFiClass::handleEvents();
		if(WiFiClass::gotResponse && WiFiClass::responseType == GET_REMOTE_DATA_CMD){
			if(WiFiClass::dataLen == 4){ // response seems ok, we've got Ip as first parameter and port as second one
				port = (WiFiClass::data[5]<<8) + WiFiClass::data[6];
			}
			break;
		}
	}
	return port;
}
