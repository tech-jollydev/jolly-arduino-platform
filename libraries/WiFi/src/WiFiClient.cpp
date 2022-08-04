
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

#include "utility/definitions.h"
#include "string.h"

#include <WiFi.h>
#include "WiFiClient.h"
#include "utility/packager.h"
#include "utility/spi/spi_drv.h"

#define ATTEMPTS 350

uint8_t client_status = 0;
int attempts_conn = 0;

uint8_t _internalBuf[25];
uint8_t _internalBufPtr = 0;
int16_t _internalBufSz = 0;
int16_t _espSockDataSz = 0;


WiFiClient::WiFiClient() : _sock(MAX_SOCK_NUM) {
}

WiFiClient::WiFiClient(uint8_t sock) : _sock(sock) {
}

int WiFiClient::connect(const char* host, uint16_t port) 
{
	IPAddress remote_addr;
	if (WiFi.hostByName(host, remote_addr)) {
		return connect(remote_addr, port);
	}
	return 0;
}

int WiFiClient::connect(IPAddress ip, uint16_t port) 
{
	_sock = getFirstSocket();
    if (_sock != NO_SOCKET_AVAIL) {
		WiFiClass::handleEvents();
		
		WiFiClass::gotResponse = false;
		WiFiClass::responseType = NONE;
    	
		if(!Packager::startClient(uint32_t(ip), port, _sock)){ // packet has not been sent. Maybe an interrupt occurred in the meantime
			// launch interrupt management function, then try to send request again
			WiFiClass::handleEvents();
			if(!Packager::startClient(uint32_t(ip), port, _sock)) // exit if another error occurs
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
    }
	else{
    	Serial.println("No Socket available");
    	return 0;
    }

   	WiFiClass::_state[_sock] = ESTABLISHED;
    return 1;
}

size_t WiFiClient::write(uint8_t b) 
{
	  return write(&b, 1);
}

size_t WiFiClient::write(const uint8_t *buf, size_t size)
{
	WiFiClass::handleEvents();
	
	if(!Packager::sendData(_sock, buf, size)) { // packet has not been sent. Maybe an interrupt occurred in the meantime
		// launch interrupt management function, then try to send request again
		WiFiClass::handleEvents();
		if(!Packager::sendData(_sock, buf, size)) // exit if another error occurs
		return 0;
	}
	return size;
}

int WiFiClient::available() 
{
	WiFiClass::handleEvents();

	if(_sock != 255){
		if(_internalBufSz > 0){
			return _internalBufSz;
		}

		if(_espSockDataSz > 0){
			return _espSockDataSz;
		}

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
				_espSockDataSz = WiFiClass::_client_data[_sock];
				return _espSockDataSz;
			}
		}
	}
	return -1;
}

int WiFiClient::read()
{
	if(_internalBufSz > 0){
		uint8_t ret = _internalBuf[_internalBufPtr++];
		_internalBufSz--;

		return ret;
	}

	WiFiClass::handleEvents();
	
	WiFiClass::gotResponse = false;
	WiFiClass::responseType = NONE;

	uint16_t sz = min(_espSockDataSz, 25);
	
	if(!Packager::getDataBuf(_sock, sz)){ // packet has not been sent. Maybe an interrupt occurred in the meantime
		// launch interrupt management function, then try to send request again
		WiFiClass::handleEvents();
		delay(1);
		WiFiClass::handleEvents();
		if(!Packager::getDataBuf(_sock, sz)) // exit if another error occurs
		return -1;
	}

	// Poll flags until we got a response or timeout occurs
	uint32_t start = millis();
	while(((millis() - start) < GENERAL_TIMEOUT)){
		WiFiClass::handleEvents();
		if(WiFiClass::gotResponse && WiFiClass::responseType == GET_DATABUF_TCP_CMD){
			_internalBufPtr = 0;
			_internalBufSz = (int8_t)WiFiClass::pktLen;
			memset(_internalBuf, 0, 25);
			if(_internalBufSz <= 0){
				_espSockDataSz = 0;
				_internalBufSz = 0;
				return -1;
			}
			memcpy(_internalBuf, (uint8_t *)WiFiClass::data, _internalBufSz);
			
			uint8_t ret = _internalBuf[_internalBufPtr++];
			_espSockDataSz -= _internalBufSz;
			_internalBufSz--;

			return ret;
		}
	}

	return -1;
}

int WiFiClient::read(uint8_t* buf, size_t size) {
	WiFiClass::handleEvents();
	WiFiClass::gotResponse = false;
	WiFiClass::responseType = NONE;

	if(!Packager::getDataBuf(_sock, size)){ // packet has not been sent. Maybe an interrupt occurred in the meantime
		// launch interrupt management function, then try to send request again
		WiFiClass::handleEvents();
		if(!Packager::getDataBuf(_sock, size)) // exit if another error occurs
		return -1;
	}

	// Poll flags until we got a response or timeout occurs
	uint32_t start = millis();
	while(((millis() - start) < GENERAL_TIMEOUT)){
		WiFiClass::handleEvents();
		if(WiFiClass::gotResponse && WiFiClass::responseType == GET_DATABUF_TCP_CMD){
			uint16_t receivedBytes;
			uint8_t chunkSize = 0;
			int32_t totalLen = WiFiClass::pktLen;
			
			if(totalLen <= 0){ // No data was read. Maybe an error occurred. Restore available flag and exit
				WiFiClass::_client_data[_sock] = 0;
				_espSockDataSz = 0;
				return -1;
			}

			if(totalLen < 26)
			receivedBytes = totalLen;
			else
			receivedBytes = 26;
			
			memcpy(buf, (uint8_t *)WiFiClass::data, receivedBytes);

			while(receivedBytes < totalLen && (millis() - start) < 15000){ // wait 15 seconds at maximum for receiving the whole message. Otherwise consider that an error occurred
				chunkSize = WiFiClass::getAvailableData();
				if(chunkSize > 0){
					memcpy(&buf[receivedBytes], (void *)WiFiClass::data, chunkSize);
					receivedBytes += chunkSize;
				}
			}
			WiFiClass::_client_data[_sock] -= receivedBytes;
			_espSockDataSz -= receivedBytes;
			return receivedBytes;
		}
	}

	commDrv.multiRead = false;
	WiFiClass::_client_data[_sock] = 0;
	_espSockDataSz = 0;
	return -1;
}

int WiFiClient::peek() {
	
	if(_internalBufSz > 0){
		uint8_t ret = _internalBuf[_internalBufPtr];
		return ret;
	}

	WiFiClass::handleEvents();

	WiFiClass::gotResponse = false;
	WiFiClass::responseType = NONE;

	uint8_t sz = min(_espSockDataSz, 25);
	
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
			_internalBufPtr = 0;
			_internalBufSz = (int16_t)WiFiClass::dataLen;
			memset(_internalBuf, 0, 25);
			if(_internalBufSz <= 0){
				_espSockDataSz = 0;
				_internalBufSz = 0;
				return -1;
			}
			memcpy(_internalBuf, (uint8_t *)WiFiClass::data, _internalBufSz);
			
			uint8_t ret = _internalBuf[_internalBufPtr];
			_espSockDataSz -= _internalBufSz;
						
			return ret;
		}
	}
	
	return -1;
}

void WiFiClient::flush() {
  while (available() > 0)
    read();
}

void WiFiClient::stop() {

	if (_sock == 255)
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
  _espSockDataSz = _internalBufPtr = _internalBufSz = 0;
  client_status = 0;
  _sock = 255;
}

uint8_t WiFiClient::connected() {

  if (_sock == 255) {
    return 0;
  } else {
      uint8_t s;
      attempts_conn++;
      if(client_status == 0 || attempts_conn > ATTEMPTS){    //EDIT by Andrea
        client_status = status();
        s = client_status;
        attempts_conn = 0;
      }
      else
        s = client_status;

        // client_status = status();
        // s = client_status;
      return !(s == LISTEN || s == CLOSED || s == FIN_WAIT_1 ||
      		s == FIN_WAIT_2 || s == TIME_WAIT ||
      		s == SYN_SENT || s== SYN_RCVD ||
      		(s == CLOSE_WAIT));
  }
}

uint8_t WiFiClient::status() {
    if (_sock == 255)
	    return CLOSED;

	WiFiClass::handleEvents();

	WiFiClass::gotResponse = false;
	WiFiClass::responseType = NONE;

	if(!Packager::getClientState(_sock)){ // packet has not been sent. Maybe an interrupt occurred in the meantime
		// launch interrupt management function, then try to send request again
		WiFiClass::handleEvents();
		if(!Packager::getClientState(_sock)) // exit if another error occurs
		return -1;
	}

	// Poll flags until we got a response or timeout occurs
	uint32_t start = millis();
	while(((millis() - start) < GENERAL_TIMEOUT)){
		WiFiClass::handleEvents();
		if(WiFiClass::gotResponse && WiFiClass::responseType == GET_CLIENT_STATE_TCP_CMD){
			// state already updated in wifiDrvCB function
			break;
		}
	}
	return WiFiClass::_state[_sock];
}

WiFiClient::operator bool() {
  return _sock != 255;
}

// Private Methods
uint8_t WiFiClient::getFirstSocket()
{
    for (int i = 0; i < MAX_SOCK_NUM; i++) {
      if (WiFiClass::_state[i] == CLOSED)
      {
          return i;
      }
    }
    return NO_SOCKET_AVAIL;
}