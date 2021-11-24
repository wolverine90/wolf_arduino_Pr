  #include <avr/io.h>
  #include <avr/interrupt.h>
  #include <IRremote.h>
  
  #define resol 255
  #define Pred 16
  #define Pgreen 15
  #define Pblue 14
  #define Fww 4     //filrter window width from 1 to 8

struct controlflags 
  {
      unsigned power: 1;
      unsigned SOS: 1;
      unsigned red: 1;
      unsigned green: 1;
      unsigned blue: 1;
      unsigned rnd: 1;
      unsigned adrr: 8;
      unsigned command: 8;
  } flags = {0,0,1,1,1,0,0,0};

struct filterbase
  {
    int lagval[Fww];
    unsigned fltr_step: 8;
  } filter = {{},0};
  
int i = 0x80;
float RFww = (float)1/Fww;

void Ddecode(void);
void SOS(void);
int slide_avr_filter (int);

void setup() 
{
    pinMode(Pred,OUTPUT);
    pinMode(Pgreen,OUTPUT); 
    pinMode(Pblue,OUTPUT);
    pinMode(17,OUTPUT); 
    pinMode(18,OUTPUT);

    pinMode(19,INPUT);
    randomSeed(analogRead(19));
    pinMode(19,INPUT_PULLUP);
    

    digitalWrite(18,LOW);
    digitalWrite(17,HIGH);

    Serial.begin(9600);
    Serial.println("Version 1.0");

    IrReceiver.begin(19, ENABLE_LED_FEEDBACK);

    TCCR0B |= 0xC0;
    OCR0A = 0x02;
    OCR0B = 0x01;
    TIMSK0 |= 0x06;
}


void loop() 
  {
    if (IrReceiver.decode()) 
      {
        storeCode(IrReceiver.read());
        IrReceiver.resume(); // resume receiver
      }
    if (flags.SOS)
      SOS();
    if (flags.rnd)
      {
        OCR0A = slide_avr_filter(random(0x3, 0xFD));
        
        delay(random(30, 120));
      }
  }


int storeCode(IRData *aIRReceivedData) 
{               
    if (aIRReceivedData->flags & IRDATA_FLAGS_IS_REPEAT) 
    {
      if((IrReceiver.decodedIRData.address == 3) && (IrReceiver.decodedIRData.command == 2))
        {
          i -= 30;
        }
      else if((IrReceiver.decodedIRData.address == 3) && (IrReceiver.decodedIRData.command == 3))
        {
          i += 30;
        }

        if(i > 0xFD)
          i = 0xFD;
        else if(i < 3)
          i = 3;
        OCR0A = i;
        return 0;
    }
    
        flags.adrr = IrReceiver.decodedIRData.address;
        flags.command = IrReceiver.decodedIRData.command;
        Serial.println(IrReceiver.decodedIRData.address);
        Serial.println(IrReceiver.decodedIRData.command);
        Ddecode();
        return 1;
}


void Ddecode(void)
 {      
      if((flags.adrr == 2) && (flags.command == 0))
        {
          flags.power = !flags.power;
        }
      else if((flags.adrr == 2) && (flags.command == 10))
        {
          flags.SOS = !flags.SOS;
        }
      else if((flags.adrr == 3) && (flags.command == 0))
        {
          flags.blue = 1;
          flags.green = 0;
          flags.red = 0;
        }
      else if((flags.adrr == 3) && (flags.command == 1))
        {
          flags.blue = 0;
          flags.green = 1;
          flags.red = 0;
        }
      else if((flags.adrr == 2) && (flags.command == 28))
        {
          flags.blue = 1;
          flags.green = 1;
          flags.red = 1;
        }
      else if((flags.adrr == 2) && (flags.command == 29))
        {
          flags.blue = 0;
          flags.green = 0;
          flags.red = 1;
        }
      else if((flags.adrr == 2) && (flags.command == 17))
        {
          flags.rnd = !flags.rnd;
        }
      else if((flags.adrr == 3) && (flags.command == 2))
        {
          i -= 5;
        }
      else if((flags.adrr == 3) && (flags.command == 3))
        {
          i += 5;
        }

    if(i > 0xFD)
      i = 0xFD;
    else if(i < 3)
      i = 3;
    OCR0A = i;

    flags.adrr = 0;
    flags.command = 0; 
 }

 void SOS(void)
  {
    for(int j = 6; j > 0; j--)
      {
        flags.power = !flags.power;
        delay(500);
      }
    for(int j = 6; j > 0; j--)
      {
        flags.power = !flags.power;
        delay(1000);
      }
    for(int j = 6; j > 0; j--)
      {
        flags.power = !flags.power;
        delay(500);
      }
    flags.power = 0;
    delay(3000);
  }

    int slide_avr_filter (int value)
    {
      filter.lagval[filter.fltr_step] = value;
      filter.fltr_step++;
      if(filter.fltr_step == Fww)
        filter.fltr_step = 0;
      value = 0;
      for(int j = 0; j < Fww; j++ )
        value = value + filter.lagval[j];
      return (int)((float)value*RFww);
    }

  ISR(TIMER0_COMPA_vect)
    {
      digitalWrite(Pred,LOW);
      digitalWrite(Pgreen,LOW);
      digitalWrite(Pblue,LOW);
    }


  ISR(TIMER0_COMPB_vect)
    {
      if(flags.power)
        {
          digitalWrite(Pblue,flags.blue);
          digitalWrite(Pgreen,flags.green);
          digitalWrite(Pred,flags.red);
        }
      else
        {
          digitalWrite(Pred,LOW);
          digitalWrite(Pgreen,LOW);
          digitalWrite(Pblue,LOW);
        }
    } 
