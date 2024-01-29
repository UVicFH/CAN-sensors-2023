#include <SPI.h>
#include "mcp2515_can.h"
#include "mcp2515_can_dfs.h"
#include "JC_Button.h"  // library for input buttons

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

unsigned char len = 0; // READ message Length
unsigned char rxBuf[8]; // READ buffer for storing message

// Global Variables and Constants
// Data containers for menu sceen data
unsigned char wheelSpeed[8];
unsigned char engineTemp[8];
unsigned char stateOfCharge[8];
//...

// TODO: The defines are used for the pin number for the associated
// sensor/button/component is attached to onn the canduino.
// the byte variables are used for encoding and passing CAN messages,
// If a sensor/button/component needs to send a CAN message, it
// will need a byte variable. Define any other constants or variables 
// needed that can be defined outside of a function.

// -> shift button vars
#define SHIFT_UP_BUTTON_PIN = 2;
#define SHIFT_DOWN_BUTTON_PIN = 3;
bool shift_up_flag = false;
bool shift_down_flag = false;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 200;

// -> engine start vars
#define ENGINE_START_BUTTON_PIN ##
Button engine_start_button(ENGINE_START_BUTTON_PIN, 100, true, true); //btn(pin, debounceTime, pullupEnabled, logicInvert)

// -> motor start vars
#define MOTOR_START_BUTTON_PIN ##
Button motor_start_button(MOTOR_START_BUTTON_PIN, 100, true, true);

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
void CAN_read_from_network();
void switch_data_store();
void shift_up();
void shift_down();
void debounce();

/********** Setup/Initialization ***************/

mcp2515_can CAN(SPI_CS_PIN); // set CS to pin 9

void setup() {
  // set the speed of the serial port
  Serial.begin(115200);

  init_CAN();
  can_timeold = 0;

  // TODO: Use setup for initializing the state of any variables
  // or initializing pinmodes/pin read or write. Anything that needs setup

  // engine and motor button setups
  engine_start_button.begin();
  motor_start_button.begin();

  // shift button setups
  pinMode( SHIFT_UP_BUTTON_PIN, INPUT_PULLUP );
  attachInterrupt( digitalPinToInterrupt(SHIFT_UP_BUTTON_PIN), shift_up, FALLING ); // Sending signal on press, not release for some reason.
  pinMode( SHIFT_DOWN_BUTTON_PIN, INPUT_PULLUP );
  attachInterrupt( digitalPinToInterrupt(SHIFT_DOWN_BUTTON_PIN), shift_down, FALLING ); // Sending signal on press, not release for some reason.
  
}

/********** Main Loop ***************/

void loop() {
  CAN_message_handler();

  can_delay_cycle = millis() - can_timeold;
  if (can_delay_cycle >= CAN_MSG_DELAY) message_cycle();


  /* Engine/Motor Start Buttons*/
  engine_start_button.read();

  // send CAN message if start engine button has been pushed
  if(engine_start_button.wasReleased()) //returns true on falling edge, when there's a change in read() value
  {
    // send engine start CAN message
    // send_CAN_msg(); //TODO: set up messageID, 
  }
  // send CAN message if start motor button has been pushed
  if(motor_start_button.wasReleased())
  {
    // send motor start CAN message
    // send_CAN_msg(); //TODO: set up messageID, 
  }

  /*Shift messages*/
  if(shift_up_flag)
  {
    // TODO: Implement rpm checking to know if shift up should occur (is rpm > Min_value_rpm)
    // reset flag
    shift_up_flag = false;
    //send can_message 
  }
  if(shift_down_flag)
  {
    // TODO: Implement rpm checking to know if shift down should occur (is rpm < Max_value_rpm)
    // reset flag
    shift_down_flag = false;
    //send can message
  }
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
            CAN network and routes the program to the appropriate handler.
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
  Function: The CAN_read_from_network() function is used to read the current message off
            off the CAN network and route the incoming message to be stored in its respective
            container. ie reading the wheel speed ID, then storing/updating the message buffer
            to the wheel speed container variable. 
*/
void CAN_read_from_network()
{
  if (CAN_MSGAVAIL == CAN.checkReceive())
  {
    unsigned long canId = CAN.getCanId();
    CAN.readMsgBuf(&len, rxBuf); 
  }

  char* data_container = nullptr;
  switch_data_store(canId, data_container);
  if (data_container == nullptr) 
  {
    Serial.println("error setting the data container...\n");
    return;
  }

  for(i=0; i < len; i++)
  {
    data_container[i] = rxBuf[i];
  }
}

/*
  Function: HELPER method used in the CAN_read_from_network() function. This method
            is used to select the appropriate container for storing the data read
            from the incoming CAN message. the method returns a pointer to the data
            container that is used for storing data in the read function.
*/
void switch_data_store(unsigned long CAN_ID, char*& data_container)
{
  switch(CAN_ID)
  {
    case WS_ID:
      //wheel speed
      data_container = wheelSpeed;
      break;

    case ET_ID:
      //engine temp
      data_container = engineTemp;
      break;

    //...

    default:
      *data_container = nullptr;
      break;
  }
}

/*
  Function: The shift_up function is an ISR that sends a CAN message from the steering wheel canduino,
            to the mid canduino to shift the gear up.
*/
void shift_up()
{
  if(debounce()) shift_up_flag = true;
}

/*
  Function: The shift_up function is an ISR that sends a CAN message from the steering wheel canduino,
            to the mid canduino to shift the gear down.
*/
void shift_down()
{
  if(debounce()) shift_down_flag = true;
}

/*
  Function: The debounce() function debounces the buttons for the shift up and down ISR functions. 
            The ISRs are still called on each "bounce", but only when debounced time has passed will it complete an action.
*/
bool debounce()
{
  // check to see if enough time has passed
  // since the last signal to ignore any bounce:
  if ((millis() - lastDebounceTime) > bounceDelay) {
    lastDebounceTime = millis();
    return true;
  }
  return false;
}