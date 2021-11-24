/*     ---------------------------------------------------------
 *     |  Arduino             |
 *     |  Black Pill tests spiflash / can |
 *     ---------------------------------------------------------
*/
/*------------------Libraries------------------*/
//#include <SPIFlash.h>
#include "mcp2515_can.h"
 

/*-------------------Defines-------------------*/
#define SPI_CS_PIN PA_1
#define CAN_INT_PIN PA_2
#define BUTTON_PIN PA_0

/*-----------------Constructors-----------------*/
//SPIFlash flash;
mcp2515_can CAN(SPI_CS_PIN);


/*------------Structure Templates-------------*/


/*--------------Global Variables--------------*/
//uint16_t manID = 0;
//uint32_t JEDEC = 0;
//uint32_t cap = 0;
//uint32_t maxPage = 0;
unsigned char flagRecv = 0;
unsigned long id = 0;
byte ext = 0;
byte rtr = 0;
byte len = 0;
byte stat = 0;
unsigned char buf[64]; 


/*------------Functions Prototypes------------*/
int not_block_func_call(int (*func)(), int);
int not_block_sos(void);
void do_IRQ_1_button(void);
void do_IRQ_2_can_receive(void);

/*--------------Pheripheral Setup--------------*/
void setup() 
{
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PA_0, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), do_IRQ_1_button, RISING);

  //pinMode(PB_2, OUTPUT);
  //digitalWrite(PB_2, 1);
  /*flash.begin(PA_4);
  manID = flash.getManID();
  JEDEC = flash.getJEDECID();
  cap = flash.getCapacity();
  maxPage = flash.getMaxPage();*/  
  
  Serial.setTimeout(55);
  Serial.begin(115200);
  while ((!Serial) && (flagRecv != 1)) //wait connection valid only for usb cdc
    {
      static unsigned long previousMillis = 0;
      unsigned long currentMillis = millis();
      if (currentMillis - previousMillis >= 5000) 
        { 
          flagRecv = 1;  
          //previousMillis = currentMillis;
        }
    }
  flagRecv = 0;
  Serial.println("Version 0.2");

  attachInterrupt(digitalPinToInterrupt(CAN_INT_PIN), do_IRQ_2_can_receive, FALLING); // start interrupt
  CAN.setMode(MODE_NORMAL);
  
  while (CAN_OK != CAN.begin(CAN_125KBPS, MCP_8MHz)) {             // init can bus : baudrate = 500k
        Serial.println("CAN init fail, retry...");
        delay(100);
    }
  Serial.println("CAN init ok!");
  //byte mode = CAN.getMode();
  //SERIAL_PORT_MONITOR.printf("CAN BUS mode = %d\n\r", mode);
  //SPI.begin(); 
}


/*--------------------Main----------------------*/
void loop() 
{
  not_block_func_call(not_block_sos, 1000);  
  if (Serial.available())       // есть что на вход?
    {
      char buff[64];
      Serial.readBytesUntil('\r', buff, 64);
      Serial.write(buff, 64);
      Serial.println();
      //Serial.println(manID, HEX);   
    }    
  if (flagRecv) 
    {
      flagRecv = 0; 
        Serial.println("Receive:");
   
        CAN.readMsgBufID(stat, &id, &ext, &rtr, &len, buf);
        stat = 0;
        if((id == 0) && (rtr == 1))
          {
            unsigned char stmp[8] = {0xA, 0xA, 0xB, 0xB, 0xC, 0xC, 0xF, 0xF};
            CAN.sendMsgBuf(0x0, 1, 8, stmp);
          }
        else if (rtr == 0)
          {
        ext == 1 ? Serial.println("extended frame") : Serial.println("standart frame");
        Serial.print("ID: ");
        Serial.println(id, HEX); 
        Serial.print("Msg: ");
        for (int i = 0; i < len; i++)
           {
              Serial.print(buf[i], HEX);
              Serial.print(' ');
           }
        Serial.println();
        
        CAN.sendMsgBuf(id, ext, len, buf);
          }
    }                                               
}


/*-------------Functions Realization------------*/
int not_block_func_call(int (*func)(), int interval)
{
  static unsigned long previousMillis = 0;
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) 
    { 
      (*func)();  
      previousMillis = currentMillis;
    }
  return 0;
}


int not_block_sos(void)
{
  static unsigned int count = 0;
  count++;
  if(count == 1||count == 3||count == 5||count == 7||count == 11||count == 15||count == 19||count == 21||count == 23)
    {
      digitalWrite(LED_BUILTIN, 0);
      return 1;
    }
  else if(count == 2||count == 4||count == 6||count == 10||count == 14||count == 18||count == 20||count == 22||count == 24)
    {
      digitalWrite(LED_BUILTIN, 1);
      return 1;
    }
  else if(count > 29)
    count = 0;
  return 0;
}


/*-------------------Interrupts------------------*/
void do_IRQ_1_button(void)
{
   unsigned char stmp[8] = {'A', 'A', 'B', 'B', 'C', 'C', 'D', 'D'};
 
   /*Serial.println(JEDEC, HEX);
   Serial.println(cap);
   Serial.println(maxPage);*/

   CAN.sendMsgBuf(0x7FF, 0, 8, stmp);
   //CAN.clearBufferTransmitIfFlags(0x1C);
   Serial.println("CAN BUS sendMsgBuf ok!");
}


void do_IRQ_2_can_receive(void)
{
  stat = CAN.readRxTxStatus();
  if(((stat && 0x3) == 1)||((stat && 0x3) == 2)||((stat && 0x3) == 3))
    {
      CAN.clearBufferReceiveIfFlags(0x3);
      flagRecv = 1;
    }
}
