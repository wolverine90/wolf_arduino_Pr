/*     ---------------------------------------------------------
 *     |  Arduino             |
 *     |  Black Pill tests spiflash / can |
 *     ---------------------------------------------------------
*/
/*------------------Libraries------------------*/
//#include "SPIFlash.h"
#include "mcp2515_can.h"
#include "commands.h"
#include "my_utility.h"

/*-------------------Defines-------------------*/
#define SPI_CS_PIN PA_1
#define CAN_INT_PIN PA_2
#define BUTTON_PIN PA_0

/*----------------Class members----------------*/
//SPIFlash flash;
  unsigned char Tickers::counter = 0;
  Tickers ticker1(100), ticker2(300);//, ticker3;

/*------------Structure Templates-------------*/


/*--------------Global Variables--------------*/
//uint16_t manID = 0;
//uint32_t JEDEC = 0;
//uint32_t cap = 0;
//uint32_t maxPage = 0;
unsigned char flagRecv = 0;
unsigned char flagEmul = 0;
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
      if (currentMillis - previousMillis >= 7000) 
        { 
          flagRecv = 1; 
        }
    }
  flagRecv = 0;
  Serial.println("Version 1.0");

  attachInterrupt(digitalPinToInterrupt(CAN_INT_PIN), do_IRQ_2_can_receive, FALLING); // start interrupt
  CAN1.setMode(MODE_NORMAL);
  
  while (CAN_OK != CAN1.begin(CAN_1000KBPS, MCP_8MHz)) {             // init can bus : baudrate = 500k
        Serial.println("CAN init fail, retry...");
        delay(100);
    }
  //CAN1.init_Mask(0, 1, 0);        //disable mask (by value) and filter (impicitly)
  //CAN1.init_Mask(1, 1, 0);
    CAN1.mcp2515_modifyRegister(0x60, 0x60, 0xFF);
    CAN1.mcp2515_modifyRegister(0x70, 0x60, 0xFF);
  
  Serial.println("CAN init ok!");

}


/*--------------------Main----------------------*/
void loop() 
{
  ticker1.not_block_ticker_call(not_block_sos, millis());
  //ticker3.set_new_default_delay(2000);
  
  if (flagEmul > 0)
    {
      switch (flagEmul) 
      {
        case 1:
          ticker2.not_block_ticker_call(common_info, millis(),2000);
          break;
        case 2:
          ticker2.not_block_ticker_call(clean_rom, millis(), 2000);
          break;
        case 3:
          ticker2.not_block_ticker_call(programm_rom, millis(), 2000);
          break;
        case 4:
          ticker2.not_block_ticker_call(check_rom, millis(), 2000);
          break;
        case 5:
          ticker2.not_block_ticker_call(convey_control, millis(), 2000);
          break;
        case 6:
          ticker2.not_block_ticker_call(take_eeprom, millis(), 2000);
          break;
        case 7:
          common_info();
          clean_rom();
          programm_rom();
          check_rom();
          convey_control();
          take_eeprom();
          break;                  
        default:
          flagEmul = 1;
      }
    }
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
   
        CAN1.readMsgBufID(stat, &id, &ext, &rtr, &len, buf);
        stat = 0;
        if((id == 0) && (rtr == 1))
          {
            unsigned char stmp[8] = {0xA, 0xA, 0xB, 0xB, 0xC, 0xC, 0xF, 0xF};
            CAN1.sendMsgBuf(0x0, 1, 8, stmp);
          }
        else if ((id == 0xA) && (rtr == 1))
          {
            flagEmul++;
            Serial.println("Cycle+");
          }
        else if ((id == 0xF) && (rtr == 1))
          {
            flagEmul = 0;
            Serial.println("Cycle off");
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
        
            CAN1.sendMsgBuf(id, ext, len, buf);
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

   CAN1.sendMsgBuf(0x7FF, 1, 8, stmp);
   Serial.println("CAN BUS sendMsgBuf ok!");
   //ticker1.show_ticker_num();
   //ticker2.show_ticker_num();
}


void do_IRQ_2_can_receive(void)
{
  stat = CAN1.readRxTxStatus();
  if(((stat && 0x3) == 1)||((stat && 0x3) == 2)||((stat && 0x3) == 3))
    {
      CAN1.clearBufferReceiveIfFlags(0x3);
      flagRecv = 1;
    }
}
