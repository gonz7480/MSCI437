/** This code just drops the Benthic Observatory, the propellors and GPS coordinates are not used
    Written by Kaitlyn Beardshear
*/

//Variables to change depth and time spent on bottom
int depth = 2; //Measured in meters
int bottomTime = 2; //Measured in seconds

#include <Servo.h>  //Servo library

int reelState; //Variable to hold if the switch is closed or not
int reelSwitch = 41; //Pin connected to the reel switch

const int motorController = 30; //Winch controller
int dropTime; //Variable to hold the droptime in ms
int winchWait; //Variable to hold the bottom time in ms
int i; //Variable to count what loop cycle winchDown in on
int dropLoops; //The number of times the winchDown loop cycles

//Declare servos
Servo winchMtr; //Winch controller

//function to pull  up the Benthic Observatory
void winchUp () {
  reelState = digitalRead(reelSwitch);
  while (reelState == 1) { //if the magnet is not connected, keep pulling up
    reelState = digitalRead(reelSwitch);
    winchMtr.writeMicroseconds(1200);
  }
  if (reelState == 0) {
    winchStop(); //once the magnet is connected, stop
  }
}

//Function to drop the Benthic Observatory
void winchDown() {
  for (i = 0; i < dropLoops; i++) { //Checks the reelSwitch every 200ms just in case the line becomes tangled while dropping
    reelState = digitalRead(reelSwitch);
    if (reelState == 0) {
      winchStop(); //Once the switch is connected, stop
    } else {
      winchMtr.writeMicroseconds(1700);
    }
    delay(200);
  }
}

//Function to stop the winch
void winchStop() {
  winchMtr.writeMicroseconds(1500);
}

void winchPause(int winchWait) {
  winchMtr.writeMicroseconds(1500);
  delay(winchWait);
}

void setup() {
  Serial.begin(9600);  //Start the serial monitor
  winchMtr.attach(30);  //Attach the motor 
  pinMode(reelSwitch, INPUT_PULLUP);  //Set the reelSwitch as an input

  dropTime = ((depth) * 7000); //The winch takes approx 7 seconds to unspool 1 meter of string, this calculates
  // how long the winch needs to unspool based on the given depth.
  dropLoops = ((dropTime) / 200); //The loop needs to cycle every 200 ms for the total amount of time
  winchWait = ((bottomTime) * 1000); //This changes the bottom time from seconds to milliseconds
}

void loop() {
}