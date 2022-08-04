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

#include <string.h>
#include <WiFi.h>
#include "utility/packager.h"

#include "WiFiServer.h"

WiFiServer::WiFiServer(uint16_t port)
{
    _port = port;
}

void WiFiServer::begin()
{
	WiFiClass::handleEvents();
	
	WiFiClass::gotResponse = false;
	WiFiClass::responseType = NONE;
 
    uint8_t _sock = WiFiClass::getSocket();

    if (_sock != NO_SOCKET_AVAIL) {
		if(!Packager::startServer(_port, _sock)){ // packet has not been sent. Maybe an interrupt occurred in the meantime
			// launch interrupt management function, then try to send request again
			WiFiClass::handleEvents();
			if(!Packager::startServer(_port, _sock)){ // exit if another error occurs
				Serial.println("Error while starting server");
				return;
			}
		}

		// Poll response for the next 10 seconds. If nothing happens print an error
		uint32_t start = millis();
		while(((millis() - start) < GENERAL_TIMEOUT)){
			WiFiClass::handleEvents();
			if(WiFiClass::gotResponse && WiFiClass::responseType == START_SERVER_TCP_CMD){
				if(WiFiClass::data[0] != 0){
					WiFiClass::_server_port[_sock] = _port;
					return;
				}
				break;
			}
		}
		Serial.println("Error while starting server");
    }
}

WiFiClient WiFiServer::available(byte* status)
{
	WiFiClass::handleEvents();
	// search for a socket with a client request
	for(uint8_t i = 0; i < MAX_SOCK_NUM; i++){
		WiFiClass::gotResponse = false;
		WiFiClass::responseType = NONE;

		if(!Packager::getClientState(i)){ // packet has not been sent. Maybe an interrupt occurred in the meantime
			// launch interrupt management function, then try to send request again
			WiFiClass::handleEvents();
			if(!Packager::getClientState(i)) // exit if another error occurs
			return -1;
		}

		// Poll flags until we got a response or timeout occurs
		uint32_t start = millis();
		while(((millis() - start) < GENERAL_TIMEOUT)){
			WiFiClass::handleEvents();
			if(WiFiClass::gotResponse && WiFiClass::responseType == GET_CLIENT_STATE_TCP_CMD){
				if(WiFiClass::_state[i] == ESTABLISHED){ // there is a client request
					if(WiFiClass::_server_port[i] == _port){ // request refers to our socket
						WiFiClient client(i);
						return client;
					}
				}
				break;
			}
		}
	}
	return WiFiClient(255);
}

uint8_t WiFiServer::status() 
{
	WiFiClass::handleEvents();

	WiFiClass::gotResponse = false;
	WiFiClass::responseType = NONE;

	if(!Packager::getServerState(0)){ // packet has not been sent. Maybe an interrupt occurred in the meantime
		// launch interrupt management function, then try to send request again
		WiFiClass::handleEvents();
		if(!Packager::getServerState(0)){ // exit if another error occurs
			return 0;
		}
	}
    
	// Poll response for the next 10 seconds. If nothing happens return 0
	uint32_t start = millis();
	while(((millis() - start) < GENERAL_TIMEOUT)){
		WiFiClass::handleEvents();
		if(WiFiClass::gotResponse && WiFiClass::responseType == GET_STATE_TCP_CMD){
			return WiFiClass::data[0];
			break;
		}
	}
	return 0;
}


size_t WiFiServer::write(uint8_t b)
{
    return write(&b, 1);
}

size_t WiFiServer::write(const uint8_t *buffer, size_t size)
{
	size_t n = 0;

	WiFiClass::handleEvents();

    for (int sock = 0; sock < MAX_SOCK_NUM; sock++) {
        if (WiFiClass::_server_port[sock] != 0) {
        	WiFiClient client(sock);

            if (WiFiClass::_server_port[sock] == _port) {
                n+=client.write(buffer, size);
            }
        }
    }
    return n;
}