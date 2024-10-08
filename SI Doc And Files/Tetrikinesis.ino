#include <Arduino_BMI270_BMM150.h>
#include "Nano33BleHID.h"
#include "signal_utils.h"

#define MUSCPIN A6

// variables for gyroscope/acceleratometer
float x, y, z;

// Thresholds and debounce
const unsigned long DEBOUNCE_TIME = 500;
const float ACCEL_THRES = 2.5;
const float FLICK_THRES = 1000;
const int MUSC_THRES = 700;

// Variables to aid debounce
unsigned long last_musc = -DEBOUNCE_TIME;
unsigned long last_flick = -DEBOUNCE_TIME;

boolean musc_activated = false;

Nano33BleKeyboard bleKb("Tetrikinesis");

// Builtin LED animation delays when disconnect. 
static const int kLedBeaconDelayMilliseconds = 1185;
static const int kLedErrorDelayMilliseconds = kLedBeaconDelayMilliseconds / 10;

// Builtin LED intensity when connected.
static const int kLedConnectedIntensity = 30;

void setup()
{
  // General setup.
  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(MUSCPIN, INPUT); // Pin for muscle sensor
  Serial.begin(9600); 

  // IMU
  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    animateLED(LED_BUILTIN, kLedErrorDelayMilliseconds);
    while (1);
  }

  // Initialize both BLE and the HID.
  bleKb.initialize();

  // Launch the event queue that will manage both BLE events and the loop. 
  // After this call the main thread will be halted.
  MbedBleHID_RunEventThread();
}

void loop()
{
  // When disconnected, we animate the builtin LED to indicate the device state.
  if (bleKb.connected() == false) {
    animateLED(LED_BUILTIN, (bleKb.has_error()) ? kLedErrorDelayMilliseconds 
                                                : kLedBeaconDelayMilliseconds);
    return;
  }

  // When connected, we slightly dim the builtin LED.
  analogWrite(LED_BUILTIN, kLedConnectedIntensity);

  // Retrieve the HIDService to update.
  auto *kb = bleKb.hid();

  // Gyroscopic
  if (IMU.gyroscopeAvailable()) {
    IMU.readGyroscope(x, y, z);

    if (millis() - last_flick > DEBOUNCE_TIME){
      if (x > FLICK_THRES) {
        Serial.println("Right Rotation");
        kb->sendCharacter('A');
        last_flick = millis();
      }
      if (x < -FLICK_THRES) {
        Serial.println("Left Rotation");
        kb->sendCharacter('C');
        last_flick = millis();
      }
    }
  }

  // Acceleration
  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(x, y, z);

    if (millis() - last_flick > DEBOUNCE_TIME){
      if (y > ACCEL_THRES) {
        Serial.println("Right Slide");
        kb->sendCharacter('R');
        last_flick = millis();
      }
      if (y < -ACCEL_THRES) {
        Serial.println("Left Slide");
        kb->sendCharacter('L');
        last_flick = millis();
      }
    }
  }

  // Myosensor
  if (analogRead(MUSCPIN) > MUSC_THRES && !musc_activated && millis() - last_musc > DEBOUNCE_TIME) {
    kb->sendCharacter('M');
    musc_activated = true;
    last_musc = millis();
  }

  if (analogRead(MUSCPIN) <= MUSC_THRES) {
    musc_activated = false; 
  }
}
/* -------------------------------------------------------------------------- */
