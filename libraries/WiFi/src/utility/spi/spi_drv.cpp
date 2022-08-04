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
#include "Arduino.h"
#include <SPI.h>
#include "spi_drv.h"
#include "pins_arduino.h"

static tpDriverIsr spiIsr;

volatile bool srLevelInMultipacket = false;
volatile bool SpiDrv::_interruptReq;
volatile teEspStatus SpiDrv::espStatus = esp_wait_ready;
volatile bool SpiDrv::multiRead = false;
volatile bool SpiDrv::multiWrite = false;

//SPI commands to manage the WiFi library
enum ESP_SPI_COMMANDS
{
	ESP8266_STATUS_READ		= 0x04,
	ESP8266_STATUS_WRITE	= 0X01,
	ESP8266_DATA_READ		= 0x03,
	ESP8266_DATA_WRITE		= 0x02
} ESP_SPI_COMMANDS;



/* -----------------------------------------------------------------
* Slave ready callback, needed to regulate the esp-328p communication.
* There are 3 states of the esp:
* - busy: when no communication is established or the esp is busy in some operation;
* - ready: when communication is established and/or the esp has some data available for the 328p;
* - isr: when without an established communication, esp requires attention from the 328p. 
* In this case the esp strobes the sr signal to communicate the 328p to perform a callback -> _interruptReq = true
*/
void _SRcallback(void)
{
	if (digitalRead(SLAVEREADY) == HIGH) {
		if(!SpiDrv::multiWrite && !SpiDrv::multiRead){
			SpiDrv::espStatus = esp_busy;	// could it be an isr or an ack
			SpiDrv::_interruptReq = true;	// assume it is an isr
		}
		else{
			if(SpiDrv::multiRead){
				SpiDrv::_interruptReq = true;	// assume it is an isr
			}
			srLevelInMultipacket = HIGH;
		}	
	}
	else {
		SpiDrv::espStatus = esp_idle;
		if(SpiDrv::multiWrite || SpiDrv::multiRead)
			srLevelInMultipacket = LOW;
	}
}

/* -----------------------------------------------------------------
* the 328p addresses the esp via SPI interf. When the CS signal is active 
* LOW the SPI interface is asserted
*/
void SpiDrv::_enableDevice(void)
{
	digitalWrite(_ss_pin, LOW);
	_ss_status = ss_low;
}

/* -----------------------------------------------------------------
* the 328p releases the esp via SPI interf. When the CS signal is active 
* HIGH the SPI interface is released
*/
void SpiDrv::_disableDevice(void)
{
	digitalWrite(_ss_pin, HIGH);
	_ss_status = ss_high;
}



/* -----------------------------------------------------------------
* Wait for esp ready to work with the 328p - checks the _espStatus set by the _SRcallback 
* waiting for ESP_SR_TIMEOUT millisecs (max) to get an _espStatus change.
* 
* return: (boolean)
*		  true if the status is the one requested
*		  false otherwise.
*/

bool SpiDrv::_checkEspStatusTimeout(teEspStatus checkStat)
{
	(void) checkStat;
	
	uint32_t t_check = millis() + ESP_SR_TIMEOUT;
	uint32_t t_hold = 0;
	
	while (t_check > millis() && espStatus != esp_idle){
		// the ESP has something for the 328
		if(espStatus == esp_busy){
			t_hold = millis() + 100;
			// wait until it finishes to flush out the message
			while(t_hold > millis() && espStatus == esp_busy){
				// read it...
				handleSPIEvents();
			}
			//Has it finished?
			if(espStatus == esp_idle)
				// let's write.
				return true;
		}
	}
	
	// is ESP ready to get a write?
	if(espStatus == esp_idle){
		// let's write
		return true;
	}
	
	return false;
}

/* -----------------------------------------------------------------
* Checks the _sr_pin status waiting for ESP_SR_TIMEOUT millisecs (max) to get the value 
* requested (checkStat) equal to the pin state.
*
* return: (boolean)
*		  true if the status is the one requested
*		  false otherwise.
*/
bool SpiDrv::_checkSRpinStatusTimeout(bool checkStat)
{
	t = millis();
	while (t + ESP_SR_TIMEOUT > millis() && digitalRead(_sr_pin) != checkStat);
	return (checkStat == digitalRead(_sr_pin));
}

/* -----------------------------------------------------------------
* Initializes the _wifiBuf buffer with the b byte and sets the buffer index to 1;
*/
void SpiDrv::_txBufInitWByte(uint8_t b)
{
	wifiBuf[0] = b;
	_txIndex = 1;
}

/* -----------------------------------------------------------------
* Appends to the _wifiBuf buffer a new b byte if there's enough space;
*
* return: (boolean)
*		  true byte queued
*		  false otherwise.
*/
bool SpiDrv::_txBufAppendByte(uint8_t b)
{
	if (_txIndex < SPI_TXBUF_LEN) {
		wifiBuf[_txIndex++] = b;
		return true;
	}
	return false;
}

/* -----------------------------------------------------------------
* Set the overall len of the command packet
*/
void SpiDrv::_txBufSetOverallLen(bool dataPkt)
{
	if(dataPkt){ // len is 4 bytes
		wifiBuf[2] = _txIndex & 0xFF;
		wifiBuf[3] = (_txIndex >> 8) & 0xFF;
		wifiBuf[4] = (_txIndex >> 16) & 0xFF;
		wifiBuf[5] = (_txIndex >> 24) & 0xFF;
	}
	else // len is 1 byte
		wifiBuf[2] = _txIndex;
}


/* -----------------------------------------------------------------
* Finalizes the packet adding the END_CMD and writing out the command
*/
bool SpiDrv::_txBufFinalizePacket(bool dataPkt)
{
	// add the END_CMD to the buffer
	_txBufAppendByte(END_CMD);
	
	_txBufSetOverallLen(dataPkt);
	
	// write out the data (32 byte minimum) to esp
	return writeData(wifiBuf, _txIndex);

}

/*
*
*/
bool SpiDrv::_askStatusInit(uint8_t attempts)
{
	uint32_t timeout;
	
	while(attempts > 0){
		attempts--;
		_enableDevice();

		// wait for the sr signal high - esp is ready
		if(_checkEspStatusTimeout(esp_busy)){
			SPI.transfer((uint8_t)(ESP8266_DATA_WRITE));
			SPI.transfer((uint8_t)(DUMMY_DATA));
			
			SPI.transfer(START_CMD);
			SPI.transfer(GET_CONN_STATUS);
			SPI.transfer(5);
			SPI.transfer(0);
			SPI.transfer(END_CMD);
			for (uint16_t j = 0; j < SPI_BUF_LEN - 5; j++) {
				SPI.transfer(0);
			}
		}
		_disableDevice();
		delay(1);
	
		timeout = millis() + 100;
		while(timeout > millis()) {
			if(espStatus == esp_busy){
				handleSPIEvents();
				_disableDevice();
				return true;
			}
		}
	}
	
	return false;
}

/* -----------------------------------------------------------------
* Class constructor
*/
SpiDrv::SpiDrv(void)
{
	_ss_pin = SLAVESELECT;
	_sr_pin = SLAVEREADY;
	
	spiIsr = NULL;
}

/* -----------------------------------------------------------------
* Class begin
*/
void SpiDrv::begin()
{
	_spi_status = SPIerror;	//currently unused
	espStatus = esp_idle;
	_ss_status = ss_high;
	_interruptReq = false;
	rxIndex = _txIndex = payloadSize = 0;
	repetedError = 10;

	pSpiDrv = this;

	memset(wifiBuf, 0, SPI_TXBUF_LEN);

	// this sets the sr pin from esp to 328 and attaches to it a PINCHANGE interrupt.
	pinMode(_sr_pin, INPUT);
	PCICR |= (1 << PCIE3);
	PCMSK3 |= (1 << PCINT24);

	// init master SPI interface
	SPI.begin();
	pinMode(_ss_pin, OUTPUT);
	digitalWrite(_ss_pin, HIGH);
}

void SpiDrv::off()
{
	pinMode(21, OUTPUT);
	digitalWrite(21, LOW);

	espStatus = esp_wait_ready;
	// NOTE: it requires to call the init to re-initialize the ESP module.
	_espFirstLink = false;
}

void SpiDrv::on()
{
	pinMode(21, OUTPUT);
	digitalWrite(21, HIGH);
}

/* -----------------------------------------------------------------
* Performs an esp HW reset sequence.
*/
void SpiDrv::reset()
{
	off();
	delay(500);
	on();
	delay(200);
}

/* -----------------------------------------------------------------
* Closes SPI interface
*/
void SpiDrv::end()
{
	SPI.end();
}

/* -----------------------------------------------------------------
* Reads the Status register of the esp
*
* return: (int)
*		  true SPI Status register
*		  false otherwise.
*/
int SpiDrv::readStatus(bool ctrlReq)
{
	uint32_t ret = 0;

	// if needed pull-down the ss signal
	if(ctrlReq)
	_enableDevice();

	// wait for the sr to go high = ready
	if(_checkSRpinStatusTimeout(HIGH)){
		// transfer the read status register request command to esp
		SPI.transfer(ESP8266_STATUS_READ);
		// read reply (big-endian mode)
		ret = (SPI.transfer(0) |
		((uint32_t)(SPI.transfer(0)) << 8) |
		((uint32_t)(SPI.transfer(0)) << 16) |
		((uint32_t)(SPI.transfer(0)) << 24));
	}
	else
	_spi_status = SPItimeout; //TODO what to do? reset device? TODO

	if(!ctrlReq)
	return((int)ret);
	
	// Pull-up the ss signal
	_disableDevice();

	// wait fot the sr to go low = idle (not busy in this case)
	if(_checkSRpinStatusTimeout(LOW))
	_spi_status = SPIerror; //TODO what to do? reset device? TODO

	_spi_status = SPIok;

	return ((int)ret);
}

/* -----------------------------------------------------------------
* Writes the Status register of the esp
* It requires also the checkLevel parameter on the base of the function that call it.
*/
void SpiDrv::writeStatus(uint32_t status, bool ctrlReq, bool checkLevel)
{
	// if needed pull-down the ss signal
	if(ctrlReq)
	_enableDevice();

	// wait for the sr go to the required level = ready
	if(_checkSRpinStatusTimeout(checkLevel)){
		// transfer the write status register request command to esp
		SPI.transfer(ESP8266_STATUS_WRITE);
		// transfer the new status (big-endian mode) 
		SPI.transfer(status & 0xFF);
		SPI.transfer((status >> 8) & 0xFF);
		SPI.transfer((status >> 16) & 0xFF);
		SPI.transfer((status >> 24) & 0xFF);
	}
	else
	_spi_status = SPItimeout; //TODO what to do? reset device? TODO

	// If require pull-up the ss signal
	if(ctrlReq)
	_disableDevice();

	if(_checkSRpinStatusTimeout(checkLevel))
	_spi_status = SPIerror; //TODO what to do? reset device? TODO

	_spi_status = SPIok;
}

/*
*
*/
bool SpiDrv::establishESPConnection()
{
	uint32_t timeout = millis() + 1000;
	if(_espFirstLink == false){
		_espFirstLink = _askStatusInit(WL_MAX_ATTEMPT_CONNECTION);
	}
	return _espFirstLink;
}

/* Cmd Struct Message
*  _________________________________________________________________________________ 
* | START CMD | C/R  | CMD  |[TOT LEN]| N.PARAM | PARAM LEN | PARAM  | .. | END CMD |
* |___________|______|______|_________|_________|___________|________|____|_________|
* |   8 bit   | 1bit | 7bit |  8bit   |  8bit   |   8bit    | nbytes | .. |   8bit  |
* |___________|______|______|_________|_________|___________|________|____|_________|
*/
/* Data Struct Message
* __________________________________________________________________________________
*| DATA PKT  | C/R  | CMD  |  OVERALL  |  SOCKET  |  PAYLOAD  | .. |  END DATA PKT |
*|___________|______|______|___ LEN____|__________|___________|____|_______________|
*|   8 bit   | 1bit | 7bit |   32bit   |  8 bit   |   nbytes  | .. |      8bit     |
*|___________|______|______|___________|__________|___________|____|_______________|
*/

/*
*
*/
uint32_t SpiDrv::sendData(uint8_t cmd, uint8_t* data, uint32_t len)
{
	uint32_t txBytes = 26;
	uint8_t nPacket = (len - txBytes) >> 5;
	uint8_t lastBytes = (len - txBytes) - (nPacket << 5);

	_enableDevice();
	
	// wait for the sr signal high - esp is ready
	if(_checkSRpinStatusTimeout(HIGH)){
		// send data write command
		SPI.transfer((uint8_t)(ESP8266_DATA_WRITE));
		SPI.transfer((uint8_t)(DUMMY_DATA));
		
		// ---------------------- Start of data packet
		// send the START_CMD
		SPI.transfer(DATA_PKT);
		// send the cmd + the complement of the REPLY_FLAG
		SPI.transfer(cmd & ~(REPLY_FLAG));
		// send the overall length
		SPI.transfer(len & 0xFF);
		SPI.transfer((len >> 8) & 0xFF);
		SPI.transfer((len >> 16) & 0xFF);
		SPI.transfer((len >> 24) & 0xFF);
		
		for(uint8_t i=0; i<26; i++){
			SPI.transfer(data[i]);
		}
	}
	
	for(uint8_t i=0; i<nPacket; i++){
		// wait for the sr signal high - esp is ready
		if(_checkSRpinStatusTimeout(HIGH)){
			for(uint8_t j=0; j<32; j++){
				SPI.transfer(data[j+txBytes]);
			}
			txBytes += 32;
		}
		else
		return txBytes;
	}
	
	if(lastBytes == 0)
	return txBytes;
	
	// wait for the sr signal high - esp is ready
	if(_checkSRpinStatusTimeout(HIGH)){
		for(uint8_t i=0; i<32; i++){
			if(i<lastBytes)
			SPI.transfer(data[i+txBytes]);
			else
			SPI.transfer(0);
		}
		txBytes += lastBytes;
	}
	
	return txBytes;
}

/* -----------------------------------------------------------------
* Prepares the _wifiBuf to be sent as a new command to esp
* It requires also the number of attached parameters.
*/
bool SpiDrv::sendCmd(uint8_t cmd, uint8_t numParam)
{
	// init the buffer (32 bytes) with the START_CMD
	_txBufInitWByte(START_CMD);
	// attach the cmd + the complement of the REPLY_FLAG
	_txBufAppendByte(cmd & ~(REPLY_FLAG));
	// put the space for the overall len - 1 byte
	_txBufAppendByte(0);
	// add the number of parameters
	_txBufAppendByte(numParam);
	
	// if no parameters
	if (numParam == 0) {
		return _txBufFinalizePacket();
	}

	return true;
}

/* -----------------------------------------------------------------
* Prepares the _wifiBuf to be sent as a new data pkt to esp
* It requires also the number of attached parameters.
*/
void SpiDrv::sendDataPkt(uint8_t cmd, uint8_t numParam)
{
	// init the buffer (32 bytes) with the START_CMD
	_txBufInitWByte(DATA_PKT);
	// attach the cmd + the complement of the REPLY_FLAG
	_txBufAppendByte(cmd & ~(REPLY_FLAG));
	// put the space for the overall len - 4 byte
	_txBufAppendByte(0);
	_txBufAppendByte(0);
	_txBufAppendByte(0);
	_txBufAppendByte(0);
	// add the number of parameters
	_txBufAppendByte(numParam);

	// if no parameters
	if (numParam == 0) {
		_txBufFinalizePacket(true);
	}
}


/* -----------------------------------------------------------------
* Add parameters to the command buffer for the esp
* It requires also the lastParam parameter to understand which is the last one.
*/
bool SpiDrv::sendParam(uint16_t param, uint8_t lastParam, bool dataPkt)
{
	// NOTE for each append be sure to not insert more than 32 byte in buffer or append function
	// won't let me do that.
	
	// since each parameter is two bytes wide, add 2 as the parameter size
	_txBufAppendByte(2);
	// then add the parameter (big-endian mode) 
	_txBufAppendByte((uint8_t)((param & 0xff00) >> 8));
	_txBufAppendByte((uint8_t)(param & 0xff));

	// if this is the last parameter
	if (lastParam == 1) {
		return _txBufFinalizePacket(dataPkt);
	}

	return true;
}

/* -----------------------------------------------------------------
* Add a list of parameters to the command buffer for the esp (overload)
* It requires the parameter's list (*param), the length of each parameter (param_len)
* and also the lastParam parameter to understand if this is the last one.
*/
bool SpiDrv::sendParam(uint8_t *param, uint8_t param_len, uint8_t lastParam)
{
	// NOTE for each append be sure to not insert more than 32 byte in buffer or append function
	// won't let me do that.
	
	// append the typ. parameter size (1 byte)
	_txBufAppendByte(param_len);

	// append all the parameters from the list to the buffer
	for (uint16_t i = 0; i < param_len; ++i) {
		_txBufAppendByte(param[i]);
	}

	// if this is the last parameter
	if (lastParam == 1) {
		return _txBufFinalizePacket();
	}

	return true;
}

/* -----------------------------------------------------------------
* Writes to the esp a pre-allocated buffer (mainly used for _wifiBuf)
*
* params: uint8_t* data:	the buffer pointer
*		  uint16_t len:		number of parameters to read
*		  bool ctrlReq:		used to control the ss signal based on the calling function structure
*/
bool SpiDrv::writeData(uint8_t *data, uint32_t len)
{
	// calculate the number of packets required to send all the data (rounded to the floor)
	uint8_t pktNum = (uint8_t)(len >> 5);
	// evaluate the extra-bytes from the floor to the real dimension that will become part of a zero-filled packet
	uint16_t extraByteNum = len - (pktNum << 5);
	uint32_t byteWritten = 0;
	bool ret = true;
	
	if(pktNum > 0)
		multiWrite = true;
	else
		multiWrite = false;

	if(extraByteNum == 0 && pktNum == 1)
		multiWrite = false;

	_enableDevice();

	// wait for the sr signal high - esp is ready
	if(_checkEspStatusTimeout(esp_idle)){
		if(_ss_status == HIGH){ // if in the meanwhile a read occurred, SS has become HIGH
			_enableDevice();
		}

		// send all the packets
		while (pktNum--) {
			// send data write command
			SPI.transfer((uint8_t)(ESP8266_DATA_WRITE));
			SPI.transfer((uint8_t)(DUMMY_DATA));
			
			for (uint8_t j = 0; j < SPI_BUF_LEN; j++) {
				SPI.transfer(data[j + byteWritten]);
			}
			byteWritten += SPI_BUF_LEN;
			
			// check ack
			uint32_t timeout = millis() + 5000;
			// wait for the SR to go HIGH
			while(srLevelInMultipacket != HIGH && timeout > millis());
			// if the SR is HIGH before the end of the timeout..
			if(srLevelInMultipacket == HIGH){
				// wait for the ack pulse to go back low
				delayMicroseconds(25);
				// if it doesn't go low -> we could have an async event.
				if(srLevelInMultipacket == HIGH){
					_interruptReq = true;
					// read the event and check if SR goes low (inside the read function)
					handleSPIEvents();
					// after the read the SR is still HIGH, we have and error
					if(srLevelInMultipacket == HIGH){
						multiWrite = false;
						_disableDevice();
						return false;
					}
				}
			}
			// timeout is over...and I have an error
			else{
				multiWrite = false;
				_disableDevice();
				return false;
			}
		}
		// write the last chunk
		if (extraByteNum > 0) {
			SPI.transfer((uint8_t)(ESP8266_DATA_WRITE));
			SPI.transfer((uint8_t)(DUMMY_DATA));
			
			for (uint8_t j = 0; j < SPI_BUF_LEN; j++) {
				if (j < extraByteNum) {
					SPI.transfer(data[j + byteWritten]);
				}
				else
				SPI.transfer(0);
			}
		}
	}
	else{
		_spi_status = SPItimeout;
		ret = false;
	}
	
	// if we were in a multipacket case, reset it and exit
	if(multiWrite)
		multiWrite = false;

	_disableDevice();
		
	return ret;
}

bool SpiDrv::writeServerData(uint8_t *data, uint32_t len)
{
	uint8_t pktNum = (uint8_t)(len >> 5);
	// evaluate the extra-bytes from the floor to the real dimension that will become part of a zero-filled packet
	uint8_t extraByteNum = (uint8_t)(len - (pktNum << 5));
	uint32_t byteWritten = 0;
	// start (1 byte), cmd (1 byte), size (4 bytes), sock (1 byte)
	uint8_t dataOffset = 7;
	uint8_t nextPktSz = 0;
	
	bool ret = true;
	uint16_t j = 0;
	
	// retrieve the address at which we have stored the data to be sent
	volatile uint16_t addrMem = (uint16_t)(data[dataOffset] + (data[dataOffset + 1] << 8));
	
	if(pktNum > 0)
		multiWrite = true;
	else
		multiWrite = false;
	
	if(extraByteNum == 0 && pktNum == 1)
		multiWrite = false;
	
	_enableDevice();

	// wait for the ESP idle
	if(_checkEspStatusTimeout(esp_idle)){
		if(_ss_status == HIGH){ // if in the meanwhile a read occurred, SS has become HIGH
			_enableDevice();
		}

		// send data write command
		SPI.transfer((uint8_t)(ESP8266_DATA_WRITE));
		SPI.transfer((uint8_t)(DUMMY_DATA));

		// send header
		for (j = 0; j < dataOffset; j++) {
			SPI.transfer(data[j]);
		}
		
		byteWritten = 7;
		j = 0;
		nextPktSz = SPI_BUF_LEN - byteWritten;
		
		// write out the data
		while(byteWritten < len){
			for(uint8_t k=0; k<nextPktSz; k++, j++){
				if((len-1) - (byteWritten + k) > 0)
					SPI.transfer(*(uint8_t*)(addrMem + j));
				else if(len - (byteWritten + k) > 0){
					SPI.transfer(END_CMD);
				}
				else
					SPI.transfer(0);
			}
			byteWritten += nextPktSz;
			
			
			if(byteWritten >= len)
				break;
			
			if((byteWritten % SPI_BUF_LEN) == 0){
				nextPktSz = SPI_BUF_LEN;
				// check ack
				uint32_t timeout = millis() + 1000;
				// wait for the SR to go HIGH
				while(srLevelInMultipacket != HIGH && timeout > millis());
				// if the SR is HIGH before the end of the timeout..
				if(srLevelInMultipacket == HIGH){
					// wait for the ack pulse to go back low
					delayMicroseconds(25);
					// if it doesn't go low -> we could have an async event.
					if(srLevelInMultipacket == HIGH){
						_interruptReq = true;
						// read the event and check if SR goes low (inside the read function)
						handleSPIEvents();
						
						if(_ss_status == HIGH)
							digitalWrite(_ss_pin, LOW);
						
						// after the read the SR is still HIGH, we have and error
						if(srLevelInMultipacket == HIGH){
							multiWrite = false;
							_disableDevice();
							
							return false;
						}
					}
				}
				// timeout is over...and I have an error
				else{
					multiWrite = false;
					_disableDevice();

					return false;
				}
				
				// send data write command of a new packet
				SPI.transfer((uint8_t)(ESP8266_DATA_WRITE));
				SPI.transfer((uint8_t)(DUMMY_DATA));
			}
		}
	}

	// if we were in a multipacket case, reset it and exit
	if(multiWrite)
		multiWrite = false;

	delay(1);

	_disableDevice();

	return ret;
}

/* -----------------------------------------------------------------
* Read data from the esp after an ISR event (32 bytes per time)
* params: uint8_t* buffer:	data buffer to store the received data
* 
* return: (uint16_t)
*		   byte read.
*/
uint16_t SpiDrv::readDataISR(uint8_t *buffer)
{
	uint16_t byteRead = SPI_BUF_LEN;
	uint16_t ret = 0;

	_enableDevice();

	// wait for the SRsr signal HIGH - esp is ready
	if(espStatus == esp_busy || srLevelInMultipacket == HIGH){
		// send data read command
		SPI.transfer((uint8_t)(ESP8266_DATA_READ));
		SPI.transfer((uint8_t)(DUMMY_DATA));

		// read all the 32 bytes available in the esp data out buffer
		for (uint8_t i = 0; i < SPI_BUF_LEN; i++) {
			buffer[i] = SPI.transfer(0);
		}
	}
	else{
		// if the SR is still low, return with an error
		if(multiRead){
			multiRead = false;
			_disableDevice();
		}
		return 0;
	}
	
	// after reading all the 32 byte long message we need to ensure that the SR goes low again
	uint32_t t_hold = millis() + 1000;
	while(t_hold > millis()){
		// if we are in a multipacket case...
		if(multiRead){
			if(srLevelInMultipacket == LOW)
				return byteRead;
		}
		// if we are in a single packet case...
		else{
			if(espStatus == esp_idle){
				// disable the CS and exit.
				_disableDevice();
				return byteRead;
			}
		}
	}
	if(multiRead)
		multiRead = false;

	_disableDevice();
	return byteRead;
}

/* -----------------------------------------------------------------
* The function watches the _interruptReq variable set in the _SRcallback
* when ESP fires an ISR. If the callback exists it will be called
*/
void SpiDrv::handleSPIEvents(void)
{
	if(spiIsr && _interruptReq){
		_interruptReq = false;
		spiIsr();
	}
}

/* -----------------------------------------------------------------
* Registers the callback
*/
void SpiDrv::registerCB(tpDriverIsr pfIsr)
{
	spiIsr = pfIsr;
}

/* -----------------------------------------------------------------
* HW interrupt that fires the sr signal callback
*/
ISR(PCINT3_vect)
{
	_SRcallback();
}

SpiDrv commDrv;