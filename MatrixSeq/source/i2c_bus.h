///////////////////////////////////////////////////////////////////////////////////
//
//                                  ~~  ~~             ~~
//  ~~~~~~    ~~~~~    ~~~~~    ~~~~~~  ~~     ~~~~~   ~~~~~~    ~~~~~   ~~    ~~
//  ~~   ~~  ~~   ~~  ~~   ~~  ~~   ~~  ~~    ~~   ~~  ~~   ~~  ~~   ~~   ~~  ~~
//  ~~   ~~  ~~   ~~  ~~   ~~  ~~   ~~  ~~    ~~~~~~~  ~~   ~~  ~~   ~~     ~~
//  ~~   ~~  ~~   ~~  ~~   ~~  ~~   ~~  ~~    ~~       ~~   ~~  ~~   ~~   ~~  ~~
//  ~~   ~~   ~~~~~    ~~~~~    ~~~~~~   ~~~   ~~~~~   ~~~~~~    ~~~~~   ~~    ~~
//
//  Serendipity Sequencer                                   CC-NC-BY-SA
//  hotchk155/2018                                          Sixty-four pixels ltd
//
//  I2C BUS HANDLING
//
///////////////////////////////////////////////////////////////////////////////////

#ifndef I2C_BUS_H_
#define I2C_BUS_H_


void i2c_master_callback(I2C_Type *base, i2c_master_handle_t *handle, status_t status, void *userData);

class CI2CBus{
	enum {
		BUFFER_SIZE = 100,
		DAC_ADDRESS = 0b1100000,
		EEPROM_ADDRESS = 0b1010000
	};
	i2c_master_handle_t m_handle;
	i2c_master_transfer_t m_xfer;
	byte m_buf[BUFFER_SIZE];
	size_t m_len;
public:

	static volatile byte s_busy;
	static volatile byte m_transaction;

	void init() {
		i2c_master_config_t masterConfig;
		I2C_MasterGetDefaultConfig(&masterConfig);
		I2C_MasterInit(I2C0, &masterConfig, CLOCK_GetFreq(kCLOCK_BusClk));
		I2C_Enable(I2C0, true);
	}

	inline byte busy() {
		return s_busy;
	}
	inline byte transaction() {
		return m_transaction;
	}
	void wait() {
		while(s_busy);
	}

	byte write(byte addr, int len) {
		m_len = len;
		m_xfer.slaveAddress = addr;
		m_xfer.direction = kI2C_Write;
		m_xfer.subaddress = 0;
		m_xfer.subaddressSize = 0;
		m_xfer.data = m_buf;
		m_xfer.dataSize = m_len;
		m_xfer.flags = kI2C_TransferDefaultFlag;
		byte txn = m_transaction;
		I2C_MasterTransferCreateHandle(I2C0, &m_handle, i2c_master_callback, NULL);
		s_busy = 1;
		I2C_MasterTransferNonBlocking(I2C0, &m_handle, &m_xfer);
		return txn;
	}

	//M24256 EEPROM
	status_t write_eeprom(uint16_t eeprom_addr, byte *data, int size) {
		status_t result = kStatus_Fail;
		while(size > 0) {

			// get the target EEPROM page base address (64 bytes per page)
			uint16_t page_mask = (eeprom_addr & 0xFFC0);

			// set up transfer block
			i2c_master_transfer_t xfer;
			xfer.slaveAddress = EEPROM_ADDRESS;
			xfer.direction = kI2C_Write;
			xfer.subaddress = eeprom_addr;
			xfer.subaddressSize = 2;
			xfer.data = data;
			xfer.dataSize = 0;
			xfer.flags = kI2C_TransferDefaultFlag;

			// we can only send continuous bytes up until the EEPROM page boundary
			// then we'll need to start a new transaction
			while(size > 0 && ((eeprom_addr & 0xFFC0) == page_mask)) {
				++eeprom_addr;
				++data;
				--size;
				xfer.dataSize++;
			}

			// the EEPROM can NAK us while internal write cycle takes place
			for(int retries = 20;retries>0;--retries) {
				result = I2C_MasterTransferBlocking(I2C0, &xfer);
				if(result != kStatus_I2C_Addr_Nak) {
					break;
				}
				g_clock.wait_ms(1);
			}
		    if(result != kStatus_Success) {
		    	break;
		    }
		}
		return result;
	}

	status_t read_eeprom(uint16_t eeprom_addr, byte *data, int size) {
		i2c_master_transfer_t xfer;
		xfer.slaveAddress = EEPROM_ADDRESS;
		xfer.direction = kI2C_Read;
		xfer.subaddress = eeprom_addr;
		xfer.subaddressSize = 2;
		xfer.data = data;
		xfer.dataSize = size;
		xfer.flags = kI2C_TransferDefaultFlag;
	    return I2C_MasterTransferBlocking(I2C0, &xfer);
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

CI2CBus g_i2c_bus;
void i2c_master_callback(I2C_Type *base, i2c_master_handle_t *handle, status_t status, void *userData)
{
	CI2CBus::s_busy = 0;
	CI2CBus::m_transaction++;
}
volatile byte CI2CBus::s_busy = 0;
volatile byte CI2CBus::m_transaction = 0;

#endif /* I2C_BUS_H_ */
