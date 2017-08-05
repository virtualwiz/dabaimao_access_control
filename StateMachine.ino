#include <Servo.h>

#define SERVO_ANG_LOCK 55
#define SERVO_ANG_UNLOCK 125

#define DEBUG_MODE 1

#define IDLE 0
#define	STANDBY	1
#define	PW1	2
#define	PW2	3
#define	COMPARE	4
#define	ERROR	5
#define	RELEASE	6
#define	LOCK	7
#define	ALARM	8
#define	KEEP	9

#define GPIO_RemoteKey_A 4
#define GPIO_RemoteKey_B 5
#define GPIO_RemoteKey_C 6
#define GPIO_RemoteKey_D 7

#define GPIO_LocalKey 2

#define GPIO_Buzzer 8
#define GPIO_Servo A0

#define DATAOUT     11   //mosi
#define DATAIN      12   //miso
#define SPICLOCK    13   //sck
#define SLAVESELECT 10   //ss

#define PU          0x01
#define STOP        0x02
#define RESET       0x03
#define CLR_INT     0x04
#define RD_STATUS   0x05
#define RD_PLAY_PTR 0x06
#define PD          0x07
#define RD_REC_PTR  0x08
#define DEVID       0x09
#define PLAY        0x40
#define REC         0x41
#define ERASE       0x42
#define G_ERASE     0x43
#define RD_APC      0x44
#define WR_APC1     0x45
#define WR_APC2     0x65
#define WR_NVCFG    0x46
#define LD_NVCFG    0x47
#define FWD         0x48
#define CHK_MEM     0x49
#define EXTCLK      0x4A
#define SET_PLAY    0x49
#define SET_REC     0x81
#define SET_ERASE   0x82

Servo DoorLatch;

unsigned char pw_buff[3]={0,0,0};
unsigned char err_count = 0;

void GPIO_Init()
{
  pinMode(GPIO_RemoteKey_A,INPUT);
  pinMode(GPIO_RemoteKey_B,INPUT);
  pinMode(GPIO_RemoteKey_C,INPUT);
  pinMode(GPIO_RemoteKey_D,INPUT);
  
  pinMode(GPIO_Buzzer,OUTPUT);
  digitalWrite(GPIO_Buzzer,HIGH);
  pinMode(GPIO_Servo,OUTPUT);
  pinMode(DATAOUT, OUTPUT);
  pinMode(DATAIN, INPUT);
  pinMode(SPICLOCK,OUTPUT);
  pinMode(SLAVESELECT,OUTPUT);
}

char spi_transfer(volatile char data)
{
 SPDR = data;                    // Start the transmission
 while (!(SPSR & (1<<SPIF)))     // Wait for the end of the transmission
 {
 };
 return SPDR;                    // return the received byte
}

void ready_wait(){

 byte byte1;
 byte byte2;
 byte byte3;

 while(byte3<<7 != 128){
 
   digitalWrite(SLAVESELECT,LOW);
   byte1 = spi_transfer(RD_STATUS); // clear interupt and eom bit
   byte2 = spi_transfer(0x00); // data byte
   byte3 = spi_transfer(0x00); // data byte
   digitalWrite(SLAVESELECT,HIGH);
 }
 delay(100);
}

void SPI_SendCmd_Reset()
{
  digitalWrite(SLAVESELECT,LOW);
  spi_transfer(RESET); // clear interupt and eom bit
  spi_transfer(0x00); // data byte
  digitalWrite(SLAVESELECT,HIGH);
  delay(300);
  digitalWrite(SLAVESELECT,LOW);
  spi_transfer(PU); // power up
  spi_transfer(0x00); // data byte
  digitalWrite(SLAVESELECT,HIGH);
  delay(100);  

  digitalWrite(SLAVESELECT,LOW);
  spi_transfer(CLR_INT); // clear interupt and eom bit
  spi_transfer(0x00); // data byte
  digitalWrite(SLAVESELECT,HIGH);
  delay(100);
 }

void SPI_SendCmd_Play()
{
 digitalWrite(SLAVESELECT,LOW);
 spi_transfer(PLAY); // clear interupt and eom bit
 spi_transfer(0x00); // data byte
 digitalWrite(SLAVESELECT,HIGH);
 ready_wait();
}

void SPI_SendCmd_FastForward(unsigned char seg)
{
 for(unsigned char i=1;i<=seg;i++)
 {
 digitalWrite(SLAVESELECT,LOW);
 spi_transfer(FWD); // clear interupt and eom bit
 spi_transfer(0x00); // data byte
 digitalWrite(SLAVESELECT,HIGH);
 delay(200);
 }
}

void SPI_SendCmd_PlayVoiceNumber(unsigned char seg)
{
  SPI_SendCmd_Reset();
  SPI_SendCmd_FastForward(seg);
  SPI_SendCmd_Play();
}

void Servo_UnlockLatch()
{
  DoorLatch.attach(GPIO_Servo);
  digitalWrite(GPIO_Buzzer,LOW);
  delay(60);
  digitalWrite(GPIO_Buzzer,HIGH);
  delay(60);
  digitalWrite(GPIO_Buzzer,LOW);
  delay(60);
  digitalWrite(GPIO_Buzzer,HIGH);
  delay(60);
  digitalWrite(GPIO_Buzzer,LOW);
  delay(60);
  digitalWrite(GPIO_Buzzer,HIGH);
  delay(500);
  DoorLatch.write(SERVO_ANG_UNLOCK);
  delay(1500);
}

void Servo_LockLatch()
{
  DoorLatch.attach(GPIO_Servo);
  digitalWrite(GPIO_Buzzer,LOW);
  delay(400);
  digitalWrite(GPIO_Buzzer,HIGH);
  DoorLatch.write(SERVO_ANG_LOCK);
  delay(1500);
  //DoorLatch.detach();
}

void Buzzer_Alert()
{
  tone(GPIO_Buzzer,2794,100);
  delay(100);
  tone(GPIO_Buzzer,4186,100);
  delay(100);  
  digitalWrite(GPIO_Buzzer,HIGH);

}

void setup() {
  byte clr;
  GPIO_Init();
  Serial.begin(115200);
  digitalWrite(SLAVESELECT,HIGH); //disable device  
  SPCR = B01111111; //data lsb, clock high when idle, samples on falling
  clr=SPSR;
  clr=SPDR;
  delay(10);
  
  digitalWrite(GPIO_Buzzer,LOW);//init begin
  
  digitalWrite(SLAVESELECT,LOW);
  spi_transfer(PU); // power up
  spi_transfer(0x00); // data byte
  digitalWrite(SLAVESELECT,HIGH);
  delay(100);  

  digitalWrite(SLAVESELECT,LOW);
  spi_transfer(CLR_INT); // clear interupt and eom bit
  spi_transfer(0x00); // data byte
  digitalWrite(SLAVESELECT,HIGH);
  delay(100);
  
  digitalWrite(GPIO_Buzzer,HIGH);//init end
  Servo_LockLatch();
  SPI_SendCmd_PlayVoiceNumber(1);
}

//----------------STATE MACHINE--------------------

byte Standby_Transfer(byte input)
{
  byte output;
  switch(input)
  { 
	case 1:
	case 3:
	case 4:output = PW1;break;
	case 2:output = ALARM;break;
	case 5:output = KEEP;break;
    case IDLE:output = STANDBY;break;
  }
  return output;
}

byte PW1_Transfer(byte input)
{
  byte output;
  switch(input)
  { 
	case 1:
	case 2:
	case 3:
	case 4:output = PW2;break;
    case IDLE:output = PW1;break;
  }
  return output;
}

byte PW2_Transfer(byte input)
{
  byte output;
  switch(input)
  { 
	case 1:
	case 2:
	case 3:
	case 4:output = COMPARE;break;
    case IDLE:output = PW2;break;
  }
  return output;
}

byte NxtState = STANDBY;
byte StateInData = 0;
unsigned long int MillisCounter;
boolean ActionTriggeredFlag = 0;

void loop() {
  switch(NxtState)
  {
    case STANDBY:
	{
	  if(DEBUG_MODE) Serial.println("STANDBY");
	  StateInData = IDLE;
	  if(digitalRead(GPIO_RemoteKey_A)==HIGH) StateInData = 1;
	  if(digitalRead(GPIO_RemoteKey_B)==HIGH) StateInData = 2;
	  if(digitalRead(GPIO_RemoteKey_C)==HIGH) StateInData = 3;
	  if(digitalRead(GPIO_RemoteKey_D)==HIGH) StateInData = 4;
	  if(digitalRead(GPIO_LocalKey)==HIGH) StateInData = 5;
	  while(!(digitalRead(GPIO_RemoteKey_A)==LOW && digitalRead(GPIO_RemoteKey_B)==LOW && digitalRead(GPIO_RemoteKey_C)==LOW && digitalRead(GPIO_RemoteKey_D)==LOW));
	  pw_buff[0]=StateInData;
	  NxtState = Standby_Transfer(StateInData);
	  break;
	}
	
	case PW1:
	{ 
	  tone(GPIO_Buzzer,1397);
	  if(DEBUG_MODE) Serial.println("PW1");
	  StateInData = IDLE;
	  if(digitalRead(GPIO_RemoteKey_A)==HIGH) StateInData = 1;
	  if(digitalRead(GPIO_RemoteKey_B)==HIGH) StateInData = 2;
	  if(digitalRead(GPIO_RemoteKey_C)==HIGH) StateInData = 3;
	  if(digitalRead(GPIO_RemoteKey_D)==HIGH) StateInData = 4;
	  while(!(digitalRead(GPIO_RemoteKey_A)==LOW && digitalRead(GPIO_RemoteKey_B)==LOW && digitalRead(GPIO_RemoteKey_C)==LOW && digitalRead(GPIO_RemoteKey_D)==LOW));
	  pw_buff[1]=StateInData;
	  NxtState = PW1_Transfer(StateInData);
	  break;
	}
	
	case PW2:
	{
	  tone(GPIO_Buzzer,1568);
	  if(DEBUG_MODE) Serial.println("PW2");
	  StateInData = IDLE;
	  if(digitalRead(GPIO_RemoteKey_A)==HIGH) StateInData = 1;
	  if(digitalRead(GPIO_RemoteKey_B)==HIGH) StateInData = 2;
	  if(digitalRead(GPIO_RemoteKey_C)==HIGH) StateInData = 3;
	  if(digitalRead(GPIO_RemoteKey_D)==HIGH) StateInData = 4;
	  while(!(digitalRead(GPIO_RemoteKey_A)==LOW && digitalRead(GPIO_RemoteKey_B)==LOW && digitalRead(GPIO_RemoteKey_C)==LOW && digitalRead(GPIO_RemoteKey_D)==LOW));
	  pw_buff[2]=StateInData;
	  NxtState = PW2_Transfer(StateInData);
	  break;
	}
	
	case COMPARE:
	{
	  tone(GPIO_Buzzer,1760,300);
	  delay(310);
	  digitalWrite(GPIO_Buzzer,HIGH);
	  if(DEBUG_MODE) Serial.println("COMPARE");
	  if(pw_buff[0] == 3 && pw_buff[1] == 1 && pw_buff[2] == 4)
		NxtState = RELEASE;
	  else NxtState = ERROR;
	  break;
	}
	
	case RELEASE:
	{
	  if(DEBUG_MODE) Serial.println("RELEASE");
	  err_count = 0;
	  //SPI_SendCmd_PlayVoiceNumber(2);
      SPI_SendCmd_Reset();
      SPI_SendCmd_FastForward(2);
      digitalWrite(SLAVESELECT,LOW);
      spi_transfer(PLAY); // clear interupt and eom bit
      spi_transfer(0x00); // data byte
      digitalWrite(SLAVESELECT,HIGH);
      delay(550);
    Servo_UnlockLatch();
	  delay(1000);
	  Servo_LockLatch();
	  NxtState = STANDBY;
      break;	  
	}
	
	case ERROR:
	{
	  if(DEBUG_MODE) Serial.println("ERROR");
	  err_count++;
	  if(err_count <= 2)
	  {
	  NxtState = STANDBY;
	  SPI_SendCmd_PlayVoiceNumber(3);
	  }
	  else if(err_count == 3)
	  {
	  NxtState = STANDBY;
	  SPI_SendCmd_PlayVoiceNumber(4);
	  }
	  else if(err_count == 4)
	  {
	  SPI_SendCmd_PlayVoiceNumber(5);
	  NxtState = LOCK;
	  }
	  break;
	}
	
	case LOCK:
	{
	  int time;
	  while(1)
	  {
	    time = random(10,100);
	    tone(GPIO_Buzzer,random(500,5000),time);
		delay(time);
	  }
	}
	
	case ALARM:
	{
	  if(DEBUG_MODE) Serial.println("ALARM");
	  SPI_SendCmd_PlayVoiceNumber(6);
	  NxtState = STANDBY;
	  break;
	}
	
	case KEEP:
	{
	  if(DEBUG_MODE) Serial.println("KEEP");
	  Servo_UnlockLatch();
	  Buzzer_Alert();
	  while(digitalRead(GPIO_LocalKey) == LOW)
	  {
	  if(ActionTriggeredFlag == 1) {MillisCounter = millis();Buzzer_Alert();}
	  if(millis() > MillisCounter + 60000){ActionTriggeredFlag = 1;}
	  else ActionTriggeredFlag = 0;
	  }
	  Servo_LockLatch();
	  NxtState = STANDBY;
	  break;
	}
  }
}
