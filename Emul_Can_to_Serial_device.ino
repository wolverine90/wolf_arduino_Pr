/*     ---------------------------------------------------------
 *     |  Arduino             |
 *     |  Emulate can to serial device |
 *     ---------------------------------------------------------
*/
/*------------------Libraries------------------*/
  //#include <avr/io.h>
  //#include <avr/interrupt.h>
  #include <stdlib.h>


/*-------------------Defines-------------------*/
  #define LEDPIN 13
  #define fifosize 20
  #define receivebuffsize 64
  #define send_interval 500

  
/*------------Structure Templates-------------*/
  struct Stensil_A
    {
      unsigned bitstart: 8;
      char dummy[3];
      double identification;
      char buff[8];
      unsigned int fakecrc;
      unsigned bitstop: 8;
    };

  struct Stensil_B
    {
      unsigned flow: 1;
      unsigned transmit_ok: 1;
      unsigned check_t: 1;
      unsigned dummy: 5;
    };

   union convertation
    {
      struct Stensil_A msg;
      byte arr[fifosize];
    };

    
/*--------------Global Variables--------------*/
  struct Stensil_A msg1 = {0x00,"00",20.0538,"appleXX",0x9ACF,0xFF};
  struct Stensil_B ctrlbits = {0,0,0,0};
  union convertation conv;
  String cmdreceive = String("receive");
  String cmdtransmit = String("transmit");
  String cmdstart = String("start");
  String cmdstop = String("stop");
  String cmdhelp = String("help");
  unsigned long previousMillis = 0;
  unsigned long interval = send_interval;

  
/*------------Functions Prototypes------------*/
  int reading(char*, String*);
  int communication(String*);
  int copy_ch_buf_from_pos_until (char*, char*, int, char);


/*--------------Pheripheral Setup--------------*/
  void setup()
    {
      for(int i = 0; i < fifosize; i++)
        conv.arr[i] = 0x2E;
      conv.msg = msg1;

      pinMode(15,OUTPUT); 
      pinMode(19,OUTPUT);
      digitalWrite(15,LOW);
      digitalWrite(19,HIGH);
      
      Serial.setTimeout(55);
      Serial.begin(115200);
      //Serial.begin(9600);
      Serial.println("Version 0.4");  // say program version
    
    }


/*--------------------Main----------------------*/
  void loop()                     
    {
      unsigned long currentMillis = millis();
       if (currentMillis - previousMillis >= interval) 
        { 
          ctrlbits.transmit_ok = 1;
          previousMillis = currentMillis;
        }
       else
         ctrlbits.transmit_ok = 0;
              
      if (Serial.available())       // есть что на вход?
        {
          char buff[receivebuffsize];
          Serial.readBytesUntil('\r', buff, receivebuffsize);
          String temp;
          reading(buff, &temp);
          communication(&temp);           
        }
      if((ctrlbits.flow == 1) && (ctrlbits.transmit_ok == 1))
        {
          Serial.println(analogRead(A3));
        }    
    } 


/*-------------Functions Realization------------*/
  int reading(char*buff, String*result)
    {
      char tbuff[receivebuffsize];
      for(int i = 0; i<15; i++)
        {
          if (buff[i] == '!')
            {
              for(int j = 0; j<i; j++)
                tbuff[j] = buff[j];
              tbuff[i] = '\0';
              *result = String(tbuff);
              if (cmdtransmit.equalsIgnoreCase(*result))
                {
                  ctrlbits.check_t = 1;
                  copy_ch_buf_from_pos_until(buff, tbuff, ++i, '!');
                  *result = String(tbuff);
                }
              else if(cmdstart.equalsIgnoreCase(*result))
                {
                  ctrlbits.flow = 1;
                  copy_ch_buf_from_pos_until(buff, tbuff, ++i, '!');
                  long interval_t = atol(tbuff);
                  if(interval_t < 1)
                    {
                      Serial.println("Incorrect set interval! Reset to default 500ms");
                      interval_t = send_interval;
                    }
                  else if (interval_t > 10000)
                    {
                      Serial.println("Interval longer than possible! Set to max 10000ms");
                      interval_t = 10000;
                    }
                  interval = (unsigned long)interval_t;
                }
              return 1;
            }  
        }
      *result = String("0");
      return 0;
    }


  int communication (String*buff)
    {
      if (cmdreceive.equalsIgnoreCase(*buff))
        {
          for(int i = 0; i < fifosize; i++)
            Serial.write(conv.arr[i]);
        }
      else if (ctrlbits.check_t == 1)
        {
          ctrlbits.check_t = 0;
          Serial.println(*buff);
        }
      else if (cmdhelp.equalsIgnoreCase(*buff))
        {
          Serial.println("Command: help!; receive!; transmit!(message)!; start!(transfer rate, ms)!; stop!");  
        }
      else if (cmdstart.equalsIgnoreCase(*buff))
        {
          //ctrlbits.flow = 1;
          //Serial.println(interval);
          return 1; 
        }
      else if (cmdstop.equalsIgnoreCase(*buff))
        {
          ctrlbits.flow = 0;   
        }      
      else
        Serial.println("uncorrecect command");
      return 0;
    }


  int copy_ch_buf_from_pos_until (char*from, char*to, int pos, char ch)
    {
      int ti = pos;
        for(int j = 0; (from[ti] != ch) && (ti < receivebuffsize); j++)
          {
            to[j] = from[ti];
            ti++;
          }
        to[ti - pos] = '\0';
      return 0;
    }
