#ifndef __MY_UTILITY_H__      //multiple includes protection
#define __MY_UTILITY_H__

/*------classes prototypes------*/


class Tickers
{
  public:
    Tickers();                        //constructor1
    Tickers(unsigned int);            //constructor2
    ~Tickers();                       //destructor
    int not_block_ticker_call(int (*func)(), unsigned long, unsigned int interval = 1000);
    int set_new_default_delay(int);
    int show_ticker_num(void);
  
  private:
    unsigned int default_delay;
    unsigned char tick_num;
    unsigned long previousMillis;
    static unsigned char counter;
};

/*----------------------------------------------------------------------------------
init "counter" like:

  unsigned char Tickers::counter = 1;
----------------------------------------------------------------------------------*/
#endif
