#pragma once
#ifndef __PiSPI_H_
#define __PiSPI_H_

// Includes
#include <mutex>
#include <cstdint>
#include <linux/spi/spidev.h>

//Namespace STD to avoid std::
using namespace std;



//PiSPI class
//Class for low level SPI communicationn
class PiSPI
{
	//Constructor / Destructor
	//Constructor - Sets up the SPI Port and opens it. Singleton due to HW.
private:
	PiSPI(uint8_t channel, int speed, int mode, uint8_t bitsperword = 8);
	static PiSPI* _pInstance[2];
	static uint32_t _u32InstanceCounter[2];
public:
	static PiSPI* GetInstance(uint8_t channel, int speed, int mode, uint8_t bitsperword = 8);
	void ReleaseInstance();
	~PiSPI();

	//Methods
public:
	bool SetMode(int mode);
	int GetMode();

	bool SetBitsPerWord(uint8_t bpw);
	uint8_t GetBitsPerWord();

	bool SetSpeed(int speed);
	int GetSpeed();

	bool Write(uint8_t reg, uint8_t* pData, size_t length);
	bool Read(uint8_t reg, uint8_t* pData, size_t length);
	bool SyncReadWrite(uint8_t* pData, size_t length);

	//Fields
public:
	static std::mutex mutexSPI[2];

private:
	uint8_t  _u8Channel;
	int _iSpeed;
	uint8_t _iBitsPerWord;
	int _iMode;
	int _iFD;

};

#endif //__PiSPI_H_

