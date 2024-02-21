
#include "rs485.h"

const char* A485_TAG = "485_MSG";
const char* A485_ERR = "485_ERR";

#define PAKET_SIZE 50
#define header_size sizeof(RS485_header_t)

void RS485::_callback_task(void *arg)
{
  RS485 *self = (RS485 *)arg; 
  while(1)
    {
       //Gelen datanın hazır olmasını bekle 
       xSemaphoreTake(self->callback_sem, portMAX_DELAY);
       //printf("callback\n");
       if (self->paket_receiver==RS485_BROADCAST)
        {
           if (self->broadcast_callback!=NULL) self->broadcast_callback(self->paket_buffer,self->paket_sender,TR_SERIAL); 
        } else {
           if (self->callback!=NULL) self->callback(self->paket_buffer,self->paket_sender,TR_SERIAL);
        }
       xSemaphoreGive(self->wait_sem);
    }
  vTaskDelete(NULL);  
}

void RS485::_sender_task(void *arg)
{
   RS485 *self = (RS485 *)arg;
   Data_t *data ;
   while(1)
   {
      if(xQueueReceive(self->send_queue, &data, (TickType_t)portMAX_DELAY)) 
      {
         //printf("QUEUE OKUNDU GIDECEK DATA : %s\n",data->data);
         self = (RS485 *) data->self;

          //Header oluşturuluyor
          RS485_header_t *head = (RS485_header_t *)malloc(header_size);
          head->sender = self->get_device_id();
          head->receiver = data->receiver;
          head->flag.paket_type = data->paket_type;
          head->flag.paket_ack = 1;
          head->flag.frame_ack = 0;
          head->id = self->paket_counter++; 
          //Paketler Hesaplanıyor 
          uint16_t size = strlen((char*)data->data);
          uint8_t PK_LONG = PAKET_SIZE+header_size+2;
          uint8_t paket_sayisi = (size / PAKET_SIZE) + ((size % PAKET_SIZE) ? 1 : 0);
          head->total_pk = paket_sayisi;
          //uint8_t error = 0;
          for (int i=0;i<paket_sayisi;i++)
          {
              //Paket oluşturuluyor
              head->current_pk = i+1;
              uint16_t start = i*PAKET_SIZE;
              char *bff = (char *) malloc(PK_LONG+3);
              memset(bff,0,PK_LONG+3);
              uint8_t uzn = PAKET_SIZE;
              if (start+PAKET_SIZE>size) uzn=size-(start);
              head->data_len = uzn;
              char *dtt= (char*)malloc(uzn+4);
              memcpy(dtt,data->data+start,uzn);dtt[uzn]=0;
              strcat(dtt,"##");              
              memcpy(bff,head,header_size);
              memcpy(bff+header_size+1,dtt,uzn+2); 
              //Gonderilecek paket header ile birlikte bff içinde
              //printf("Paket %d %d:%d\n", head->id,head->total_pk,head->current_pk);
              //Paket 3 kez gondermeye çalışılacak  
              bool sended = true;
              uint8_t send_counter=0;
              
              while(sended)
              {   
                  send_counter++;
                  uint8_t error = 0;
                  uart_flush(self->get_uart_num());
                  uart_wait_tx_done(self->get_uart_num(), RS485_READ_TIMEOUT); 
                  uint8_t s = uart_write_bytes(self->get_uart_num(), bff, header_size + head->data_len+3);
                  uart_wait_tx_done(self->get_uart_num(), RS485_READ_TIMEOUT); 
                  
                  /*bool col;
                  uart_get_collision_flag(self->get_uart_num(), &col);

                  printf("COLLISION1: %d\n", col);
                  */

                  if (s!=header_size + head->data_len+3) error=1; //gonderilemedi
                  if (error==0)
                    {
                       if (head->receiver!=255)
                       {
                          self->ack_ok=false;
                          xSemaphoreTake(self->ack_sem, 1000 / portTICK_PERIOD_MS);
                          if (!self->ack_ok) {error = 2; break;}
                          if (error==2) printf("ACK ERROR\n");
                       }
                       if (error==0) sended=false;  
                        
                    }
                  if(send_counter>3)
                  {
                    sended=false;
                    self->send_paket_error=1; //3 kez denendi no ack
                  }
              } //While sended
              free(bff); 
              if (self->send_paket_error>0)
                {
                  //break; //for break
                }               
          }//For paket sayısı
         free(data->data);data->data=NULL;
         free(data);data=NULL;
         if (self->send_paket_error>0) printf("paket error %d\n", self->send_paket_error);
         if (self->send_paket_sem!=NULL) 
           xSemaphoreGive(self->send_paket_sem);  
      } //queue
   } //While
   vTaskDelete(NULL);
}

//----------------------------------------------------------
return_type_t RS485::_Sender(Data_t *param)
{  
    return_type_t ret = RET_OK;
    send_paket_sem = xSemaphoreCreateBinary();
    assert(send_paket_sem);
    send_paket_error = 0;
    xQueueSend( send_queue, ( void * ) &param, ( TickType_t ) 0 );
    xSemaphoreTake(send_paket_sem, 2000 / portTICK_PERIOD_MS); //Paketin gönderilmesini bekle
    vSemaphoreDelete(send_paket_sem);
    send_paket_sem=NULL; 
    if(send_paket_error==1) ret=RET_HARDWARE_ERROR;
  return ret;    
}

return_type_t RS485::Sender(const char *data, uint8_t receiver, bool response) 
{
    return_type_t ret = RET_OK;
    if (busy) return RET_BUSY;
    busy=true;
    ESP_LOGI(A485_TAG,"GIDEN >> %d:%s",receiver,data);
    //printf("UART GIDEN %s\n",data);
   
    //while (ARX_Stat(255)) {vTaskDelay(50 / portTICK_PERIOD_MS);}
    Data_t *param = (Data_t *) malloc(sizeof(Data_t));
    param->data = (uint8_t *)malloc(strlen((char*)data)+4);
    strcpy((char*)param->data,(char *)data);
    param->receiver = receiver;
    param->self = (RS485 *)this;
    param->paket_type = PAKET_NORMAL;
    //if (response) param->paket_type = PAKET_RESPONSE;
    ret = _Sender(param);   
    busy=false; 

    return ret;
}    

//----------------------------------------------
bool RS485::paket_decode(uint8_t *data)
{
  bool ret=false;
  RS485_header_t *hd = (RS485_header_t*) malloc(header_size);
  memcpy(hd,data,header_size);

  if (hd->current_pk==1) {
    if (paket_buffer!=NULL) { free(paket_buffer);}
    paket_buffer = (char*) malloc((hd->total_pk * PAKET_SIZE)+2);
    memset(paket_buffer,0,((hd->total_pk * PAKET_SIZE)+2));
    paket_length = 0;
  }
  if (paket_buffer!=NULL) {
      memcpy(paket_buffer+((hd->current_pk-1)*PAKET_SIZE),data+header_size+1,hd->data_len);
      paket_header = (1ULL<<hd->current_pk);
      paket_length += hd->data_len;
      if(pow(2,hd->total_pk)==paket_header) ret=true;
  }
  free(hd);
  return ret;
}
//----------------------------------------------------------

void RS485::_event_task(void *param)
{
 
    RS485 *mthis = (RS485 *)param;
    uart_event_t event;
    size_t buffered_size;
    uint8_t* dtmp = (uint8_t*) malloc(BUF_SIZE);
    for(;;) {      
        if(xQueueReceive(mthis->u_queue, (void * )&event, (TickType_t)portMAX_DELAY)) {
            bzero(dtmp,BUF_SIZE);
            switch(event.type) {
                case UART_DATA:
                {   
                    if (event.size>=header_size) 
                      { 
                        uint8_t *stmp = (uint8_t*) malloc(event.size+2);
                        uart_read_bytes(mthis->get_uart_num(), stmp, event.size, portMAX_DELAY); 
                        RS485_header_t *hd = (RS485_header_t *) malloc(header_size);
                        memcpy(hd,stmp,header_size);
                        if (hd->receiver==mthis->get_device_id() && hd->flag.paket_type==PAKET_NORMAL)
                            {
                              mthis->ack_ok=true;
                              xSemaphoreGive(mthis->ack_sem);
                            };
                        if (hd->receiver==mthis->get_device_id() && hd->flag.paket_type==PAKET_PONG)
                            {
                              mthis->ping_ok=true;
                              xSemaphoreGive(mthis->ping_sem);
                            };    
                        if (hd->receiver==mthis->get_device_id() && hd->flag.paket_type==PAKET_PING)
                        {
                            gpio_set_level(mthis->led,1);
                            hd->flag.paket_type=PAKET_PONG;
                            uint8_t sender = hd->sender;
                            hd->sender = hd->receiver;
                            hd->receiver=sender;
                            uart_write_bytes(mthis->get_uart_num(),hd,sizeof(RS485_header_t));
                            uart_wait_tx_done(mthis->get_uart_num(), RS485_READ_TIMEOUT); 
                            gpio_set_level(mthis->led,0);
                        }     
                        free(hd);
                        free(stmp); stmp=NULL;
                      } else {
                     if (event.size>0) {
                      uart_read_bytes(mthis->get_uart_num(), dtmp, event.size, portMAX_DELAY);     
                      ESP_LOGI(A485_TAG,"   >> %d %s",event.size,dtmp);
                      //for (int i=0;i<event.size;i++) printf("%02X ",dtmp[i]);
                      //printf("\n");
                     }
                      }
                }
                    break;
                case UART_FIFO_OVF:
                {
                    uart_flush_input(mthis->get_uart_num());
                    xQueueReset(mthis->u_queue);
                }
                    break;
                case UART_BUFFER_FULL:
                {
                    uart_flush_input(mthis->get_uart_num());
                    xQueueReset(mthis->u_queue);
                }
                    break;
                case UART_BREAK:                   
                    break;
                //Event of UART parity check error
                case UART_PARITY_ERR:                   
                    break;
                //Event of UART frame error
                case UART_FRAME_ERR:
                    break;    
                //UART_PATTERN_DET
                case UART_PATTERN_DET:
                {     
                    uart_get_buffered_data_len(mthis->get_uart_num(), &buffered_size);                    
                    int pos = uart_pattern_pop_pos(mthis->get_uart_num());
                    if (pos == -1) {
                        uart_flush_input(mthis->get_uart_num());
                    } else  {
                      if (pos>0)
                      {
                        uart_read_bytes(mthis->get_uart_num(), dtmp, pos, 200 / portTICK_PERIOD_MS);
                        RS485_header_t *hd = (RS485_header_t *) malloc(header_size);
                        memcpy(hd,dtmp,header_size);
                        uint8_t sender = hd->sender;
                        uint8_t receiver = hd->receiver;
                        //if(hd->sender==mthis->ping_device) mthis->ping_timer_restart();
                        if (receiver==mthis->get_device_id())
                          {    
                            hd->receiver = hd->sender;
                            hd->sender = mthis->get_device_id();
                            uart_write_bytes(mthis->get_uart_num(),hd,sizeof(RS485_header_t));
                          }
                        
                        //printf("paket geldi %d:%d %d=%d\n",hd->total_pk,hd->current_pk, receiver_id,mthis->get_device_id()); 

                        if (receiver==mthis->get_device_id() || receiver==RS485_BROADCAST)
                           if (mthis->paket_decode(dtmp))
                              {
                                //printf("geldi\n");
                                mthis->paket_sender = sender;
                                mthis->paket_receiver = receiver;
                                xSemaphoreGive(mthis->callback_sem);
                                xSemaphoreTake(mthis->wait_sem, 1000 / portTICK_PERIOD_MS);
                              } 
                        free(hd);    
                        } //if pos
                       //free(hd);     
                      } //else 
                        uint8_t pat[mthis->get_uart_num() + 1];
                        memset(pat, 0, sizeof(pat));
                        uart_read_bytes(mthis->get_uart_num(), pat, RS485_PATTERN_CHR_NUM, 200 / portTICK_PERIOD_MS);
                 } //case                   
     
                break;
              default:
                break;
            }
        } //switch
    } //for
    free(dtmp);
    dtmp = NULL;
    vTaskDelete(NULL);
    
}

void RS485::initialize(RS485_config_t *cfg, rs485_callback_t cb, rs485_callback_t bcb, gpio_num_t ld)
{
    device_id = cfg->dev_num;
    callback = cb; 
    broadcast_callback = bcb;
    uart_number = (uart_port_t)cfg->uart_num;
    led = ld;

    uart_config_t uart_config = {};
        uart_config.baud_rate = cfg->baud;
        uart_config.data_bits = UART_DATA_8_BITS;
        uart_config.parity = UART_PARITY_DISABLE;
        uart_config.stop_bits = UART_STOP_BITS_1;
        uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
        uart_config.rx_flow_ctrl_thresh = 122;

    send_queue = xQueueCreate( 5, sizeof( Data_t *) );
    callback_sem = xSemaphoreCreateBinary();
    assert(callback_sem);
    wait_sem = xSemaphoreCreateBinary();
    assert(wait_sem);
    ack_sem = xSemaphoreCreateBinary();
    assert(ack_sem);
    ping_sem = xSemaphoreCreateBinary();
    assert(ping_sem);
    
    OE_PIN = cfg->oe_pin;

    ESP_ERROR_CHECK(uart_driver_install(uart_number, BUF_SIZE * 2, BUF_SIZE * 2, 20, &u_queue, 0));
    ESP_ERROR_CHECK(uart_param_config(uart_number, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(uart_number, cfg->tx_pin, cfg->rx_pin, cfg->oe_pin, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_set_mode(uart_number, UART_MODE_RS485_HALF_DUPLEX));
    
    ESP_ERROR_CHECK(uart_set_rx_timeout(uart_number, RS485_READ_TIMEOUT));
    uart_enable_pattern_det_baud_intr(uart_number, '#', RS485_PATTERN_CHR_NUM, 9, 0, 0);
    uart_pattern_queue_reset(uart_number, 20);
    
    xTaskCreate(_sender_task, "sendtask", 4096, (void *) this, 1, &SenderTask);
    xTaskCreate(_event_task, "uart_event_task", 4096, (void *)this, 12, &ReceiverTask);
    xTaskCreate(_callback_task, "callbacktask", 4096, (void *) this, 1, NULL);
} 

bool RS485::ping(uint8_t dev)
{
     bool ret = true;
     RS485_header_t *hd = (RS485_header_t *)malloc(header_size);
     hd->flag.paket_type = PAKET_PING;
     hd->sender = get_device_id();
     hd->receiver = dev;
     hd->id = 0;
     hd->total_pk = 1;
     hd->current_pk = 1;
     hd->data_len = 0;
     int hh = uart_write_bytes(get_uart_num(),hd,header_size);
     if (hh!=header_size) ret = false;
     if (ret)
       {  
          ping_ok = false;
          xSemaphoreTake(ping_sem, 1000 / portTICK_PERIOD_MS);
          //printf( "PING OK %d\n",ping_ok);
          if (!ping_ok) ret=false;
       }
      free(hd);
      return ret;
}
