/*     ---------------------------------------------------------
 *     |  Arduino             |
 *     |  Измерение индуктивности |
 *     ---------------------------------------------------------
*/
/*------------------Libraries------------------*/
  #include <avr/io.h>
  #include <avr/interrupt.h>


/*-------------------Defines-------------------*/
  #define LEDPIN 13
  #define FPin 12
  #define ZptPinU 3
  #define ZptPinD 19
  #define ZptPinT 18
  #define ThPin 7
  #define SRngPin 17
  #define Impuls 2
  #define Fww 8     //filrter window width from 1 to 8
  #define window 10000000 
  
  #define HndrS 4
  #define DoznS 5
  #define UnitS 6

  
/*------------Structure Templates-------------*/
  struct Stensil_A
    {
      unsigned flagTh: 1;
      unsigned flagDot: 2;
      unsigned FState: 1;
      unsigned fltr_step: 4;
      unsigned dummy: 8;
    };

  struct Stensil_B
    {
      unsigned Digit3: 4;
      unsigned Digit2: 4;
      unsigned Digit1: 4;
      unsigned dummy: 4;
    };

    
/*--------------Global Variables--------------*/
  struct Stensil_A flags = {0,0,0,0,0};
  struct Stensil_B digits = {14,14,14,0};
  struct Stensil_B calc_buff = {0,0,0,0};

  float RFww = (float)1/Fww;
  double result = 0;
  unsigned int buff = 0;
  unsigned int fltr_delay[Fww];

  
/*------------Functions Prototypes------------*/
  int indication_calc (double);
  void draw(void);
  int slide_avr_filter (int);


/*--------------Pheripheral Setup--------------*/
  void setup()
    {
      cli(); //global interrupt disable
      // инициализация Timer2    
      TCCR2A = 0x00;
      TCCR2B = 0x00;
      TCCR2B |= 0x07;
      TCNT2 = 0x00; 
      TIMSK2 |= 0x01;
      
    pinMode(8,OUTPUT); //выход - 1
    pinMode(9,OUTPUT); //выход - 2
    pinMode(10,OUTPUT); //выход - 4
    pinMode(11,OUTPUT); //выход - 8
    pinMode(4,OUTPUT); //выход - строб единиц
    pinMode(5,OUTPUT); //выход - строб десятков
    pinMode(6,OUTPUT); //выход - строб сотен
    pinMode(ThPin,OUTPUT); //выход - тысяча 0 или 1
    pinMode(FPin,OUTPUT); //выход частота индикации
    pinMode(ZptPinU,OUTPUT); // выход запятая 0.1
    pinMode(ZptPinD,OUTPUT); // выход запятая 0.01
    pinMode(ZptPinT,OUTPUT); // выход запятая 0.001

    pinMode(SRngPin,INPUT_PULLUP);  //set rang pin
    pinMode(Impuls,INPUT); 

    Serial.begin(9600);
    Serial.println("Version 1.6");  // say program version
      sei();  //global interrupt enable

      
    for (unsigned int i = 0; i < 16; i++)
      {
        digits = {i,i,i,0}; 
        delay(500);
      }
      
    for(int i = 0; i < Fww; i++)
      fltr_delay[i] = 0;
    delay(500);
    }


/*--------------------Main----------------------*/
  void loop()                     
    {    
      /*if (Serial.available())       // есть что на вход?
        {
          result = Serial.parseFloat();
          while (Serial.available()) 
            Serial.read();
          indication_calc(result);
          Serial.println(result, 4);
        }*/
      result = (double)pulseIn(Impuls, HIGH, window);
      result = result*0.0001220703125;       //  == result/8192
      result = sq(result);
      if(digitalRead(SRngPin))
        result = result*0.001;
      indication_calc(result);     
    } 


/*-------------Functions Realization------------*/
  int indication_calc (double res)
    {    
      if(res > 1999.0)
        {
          digits = {14,0,14,0};
          flags.flagDot = 0;
          flags.flagTh = 0;
          return 0;           //overflow exit
        }
      else if ((res >= 1000.0)&&(res <= 1999.0))
        {
          res = res - 1000;
          flags.flagDot = 0;
          flags.flagTh = 1;
        }
      else if ((res >= 200.0)&&(res <= 999.9))
        {
          flags.flagDot = 0;
          flags.flagTh = 0;
        }
      else if ((res >= 100.0)&&(res <= 199.9))
        {
          res = res - 100;
          res = res*10;
          flags.flagDot = 1;
          flags.flagTh = 1;
        }
      else if ((res >= 20.0)&&(res <= 99.9))
        {
          res = res*10;
          flags.flagDot = 1;
          flags.flagTh = 0;
        }
      else if ((res >= 10.00)&&(res <= 19.99))
        {
          res = res - 10;
          res = res*100;
          flags.flagDot = 2;
          flags.flagTh = 1;
        }
      else if ((res >= 2.00)&&(res <= 9.99))
        {
          res = res*100;
          flags.flagDot = 2;
          flags.flagTh = 0;
        }
      else if ((res >= 1.000)&&(res <= 1.999))
        {
          res = res - 1;
          res = res*1000;
          flags.flagDot = 3;
          flags.flagTh = 1;
        }
      else if ((res >= 0.001)&&(res <= 0.999))
        {
          res = res*1000;
          flags.flagDot = 3;
          flags.flagTh = 0;
        }
      else
        {
          digits = {0,0,0,0};
          flags.flagDot = 0;
          flags.flagTh = 0;
        }
      buff = (int)res;
      buff = slide_avr_filter (buff);
      calc_buff = {0,0,0,0};
      while(buff >= 100)
        {
          calc_buff.Digit3 ++;
          buff -= 100;
        }
      while(buff >= 10)
        {
          calc_buff.Digit2 ++;
          buff -= 10;
        }
      while(buff >= 1)
        {
          calc_buff.Digit1 ++;
          buff --;
        }
        
      digits = calc_buff;
      
      return 0;
    }


  void draw (void)
    {
      PORTB &= 0xF0;
      PORTB |= (int)digits.Digit1;
      digitalWrite(HndrS,HIGH); //строб записи сотен в регистр
      digitalWrite(HndrS,LOW);
      //delayMicroseconds(5); 
      
      PORTB &= 0xF0;
      PORTB |= (int)digits.Digit2;
      digitalWrite(DoznS,HIGH); //строб записи десятков в регистр
      digitalWrite(DoznS,LOW);
      //delayMicroseconds(5);
      
      PORTB &= 0xF0;
      PORTB |= (int)digits.Digit3;
      digitalWrite(UnitS,HIGH); //строб записи единиц в регистр
      digitalWrite(UnitS,LOW);
      //delayMicroseconds(5); 
    }


  int slide_avr_filter (int value)
    {
      fltr_delay[flags.fltr_step] = value;
      flags.fltr_step++;
      if(flags.fltr_step == Fww)
        flags.fltr_step = 0;
      value = 0;
      for(int j = 0; j < Fww; j++ )
        value = value + fltr_delay[j];
      return (int)((float)value*RFww);
    }


/*-------------------Interrupts------------------*/
  ISR(TIMER2_OVF_vect)                             //screen refreshed if timer to counted to top
    {
      flags.FState = !flags.FState;         // square wave
      digitalWrite(FPin, flags.FState);
          
      switch(flags.flagDot)                 // draw sole dot
        {
          /*---1---*/
          case 1:
            digitalWrite(ZptPinU,!flags.FState);
            digitalWrite(ZptPinD,flags.FState);
            digitalWrite(ZptPinT,flags.FState);
            break;
          /*---2---*/
          case 2:
            digitalWrite(ZptPinU,flags.FState);
            digitalWrite(ZptPinD,!flags.FState);
            digitalWrite(ZptPinT,flags.FState);
            break;
          /*---3---*/
          case 3:
            digitalWrite(ZptPinU,flags.FState);
            digitalWrite(ZptPinD,flags.FState);
            digitalWrite(ZptPinT,!flags.FState);
            break;
          /*-other-*/
          default:
            digitalWrite(ZptPinU,flags.FState);
            digitalWrite(ZptPinD,flags.FState);
            digitalWrite(ZptPinT,flags.FState);
            break;
        }
            
      if (flags.flagTh == 0)
        digitalWrite(ThPin,flags.FState);
      else
        digitalWrite(ThPin,!flags.FState);          // draw '1'
        
      draw();
    }
  
