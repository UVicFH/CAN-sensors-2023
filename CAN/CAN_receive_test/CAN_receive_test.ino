// CAN Receive Testing

#include <stdlib.h>
#include <SPI.h>
#include "mcp2515_can.h"
#include "mcp2515_can_dfs.h"

typedef unsigned char byte;

const int CAN_INT_PIN = 2;
const int SPI_CS_PIN = 9;

unsigned long canId = 0x00; // CAN message ID
byte msg_length = 0; // length of message from CAN
byte msg_buffer[8]; // buffer for storing message from CAN

#define  CAN_MSG_DELAY 100
unsigned long can_timeold;
int can_delay_cycle;
int message_num = 0;

/* TODO: Test with larger arrays */
// unsigned char Brake_Status[8] = {9,3,8,9,9,3,8,9};
// unsigned char Fuel_Status[8];
// unsigned char Fan_Status[8];
// unsigned char WheelSpeed[8];

// -> Example Global Vars
byte Brake_OK_data[1] = {0x00}; const byte BRAKE_CAN_ID = 0x55; 
byte Fuel_Pump_OK_data[1] = {0x00}; const byte FUEL_CAN_ID = 0x66;
byte Fan_OK_data[1] = {0x00}; const byte FAN_CAN_ID = 0x77;
byte Wheel_Speed_data[1] = {0x00}; const byte WSS_CAN_ID = 0x88;
byte Shift_Up_data[1] = {0x00}; const byte SHIFT_UP_CAN_ID = 0x99;

// Function Definitions
void init_CAN();
mcp2515_can CAN(SPI_CS_PIN); // set CS to pin 9
void CAN_message_handler();
void CAN_read_from_network();
void switch_data_store(unsigned long CAN_ID, unsigned char*& data_container);


void setup() 
{
  Serial.begin(115200);
  init_CAN();

  Serial.println("\n-- RECEIVE --\n");
}

void loop() 
{  
  CAN_message_handler();

  can_delay_cycle = millis() - can_timeold;
  if (can_delay_cycle >= CAN_MSG_DELAY) {
    can_timeold = millis();
    
  }
  
}

// Initialize MCP2515 running with a baudrate of 500kb/s.
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
    CAN.readMsgBuf(&msg_length, msg_buffer);

    /* Debugging Prints */
    Serial.print("\nCanID: ");
    Serial.println(canId);
    // Serial.println("Printing Message Buffer");
    // for (int i=0; i < msg_length; i++) {
    //   Serial.println(msg_buffer[i]);
    // }

    /* Could change this switch case to a range of CAN IDs instead of having one for each case */
    /* Like: if (canID <= 0x99 && canID >= 0x55 ) CAN_read_from_network(); */
    switch (canId)
    {
      case BRAKE_CAN_ID:
        // Serial.println("BRAKE CAN MESSAGE RECEIVED");
        CAN_read_from_network();
        break;

      case FUEL_CAN_ID:
        // Serial.println("FUEL CAN MESSAGE RECEIVED");
        CAN_read_from_network();
        break;

      case FAN_CAN_ID:
        // Serial.println("FAN CAN MESSAGE RECEIVED");
        CAN_read_from_network();
        break;

      case WSS_CAN_ID:
        // Serial.println("WSS CAN MESSAGE RECEIVED");
        CAN_read_from_network();
        break;

      case SHIFT_UP_CAN_ID:
        Serial.println("SHIFT UP RECEVIED");
        // Call shift up function or use TI idk where this signal is going
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
    Serial.println("=== Printing Data Containers");
    Serial.print("Brake Data: "); Serial.print(Brake_OK_data[0]); Serial.println("");
    Serial.print("Fuel Data: "); Serial.print(Fuel_Pump_OK_data[0]); Serial.println("");
    Serial.print("Fan Data: "); Serial.print(Fan_OK_data[0]); Serial.println("");
    Serial.print("WheelSpeed Data: "); Serial.print(Wheel_Speed_data[0]); Serial.println("\n\n");
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
  unsigned char* data_container = NULL;
  switch_data_store(canId, data_container);
  if (data_container == NULL) 
  {
    Serial.println("error setting the data container...\n");
    return;
  }

  for(int i=0; i < msg_length; i++)
  {
    // Serial.print(msg_buffer[i]);
    data_container[i] = msg_buffer[i];
  }
}

/*
  Function: HELPER method used in the CAN_read_from_network() function. This method
            is used to select the appropriate container for storing the data read
            from the incoming CAN message. the method returns a pointer to the data
            container that is used for storing data in the read function.
*/
void switch_data_store(unsigned long CAN_ID, unsigned char*& data_container)
{
  switch(CAN_ID)
  {
    case BRAKE_CAN_ID:
      data_container = Brake_OK_data;
      break;

    case FUEL_CAN_ID:
      //engine temp
      data_container = Fuel_Pump_OK_data;
      break;

    case FAN_CAN_ID:
      //engine temp
      data_container = Fan_OK_data;
      break;

    case WSS_CAN_ID:
      //engine temp
      data_container = Wheel_Speed_data;
      break;

    //...

    default:
      *data_container = NULL;
      break;
  }
}