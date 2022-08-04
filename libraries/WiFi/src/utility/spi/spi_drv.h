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

#include "WString.h"

#ifndef SPI_Drv_h
#define SPI_Drv_h

#ifndef NULL
#ifdef __cplusplus
#define NULL 0
#else
#define NULL ((void *)0)
#endif
#endif

#include <inttypes.h>
#include "utility/definitions.h"

#define NO_LAST_PARAM       0
#define LAST_PARAM          1
#define DUMMY_DATA          0x00
#define SPI_BUF_LEN			32
#define SPI_TXBUF_LEN		SPI_BUF_LEN << 1

#define ESP_SR_TIMEOUT      1000

#	define SLAVESELECT      22
#	define SLAVEREADY       20

typedef enum {
	SPIerror = -2,
	SPItimeout = -1,
	SPIok = 0,
} teSPI_Stat;

typedef enum {
	ss_low = 0,
	ss_high = 1,
	ss_rising_edge = 2,
	ss_falling_edge = 3,
} teSSStatus;

typedef enum {
	esp_wait_ready = -1,
	esp_idle = 0,
	esp_busy = 1,
	esp_ack = 2,
} teEspStatus;

typedef void (*tpDriverIsr)(void);

class SpiDrv
{
	private:
	uint8_t _ss_pin;
	uint8_t _sr_pin;
	uint32_t t;
	uint8_t repetedError;

	teSPI_Stat _spi_status;
	teSSStatus _ss_status = ss_high;
	static volatile bool _interruptReq;
	bool _espFirstLink = false;
	uint8_t _txIndex;

	// function used to establish SPI communication after ESP reset
	bool _askStatusInit(uint8_t attempts);
	bool _checkSRpinStatusTimeout(bool checkStat);
	bool _checkEspStatusTimeout(teEspStatus checkStat);
	void _enableDevice(void);
	void _disableDevice(void);
	
	void _txBufInitWByte(uint8_t b);
	bool _txBufAppendByte(uint8_t b);
	bool _txBufFinalizePacket(bool dataPkt = false);
	void _txBufSetOverallLen(bool dataPkt = false);

	public:
	uint8_t _rxBuf[SPI_TXBUF_LEN];
	uint8_t wifiBuf[SPI_TXBUF_LEN];
	uint16_t rxIndex;
	uint16_t payloadSize;
	static volatile teEspStatus espStatus;
	static volatile bool multiRead;
	static volatile bool multiWrite;
	
	// ESP generic functions
	SpiDrv();
	void begin();
	void end();
	bool establishESPConnection();
	void reset();
	void off();
	void on();

	// ESP SPI Status Register functions
	int readStatus(bool ctrlReq);
	void writeStatus(uint32_t status, bool ctrlReq, bool checkLevel);
	
	// ESP callback related functions
	void handleSPIEvents(void);
	void registerCB(tpDriverIsr pfIsr);
	
	// ESP command related functions
	//void sendBuffer(uint8_t *param, uint16_t param_len, uint8_t lastParam = NO_LAST_PARAM);
	bool sendCmd(uint8_t cmd, uint8_t numParam);
	void sendDataPkt(uint8_t cmd, uint8_t numParam);
	bool sendParam(uint8_t *param, uint8_t param_len, uint8_t lastParam = NO_LAST_PARAM);
	bool sendParam(uint16_t param, uint8_t lastParam = NO_LAST_PARAM, bool dataPkt = false);
	uint32_t sendData(uint8_t cmd, uint8_t* data, uint32_t len);
	
	// ESP SPI Data Register functions
	uint16_t readDataISR(uint8_t *buffer);
	bool writeData(uint8_t *data, uint32_t len);
	bool writeServerData(uint8_t *data, uint32_t len);
	
	friend void wifiDrvCB(void);
	friend void _SRcallback(void);
	friend class WiFiClass;
};

extern SpiDrv commDrv;

static SpiDrv *pSpiDrv;

#endif