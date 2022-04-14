//Temperature sensor libraries
#include <OneWire.h>
#include <DallasTemperature.h>

//Display libraries
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>

//Define PIN numbers
#define TEMP 2
#define HEATER 4
#define MOTOR 3
#define BUTTON 6
#define BUZZER 7

// Declare LCD object for software SPI
//Adafruit_PCD8544(CLK,DIN,D/C,CE,RST);
Adafruit_PCD8544 display = Adafruit_PCD8544(13, 11, 9, 10, 8);

//Create objects for reading from temperature sensor
OneWire oneWire(TEMP); //Connect a wire object to the sensor
DallasTemperature temp_sensor(&oneWire); //Connect a DallasTemperature object to read from the sensor

//Stores current temperature
float current_temperature;

//Store target temperatures
const int low_temp_bound = 50;
const int high_temp_bound = 55;

//Stores details about the current state/
#define MENU 0
#define CYCLE_RUN 1
#define CYCLE_PAUSE 2
int state = MENU;

//Stores milis of cycle beginning, 
unsigned long int cycle_timestamp;
//Stores total time elapsed in the cycle
unsigned long int cycle_total_elapsed = 0;
//Stores time elapsed in cycle since last unpause
unsigned long int cycle_last_elapsed = 0;

//Total cycle duration millis
const unsigned long int cycle_duration = 120000;
//Interval of motor activation
const unsigned long int motor_interval = 30000;
//Amount of time to wait in the beginning of the cycle before starting the heater
const unsigned long int cycle_start_duration = 7500;
//Stores a flag that allows us to check whether a tick in the cycle is the first tick
boolean cycle_flag = false;

//Stores button state
boolean button_flag = false;
unsigned long int pressed_timestamp;

//Stores heater state
boolean heater_flag = false;

//Stores whether or not the motor is activated
boolean motor_flag = false;
//Stores whether or not the motor is currently in a pulse
boolean motor_pulse_flag = false;
//Stores the time that the motor was activated
unsigned long int motor_timestamp;
//Stores the width, gaps, and amount of pulses for current motor activation
int motor_pulse_width;
int motor_pulse_gap;
int motor_pulse_count;
//Stores the timestamp of the next motor activation phase
unsigned long int motor_next_activation;

//Stores whether or not the buzzer is activated
boolean buzzer_flag = false;
//Stores whether or not the buzzer is currently in a pulse
boolean buzzer_pulse_flag = false;
//Stores the time that the buzzer was activated
unsigned long int buzzer_timestamp;
//Stores the width, gaps, amount of pulses, and strength of buzzer for current buzzer activation
int buzzer_pulse_width;
int buzzer_pulse_gap;
int buzzer_pulse_count;
int buzzer_pulse_strength;

//Amount of milliseconds before a short press becomes a long press
const int button_threshold = 3000;

void setup() {  

  //Set pin modes
  pinMode(HEATER, OUTPUT);
  pinMode(MOTOR, OUTPUT);
  pinMode(BUTTON, INPUT);
  pinMode(BUZZER, OUTPUT);
  digitalWrite(HEATER, LOW);
  digitalWrite(MOTOR, LOW);
  digitalWrite(BUZZER, LOW);

  Serial.begin(9600);

  //Initialize sensor
  temp_sensor.begin();

  //Initialize display
  display.begin();
  display.setContrast(57 );
  display.display(); // show splashscreen
  delay(2000);
  display.clearDisplay();   // clears the screen and buffer
  
}

void loop() {

  //Output current time to serial monitor
  Serial.println("-------------------------------------------------");
  Serial.print("Time: ");
  Serial.println(millis());

  //Update and output current temperature to serial monitor
  current_temperature = read_temp();
  Serial.print("Current temperature:" );
  Serial.println(current_temperature);

  //Loop functions
  loop_cycle();
  loop_button();
  loop_heater();
  loop_motor();
  loop_buzzer();
  loop_display();

  delay(10);

}

//Check and update cycle information
void loop_cycle(){

    //If cycle is running
    if(state == CYCLE_RUN){

      //Update timestamp
      cycle_last_elapsed = millis() - cycle_timestamp;

      //Close cycle if it's completed
      if((cycle_total_elapsed + cycle_last_elapsed) >= cycle_duration){
  
          Serial.println("Cycle complete");
          state = MENU;
          cycle_flag = false;
          activate_buzzer(150, 75, 3, 100);
          deactivate_all();
        
        }

      //If the cycle is beginning
      if((cycle_total_elapsed + cycle_last_elapsed) <= cycle_start_duration){
        
        heater_flag = false;

        //If this is the first tick in the cycle
        if(!cycle_flag){

          //Do the initial blending
          activate_motor(cycle_start_duration / 2, 1000, 2);
          cycle_flag = true;

          //Set first motor timestamp
          motor_next_activation = cycle_start_duration + motor_interval;

          activate_buzzer(750, 250, 2, 50);
          
          }

        //If the cycle is not in the beginning
        }else{
          
          heater_flag = true;

          //Activate motor if the next time for activation has passed
          if((cycle_last_elapsed + cycle_total_elapsed) >= motor_next_activation){
            
            activate_motor(2000, 1000, 5);
            motor_next_activation = cycle_last_elapsed + cycle_total_elapsed + motor_interval;
            
            }
          
          }
  
      //Update serial monitor
      Serial.print("Total time elapsed in cycle: ");
      Serial.print(cycle_total_elapsed);
      Serial.print(", Total time elapsed since last unpause: ");
      Serial.print(cycle_last_elapsed);
      Serial.print(", Time remaining in cycle: ");
      Serial.println(cycle_duration - cycle_total_elapsed - cycle_last_elapsed);

    }else{
      
      deactivate_all();
      
      }

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
                //Start cycle, save timestamp, restart total timer
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
                activate_buzzer(75, 100, 1, 50);
                break;

              case CYCLE_PAUSE:
                //Unpause cycle
                Serial.println("Unpausing cycle");
                state = CYCLE_RUN;
                cycle_timestamp = millis();
                cycle_last_elapsed = 0;
                activate_buzzer(75, 100, 1, 50);
                break;
              
              }         
            
            }else{

              Serial.println("Button long pressed");

              //On long press, return to menu
              Serial.println("Returning to menu");
              state = MENU;
              activate_buzzer(250, 50, 5, 50);
              deactivate_all();
              
              }
        
        }

      //Update flag
      button_flag = false;
      
      }
  
  
  }

//Activate heater only if the cycle is running and temperature is within rangee
void loop_heater(){

  if(heater_flag){
    
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

//Turns on/off the motor based on the motor state, stored in global variables
void loop_motor(){

  //If the motor has been activated
  if(motor_flag){

    //If we're in the middle of a pulse
    if(millis() - motor_timestamp <= motor_pulse_width){

      //If the flag is set to false, this is the first tick in the pulse, so activate the motor
      if(!motor_pulse_flag){
        
        digitalWrite(MOTOR, HIGH);
        Serial.println("Motor pulse on");
        motor_pulse_flag = true;
        
        }

      //If we're in the gap between pulses
      }else if((millis() - motor_timestamp > motor_pulse_width) && (millis() - motor_timestamp < motor_pulse_width + motor_pulse_gap)){
        
        //If the flag is set to true, this is the first tick in the gap, so deactivate the motor
        if(motor_pulse_flag){
          
          digitalWrite(MOTOR, LOW);
          Serial.println("Motor pulse off");
          motor_pulse_flag = false;
          
          }

        //If we're at the end of a gap
        }else if(millis() - motor_timestamp >= motor_pulse_width + motor_pulse_gap){
          
          //Update motor timestamp, update count variable
          motor_timestamp = millis();
          motor_pulse_count--;
      
          //Stop the pulses if the pulse count has been reached
          if(motor_pulse_count <= 0){

            motor_flag = false;
            
            }
          
          }
    
    //If the motor is not currently activated in the global variables, make sure it's turned off
    }else{
      
      deactivate_motor();
      
      }
  
  }

//Changes the global variables of the motor based on input
void activate_motor(int width, int gap, int count){

  //Change global variables
  motor_pulse_width = width;
  motor_pulse_gap = gap;
  motor_pulse_count = count;
  motor_flag = true;
  motor_timestamp = millis();

  //Update serial monitor
  Serial.print("Motor activated, pulsing ");
  Serial.print(count);
  Serial.print(" times for ");
  Serial.print(width);
  Serial.print("ms at ");
  Serial.print(gap);
  Serial.println("ms apart");
  
  }

//Turns on/off the buzzer based on the buzzer state, stored in global variables
void loop_buzzer(){

  //If the motor has been activated
  if(buzzer_flag){

    //If we're in the middle of a pulse
    if(millis() - buzzer_timestamp <= buzzer_pulse_width){

      //If the flag is set to false, this is the first tick in the pulse, so activate the buzzer
      if(!buzzer_pulse_flag){
        
        analogWrite(BUZZER, buzzer_pulse_strength);
        Serial.println("Buzzer pulse on");
        buzzer_pulse_flag = true;
        
        }

      //If we're in the gap between pulses
      }else if((millis() - buzzer_timestamp > buzzer_pulse_width) && (millis() - buzzer_timestamp < buzzer_pulse_width + buzzer_pulse_gap)){
        
        //If the flag is set to true, this is the first tick in the gap, so deactivate the buzzer
        if(buzzer_pulse_flag){
          
          digitalWrite(BUZZER, LOW);
          Serial.println("Buzzer pulse off");
          buzzer_pulse_flag = false;
          
          }

        //If we're at the end of a gap
        }else if(millis() - buzzer_timestamp >= buzzer_pulse_width + buzzer_pulse_gap){
          
          //Update buzzer timestamp, update count variable
          buzzer_timestamp = millis();
          buzzer_pulse_count--;
      
          //Stop the pulses if the pulse count has been reached
          if(buzzer_pulse_count <= 0){

            buzzer_flag = false;
            
            }
          
          }
    
    //If the buzzer is not currently activated in the global variables, make sure it's turned off
    }else{
      
      deactivate_buzzer();
      
      }
  
  }

//Changes the global variables of the buzzer based on input
void activate_buzzer(int width, int gap, int count, int strength){

  //Change global variables
  buzzer_pulse_width = width;
  buzzer_pulse_gap = gap;
  buzzer_pulse_count = count;
  buzzer_pulse_strength = strength;
  buzzer_flag = true;
  buzzer_timestamp = millis();

  //Update serial monitor
  Serial.print("Buzzer activated, pulsing ");
  Serial.print(count);
  Serial.print(" times for ");
  Serial.print(width);
  Serial.print("ms at ");
  Serial.print(gap);
  Serial.println("ms apart");
  
  }

void loop_display(){

  if(state == MENU){
    
    display.clearDisplay();
    display.println("COMPOSTER TOM");
    display.println("-------------");
    display.print("Temp(C): ");
    display.println(current_temperature, 1); // one decimal place
    display.println("Press to start cycle");
    display.display();
    
    }else{
      
      unsigned long currentMillis = cycle_duration - (cycle_total_elapsed + cycle_last_elapsed);
      unsigned long seconds = currentMillis / 1000;
      unsigned long minutes = seconds / 60;
      unsigned long hours = minutes / 60;
      unsigned long days = hours / 24;
      currentMillis %= 1000;
      seconds %= 60;
      minutes %= 60;
      hours %= 24;
    
      display.clearDisplay();
      display.println("COMPOSTER TOM");
      display.println("-------------");
      display.print("Temp(C): ");
      display.println(current_temperature, 1); // one decimal place
      display.print("Time: ");
      if(hours > 0){
        display.print(hours);
        display.print(":");
        }
      if(minutes > 0){
        if(minutes < 10){
          display.print("0");
          }
        display.print(minutes);
        display.print(":");
        }
      if(seconds > 0){
        if(seconds < 10){
          display.print("0");
          }
        display.println(seconds); 
      }
      display.display();
      
      }
  
  }

//Stops motor and updates serial monitor
void deactivate_motor(){

  if(digitalRead(MOTOR) == HIGH){

    digitalWrite(MOTOR, LOW);
    Serial.println("Motor Deactivated");
    
    }

  motor_flag = false;
  
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

  heater_flag = false;
  
  }

//Stops buzzer and updates serial monitor
void deactivate_buzzer(){

  if(digitalRead(BUZZER) == HIGH){

    digitalWrite(BUZZER, LOW);
    Serial.println("Buzzer Deactivated");
    
    }

  buzzer_flag = false;
  
  }

//Deactivates all components and sets flags
void deactivate_all(){
  
    deactivate_heater();
    deactivate_motor();
    deactivate_buzzer();
  
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
