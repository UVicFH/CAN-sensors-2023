// CAN Send Testing

// 1 button worked, need to test both buttons and interrupts

#include <SPI.h>
#include "mcp2515_can.h"
#include "mcp2515_can_dfs.h"

const int SPI_CS_PIN = 9;
const int CAN_INT_PIN = 2;

const int SHIFT_UP_BUTTON_PIN = 2;
const int SHIFT_DOWN_BUTTON_PIN = 3;
bool shift_up_flag = false;
bool shift_down_flag = false;

// Function Declaration
void init_CAN();
mcp2515_can CAN(SPI_CS_PIN); // set CS to pin 9


void setup() {
  Serial.begin(115200);
  init_CAN();

  Serial.println("\n-- SEND --\n");

  pinMode( SHIFT_UP_BUTTON_PIN, INPUT_PULLUP );
  attachInterrupt( digitalPinToInterrupt(SHIFT_UP_BUTTON_PIN), shift_up, FALLING ); // Sending signal on press, not release for some reason.
  pinMode( SHIFT_DOWN_BUTTON_PIN, INPUT_PULLUP );
  attachInterrupt( digitalPinToInterrupt(SHIFT_DOWN_BUTTON_PIN), shift_down, FALLING ); // Sending signal on press, not release for some reason.

}

// test data to transmit onto CAN network
unsigned char data[8] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x5d, 0x06, 0x07};

void loop() {
  if(shift_flag) {
    unsigned char send_status = CAN.sendMsgBuf(0x1e, 0, 8, data);
    if (send_status == MCP2515_OK) 
    {
      Serial.println("Message Sent Successfully!");
    } else Serial.println("uh oh...");

    delay(100);
    shift_flag = false;
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
  Function: The shift_up function is an ISR that sends a CAN message from the steering wheel canduino,
            to the mid canduino to shift the gear up.
*/
void shift_up()
{
  debounce();
  shift_up_flag = true;
}

/*
  Function: The shift_up function is an ISR that sends a CAN message from the steering wheel canduino,
            to the mid canduino to shift the gear down.
*/
void shift_down()
{
  debounce();
  shift_down_flag = true;
}

/*
  Function: The debounce() function debounces the buttons for the shift up and down ISR functions. 
            The ISRs are still called on each "bounce", but only when debounced time has passed will it complete an action.
*/
bool debounce()
{
  // check to see if enough time has passed
  // since the last press to ignore any noise:
  if ((millis() - lastDebounceTime) > bounceDelay) {
    lastDebounceTime = millis();
    return true;
  }
  return false;
}