#include <EEPROM.h>
#include <Wire.h>
#include <FlexiTimer2.h>
#include <DualMC33926MotorShield.h>
#include <CmdMessenger.h>

DualMC33926MotorShield md;

int I2CAddress     = 100;

int Sensor1Pin = 2;
int Sensor2Pin = 3;

int Sensor1Voltage;
int Sensor2Voltage;

boolean RunMotor1;
boolean RunMotor2;

boolean RenewMotor1;
boolean RenewMotor2;

char FieldSeparator = ',';
char CommandSeparator = ';';
CmdMessenger cmdMessenger = CmdMessenger(Serial, FieldSeparator, CommandSeparator);


enum
{
  kCOMM_ERROR    = 000, // Communication error 
  kACK           = 001, // Acknowledgement
  kARDUINO_READY = 002, // Arduino Ready
  kERR           = 003, // Error
  kHWERR         = 004, // Hardware Error
  kSEND_CMDS_END, 
};

messengerCallbackFunction messengerCallbacks[] = 
{
  turn_pump_on,        // 005 
  turn_pump_off,       // 006
  sensor_control,      // 007
  renew_motor_timer,   // 008
  set_i2c_address,     // 009
  get_i2c_address,     // 010
  send_i2c_message,    // 011
  test,                // 012
  NULL
};

void turn_pump_off()
{
  md.setM1Speed(0);
  stopIfFault();
}

void turn_pump_on()
{
  int i = -500;
  char buf[350] = { '\0' };
  cmdMessenger.copyString(buf, 350);
  if(buf[0]){
      int number = atoi( buf );
      md.setM1Speed(number);
      stopIfFault();
  }
}
void sensor_control(){
  
  char buf[350] = { '\0' };
  cmdMessenger.copyString(buf, 350);
  if(buf[0]){
      int number = atoi( buf );
      
      if(number == 0){
        FlexiTimer2::stop();
      } else if ( number == 1){
        FlexiTimer2::start();
      }
  }
  cmdMessenger.sendCmd(kACK,"OK");
}

void renew_motor_timer()
{
  char buf[350] = { '\0' };
  cmdMessenger.copyString(buf, 350);
  if(buf[0]){
      int number = atoi( buf );
      if(number == 1){
        RenewMotor1 = true;
      } else if ( number == 2){
        RenewMotor2 = true;
      }
  }
}
void set_i2c_address(){
  char buf[350] = { '\0' };
  cmdMessenger.copyString(buf, 350);
  if(buf[0]){
      int number = atoi( buf );
      EEPROM.write(I2CAddress,number);
  }
  cmdMessenger.sendCmd(kACK,"Test OK");
}

void get_i2c_address(){
  int address = EEPROM.read(I2CAddress);
  Serial.print("Address: ");
  Serial.println(address);
}

void send_i2c_message(){
  int i2c_address = 0;
  int i = 0;
  while ( cmdMessenger.available() ){
    
    char buf[350] = { '\0' };
    cmdMessenger.copyString(buf, 350);
    if ((i==0) && (buf[0])){
      int number = atoi( buf );
      i2c_address = number;
    } else if ((i>0) && (buf[0])){
      int number = atoi( buf );
      Wire.beginTransmission(i2c_address);
      Wire.write(buf);
      Serial.println(buf);
      Wire.endTransmission();
    }
    i++;
  }
}

void test(){
  cmdMessenger.sendCmd(kACK,"Test OK");
}


void ready()
{
  cmdMessenger.sendCmd(kACK,"Ready");
}

void unknownCmd()
{
  cmdMessenger.sendCmd(kERR,"Unknown command");
}

void attach_callbacks(messengerCallbackFunction* callbacks)
{
  int i = 0;
  int offset = kSEND_CMDS_END;
  while(callbacks[i])
  {
    cmdMessenger.attach(offset+i, callbacks[i]);
    i++;
  }
}

void stopIfFault()
{
  if (md.getFault())
  {
    
    Serial.println(md.getFault());
    cmdMessenger.sendCmd(kHWERR,"Hardware Error");
  } else{
    cmdMessenger.sendCmd(kACK,"OK");
  }
}

void setup()
{
  int address = EEPROM.read(I2CAddress);
  if(address==0){
    Wire.begin();
  } else {
    Wire.begin(address);
  }
  Wire.onRequest(requestEvent);
  Wire.onReceive(receiveEvent);
  
  Serial.begin(115200);
  
  //Serial CMDMessenger
  cmdMessenger.print_LF_CR();
  cmdMessenger.attach(kARDUINO_READY, ready);
  cmdMessenger.attach(unknownCmd);
  
  //Attach the callbacks
  attach_callbacks(messengerCallbacks);
  md.init();
  
  FlexiTimer2::set(1000,run_motor);
  FlexiTimer2::start();
}

void requestEvent()
{
  
}

void receiveEvent(int numBytes)
{
  cmdMessenger.field_separator = '-';
  cmdMessenger.command_separator = '!';
  while(Wire.available())    // slave may send less than requested
  { 
    int c = Wire.read();    // receive a byte as character
    cmdMessenger.process(c);
  }
  cmdMessenger.field_separator = ',';
  cmdMessenger.field_separator = ';';
}

void loop()
{
  cmdMessenger.feedinSerialData();
}

void read_sensor_values(){
  Sensor1Voltage = analogRead(Sensor1Pin);
  Sensor2Voltage = analogRead(Sensor2Pin);
}

void run_motor() {
  
  read_sensor_values();
  
   if(( RenewMotor1 == true) && (Sensor1Voltage < 1000)){
     //Renew Motor 1
     md.setM1Speed(400);
   } else{
     md.setM1Speed(0);
   }
  
   if( (RenewMotor2 == true) && (Sensor2Voltage < 1000)){
    //Renew Motor 2
     md.setM2Speed(400);
    } else {
     md.setM2Speed(0);
   }
  
  
  RenewMotor1 = false ;
  RenewMotor2 = false ;
}

