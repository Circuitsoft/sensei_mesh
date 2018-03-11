#ifdef NRF51

#include "i2c.h"
#include "twi_master.h"
#include <stdbool.h>
#include <stdint.h>

void i2c_init() { twi_master_init(); }

// read n-bytes from i2c register at the given address where subsequent bytes
// are read from incrementally increasing register addresses.
bool i2c_read_data(uint8_t address, uint8_t reg, uint8_t *data, uint8_t len) {

  address = address << 1;

  // initialize data to zero so we don't return random values.
  for (int i = 0; i < len; i++) {
    data[i] = 0;
  }

  // Write: register address we want to start reading from
  if (twi_master_transfer(address, &reg, 1, TWI_DONT_ISSUE_STOP)) {
    // Read: the number of bytes requested.
    if (twi_master_transfer(address | TWI_READ_BIT, data, len,
                            TWI_ISSUE_STOP)) {
      // Read succeeded.
      return true;
    }
  }

  // read or write failed.
  return false;
}

// read the i2c register at the given address (see table 13 in the LSM9DS0 spec)
// first we write the register address to tell the device to prepare that value
// then we read 1 byte in response from the same i2c device.
uint8_t i2c_read_reg(uint8_t address, uint8_t reg) {
  uint8_t data = 0;

  if (i2c_read_data(address, reg, &data, 1)) {
    return data;
  }
  return 0;
}

// write 1 byte to the i2c register at the given address (see table 11 in the
// LSM9DS0 spec)
bool i2c_write_reg(uint8_t address, uint8_t reg, uint8_t value) {
  uint8_t data[2];
  data[0] = reg;
  data[1] = value;

  address = address << 1;

  // Write: register protocol
  if (twi_master_transfer(address, data, 2, TWI_ISSUE_STOP)) {
    return true;
  }

  // read or write failed.
  return false;
}

/*lint --flb "Leave library region" */

#endif // NRF51