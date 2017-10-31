/* Copyright (c) 2013 Microsoft Corporation. All Rights Reserved.
 *
 */

#include <stdbool.h>
#include <stdint.h>
#include "app_util_platform.h"
#include "nrf_drv_twi.h"
#include "i2c.h"
#include "boards.h"

const nrf_drv_twi_t twi = NRF_DRV_TWI_INSTANCE(0);

void i2c_init() {
  ret_code_t err_code;

  nrf_drv_twi_config_t config = NRF_DRV_TWI_DEFAULT_CONFIG(0);
  config.sda = SDA_PIN_NUMBER;
  config.scl = SCL_PIN_NUMBER;
  config.frequency = TWI_FREQUENCY_FREQUENCY_K400;

  err_code = nrf_drv_twi_init(&twi, &config, NULL, NULL);
  APP_ERROR_CHECK(err_code);

  nrf_drv_twi_enable(&twi);
}

// read n-bytes from i2c register at the given address where subsequent bytes
// are read from incrementally increasing register addresses.
bool i2c_read_data(uint8_t address, uint8_t reg, uint8_t* data, uint8_t len)
{

  // initialize data to zero so we don't return random values.
  for (int i = 0; i < len; i++)
  {
    data[i] = 0;
  }

  if (nrf_drv_twi_tx(&twi, address, &reg, 1, true) == NRF_SUCCESS)
  {
    // Read: the number of bytes requested.
    if (nrf_drv_twi_rx(&twi, address, data, len) == NRF_SUCCESS)
    {
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
uint8_t i2c_read_reg(uint8_t address, uint8_t reg)
{
  uint8_t data = 0;

  if (i2c_read_data(address, reg, &data, 1))
  {
    return data;
  }
  return 0;
}


// write 1 byte to the i2c register at the given address (see table 11 in the LSM9DS0 spec)
bool i2c_write_reg(uint8_t address, uint8_t reg, uint8_t value)
{
  uint8_t data[2];
  data[0] = reg;
  data[1] = value;

  // Write: register protocol
  if (nrf_drv_twi_tx(&twi, address, data, 2, false) == NRF_SUCCESS)
  {
      return true;
  }

  // read or write failed.
  return false;
}


/*lint --flb "Leave library region" */
