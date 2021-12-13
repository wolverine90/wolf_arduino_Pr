#include "commands.h"
#include "mcp2515_can.h"

mcp2515_can CAN1(PA_1);

char rom_eeprom = 0;
char prgr_data[48] = "The quick brown fox jumps over the lazy dog....";
struct data_frame message[2] = {0,1,0,NULL,0,1,0,NULL};
struct data_frame *cpu_msg = &message[0];
struct data_frame *bload_msg = &message[1];
unsigned int prgr_data_size = 384; //in bytes

/*----------------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------------*/
int buss_off(void)
{
  if(random(0, 100) > 85)
  {
    CAN1.sendMsgBuf(bloader_bus_off, 1, 0, bload_msg->data);
    return 1;
  }
  return 0;
}

/*----------------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------------*/
unsigned char* data_to_out(long long int data, char* buff)
{ 
  int mask = 0xFF;
  
  buff += 7;
  for(int i = 0; i <= 56; i += 8)
  {
    *buff = (char)((data >> i) & mask);
    buff--;
  }
  buff++;
  return (unsigned char*)buff;
}

/*----------------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------------*/
int common_info(void)
{
  cpu_msg->id = cpu_request_inf;
  bload_msg->id = bloader_provide_inf;

  //1
  CAN1.sendMsgBuf(cpu_msg->id, 1, 0, cpu_msg->data);
  if(buss_off() == 1)
    return 0;
  bload_msg->id = bload_msg->id | 0x555;
  CAN1.sendMsgBuf(bload_msg->id, 1, 0, bload_msg->data);
  
  //2
  cpu_msg->id = cpu_msg->id | 0x1000;
  CAN1.sendMsgBuf(cpu_msg->id, 1, 0, cpu_msg->data);
  if(buss_off() == 1)
    return 0;
  bload_msg->id = bload_msg->id & bloader_provide_inf_m;
  bload_msg->id = bload_msg->id | 0x1ACF;
  CAN1.sendMsgBuf(bload_msg->id, 1, 0, bload_msg->data);
  return 1;
}

/*----------------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------------*/
int clean_rom(void)
{
  if(rom_eeprom == 0)
  {
    CAN1.sendMsgBuf(cpu_request_erase_fl, 1, 0, cpu_msg->data); //clear rom
    bload_msg->id = bloader_erase_rslt;
  }
  else
  {
    cpu_msg->id = cpu_request_erase_fl;       //clear flash
    cpu_msg->id = cpu_msg->id & action;
    cpu_msg->id = cpu_msg->id | 0x00800000;
    CAN1.sendMsgBuf(cpu_msg->id, 1, 0, cpu_msg->data);
    
    bload_msg->id = bloader_erase_rslt;
    bload_msg->id = bload_msg->id & action;
    bload_msg->id = bload_msg->id | 0x00800000;
  }
  
  if(random(0, 100) < 85)
  {
    CAN1.sendMsgBuf(bload_msg->id, 1, 0, bload_msg->data);   
  }
  else
  {
    int err = random(1, 7);             // create error code
    err = err << 16;        
    //bload_msg->id = bloader_erase_rslt;
    bload_msg->id = bload_msg->id | err;

    bload_msg->message_len = 8;  
    unsigned int err_data = random(0, 0xFFFF);      // create error data
    char buff[8];
    bload_msg->data = data_to_out((bloader_ers_rslt_d_m_err | err_data), buff);
    
    CAN1.sendMsgBuf(bload_msg->id, 1, bload_msg->message_len, bload_msg->data);
    bload_msg->message_len = 0;
    bload_msg->data = NULL;
  }
  rom_eeprom = !rom_eeprom;
  return 1;
}

/*----------------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------------*/
int programm_rom(void)
{ 
  char buff[8];
  cpu_msg->message_len = 8;
  
  ////// запрос программирования
  cpu_msg->data = data_to_out((cpu_prgrm_data_size | prgr_data_size), buff); 
  if(rom_eeprom == 0)
  {
    CAN1.sendMsgBuf(cpu_request_prgrm_fl, 1, cpu_msg->message_len, cpu_msg->data); //programm rom
    bload_msg->id = bloader_request_data;
  }
  else
  {
    cpu_msg->id = cpu_request_prgrm_fl;       //programm flash
    cpu_msg->id = cpu_msg->id & action;
    cpu_msg->id = cpu_msg->id | 0x00A00000;
    CAN1.sendMsgBuf(cpu_msg->id, 1, cpu_msg->message_len, cpu_msg->data);
    
    bload_msg->id = bloader_request_data;
    bload_msg->id = bload_msg->id & action;
    bload_msg->id = bload_msg->id | 0x00A00000;  
  }
  cpu_msg->data = NULL;

  int shift = random(8, 456);
  unsigned char data_start[8] = {0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA};
  char* d_data = &prgr_data[0];
  unsigned char data_end[8] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
  
  for(int d = 0, s = 0; s <= prgr_data_size; s += 256, d++)
  {
    //////запрос пакета данных
    //bload_msg->id = bloader_request_data + d;
    bload_msg->id = bload_msg->id + 1;
    CAN1.sendMsgBuf(bload_msg->id, 1, 0, bload_msg->data);

    //////передача пакета данных
    for(int f = 0, k = 1; f <= 255; f += 8, k++)
    {
      if(rom_eeprom == 0)
        cpu_msg->id = cpu_send_data + k;
      else
        {
          cpu_msg->id = cpu_send_data & action;
          cpu_msg->id = cpu_msg->id | 0x00A00000;
          cpu_msg->id = cpu_msg->id + k;
        }
      
      if(shift > 7)
      {
        cpu_msg->data = data_start;
        shift -= 8;
      }
      else if((shift <= 7) && (d_data < (prgr_data + 48)))
      {
        cpu_msg->data = (unsigned char*)d_data;
        d_data += 8;
      }
      else
      {
        cpu_msg->data = data_end;
      }
    CAN1.sendMsgBuf(cpu_msg->id, 1, 8, cpu_msg->data);    
    }
    cpu_msg->message_len = 0;
    cpu_msg->data = NULL;
    
    //////сообщение о передаче пакета данных
    
    if(rom_eeprom == 0)
      cpu_msg->id = cpu_end_data + d;
    else
      {
        cpu_msg->id = cpu_end_data & action;
        cpu_msg->id = cpu_msg->id | 0x00A00000;
        cpu_msg->id = cpu_msg->id + d;
      }
    CAN1.sendMsgBuf(cpu_msg->id, 1, 0, cpu_msg->data);
    }

    //////результат программирования
    if(rom_eeprom == 0)
      bload_msg->id = bloader_prgr_result;
    else
      {
        bload_msg->id = bloader_prgr_result & action;
        bload_msg->id = bload_msg->id | 0x00A00000;
      }
      
    if(random(0, 100) < 85)
    {
      CAN1.sendMsgBuf(bload_msg->id, 1, 0, bload_msg->data);   
    }
    else
    {
      int err = random(1, 7);             // create error code
      err = err << 16;        
      //bload_msg->id = bloader_prgr_result;
      bload_msg->id = bload_msg->id | err;
  
      bload_msg->message_len = 8;  
      unsigned int err_data = random(0, 0xFFFF);      // create error data
      bload_msg->data = data_to_out((bloader_prgr_m_data_err | err_data), buff);
    
      CAN1.sendMsgBuf(bload_msg->id, 1, bload_msg->message_len, bload_msg->data);
      bload_msg->message_len = 0;
      bload_msg->data = NULL;
    }
  rom_eeprom = !rom_eeprom;
  return 1;
}

/*----------------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------------*/
int check_rom(void)
{
  char buff[8];
  cpu_msg->message_len = 8;
  
  ////// запрос запрос сравнения
  cpu_msg->data = data_to_out((cpu_check_data_smpl | prgr_data_size), buff); 
  if(rom_eeprom == 0)
  {
    CAN1.sendMsgBuf(cpu_check_request, 1, cpu_msg->message_len, cpu_msg->data); //check rom
    bload_msg->id = bloader_request_data_smpl;
  }
  else
  {
    cpu_msg->id = cpu_check_request;       //check flash
    cpu_msg->id = cpu_msg->id & action;
    cpu_msg->id = cpu_msg->id | 0x00C00000;
    CAN1.sendMsgBuf(cpu_msg->id, 1, cpu_msg->message_len, cpu_msg->data);
    
    bload_msg->id = bloader_request_data_smpl;
    bload_msg->id = bload_msg->id & action;
    bload_msg->id = bload_msg->id | 0x00C00000;
  }
  cpu_msg->data = NULL;

  int shift = random(8, 456);
  unsigned char data_start[8] = {0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55};
  char* d_data = &prgr_data[0];
  unsigned char data_end[8] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
  
  for(int d = 0, s = 0; s <= prgr_data_size; s += 256, d++)
  {
    //////запрос пакета данных образца
    //bload_msg->id = bloader_request_data_smpl + d;
    bload_msg->id = bload_msg->id + 1;
    CAN1.sendMsgBuf(bload_msg->id, 1, 0, bload_msg->data);

    //////передача пакета данных образца
    for(int f = 0, k = 1; f <= 255; f += 8, k++)
    {
      if(rom_eeprom == 0)
        cpu_msg->id = cpu_transmit_data_smpl + k;
      else
        {
          cpu_msg->id = cpu_transmit_data_smpl & action;
          cpu_msg->id = cpu_msg->id | 0x00C00000;
          cpu_msg->id = cpu_msg->id + k;
        }
      
      if(shift > 7)
      {
        cpu_msg->data = data_start;
        shift -= 8;
      }
      else if((shift <= 7) && (d_data < (prgr_data + 48)))
      {
        cpu_msg->data = (unsigned char*)d_data;
        d_data += 8;
      }
      else
      {
        cpu_msg->data = data_end;
      }
    CAN1.sendMsgBuf(cpu_msg->id, 1, 8, cpu_msg->data);    
    }
    cpu_msg->message_len = 0;
    cpu_msg->data = NULL;

    //////сообщение о передаче пакета данных образца
    if(rom_eeprom == 0)
      cpu_msg->id = cpu_end_data_smpl + d;
    else
      {
        cpu_msg->id = cpu_end_data_smpl & action;
        cpu_msg->id = cpu_msg->id | 0x00C00000;
        cpu_msg->id = cpu_msg->id + d;
      } 
    
    CAN1.sendMsgBuf(cpu_msg->id, 1, 0, cpu_msg->data);
    }

    //////результат сравнения
    if(rom_eeprom == 0)
      bload_msg->id = bloader_check_result;
    else
      {
        bload_msg->id = bloader_check_result & action;
        bload_msg->id = bload_msg->id | 0x00C00000;
      }
      
    if(random(0, 100) < 85)
    {
      CAN1.sendMsgBuf(bload_msg->id, 1, 0, bload_msg->data);   
    }
    else
    {
      int err = random(1, 7);             // create error code
      err = err << 16;        
      //bload_msg->id = bloader_check_result;
      bload_msg->id = bload_msg->id | err;

      bload_msg->message_len = 8;  
      unsigned int err_data = random(0, 0xFFFF);      // create error data
      bload_msg->data = data_to_out((bloader_check_data_err | err_data), buff);
    
      CAN1.sendMsgBuf(bload_msg->id, 1, bload_msg->message_len, bload_msg->data);
      bload_msg->message_len = 0;
      bload_msg->data = NULL;
    }
  rom_eeprom = !rom_eeprom;
  return 1;
}

/*----------------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------------*/
int convey_control(void)
{
  CAN1.sendMsgBuf(cpu_transfer_ctrl_request, 1, 0, cpu_msg->data);
  CAN1.sendMsgBuf(bloader_tr_ctrl_confrime, 1, 0, bload_msg->data);
  return 1;
}

/*----------------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------------*/
int take_eeprom(void)
{ 
  char* d_data = &prgr_data[0];
  bload_msg->message_len = 8;
  
  for(int k = 0; k < 255; k++)
  {
    cpu_msg->id = cpu_request_read_fl + k;
    CAN1.sendMsgBuf(cpu_msg->id, 1, 0, cpu_msg->data);

    bload_msg->id = bloader_send_data_fl + k;
    bload_msg->data = (unsigned char*)d_data;
    d_data +=8;
    if (d_data > (&prgr_data[47]))
      d_data = &prgr_data[0];
    CAN1.sendMsgBuf(bload_msg->id, 1, bload_msg->message_len, bload_msg->data);  
  }
  bload_msg->message_len = 0;
  bload_msg->data = NULL;

  CAN1.sendMsgBuf(cpu_end_data_fl, 1, 0, cpu_msg->data);
  
  if(random(0, 100) > 70)
    CAN1.sendMsgBuf( bloader_data_fl_err, 1, 0, bload_msg->data);
  return 1;
}
