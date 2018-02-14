/*
 * cv.h
 *
 *  Created on: 14 Feb 2018
 *      Author: jason
 */

#ifndef I2C_BUS_H_
#define I2C_BUS_H_

#include "fsl_i2c.h"


void i2c_master_callback(I2C_Type *base, i2c_master_handle_t *handle, status_t status, void *userData);

class CI2CBus{
	enum {
		BUFFER_SIZE = 100,
		DAC_ADDRESS = 0b1100000
	};
	i2c_master_handle_t m_handle;
	i2c_master_transfer_t m_xfer;
	byte m_buf[BUFFER_SIZE];
	size_t m_len;
public:

	static volatile byte s_busy;
	void init() {
		i2c_master_config_t masterConfig;
		I2C_MasterGetDefaultConfig(&masterConfig);
		I2C_MasterInit(I2C0, &masterConfig, CLOCK_GetFreq(kCLOCK_BusClk));
		I2C_Enable(I2C0, true);
	}

	inline byte busy() {
		return s_busy;
	}
	void wait() {
		while(s_busy);
	}

	void write(byte addr, int len) {
		m_len = len;
		m_xfer.slaveAddress = addr;
		m_xfer.direction = kI2C_Write;
		m_xfer.subaddress = 0;
		m_xfer.subaddressSize = 0;
		m_xfer.data = m_buf;
		m_xfer.dataSize = m_len;
		m_xfer.flags = kI2C_TransferDefaultFlag;
		I2C_MasterTransferCreateHandle(I2C0, &m_handle, i2c_master_callback, NULL);
		s_busy = 1;
		status_t ret = I2C_MasterTransferNonBlocking(I2C0, &m_handle, &m_xfer);
	}
	void write_blocking(byte addr, int len) {
		write(addr, len);
		wait();
	}
	void dac_init() {
		m_buf[0] = 0b10001111; // set each channel to use internal vref
		write_blocking(DAC_ADDRESS, 1);
		m_buf[0] = 0b11001111; // set x2 gain on each channel
		write_blocking(DAC_ADDRESS, 1);
	}
	void dac_write(uint16_t dac[4]) {
		m_buf[0] = ((dac[0]>>8) & 0xF);
		m_buf[1] = (byte)dac[0];
		m_buf[2] = ((dac[2]>>8) & 0xF);
		m_buf[3] = (byte)dac[2];
		m_buf[4] = ((dac[1]>>8) & 0xF);
		m_buf[5] = (byte)dac[1];
		m_buf[6] = ((dac[3]>>8) & 0xF);
		m_buf[7] = (byte)dac[3];
		write(DAC_ADDRESS, 8);
	}
};
extern CI2CBus g_i2c_bus;

void i2c_master_callback(I2C_Type *base, i2c_master_handle_t *handle, status_t status, void *userData)
{
    /* Signal transfer success when received success status. */
    if (status == kStatus_Success)
    {
    	CI2CBus::s_busy = 0;
    }
    else {
    	CI2CBus::s_busy = 2;
    }
}
volatile byte CI2CBus::s_busy = 0;

#endif /* I2C_BUS_H_ */
