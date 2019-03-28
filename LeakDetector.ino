/**
 * Code for the Robobuoy leak detector
 * MSCI 437 Spring 2019
 * Created by Kaitlyn Beardshear
 * v1: 3/27/19
 * 
 * returns are so water level can be sent between the ethernet 
 * shields in only one 
 * 
 **/
 
int leakDetector = A0; //Assign pin number to the leak detector
int waterCheck;  //variable used to determine water level
byte waterLevel;  //variable for the water level

void setup() {
  pinMode(leakDetector, INPUT);
  Serial.begin(9600);
}

void loop() {
  waterCheck = 0;
  checkForLeaks();
  Serial.println(waterCheck);
  delay(1000);
}

byte checkForLeaks() {
  waterCheck = analogRead(leakDetector);  //read the water level
  if (waterCheck==0){ 
    Serial.println("No water");
    return (waterLevel = '1'); 
  }
  else if ((waterCheck>1) && (waterCheck<=290)){ 
    Serial.println("Water level: 0mm to 5mm");
    return (waterLevel = '2'); 
  }
  else if ((waterCheck>290) && (waterCheck<=300)){ 
    Serial.println("Water level: 5mm to 10mm");
    return (waterLevel = '3'); 
  }
  else if ((waterCheck>300) && (waterCheck<=350)){ 
    Serial.println("Water level: 10mm to 15mm");
    return (waterLevel = '4'); 
  } 
  else if ((waterCheck>350) && (waterCheck<=370)){ 
    Serial.println("Water level: 15mm to 20mm");
    return (waterLevel = '5'); 
  }
  else if ((waterCheck>370) && (waterCheck<=370)){ 
    Serial.println("Water level: 20mm to 25mm");
    return (waterLevel = '6'); 
  }
  else if ((waterCheck>370) && (waterCheck<=380)){ 
    Serial.println("Water level: 25mm to 30mm");
    return (waterLevel = '7'); 
  }
  else if (waterCheck>380){ 
    Serial.println("Water level: over 30mm");
    return (waterLevel = '8'); 
  } 
}
