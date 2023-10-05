#include <SPI.h>
#include "mcp2515_can.h"
#include "mcp2515_can_dfs.h"

typedef unsigned char byte;

// -> CAN message vars
#define SPI_CS_PIN 9 // CAN specific pins
#define CAN_INT_PIN 2 // CAN interrupt pin
unsigned long canId = 0x00; // CAN message ID
byte msg_length = 0; // length of message from CAN
byte msg_buffer[8]; // buffer for storing message from CAN
#define  CAN_MSG_DELAY 100
unsigned long can_timeold;
int can_delay_cycle;
int message_num = 0;

// Global Variables and Constants

// TODO: The defines are used for the pin number for the associated
// sensor/button/component is attached to onn the canduino.
// the byte variables are used for encoding and passing CAN messages,
// If a sensor/button/component needs to send a CAN message, it
// will need a byte variable. Define any other constants or variables 
// needed that can be defined outside of a function.

// -> engine start vars
#define ENGINE_START_BUTTON_PIN ##

// -> motor start vars
#define MOTOR_START_BUTTON_PIN ##

// -> fuel pump status vars
#define FUEL_SIGNAL_PIN ##
byte Fuel_Pump_OK_data[1] = {0x00};

// -> fan status vars
#define FAN_SIGNAL_PIN ##
byte Fan_OK_data[1] = {0x00};

// -> brake status vars
#define BRAKE_SIGNAL_PIN ##
byte Brake_OK_data[1] = {0x00};

// -> start status vars
#define START_SIGNAL_PIN ##
byte Start_OK_data[1] = {0x00};

// -> current sensor vars
#define CURRENT_SENSOR_PIN ##
byte Current_Sensor_Voltage_Data[2] ={0x00,0x00};

// -> shifting functionality vars
unsigned long shift_CAN_ID = 0x01; // shift message CAN ID
#define SHIFT_ACTUATOR_PIN ##
byte shift_signal_data[1] = {0x00};

// -> throttle functionality vars
unsigned long aps_CAN_ID = 0x02; // accelerator position sensor CAN ID
#define THROTTLE_PIN ##
byte Throttle_signal_data[1] = {0x00};

// Function Declarations
void init_CAN();
void send_CAN_msg(unsigned long id, byte ext, byte len, const byte * msg_buf);
void message_cycle();
void CAN_message_handler();

// TODO: for any functions you write at the base of the file, define the
// signature of the function here for everything to compile and be happy

void shift();

/********** Setup/Initialization ***************/

mcp2515_can CAN(SPI_CS_PIN); // set CS to pin 9

void setup() {
  // set the speed of the serial port
  Serial.begin(115200);

  init_CAN();
  can_timeold = 0;

  // TODO: Use setup for initializing the state of any variables
  // or initializing pinmodes/pin read or write. Anything that needs setup

  // Engine and motor button setups
  pinMode( ENGINE_START_BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt( digitalPinToInterrupt(ENGINE_START_BUTTON_PIN), engine_start, RISING);
  pinMode( MOTOR_START_BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt( digitalPinToInterrupt(ENGINE_START_BUTTON_PIN), motor_start, RISING);
}

/********** Main Loop ***************/

void loop() {
  CAN_message_handler();

  can_delay_cycle = millis() - can_timeold;
  if (can_delay_cycle >= CAN_MSG_DELAY) message_cycle();

}

/********** Function Implementations ***************/
/* 
  Function: Initialize MCP2515 (CAN system) running with a 
            baudrate of 500kb/s. Code will not continue until 
            initialization is OK.
*/
void init_CAN()
{
  while (CAN_OK != CAN.begin(CAN_500KBPS))
  {
    Serial.println("CAN init fail, retry...");
    delay(100);
  }
  Serial.println("CAN init ok!");

  
}

/* 
  Function: Sending a CAN message over the network.
  Params:   unsigned long id - The message ID.
            byte ext - The extension code for the message. typically 0.
            byte len - The message length. (max 8 bytes)
            const byte * msg_buf - The message byte array to be transmitted over CAN.
*/
void send_CAN_msg(unsigned long id, byte ext, byte len, const byte * msg_buf)
{
  byte send_status = CAN.sendMsgBuf(id, ext, len, msg_buf);

  if (send_status == MCP2515_OK) Serial.println("Message Sent Successfully!");
  else Serial.println("message Error...");
}

/*
  Function: The message_cycle() function is used to perpetually cycle through and
            send messages from the various sensors connected to the front canduino.
            Each global data array for the sensors is constantly updated from the main
            loop from which message_cycle() takes the data array and transmits the 
            message over the CAN network. 
  Params:   global sensor data buffers - The global variables are used to as containers
            to allow for indefinite updating of the sensor values. The fuction then reads
            those values and transmits them.
  Pre-conditions: CAN must first be initialized.
*/
void message_cycle()
{
  switch (message_num)
  {
    case 0:
      // send_CAN_msg(0x55, 0, 1, YOUR_VARIABLE);
      message_num++;
      break;
    
    case 1:
      // send_CAN_msg(0x66, 0, 1, YOUR_VARIABLE);
      message_num++;
      break;
    
    case 2:
      // send_CAN_msg(0x77, 0, 1, YOUR_VARIABLE);
      message_num++;
      break;

    case 3:
      // send_CAN_msg(0x88, 0, 1, YOUR_VARIABLE);
      message_num = 0;
      break;

    default:
      //reset message number if issue is encountered.
      message_num = 0;
  }

  can_timeold = millis();
}

/*
  Function: The message_read() function gets the CAN ID of the current message on the
            CAN network and routes the program to the apropriate handler.
*/
void CAN_message_handler()
{
  // TODO: This function is used as the main point of executing any function associated
  // to a read CAN message. Instead of calling your functions in "loop()", add cases
  // for each can ID and call your function from within the case

  if (CAN_MSGAVAIL == CAN.checkReceive())
  {
    canId = CAN.getCanId();
    CAN.readMsgBuf(&msg_length, msg_buffer)

    switch (canId)
    {
      case shift_CAN_ID:
        shift();
        break;

      case aps_CAN_ID:
        throttle();
        break;
/*
      case some_CAN_ID:
        some_event_handler_function();
        break;
*/
      default:
        // do nothing if not an accepted CAN ID
        break;
    }
  }
}

/*
  Function: The engine_start() function starts the ICE (Internal Combustion Engine) of the car. 
            It is an ISR called by pushing the Engine Start button on the steering wheel.
*/

void engine_start() 
{
  // TODO: Implement process for starting engine.
  // Does it need to turn off the motor if motor running?
}

/*
  Function: The motor_start() function starts the electric motor of the car.
            It is an ISR called by pushing the Motor Start button on the steering wheel.
*/
void motor_start()
{
  // TODO: Implement process for starting motor.
  // Does it need to turn off engine if engine running?
} 

/*
  Function: The shift function reads the data message sent by the steering
            wheel canduino and triggers the actuator attached to the shifter
            such that the gear is shifted up or down relative to what value
            is passed in the CAN message.
*/
void shift()
{
  // verify that we are handling the correct task
  if (canId == shift_CAN_ID && msg_length == 1)
  {
    // shift function implementation
  }
}


