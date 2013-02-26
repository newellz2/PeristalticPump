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
  kCOMM_ERROR    = 000, // Communication error 
  kACK           = 001, // Acknowledgement
  kARDUINO_READY = 002, // Arduino Ready
  kERR           = 003, // Error
  kHWERR         = 004, // Hardware Error
  kSEND_CMDS_END, 
};

messengerCallbackFunction messengerCallbacks[] = 
{
  turn_pump_on,    // 006 
  turn_pump_off,   // 007
  get_current,     // 008
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

void get_current()
{
  int current = md.getM1CurrentMilliamps();
  stopIfFault();
}


void arduino_ready()
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
  Serial.begin(115200);
  cmdMessenger.print_LF_CR();
  cmdMessenger.attach(kARDUINO_READY, arduino_ready);
  cmdMessenger.attach(unknownCmd);


  attach_callbacks(messengerCallbacks);

  md.init();
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

  // handle timeout function
  if (  millis() - previousMillis > timeoutInterval )
  {
    timeout();
    previousMillis = millis();
  }
}
