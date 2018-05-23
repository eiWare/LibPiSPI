#include "PiSPI.h"
#include <cstdio>
#include <string.h>
#include <string>
#include <ios>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <unistd.h>

PiSPI* PiSPI::_pInstance[2] = { NULL };
uint32_t PiSPI::_u32InstanceCounter[2] = { 0 };
mutex PiSPI::mutexSPI[2];

PiSPI::PiSPI(uint8_t channel, int speed, int mode, uint8_t bitsperword)
{
	char device[16] = "";
	this->_u8Channel = channel;
	this->_iFD = 0;

	snprintf(device, sizeof(device), "/dev/spidev0.%d", channel);
	this->_iFD = open(device, O_RDWR);
	
	if (this->_iFD < 0)
		throw ios_base::failure(string("Could not open device!"));
		
	if(!this->SetMode(mode))
		throw ios_base::failure(string("Could set SPI mode!"));

	if(!this->SetBitsPerWord(bitsperword))
		throw ios_base::failure(string("Could set SPI bits per word!"));

	if (!this->SetSpeed(speed))
		throw ios_base::failure(string("Could set SPI speed!"));
}

//Singleton Constructor
PiSPI * PiSPI::GetInstance(uint8_t channel, int speed, int mode, uint8_t bitsperword)
{
	if (channel > 1)
		return NULL;

	if (PiSPI::_pInstance[channel] == NULL)
		PiSPI::_pInstance[channel] = new PiSPI(channel, speed, mode, bitsperword);

	if(PiSPI::_pInstance[channel] != NULL)
		PiSPI::_u32InstanceCounter[channel]++;
	return PiSPI::_pInstance[channel];
}

//Releases one instance from the singleton class and deconstructs the class once it reaches zero instances
void PiSPI::ReleaseInstance()
{
	PiSPI::_pInstance[this->_u8Channel] = NULL;
	if (PiSPI::_u32InstanceCounter > 0)
		PiSPI::_u32InstanceCounter[this->_u8Channel]--;
	if(PiSPI::_u32InstanceCounter[this->_u8Channel])
		delete this;
}

PiSPI::~PiSPI()
{
	close(this->_iFD);
}

bool PiSPI::SetBitsPerWord(uint8_t bpw)
{
	int retVal = 0;
	if ((retVal = ioctl(this->_iFD, SPI_IOC_WR_BITS_PER_WORD, &bpw)) != -1)
	{
		this->_iBitsPerWord = bpw;
		return true;
	}
	return false;
}

uint8_t PiSPI::GetBitsPerWord()
{
	int retVal = 0;
	uint8_t bpw = 0;
	if ((retVal = ioctl(this->_iFD, SPI_IOC_RD_BITS_PER_WORD, &bpw)) == -1)
		return -1;

	return this->_iBitsPerWord = bpw;
}

bool PiSPI::SetSpeed(int speed)
{
	int retVal = 0;
	if ((retVal = ioctl(this->_iFD, SPI_IOC_WR_MAX_SPEED_HZ, &speed)) != -1)
	{
		this->_iSpeed = speed;
		return true;
	}
	return false;
}

int PiSPI::GetSpeed()
{
	int retVal = 0;
	int speed = 0;
	if ((retVal = ioctl(this->_iFD, SPI_IOC_WR_MAX_SPEED_HZ, &speed)) == -1)
		return -1;

	return this->_iSpeed = speed;
}

bool PiSPI::Write(uint8_t reg, uint8_t * pData, size_t length)
{
	struct spi_ioc_transfer spi = { 0 };
	uint8_t buffer[length + 1];
	int retVal = 0;

	if (pData == NULL)
		return false;

	memset(buffer, 0, length + 1);

	buffer[0] = reg;

	memcpy(&buffer[1], pData, length);

	spi.tx_buf = (unsigned long)buffer;
	spi.rx_buf = (unsigned long)NULL;
	spi.len = sizeof(reg) + length;
	spi.cs_change = 0;
	
	PiSPI::mutexSPI[this->_u8Channel].lock();
	retVal = ioctl(this->_iFD, SPI_IOC_MESSAGE(1), &spi);
	PiSPI::mutexSPI[this->_u8Channel].unlock();

	return retVal != -1;
}

bool PiSPI::Read(uint8_t reg, uint8_t * pData, size_t length)
{
	struct spi_ioc_transfer spi[2];
	int retVal = 0;

	if (pData == NULL)
		return false;

	memset(&spi[0], 0, sizeof(spi[0]));
	memset(&spi[1], 0, sizeof(spi[1]));

	spi[0].tx_buf = (unsigned long)&reg;
	spi[0].rx_buf = (unsigned long)NULL;
	spi[0].len = sizeof(reg);
	spi[0].cs_change = 0;

	spi[1].tx_buf = (unsigned long)NULL;
	spi[1].rx_buf = (unsigned long)pData;
	spi[1].len = length;
	spi[1].cs_change = 0;

	PiSPI::mutexSPI[this->_u8Channel].lock();
	retVal = ioctl(this->_iFD, SPI_IOC_MESSAGE(2), &spi[0]);
	PiSPI::mutexSPI[this->_u8Channel].unlock();

	return retVal != -1;
}

bool PiSPI::SyncReadWrite(uint8_t* pData, size_t length)
{
	struct spi_ioc_transfer spi = { 0 };
	int retVal = 0;

	if (pData == NULL)
		return false;
	
	spi.tx_buf = (unsigned long)pData;
	spi.rx_buf = (unsigned long)pData;
	spi.len = length;
	spi.cs_change = 0;

	PiSPI::mutexSPI[this->_u8Channel].lock();
	retVal = ioctl(this->_iFD, SPI_IOC_MESSAGE(1), &spi);
	PiSPI::mutexSPI[this->_u8Channel].unlock();
	return retVal != -1;
}

bool PiSPI::SetMode(int mode)
{
	int retVal = 0;
	if ((retVal = ioctl(this->_iFD, SPI_IOC_WR_MODE, &mode)) != -1)
	{
		this->_iMode = mode;
		return true;
	}
	return false;
}

int PiSPI::GetMode()
{
	int retVal = 0;
	int mode = 0;
	if ((retVal = ioctl(this->_iFD, SPI_IOC_RD_MODE, &mode)) == -1)
		return -1;

	return this->_iMode = mode;
}
