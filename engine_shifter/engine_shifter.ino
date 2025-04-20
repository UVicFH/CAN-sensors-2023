/* Notes:
  - Reverse Mode increases encoder value
  - Forward Mode decreases encoder value
  TODO: Change UP_SHIFT and DOWN_SHIFT setpoints to the engine's liking
  If up shifting and down shifting is reversed, change the sign of the setpoints... Should work?
*/

#define ENCODER_OPTIMIZE_INTERRUPTS // For optimized interrupts in encoder
#include <Encoder.h>
#include <Servo.h>
#include <PID_v1.h>
const int PWM_PIN = 44;  // The PWM output pin to the SPARK MAX
const int SHIFT_UP_PIN = 49;
const int SHIFT_DOWN_PIN = 46;
const int SHIFT_NEUTRAL_PIN = 48;
const int MOTOR_ON_PIN = 47;

// Previous button states
bool prevShiftUpState = HIGH;
bool prevShiftDownState = HIGH;
bool prevNeutralState = HIGH;
bool prevMotorOnState = HIGH;

// Previous setpoints
int Prev_setpoint = 999;


const int UP_SHIFT_SETPOINT = -600;      /** CHANGE WHEN MOUNTED ON CAR **/
const int DOWN_SHIFT_SETPOINT = 600;   /** CHANGE WHEN MOUNTED ON CAR **/
const int NEUTRAL_SETPOINT = 0;

// Define the pins connected to Channel A and B of the encoder
Encoder myEnc(2, 3);  // Replace with your actual pin numbers
Servo sparkmax;

// Define Variables we'll be connecting to for PID
double Setpoint, Input, Output;
const double Kp = 2.3, Ki = 0.005, Kd = 0.008;

// Specify the links and initial tuning parameters for PID
PID myPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT);

double oldPosition = -99;

unsigned long neutralStartTime;
const unsigned long NEUTRAL_TIMEOUT = 200;  // Time in milliseconds to wait before returning

void setup() {
  Serial.begin(9600);
  pinMode(PWM_PIN, OUTPUT);
  sparkmax.attach(PWM_PIN);

  // Need setpoints for Forward, Neutral and Reverse
  Setpoint = 0.0;  // Set desired position for the motor

  myPID.SetOutputLimits(-5000.0, 5000.0);
  myPID.SetMode(AUTOMATIC);

  // Shifter Paddles
  pinMode(SHIFT_UP_PIN, INPUT_PULLUP);
  pinMode(SHIFT_DOWN_PIN, INPUT_PULLUP);
  pinMode(SHIFT_NEUTRAL_PIN, INPUT_PULLUP);
  pinMode(MOTOR_ON_PIN, INPUT_PULLUP);

  neutralStartTime = 0;  // Time when we first entered neutral state
}



void loop() {
  bool shiftUpState = digitalRead(SHIFT_UP_PIN);
  bool shiftDownState = digitalRead(SHIFT_DOWN_PIN);
  bool neutralState = digitalRead(SHIFT_NEUTRAL_PIN);
  bool motorOnState = digitalRead(MOTOR_ON_PIN);
  
  // Serial.print("Output Value: ");
  // Serial.println(Output);
  

  if (shiftUpState == LOW && prevShiftUpState == HIGH) {
    Serial.println("Shift Up Pressed");
    sparkmax.writeMicroseconds(2000);
    while(Input > UP_SHIFT_SETPOINT) {Input = myEnc.read();}
  }
 
  if (shiftDownState == LOW && prevShiftDownState == HIGH) {
    Serial.println("Shift Down Pressed");
    sparkmax.writeMicroseconds(1000);
    while(Input < DOWN_SHIFT_SETPOINT) {Input = myEnc.read();}
  }

  if (neutralState == LOW && prevNeutralState == HIGH) {
    Serial.println("Neutral Pressed");
    Setpoint = NEUTRAL_SETPOINT;
  }

  // Update previous button states
  prevShiftUpState = shiftUpState;
  prevShiftDownState = shiftDownState;
  prevNeutralState = neutralState;
  prevMotorOnState = motorOnState;

  delay(20);  // Button debounce


  // Update PID input with the current encoder position
  Input = myEnc.read();
  // Compute PID output
  myPID.Compute();

  if(Output < -700) { // Error is large = position - setpoint. Need to increase encoder value, which is reversing motor
    /*  Full Reverse: p <= 1000 */
    // Serial.println("FULL REVERSE");
    sparkmax.writeMicroseconds(1000);
  } 
  else if(Output >= -700 && Output <= -10) {
    /* Proportional Reverse: 1000 < p < 1475 */
    // Serial.println("PROP REVERSE");
    int speed = map(Output, -10, -700, 1450, 1100);
    sparkmax.writeMicroseconds(speed);
  } 
  else if(Output > 700) { 
    /* Full Forward: p >= 2000 */
    // Serial.println("FULL FORWARD");
    sparkmax.writeMicroseconds(2000);
  }
  else if(Output >= 10 && Output <= 700) {
    /* Proportional Forward: 1525 < p < 2000 */
    // Serial.println("PROP FORWARD");
    int speed = map(Output, 700, 10, 1900, 1550);
    sparkmax.writeMicroseconds(speed);
  }
  else if(Output > -10 && Output < 10) { // Set Neutral
    /* Neutral: 1475 <= p <= 1525 */
    // Serial.println("Neutral");
    sparkmax.writeMicroseconds(1500);
  }
}