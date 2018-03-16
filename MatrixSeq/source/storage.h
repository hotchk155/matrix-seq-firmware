/*
 * patch.h
 *
 *  Created on: 14 Mar 2018
 *      Author: jason
 */

#ifndef STORAGE_H_
#define STORAGE_H_

byte qq[] = {
		1,2,3,4,5,6,7,8,9,10,
		11,12,13,14,15,16,17,18,19,20,
		21,22,23,24,25,26,27,28,29,30,
		31,32,33,34,35,36,37,38,39,40,
		41,42,43,44,45,46,47,48,49,50,
		51,52,53,54,55,56,57,58,59,60,
		61,62,63,64,65,66,67,68,69,70
};

class CStorage {
public:
	void test() {
		byte r[200];
		status_t p = 0;
		g_i2c_bus.wait();
		//p = g_i2c_bus.write_eeprom(10, qq, sizeof qq);
		p = g_i2c_bus.read_eeprom(10, r, sizeof r);
		g_i2c_bus.wait();
		p=p;
	}
};
extern CStorage g_storage;
#ifdef MAIN_INCLUDE
CStorage g_storage;
#endif

#endif /* STORAGE_H_ */
