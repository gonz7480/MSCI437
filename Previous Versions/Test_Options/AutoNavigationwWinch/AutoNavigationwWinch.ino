/* This is one version of the code that just uses the Mega. It moves the buoy to the destination with the reed switch
   interrupt included. The interrupt allows for the motors to be stopped at any point. This version allows for the Benthic
   Observatory to be dropped.

   Compass code based on the example in the Adafuit_LSM303_U library.
   Select GPS lines are from the example in the TinyGPS++ library.
   All movement functions were written primarily by: Sophia Rose, Kaitlyn Beardshear, Andrew Reyna
   Reed and switch interrupt code written by Kaitlyn Beardshear
   Motor code written by Cristian Arguera and Kaitlyn Beardshear
*/

//Variables to change based on the launch
int depth = 3; //How deep the Benthic Observatory is lowered (measured in meters)
int bottomTime = 30; //How long the Benthic Observatory sits on the seafloor (measured in seconds)
int delayStart = 5; //How long to wait before dropping the Benthic Observatory (measured in minutes)
float des_LAT = 36.60337222; //Destination Latitude
float des_LNG = -121.88472222; //Destination Longitude

//Toggle this boolean to turn on print statements for debuggings
bool debug = false;

//Libraries
#include <SoftwareSerial.h> //TX RX library
#include <Servo.h>  //Servo library
#include <TinyGPS++.h>  //GPS library
#include <Wire.h>  //I2C library
#include <Adafruit_Sensor.h>  //Sensor library
#include <Adafruit_LSM303_U.h>  //Compass library

//Defined pins and global variables
//Serial connection with the GPS
#define RXfromGPS 52 //Receiving from GPS (the GPS' TX)
#define TXtoGPS 53 //Sending to the GPS (the GPS' RX)

//The box switch is used to stop the propellors manually
#define boxSwitch 2 //Pin used to stop the motors manually, MUST be in pin 2,3,18,19,20,or 21 for interrupt sequence to work
volatile byte boxState = HIGH; //Set box switch to high

//The reel switch is used to stop the winch
int reelState; //Variable to hold if the reel switch is closed or not
#define reelSwitch 41 //Pin connected to the reel switch

//Serial connection between the Mega and onboard Uno
#define RXfromUno 23 //receiving from the Uno (the Uno's TX)
#define TXtoUno 22 //sending to the Uno (the Uno's RX)

//Winch variables
#define motorController 30 //Winch controller
int delayTime; //Variable to hold the start delay in ms
int dropTime; //Variable to hold the droptime in ms
int winchWait; //Variable to hold the bottom time in ms
int dropLoops; //The number of times the winchDown loop cycles

//Baud rates
#define GPSBaud 9600 //GPS communication
#define UnoBaud 9600 //Uno communication
#define ConsoleBaud 115200 //Used for debugging

#define Pi 3.14159 //Used for compass calculations 

//Declare servos
Servo RTmtr; //Right propellor
Servo LTmtr; //Left propellor
Servo winchMtr; //Winch controller

/** The serial connection to the GPS device, input is (ArduinoRX, ArduinoTX). RX and TX need to be paired opposite
    (eg GPS RX connects to Arduino TX). This is written from the Arduino's perspective so the Arduino's RX
    recieves from the GPS' TX. **/
SoftwareSerial gpsSerial(RXfromGPS, TXtoGPS);
SoftwareSerial unoSerial(RXfromUno, TXtoUno); //Same description as above but with the Uno instead of GPS

TinyGPSPlus gps; //The TinyGPS++ object
unsigned long lastUpdateTime = 0; //Set last updated time to zero

Adafruit_LSM303_Mag_Unified mag = Adafruit_LSM303_Mag_Unified(12345); //Compass object

//Lat and Long of launching point
float home_LAT;
float home_LNG;

//Current Lat, Lon, and heading
float curr_LAT;
float curr_LNG;
float heading;

//Compass variables
float distanceToDestination;
float courseToDestination;
int courseChangeNeeded;

//Boolean used with getHome()
bool launch = true;

// Function to read the magnetometer and convert output to degrees
float compass() { //Get a new sensor event
  sensors_event_t event;
  mag.getEvent(&event);
  float heading = (atan2(event.magnetic.y, event.magnetic.x) * 180) / Pi;  // Calculate the angle of the vector y,x
  if (heading < 0) {  // Normalize to 0-360
    heading = 360 + heading;
  }
  return heading;
}

/* Function to determine which direction RoboBuoy needs to turn to correct its heading
   Returns a character defining which direction to turn.
   'R' for right, 'L' for left, 'N' for none */
char courseChange() {
  if (courseChangeNeeded > -10 && courseChangeNeeded < 10) {
    return 'N';
  }
  else if (courseChangeNeeded < -180) {
    return 'L';
  }
  else if (courseChangeNeeded < 180 && courseChangeNeeded > 0) {
    return 'L';
  }
  else if (courseChangeNeeded >= 180) {
    return 'R';
  }
  else if (courseChangeNeeded >= -180  && courseChangeNeeded < 0) {
    return 'R';
  }
  else {
    return 'N';
  }
}

/* Function to define how the thrusters should move/turn
   int mod: an integer that modifies the speed of the thrusters.
            Must be greater than 25 if using BlueRobotics T100 Thrusters
   char dir: a character indicating the direction to turn
             'L' for left, 'R' for right
   int wait: an integer to set the delay when not turning */
void moveMotor(int mod, char dir, int wait = 1000) {
  switch (dir) {
    case 'L':
      RTmtr.writeMicroseconds(1500 - mod);
      LTmtr.writeMicroseconds(1500);
      delay(500);
      break;
    case 'R':
      //while(mod > 0){
      RTmtr.writeMicroseconds(1500);
      LTmtr.writeMicroseconds(1500 + mod);
      delay(500);
      break;
    default:
      RTmtr.writeMicroseconds(1500 - mod);
      LTmtr.writeMicroseconds(1500 + mod);
      delay(wait);
  }
}

/* Function to set destination coordinates
   float lat: latitude
   float lon: longitude */
void setDest(float lat, float lon) {
  des_LAT = lat;
  des_LNG = lon;
}

//Function to save the GPS coordinates of where RoboBuoy is launched
void getHome() {
  if (launch) {
    home_LAT = gps.location.lat();
    home_LNG = gps.location.lng();
  }
  launch = false;
}

//Function to set destination to launching point for retrieval
void goHome() {
  des_LAT = home_LAT;
  des_LNG = home_LNG;
}

//Function for the box reed interrrupt
void boxCheck() { //If the box reed reads low (when the magnet is connected) stop motors
  RTmtr.writeMicroseconds(1500);
  LTmtr.writeMicroseconds(1500);
}

//Function to pull  up the Benthic Observatory
void winchUp () {
  reelState = digitalRead(reelSwitch);
  while (reelState == 1) { //If the switch is not connected, keep pulling up
    reelState = digitalRead(reelSwitch);
    winchMtr.writeMicroseconds(1200);
  }
  if (reelState == 0) {
    winchStop(); //Once the switch is connected, stop
  }
}

//Function to drop the Benthic Observatory
void winchDown() {
  for (int i = 0; i < dropLoops; i++) { //Checks the reelSwitch every 200 ms just in case the line becomes tangled while dropping
    reelState = digitalRead(reelSwitch);
    if (reelState == 0) { //If the switch is connected, stop
      winchStop(); 
    } else {
      winchMtr.writeMicroseconds(1700);
    }
    delay(200);
  }
}

//Function to stop the winch
void winchStop () {
  winchMtr.writeMicroseconds(1500);
}

//Function for the winch to hold the Benthic Observatory at the desired depth
void winchPause(int winchWait) {
  winchMtr.writeMicroseconds(1500);
  delay(winchWait);
}

void setup() {
  Serial.begin(ConsoleBaud); //Begin connection with the serial monitor
  gpsSerial.begin(GPSBaud); //Begin software serial connection with GPS
  unoSerial.begin(UnoBaud); //Begin the software serial connection with the Uno

  RTmtr.attach(10); //Attach the servos to the pins
  LTmtr.attach(11);
  winchMtr.attach(30);

  pinMode(boxSwitch, INPUT_PULLUP); //Set the box reed switch as an input
  pinMode(reelSwitch, INPUT_PULLUP); //Set the winch switch as an input
  attachInterrupt(digitalPinToInterrupt(boxSwitch), boxCheck, LOW); //Create the interrupt sequence for the reed switch

  setDest(des_LAT, des_LNG); //Set the destination coordinates (lat, long)

  dropTime = ((depth) * 7000); //The winch takes approx 7 seconds to unspool 1 meter of string, this calculates
  // how long the winch needs to unspool based on the given depth
  dropLoops = ((dropTime) / 200); //The loop needs to cycle every 200 ms for the total amount of time
  winchWait = ((bottomTime) * 1000); //This changes the bottom time from seconds to milliseconds

  //Initialise the Compass
  if (!mag.begin()) { //Compass failed to initialize, check the connections
    Serial.println("Oops, no Compass detected. Check your wiring!");
    while (1);
  }

  //T100 trusters need a stop command to initialize
  RTmtr.writeMicroseconds(1500);
  LTmtr.writeMicroseconds(1500);
  delay(2000);
}

void loop() {
  //Check if manual override has been initiated
  //If yes, use manual override
  //break;

  //Check if signal from Winch has been received
  //If yes, goHome();

  //If any characters have arrived from the GPS,
  //send them to the TinyGPS++ object
  while (gpsSerial.available() > 0) {
    if (gps.encode(gpsSerial.read()));
  }

  //Save initial GPS location
  if (gps.location.isValid()) {
    getHome();
  }

  //Save current lat, lon, and heading
  curr_LAT = gps.location.lat();
  curr_LNG = gps.location.lng();
  heading = compass();


  //Establish our current status
  //These two lines are from the TinyGPS++ example
  distanceToDestination = TinyGPSPlus::distanceBetween(curr_LAT, curr_LNG, des_LAT, des_LNG);
  courseToDestination = TinyGPSPlus::courseTo(curr_LAT, curr_LNG, des_LAT, des_LNG);

  if (debug) {
    Serial.println();
    Serial.println(gps.location.isValid());
    Serial.print("LAT: "); Serial.print(curr_LAT, 6); Serial.print("  LON: "); Serial.println(curr_LNG, 6);
    Serial.print("CURRENT ANGLE: "); Serial.println(heading);
    Serial.print("COURSE TO DEST: "); Serial.println(courseToDestination);
    Serial.print("DISTANCE: "); Serial.print(distanceToDestination);
    Serial.println(" meters to go."); Serial.print("INSTRUCTION: ");
  }

  //Calculate the difference in angle between current heading and a heading that would lead RoboBuoy straight to the destination
  courseChangeNeeded = heading - courseToDestination;

  //If less than 1 meter away from destination, stay put
  if (distanceToDestination <= 1) {
    //When initially true, send signal to winch to lower benthic observatory

    moveMotor(25, courseChange());

    winchDown();
    winchPause(winchWait);
    winchUp();
    goHome();

    if (debug) {
      Serial.print("crawl ");
      Serial.println(courseChange());
    }
  } //If less than 2 meters away, go slow
  else if (distanceToDestination <= 2) {
    moveMotor(50, courseChange());
    moveMotor(50, 'N');

    if (debug) {
      Serial.print("slow ");
      Serial.println(courseChange());
    }
  } //If less than 10 meters away, go fairly fast
  else if (distanceToDestination <= 10) {
    moveMotor(150, courseChange());
    moveMotor(150, 'N');

    if (debug) {
      Serial.print("medium ");
      Serial.println(courseChange());
    }
  } else { //Else, go fast
    moveMotor(200, courseChange());
    moveMotor(200, 'N', 5000);

    if (debug) {
      Serial.print("fast ");
      Serial.println(courseChange());
    }
  }
}
