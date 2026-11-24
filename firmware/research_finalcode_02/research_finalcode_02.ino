#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

int setpoint;

int ambient=30;

double kp = 55;  
double ki = 0.6;  
double kd = 2.5; 

double input, output, error, lastError = 0;
double integral = 0;

const int mosfetpin1 = 5 ;
const int mosfetpin2 = 6 ;
const int mosfetpin3 = 7 ;
const int mosfetpin4 = 8 ;
//const int buttonPin = 7; 

/*int buttonState = HIGH; 
int lastButtonState = HIGH; 
unsigned long lastDebounceTime = 0; 
unsigned long debounceDelay = 50;*/

double emergencyShutdownTemperature = 120;

float A=0;
int C=0;
float t=0;
int x=0;

int ThermistorPin = A0;
int Vo;
float R1 = 10000;
float logR2, R2, T;
float c1 = 1.009249522e-03, c2 = 2.378405444e-04, c3 = 2.019202697e-07;

int AmbientPin = A1;
int V1;
float R3 = 10000;
float logR4, R4, T1;
float c4 = 1.009249522e-03, c5 = 2.378405444e-04, c6 = 2.019202697e-07;

void setup() {
Serial.begin(9600);

//pinMode(buttonPin,INPUT);
pinMode(mosfetpin1, OUTPUT);
pinMode(mosfetpin2, OUTPUT);
pinMode(mosfetpin3, OUTPUT);
pinMode(mosfetpin4, OUTPUT);
analogWrite(mosfetpin2, 0);
digitalWrite(mosfetpin3, HIGH);
digitalWrite(mosfetpin4, HIGH);
analogWrite(mosfetpin1, 0);

 lcd.init();
  lcd.backlight();

  lcd.begin(16, 2);          
  
  lcd.setCursor(4,0);   
  lcd.print("PELTIER");
  delay(1000);
  lcd.setCursor(5,1);
  lcd.print("BASED");
  delay(1000);

  lcd.clear();
  lcd.setCursor(2,0);   
  lcd.print("TEMPERATURE");
  delay(1000);
  lcd.setCursor(3,1);
  lcd.print("REGULATOR");
  delay(1000);

  lcd.clear();
  lcd.setCursor(0,0);   
  lcd.print("ENTER SETPOINT:");

Serial.println("Enter the desired temperature setpoint:");
  while (!Serial.available()) {   
    
}
  setpoint = Serial.parseFloat();
  lcd.clear();
  lcd.setCursor(0,0);   
  lcd.print("ENTER SETPOINT:");
  lcd.setCursor(7,1);   
  lcd.print(setpoint);
  delay(2000);
  }

  

void loop() {

  /*int reading = digitalRead(buttonPin);

  if (reading==0) {
    resetToBeginning();
  }*/

  Vo = analogRead(ThermistorPin);
  R2 = R1 * (1023.0 / (float)Vo - 1.0);
  logR2 = log(R2);
  T = (1.0 / (c1 + c2*logR2 + c3*logR2*logR2*logR2));
  T = T - 273.15;

  Vo = analogRead(AmbientPin);
  R4 = R3 * (1023.0 / (float)V1 - 1.0);
  logR4 = log(R4);
  T1 = (1.0 / (c1 + c2*logR2 + c3*logR2*logR2*logR2));
  T1 = T1 - 273.15;
  
  C=C+1;
  A=A+T;

  if (C==10) {
    A=A/C;
    C=0;
    Serial.print(setpoint);
    Serial.print(",");
      Serial.println(A);

    input=A;
  /*Serial.print(t); 
  Serial.print(",");
  Serial.println(A);*/
  //Serial.print(setpoint); 
  //Serial.print(",");
  //Serial.println(A);

  lcd.clear();
  lcd.setCursor(0,0);   
  lcd.print("SET:");

  lcd.setCursor(4,0);
  lcd.print(setpoint);

  lcd.setCursor(8,0);
  lcd.print(" ");
  
  lcd.setCursor(0,1);   
  lcd.print("TEMP:");

  lcd.setCursor(5,1);   
  lcd.print(A);

  lcd.setCursor(11,1);
  lcd.print("P:");

  
  /*if (reading==1) {
    reading=HIGH;
  }
  if (reading==0) {
    reading=LOW;
  }*/

  /*if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }*/

  /*if ((millis() - lastDebounceTime) > debounceDelay) {

    if (reading != buttonState) {
      buttonState = reading;

      if (buttonState == LOW) {
        
        resetToBeginning();
      }
    }
  }*/


  if(input > emergencyShutdownTemperature){

    analogWrite(mosfetpin1,0);
    analogWrite(mosfetpin2,0);
    digitalWrite(mosfetpin3, LOW);
    digitalWrite(mosfetpin4, HIGH);
    lcd.clear();
    lcd.setCursor(4,0);   
    lcd.print("EMERGENCY");
    lcd.setCursor(4,1);   
    lcd.print("SHUTDOWN!");
    Serial.println("Emergency Shutdown!Temperature exceeded 120 degree Celsiuc.");
  }
  if (ambient>setpoint){

    lcd.setCursor(8,0);
    lcd.print(" Cooling");

    error = A-setpoint;

    integral += error;
    if (integral > 100) {
    integral = 100;
    } else if (integral < -100) {
    integral = -100;
    }

  
    double derivative = error - lastError;

  
    output = kp * error + ki * integral + kd * derivative;

  
    output = map(output, -100, 100, 0, 255);
    if (output < 0) output = 0;
    if (output > 255) output = 255;

    analogWrite(mosfetpin2, 0);
    digitalWrite(mosfetpin3, HIGH);
    digitalWrite(mosfetpin4, LOW);
    analogWrite(mosfetpin1, output);

    lcd.setCursor(13,1);
    lcd.print(output);

  lastError = error;
  }
  if (ambient<setpoint){

    lcd.setCursor(8,0);
    lcd.print(" Heating");

    error = setpoint-A;

    integral += error;
    if (integral > 100) {
    integral = 100;
    } else if (integral < -100) {
    integral = -100;
    }

  
    double derivative = error - lastError;

  
    output = kp * error + ki * integral + kd * derivative;

  
    output = map(output, -100, 100, 0, 255);
    if (output < 0) output = 0;
    if (output > 255) output = 255;

    analogWrite(mosfetpin1, 0);
    digitalWrite(mosfetpin4,HIGH);
    digitalWrite(mosfetpin3, LOW);
    analogWrite(mosfetpin2, output);

    lcd.setCursor(13,1);
    lcd.print(output);

    lastError = error;
  }
  

  A=0;
  }

}
/*void resetToBeginning() {

  analogWrite(mosfetpin1,0);
  analogWrite(mosfetpin2,0);
  digitalWrite(mosfetpin3, LOW);
  digitalWrite(mosfetpin4, LOW);

  lcd.clear();
  lcd.setCursor(0,0);   
  lcd.print("ENTER SETPOINT:");
  Serial.println("Enter a new setpoint");
  while (!Serial.available()) {
    
  }
  setpoint = Serial.parseFloat();
  integral = 0;
  lastError = 0;
}*/