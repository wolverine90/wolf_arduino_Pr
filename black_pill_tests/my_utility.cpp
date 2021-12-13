#include "my_utility.h"
#include "Arduino.h"
/*----------------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------------*/

/*----- tickers realisation-----*/
Tickers::Tickers(): default_delay(1000), tick_num(0), previousMillis(0)
{
  counter++;
  tick_num = counter;
}

Tickers::Tickers(unsigned int i): default_delay(i), tick_num(0), previousMillis(0)
{
  counter++;
  tick_num = counter;
  //Serial.print("Create ticker № ");
  //Serial.println(tick_num);
}

Tickers::~Tickers()
{
  counter--;
  //Serial.print("Destroy ticker № ");
  //Serial.println(tick_num);
}

int Tickers::not_block_ticker_call(int (*func)(), unsigned long currentMillis, unsigned int interval)
{
  if((default_delay != 1000)&&(interval == 1000))
    interval = default_delay;
  
  if (currentMillis - previousMillis >= interval) 
  { 
    (*func)();  
    previousMillis = currentMillis;
  }
  return 0;
}

int Tickers::set_new_default_delay(int del)
{
  default_delay = del;
  return 0;
}

int Tickers::show_ticker_num(void)
{
  Serial.print("ticker № ");
  Serial.println(tick_num);
  return (int)tick_num;
}

/*----------------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------------*/
