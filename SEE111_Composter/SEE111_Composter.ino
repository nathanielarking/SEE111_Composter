//Temperature sensor libraries
#include <OneWire.h> //Temp sensor
#include <DallasTemperature.h> //Temp sensor

//Display libraries
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>

//Define PIN numbers
#define TEMP_PIN 2
#define HEATING_PIN 4
#define MOTOR_PIN 3

// Declare LCD object for software SPI
// Adafruit_PCD8544(CLK,DIN,D/C,CE,RST);
//Adafruit_PCD8544 display = Adafruit_PCD8544(9, 10, 11, 12, 13);

//Create objects for reading from temperature sensor
OneWire oneWire(TEMP_PIN); //Connect a wire object to the sensor
DallasTemperature temp_sensor(&oneWire); //Connect a DallasTemperature object to read from the sensor

void setup() {  

  //Set pin modes
  pinMode(HEATING_PIN, OUTPUT);
  pinMode(MOTOR_PIN, OUTPUT);

  digitalWrite(HEATING_PIN, LOW);
  digitalWrite(MOTOR_PIN, LOW);

  Serial.begin(9600);

  //Initialize sensor
  temp_sensor.begin();

  digitalWrite(HEATING_PIN, HIGH);

  while(true){
  Serial.print("Temperature: ");
  Serial.println(read_temp());
  delay(1000);
  }
  

}

void loop() {
  // put your main code here, to run repeatedly:

}

//Turns on heater for 15 seconds
void test_heater(){

  Serial.println("Activating heater");
  digitalWrite(HEATING_PIN, HIGH);
  delay(45000);
  Serial.println("Deactivating heater");
  digitalWrite(HEATING_PIN, LOW);

  Serial.println();
  
  }

//Pulses motor
void test_motor(){

    Serial.println("Activating motor");  
    digitalWrite(MOTOR_PIN, HIGH);
    delay(1000);
    Serial.println("Deactivating motor");
    digitalWrite(MOTOR_PIN, LOW);  
  
  }

void test_sensor(){
  
  for(int i = 0; i < 5; i++){

    float temp = read_temp();
    
    Serial.print("Reading temperature value: ");
    Serial.println(temp);
    delay(1000);
    
    }

    Serial.println();
  
  }

void test_speaker();

//Returns the floating point temperature value
float read_temp(){
  
    temp_sensor.requestTemperatures();
    float temp = temp_sensor.getTempCByIndex(0);

    //Sometimes the sensor gives a bad value of -127, so loop call the sensor to prevent this
    while(temp < -100){
      delay(500);
      temp = temp_sensor.getTempCByIndex(0);
      }

    return temp;
    
  }
