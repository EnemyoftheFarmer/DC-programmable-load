#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include <U8g2lib.h>
#include <MUIU8g2.h>
#include "ads1x1x.h"


/* Input Shift Register Commands */
#define CMD_NOP                 0x0
#define CMD_WR_TO_INPUT_REG     0x1
#define CMD_UPDATE_DAC_REG      0x2    
#define CMD_WR_UPDATE_DAC_REG   0x3    //updates the dac output voltage
#define CMD_WR_CTRL_REG         0x4    //configures dac settings
#define CMD_NOP_ALT_1           0x5
#define CMD_NOP_ALT_2           0x6
#define CMD_SW_DATA_RESET       0x7
#define CMD_RESERVED            0x8
#define CMD_DIS_DAISY_CHAIN     0x9
#define CMD_RD_INPUT_REG        0xA
#define CMD_RD_DAC_REG          0xB
#define CMD_RD_CTRL_REG         0xC
#define CMD_NOP_ALT_3           0xD
#define CMD_NOP_ALT_4           0xE
#define CMD_SW_FULL_RESET       0xF

//menu options
#define MAIN_MENU 1
#define CONSTANT_CURRENT 2
#define CONSTANT_POWER 3
#define CONSTANT_RESISTANCE 4

//SPI pins
#define RESET_PIN 22
#define DC_PIN 21
#define CS_DISPLAY 20
#define CS_ADC 17
#define MISO 16
#define SCLK 18
#define MOSI 19

//input pins
#define SELECT_PIN 0  //used for selecting fields on the display
#define ROTARY_CH_A 14
#define ROTARY_CH_B 15

//encoder count
volatile int COUNT = 0;
int LASTCOUNT = 0;

uint8_t current_user_input[4];
const float max_allowed_dac_voltage = 4;
float measured_current = 0;

//a structure which will contain all the data for one display
u8g2_t u8g2; 
//a structure which will contain all the data for the UI
mui_t ui;

ADS1x1x_config_t adc;

fds_t fds_data[] = 
//Main Menu
MUI_FORM(1)
MUI_STYLE(1)
MUI_LABEL(1, 10, "MAIN MENU")
MUI_XYAT("G0", 1, 25, 2, "Constant Current")
MUI_XYAT("G0", 1, 37, 3, "Constant Power")
MUI_XYAT("G0", 1, 49, 4, "Constant Resistance")

MUI_FORM(2)
MUI_STYLE(3)
MUI_XYAT("G1", 121, 16, 1, "CC")
MUI_LABEL(83, 16, "Mode:")
MUI_LABEL(0, 60, "Set")
MUI_LABEL(63, 60, "A")
MUI_STYLE(1)
MUI_XY("N0", 21, 60)
MUI_XY("N1", 31, 60)  
MUI_LABEL(40, 60, ".")
MUI_XY("N2", 43, 60) 
MUI_XY("N3", 53, 60) 

MUI_FORM(3)
MUI_STYLE(1)
MUI_LABEL(1, 10, "Constant Power")
MUI_XYAT("G1", 100, 55, 1, "EXIT")

MUI_FORM(4)
MUI_STYLE(1)
MUI_LABEL(1, 10, "Constant Resistance")
MUI_XYAT("G1", 110, 55, 1, "EXIT")
;

muif_t muif_list[] = {  
MUIF_U8G2_LABEL(),
MUIF_BUTTON("G0", mui_u8g2_btn_goto_w1_pi),
MUIF_BUTTON("G1", mui_u8g2_btn_goto_wm_fi),
MUIF_U8G2_U8_MIN_MAX("N0", &current_user_input[0], 0, 9, mui_u8g2_u8_min_max_wm_mud_pf), //input field for current set
MUIF_U8G2_U8_MIN_MAX("N1", &current_user_input[1], 0, 9, mui_u8g2_u8_min_max_wm_mud_pf), //input field for current set
MUIF_U8G2_U8_MIN_MAX("N2", &current_user_input[2], 0, 9, mui_u8g2_u8_min_max_wm_mud_pf), //input field for current set
MUIF_U8G2_U8_MIN_MAX("N3", &current_user_input[3], 0, 9, mui_u8g2_u8_min_max_wm_mud_pf), //input field for current set
MUIF_U8G2_FONT_STYLE(0, u8g2_font_squeezed_b6_tr),
MUIF_U8G2_FONT_STYLE(1, u8g2_font_crox1hb_tf),
MUIF_U8G2_FONT_STYLE(2, u8g2_font_profont10_tf),
MUIF_U8G2_FONT_STYLE(3, u8g2_font_profont12_tf),

};


void setup(){
  //Initialize CS pin and set to high 
  gpio_init(CS_ADC);
  gpio_set_dir(CS_ADC, GPIO_OUT);
  gpio_init(CS_DISPLAY);
  gpio_set_dir(CS_DISPLAY, GPIO_OUT);
  gpio_init(DC_PIN);
  gpio_set_dir(DC_PIN, GPIO_OUT);
  gpio_init(RESET_PIN);
  gpio_set_dir(RESET_PIN, GPIO_OUT);
  gpio_put(CS_ADC, 1);

  gpio_init(SELECT_PIN);
  gpio_set_dir(SELECT_PIN, GPIO_IN);
  gpio_pull_up(SELECT_PIN);
  //enable interrupts
  gpio_set_irq_enabled_with_callback(SELECT_PIN, GPIO_IRQ_EDGE_FALL, 1, isr_handler);  

  gpio_init(ROTARY_CH_A);
  gpio_init(ROTARY_CH_B);
  gpio_set_dir(ROTARY_CH_A, GPIO_IN);
  gpio_set_dir(ROTARY_CH_B, GPIO_IN);
  gpio_set_pulls(ROTARY_CH_A,1,0);
  gpio_set_pulls(ROTARY_CH_B,1,0);

  //enable interrupt for channel A of the rotary encoder
  gpio_set_irq_enabled(ROTARY_CH_A, GPIO_IRQ_EDGE_RISE, 1);  
  gpio_add_raw_irq_handler(ROTARY_CH_A, rotary_callback);
  
  //configure pins to be ready for SPI 
  gpio_set_function(MOSI, GPIO_FUNC_SPI);
  gpio_set_function(MISO, GPIO_FUNC_SPI);
  gpio_set_function(SCLK, GPIO_FUNC_SPI);

  //configure pins to be ready for i2c
  gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
  gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
  gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
  gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);

  //initialize i2c hardware
  i2c_init(i2c_default, 100 * 1000);

  //initialize spi hardware
  spi_init(spi0, 1000000); //initialize spi hardware
  spi_set_format(spi0, 8, SPI_CPOL_1, SPI_CPHA_0, SPI_MSB_FIRST); //THIS LINE IS SUPER IMPORTANT: WILL NOT WORK OTHERWISE

  //update the ad5761r CMD register with desired configuration settings
  ad5761r_set_configuration();

  //setup u8g2 for our specific display
  u8g2_Setup_ssd1309_128x64_noname0_f(&u8g2, U8G2_R2, u8x8_byte_pico_hw_spi, u8x8_gpio_and_delay_pico); 

  u8g2_InitDisplay(&u8g2); // send init sequence to the display, display is in sleep mode after this,
  u8g2_SetPowerSave(&u8g2, 0); // wake up display
  //u8g2_SetFont(&u8g2, u8g2_font_6x12_tr);

  mui_Init(&ui, &u8g2, fds_data, muif_list, sizeof(muif_list)/sizeof(muif_t));
  mui_GotoForm(&ui, 2, 0);
  mui_Draw(&ui);
  u8g2_SendBuffer(&u8g2);
  u8g2_ClearBuffer(&u8g2);
  Serial.begin(9600);

  ADS1x1x_init(&adc, ADS1115, ADS1x1x_I2C_ADDRESS_ADDR_TO_GND, MUX_SINGLE_0, PGA_512);
  ADS1x1x_set_mode(&adc, MODE_CONTINUOUS);
  ADS1x1x_set_data_rate(&adc, DATA_RATE_ADS111x_16);

  COUNT = 0;
}

void loop(){
  //boot screen
  float voltage_request = current_user_input_to_dac_voltage(get_input(current_user_input[0], current_user_input[1], current_user_input[2], current_user_input[3]), 0.0001);
  //crude current and temp measurements
  ADS1x1x_set_multiplexer(&adc,(ADS1x1x_mux_t)MUX_SINGLE_0);
  ADS1x1x_start_conversion(&adc);
  busy_wait_ms(64);
  measured_current = ((double)ADS1x1x_read(&adc)*512)/(32768*11); //11 stems from the noninverting amplifiers gain
  //end of crudeness
  switch(mui_GetCurrentFormId(&ui)){ //move into the logic for the present menu option
    case CONSTANT_CURRENT:
    //the behavoir expected for a constant current loop is:
    //-current should be held constant regardless of input voltage
    //-the setting of current should have an upper bound as to prevent damage to the mosfet(in regards to specs. and temp)
    //todo: get voltage/current reading, add total power consumed option, timer, undervoltage limit
      if(ui.is_mud){ //check if a digit is selected -> input won't change if none are selected
        if(COUNT > LASTCOUNT){
          mui_NextField(&ui);
          voltage_request = current_user_input_to_dac_voltage(get_input(current_user_input[0], current_user_input[1], current_user_input[2], current_user_input[3]), 0.0001);
          //if the incrementation of the the digit on which the cursor is focused is larger than max current...
          if(voltage_request > max_allowed_dac_voltage){ //replace with max current criteria
            current_user_input[0] = 4;
            current_user_input[1] = 0;
            current_user_input[2] = 0;
            current_user_input[3] = 0;
          }
        }
        else if(COUNT < LASTCOUNT){
          mui_PrevField(&ui);
          voltage_request = current_user_input_to_dac_voltage(get_input(current_user_input[0], current_user_input[1], current_user_input[2], current_user_input[3]), 0.0001);
          //if the incrementation of the the digit on which the cursor is focused is larger than max current...
          if(voltage_request > max_allowed_dac_voltage){ //replace with max current criteria
            current_user_input[0] = 4;
            current_user_input[1] = 0;
            current_user_input[2] = 0;
            current_user_input[3] = 0;
          }
        }
        ad5761r_output_write(float_to_dac_code(voltage_request, 5, 0));
      }
      else{ //no digit selected
        menu_handle_fields();
      }
      char str[10];
      sprintf(str, "% 07.3f", measured_current);
      u8g2_SetFont(&u8g2, u8g2_font_crox4hb_tf);
      u8g2_DrawStr(&u8g2, 0, 16, str);
      u8g2_DrawStr(&u8g2, 0, 34, str);
      u8g2_SetFont(&u8g2, u8g2_font_profont12_tf);
      u8g2_DrawStr(&u8g2, 70, 16, "V");
      u8g2_DrawStr(&u8g2, 70, 34, "A");
      u8g2_DrawStr(&u8g2, 70, 47, "W");
      u8g2_SetFont(&u8g2, u8g2_font_crox1hb_tf);
      u8g2_DrawStr(&u8g2, 24, 47, str);
      u8g2_SetContrast(&u8g2, 50);
      /*list of good fonts:
      u8g2_font_crox2hb_tf
      u8g2_font_profont22_tf
      */
      break;
    case CONSTANT_POWER:
      //current = POWER_SET/measured_voltage
      break;
    case CONSTANT_RESISTANCE:
      break;
    default: //MAIN_MENU case
      menu_handle_fields();
      break;
  }
  
  LASTCOUNT = COUNT;
  //update the display
  mui_Draw(&ui);
  u8g2_SendBuffer(&u8g2);
  u8g2_ClearBuffer(&u8g2);
}


float get_input(uint8_t d1, uint8_t d2, uint8_t d3, uint8_t d4){
  return d1 * 10000 + d2 * 1000 + d3 * 100 + d4 * 10; //return the input in milliamps
}

float current_user_input_to_dac_voltage(float input, float divider){
  //takes the input given in the form of a requested current and returns a a voltage that the dac should output
  //calibration, resistor dividers and so on should all be included in here 
  //TODO:
  //calibration
  //should current be passed  arround in amps or milliamps????
  return (float)(input*divider);
}

uint16_t float_to_dac_code(float input, float dac_max_output, float dac_min_output){
  //takes the requested dac voltage (after voltage divider) given as a float and maps it to the uint16 voltage code for the dac
  //ex: dac settings are 0V - 5V (0-50mV due to the divider), so we wish to map the range 0 - 5V over a uint16_t -> float_to_dac_code(requested value, 50000, 0);
  //ex2: dac settings are -2.5V - 7.5V (-25mV - 75mV due to the divider) -> float_to_dac_code(requested value, 75000, -25000);
  if(((((input - dac_min_output)*(UINT16_MAX))/(dac_max_output - dac_min_output))) > UINT16_MAX){ //check for overflow
    return UINT16_MAX;
  }
  else if(((((input - dac_min_output)*(UINT16_MAX))/(dac_max_output - dac_min_output))) < 0){ //check for underflow
    return 0; 
  }
  else{ //return based on the mapping formula
    return (((input - dac_min_output)*(UINT16_MAX))/(dac_max_output - dac_min_output));
  }
}

void menu_handle_fields(){ //used to determine what field the rotary encoder selects next
  if(COUNT > LASTCOUNT){
    mui_NextField(&ui);
  }
  else if(COUNT < LASTCOUNT){
    mui_PrevField(&ui);
  }
}









//general interrupt routine handler
void isr_handler(uint gpio, uint32_t events){ 
  if(gpio == SELECT_PIN){
    mui_SendSelect(&ui);
    if(mui_GetCurrentFormId(&ui) == 1){ //if we exited to the main menu, we want the load to stop all current draw
      
      ad5761r_output_write(0);
    }
  }
}

//rotary encoder interrupt handler
void rotary_callback(void){
  if (gpio_get_irq_event_mask(ROTARY_CH_A) & GPIO_IRQ_EDGE_RISE){
    if(gpio_get(ROTARY_CH_B) != gpio_get(ROTARY_CH_A)){
      COUNT ++;
    }
    else{
      COUNT --;
    }
  }
  gpio_acknowledge_irq(ROTARY_CH_A, GPIO_IRQ_EDGE_RISE);
}







void ad5761r_set_configuration(){
  //sets the configuration of the ad5761r 
  uint8_t configuration_data[3];
  configuration_data[0] = CMD_WR_CTRL_REG;
  configuration_data[1] = 0b00000000;
  configuration_data[2] = 0b01100011; 
  spi_set_format(spi0, 8, SPI_CPOL_1, SPI_CPHA_0, SPI_MSB_FIRST);
  gpio_put(CS_ADC,0);
  spi_write_blocking(spi0, configuration_data, 3);
  gpio_put(CS_ADC,1);
}

void ad5761r_output_write(uint16_t current_user_input){
  /*Function to update the output of the ad5761r, parameter is a 16bit integer that is mapped to the output 
  according to the bits set in the configuration register of the ad5761r*/
  //The desired output voltage is a 16 bit number that is sent after sending the update dac register address 
  uint8_t DATA[3];
  DATA[0] = CMD_WR_UPDATE_DAC_REG;
  DATA[1] = (current_user_input >> 8) & 0xFF;
  DATA[2] = (current_user_input >> 0) & 0xFF;
  spi_set_format(spi0, 8, SPI_CPOL_1, SPI_CPHA_0, SPI_MSB_FIRST); 
  gpio_put(CS_ADC, 0);
  spi_write_blocking(spi0, DATA, 3);
  gpio_put(CS_ADC, 1);
}

void ADS1x1x_write_register(uint8_t i2c_address, uint8_t reg, uint16_t value)
{
  uint8_t buffer[3];
  buffer[0] = reg;
  buffer[1] = (uint8_t)(value>>8); 
  buffer[2] = (uint8_t)(value&0xff);
  i2c_write_blocking(i2c0, i2c_address, buffer, 3, 0);
}

uint16_t ADS1x1x_read_register(uint8_t i2c_address, uint8_t reg)
{
  uint8_t data_buffer[2];
  uint8_t reg_buffer[1];
  uint16_t result;
  reg_buffer[0] = reg;
  i2c_write_blocking(i2c0, i2c_address, reg_buffer, 1, 0);
  i2c_read_blocking(i2c0, i2c_address, data_buffer, 2, 0); 		
  result = (uint16_t)data_buffer[0] << 8 | data_buffer[1];
  return result;
}

////U8G2 library porting functions
uint8_t u8x8_gpio_and_delay_pico(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  switch(msg){

    case U8X8_MSG_GPIO_AND_DELAY_INIT:	// called once during init phase of u8g2/u8x8
      break;							// can be used to setup pins
    case U8X8_MSG_DELAY_10MICRO:		// delay arg_int * 10 micro seconds
      busy_wait_us(arg_int*10);
      break;
    case U8X8_MSG_DELAY_MILLI:			// delay arg_int * 1 milli second
      busy_wait_ms(arg_int);
      break;
    case U8X8_MSG_DELAY_I2C:				// arg_int is the I2C speed in 100KHz, e.g. 4 = 400 KHz
      busy_wait_us(5);
      break;							// arg_int=1: delay by 5us, arg_int = 4: delay by 1.25us
    case U8X8_MSG_GPIO_CS:				// CS (chip select) pin: Output level in arg_int
      gpio_put(CS_DISPLAY, arg_int);
      break;
    case U8X8_MSG_GPIO_DC:				// DC (data/cmd, A0, register select) pin: Output level in arg_int
      gpio_put(DC_PIN, arg_int);
      break;
    case U8X8_MSG_GPIO_RESET:			// Reset pin: Output level in arg_int
      gpio_put(RESET_PIN, arg_int);
      break;
    case U8X8_MSG_GPIO_I2C_CLOCK:		// arg_int=0: Output low at I2C clock pin
      break;							// arg_int=1: Input dir with pullup high for I2C clock pin
    case U8X8_MSG_GPIO_I2C_DATA:			// arg_int=0: Output low at I2C data pin
      break;							// arg_int=1: Input dir with pullup high for I2C data pin*/
    default:
      u8x8_SetGPIOResult(u8x8, 1);			// default return value
      break;
  }
  return 1;
}

uint8_t u8x8_byte_pico_hw_i2c(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr){
  /* u8g2/u8x8 will never send more than 32 bytes between START_TRANSFER and END_TRANSFER */
  static uint8_t buffer[32];
  static uint8_t buf_idx;
  uint8_t *data;
 
  switch(msg){
    case U8X8_MSG_BYTE_SEND:
      data = (uint8_t *)arg_ptr;      
      while(arg_int > 0){
	    buffer[buf_idx++] = *data;
	    data++;
	    arg_int--;
      }      
      break;
    case U8X8_MSG_BYTE_INIT:
      /* add your custom code to init i2c subsystem */
      break;
    case U8X8_MSG_BYTE_SET_DC:
      /* ignored for i2c */
      break;
    case U8X8_MSG_BYTE_START_TRANSFER:
      buf_idx = 0;
      break;
    case U8X8_MSG_BYTE_END_TRANSFER:
      //i2c_transfer(u8x8_GetI2CAddress(u8x8) >> 1, buf_idx, buffer);
      i2c_write_blocking(i2c_default, u8x8_GetI2CAddress(u8x8) >> 1, buffer, buf_idx, false);
      break;
    default:
      return 0;
  }
  return 1;
}

uint8_t u8x8_byte_pico_hw_spi(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr){ 
  switch(msg){
    case U8X8_MSG_BYTE_SEND:
      spi_write_blocking(spi0, (uint8_t *) arg_ptr, arg_int); 
      break;
    case U8X8_MSG_BYTE_INIT:
      break;
    case U8X8_MSG_BYTE_SET_DC:
      gpio_put(DC_PIN, arg_int);
      break;
    case U8X8_MSG_BYTE_START_TRANSFER:
      switch(u8x8->display_info->spi_mode) {
        case 0: 
          spi_set_format(spi0, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
          break;
        case 1: 
          spi_set_format(spi0, 8, SPI_CPOL_0, SPI_CPHA_1, SPI_MSB_FIRST);
          break;
        case 2: 
          spi_set_format(spi0, 8, SPI_CPOL_1, SPI_CPHA_0, SPI_MSB_FIRST);
          break;
        case 3: 
          spi_set_format(spi0, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
          break;
      }
      gpio_put(CS_DISPLAY, LOW);
      break;
    case U8X8_MSG_BYTE_END_TRANSFER:
      gpio_put(CS_DISPLAY, HIGH);
      break;
    default:
      return 0;
  }
  return 1;
}
