//Temperature sensor libraries
#include <OneWire.h> //Temp sensor
#include <DallasTemperature.h> //Temp sensor

//Display libraries
//#include <SPI.h>
//#include <Adafruit_GFX.h>
//#include <Adafruit_PCD8544.h>

//Define PIN numbers
#define TEMP 2
#define HEATER 3
#define MOTOR 4
#define BUTTON 5

// Declare LCD object for software SPI
// Adafruit_PCD8544(CLK,DIN,D/C,CE,RST);
//Adafruit_PCD8544 display = Adafruit_PCD8544(9, 10, 11, 12, 13);

//Create objects for reading from temperature sensor
OneWire oneWire(TEMP); //Connect a wire object to the sensor
DallasTemperature temp_sensor(&oneWire); //Connect a DallasTemperature object to read from the sensor

//Stores current temperature
float current_temperature = 20;

//Store target temperatures
const int low_temp_bound = 50;
const int high_temp_bound = 55;

//Stores details about the current state
#define MENU 0
#define CYCLE_RUN 1
#define CYCLE_PAUSE 2
int state = MENU;

//Stores milis of cycle beginning, 
unsigned long cycle_timestamp;
//Stores total time elapsed in the cycle
unsigned long cycle_total_elapsed = 0;
//Stores time elapsed in cycle since last unpause
unsigned long cycle_last_elapsed = 0;

//Total cycle duration millis
const long cycle_duration = 30 * 1000;

//Stores button state
boolean button_flag = false;
long pressed_timestamp;

//Amount of milliseconds before a short press becomes a long press
const int button_threshold = 3000;

void setup() {  

  //Set pin modes
  pinMode(HEATER, OUTPUT);
  pinMode(MOTOR, OUTPUT);
  pinMode(BUTTON, INPUT);
  digitalWrite(HEATER, LOW);
  digitalWrite(MOTOR, LOW);

  Serial.begin(9600);

  //Initialize sensor
  temp_sensor.begin();

}

void loop() {

  //Output current time to serial monitor
  Serial.println("-------------------------------------------------");
  Serial.print("Time: ");
  Serial.println(millis());

  //Update and output current temperature to serial monitor
  //current_temperature = read_temp();
  Serial.print("Current temperature:" );
  Serial.println(current_temperature);

  //Loop functions
  loop_cycle();
  loop_button();
  loop_heater();
  loop_mock_heating();

  delay(500);

}

//Check the states of the button, and use changes of button state to update the programs state
void loop_button(){

  //Check pin and change state if button is pressed
  if(digitalRead(BUTTON) == HIGH){

    //Update timestamp only if button flag hasn't been changed yet
    if(!button_flag){
      
      pressed_timestamp = millis();
      
      }

    button_flag = true;

    //If button is not pressed
    }else{

      //Executes button function only when flag has been changed
      if(button_flag){

          //Executes functions based on if the button has been short or long pressed
          if(millis() - pressed_timestamp < button_threshold){

            Serial.println("Button short pressed");

            //On short press, either start cycle, pause cycle, or unpause cycle
            switch (state){
              
              case MENU:
                //Start cycle, save timestamp and restart total timer
                Serial.println("Starting cycle");
                state = CYCLE_RUN;
                cycle_timestamp = millis();
                cycle_total_elapsed = 0;
                break;

              case CYCLE_RUN:
                //Pause cycle, add elapsed time to timer
                Serial.println("Pausing cycle");
                state = CYCLE_PAUSE;
                deactivate_all();
                cycle_total_elapsed += cycle_last_elapsed;
                break;

              case CYCLE_PAUSE:
                //Unpause cycle
                Serial.println("Unpausing cycle");
                state = CYCLE_RUN;
                cycle_timestamp = millis();
                cycle_last_elapsed = 0;
                break;
              
              }         
            
            }else{

              Serial.println("Button long pressed");

              //On long press, return to menu
              Serial.println("Returning to menu");
              state = MENU;
              deactivate_all();
              
              }
        
        }

      //Update flag
      button_flag = false;
      
      }
  
  
  }

//Simulate the heater heating up the air
void loop_mock_heating(){

  if(digitalRead(HEATER) == HIGH){

    current_temperature++;
    
    }else{
      
    if (current_temperature > 20) current_temperature--;
      
      }
  
  }

//Activate heater only if the cycle is running and temperature is within rangee
void loop_heater(){

  if(state == CYCLE_RUN){
    
    //Activate or deactivate the heater depending on the temperature range
    if(current_temperature <= low_temp_bound){
      
      activate_heater();
      
      }else if(current_temperature >= high_temp_bound){
  
        deactivate_heater();
        
        }

    }else{

      deactivate_heater();
      
      }
  
  }

//Check and update cycle information
void loop_cycle(){

    //If cycle is running
    if(state == CYCLE_RUN){

      //Update timestamp
      cycle_last_elapsed = millis() - cycle_timestamp;
  
      //Update serial monitor
      Serial.print("Total time elapsed in cycle: ");
      Serial.print(cycle_total_elapsed);
      Serial.print(", Total time elapsed since last unpause: ");
      Serial.print(cycle_last_elapsed);
      Serial.print(", Time remaining in cycle: ");
      Serial.println(cycle_duration - cycle_total_elapsed - cycle_last_elapsed);
  
  
      if((cycle_total_elapsed + cycle_last_elapsed) >= cycle_duration){
  
          Serial.println("Cycle complete");
          state = MENU;
          deactivate_all();
        
        }

    }

  }

//Activates heater and updates the serial monitor
void activate_heater(){

  if(digitalRead(HEATER) == LOW){
      
    digitalWrite(HEATER, HIGH);
    Serial.println("Heater Activated");
    
    }
  
  }

//Deactivates heater and updates the serial monitor
void deactivate_heater(){

  if(digitalRead(HEATER) == HIGH){
    
    digitalWrite(HEATER, LOW);
    Serial.println("Heater Deactivated");
    
    }
  
  }

//Deactivates all components and updates the serial monitor
void deactivate_all(){
  
    deactivate_heater();
    //deactivate_motor();
  
  }

//Returns the floating point temperature value
float read_temp(){

    //Get temperature from the sensor object
    temp_sensor.requestTemperatures();
    float temp = temp_sensor.getTempCByIndex(0);

    //Sometimes the sensor gives a bad value of -127, so loop call the sensor to prevent this
    while(temp < -100){
      delay(500);
      temp = temp_sensor.getTempCByIndex(0);
      }

    return temp;
    
  }
