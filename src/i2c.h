#ifndef _SENSEI_I2C_H
#define _SENSEI_I2C_H

void i2c_init();

// read n-bytes from i2c register at the given address where subsequent bytes
// are read from incrementally increasing register addresses.
bool i2c_read_data(uint8_t address, uint8_t reg, uint8_t *data, uint8_t len);

// read the i2c register at the given address
// first we write the register address to tell the device to prepare that value
// then we read 1 byte in response from the same i2c device.
// Returns true on success, false on failure
uint8_t i2c_read_reg(uint8_t address, uint8_t reg);

// write 1 byte to the i2c register at the given address
bool i2c_write_reg(uint8_t address, uint8_t reg, uint8_t value);

void i2c_shutdown();

#endif // _SENSEI_I2C_H
