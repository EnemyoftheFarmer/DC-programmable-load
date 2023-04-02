void ad5761r_set_configuration(){
  uint8_t configuration_data[3];
  configuration_data[0] = CMD_WR_CTRL_REG;
  configuration_data[1] = 0b00000000;
  configuration_data[2] = 0b01101011; 
  spi_set_format(spi0, 8, SPI_CPOL_1, SPI_CPHA_0, SPI_MSB_FIRST);
  gpio_put(CS_ADC,0);
  spi_write_blocking(spi0, configuration_data, 3);
  gpio_put(CS_ADC,1);
  
  //sets the configuration of the ad5761r 
}
void ad5761r_output_write(uint16_t VOLTAGE_CODE){
  /*Function to update the output of the ad5761r, parameter is a 16bit integer that is mapped to the output 
  according to the bits set in the configuration register of the ad5761r*/
  //The desired output voltage is a 16 bit number that is sent after sending the update dac register address 
  uint8_t DATA[3];
  DATA[0] = CMD_WR_UPDATE_DAC_REG;
  DATA[1] = (VOLTAGE_CODE >> 8) & 0xFF;
  DATA[2] = (VOLTAGE_CODE >> 0) & 0xFF;
  spi_set_format(spi0, 8, SPI_CPOL_1, SPI_CPHA_0, SPI_MSB_FIRST); 
  gpio_put(CS_ADC, 0);
  spi_write_blocking(spi0, DATA, 3);
  gpio_put(CS_ADC, 1);
}