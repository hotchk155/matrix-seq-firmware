#ifndef DIGITAL_OUT_H_
#define DIGITAL_OUT_H_

#include "fsl_gpio.h"

template<gpio_port_num_t _P, uint32_t _B> class CDigitalOut {
public:
	CDigitalOut() {
		gpio_pin_config_t config =
		{
				kGPIO_DigitalOutput,
				0,
		};
		GPIO_PinInit(_P, _B, &config);
	}
	void set(int state) {
		GPIO_PinWrite(_P, _B, state);
	}
};

#endif /* DIGITAL_OUT_H_ */
