#include <FlexiTimer2.h>
#include <DualMC33926MotorShield.h>
#include <CmdMessenger.h>
#include <Base64.h>
#include <Streaming.h>

DualMC33926MotorShield md;


char field_separator = ',';
char command_separator = ';';
CmdMessenger cmdMessenger = CmdMessenger(Serial, field_separator, command_separator);


enum
{
  kCOMM_ERROR    = 000, // Lets Arduino report serial port comm error back to the PC (only works for some comm errors)
  kACK           = 001, // Arduino acknowledges cmd was received
  kARDUINO_READY = 002, // After opening the comm port, send this cmd 02 from PC to check arduino is ready
  kERR           = 003, // Arduino reports badly formatted cmd, or cmd not recognised
  kHWERR         = 004, // Arduino reports badly formatted cmd, or cmd not recognised
  kSEND_CMDS_END, // Mustnt delete this line
};

messengerCallbackFunction messengerCallbacks[] = 
{
  turn_pump_on,    // 005 in this example
  turn_pump_off,   // 006
  get_current,     // 007
  NULL
};

void turn_pump_off()
{
  // Message data is any ASCII bytes (0-255 value). But can't contain the field
  // separator, command separator chars you decide (eg ',' and ';')
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

void get_current()
{

  int current = md.getM1CurrentMilliamps();
  stopIfFault();
}


void arduino_ready()
{
  // In response to ping. We just send a throw-away Acknowledgement to say "im alive"
  cmdMessenger.sendCmd(kACK,"Arduino ready");
}

void unknownCmd()
{
  // Default response for unknown commands and corrupt messages
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
  Serial.begin(115200); // Arduino Uno, Mega, with AT8u2 USB

  // cmdMessenger.discard_LF_CR(); // Useful if your terminal appends CR/LF, and you wish to remove them
  cmdMessenger.print_LF_CR();   // Make output more readable whilst debugging in Arduino Serial Monitor
  
  // Attach default / generic callback methods
  cmdMessenger.attach(kARDUINO_READY, arduino_ready);
  cmdMessenger.attach(unknownCmd);

  // Attach my application's user-defined callback methods
  attach_callbacks(messengerCallbacks);

  md.init();

  FlexiTimer2::set(1000, 1.0/1000, flash);
  FlexiTimer2::start();
}
void flash()
{
	cmdMessenger.sendCmd(kACK,"Timer2");
}

// Timeout handling
long timeoutInterval = 2000; // 2 seconds
long previousMillis = 0;
int counter = 0;

void timeout()
{
  // blink
  if (counter % 2)
    digitalWrite(13, HIGH);
  else
    digitalWrite(13, LOW);
  counter ++;
}  


void loop()
{
  cmdMessenger.feedinSerialData();

  // handle timeout function, if any
  if (  millis() - previousMillis > timeoutInterval )
  {
    timeout();
    previousMillis = millis();
  }
}
