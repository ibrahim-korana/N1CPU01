
#include "rs485.h"

const char* RS485_TAG = "RS485_MSG";
const char* RS485_ERR = "RS485_ERR";

#define PAKET_SIZE 50
#define header_size sizeof(RS485_header_t)

void RS485::send_fin(uint8_t receiver)
{
  RS485_header_t *head = (RS485_header_t *)malloc(header_size);
  head->sender = get_device_id();
  head->receiver = receiver;
  head->flag.paket_type = PAKET_FIN;
  head->flag.paket_ack = 0;
  head->flag.frame_ack = 0;
  head->id = paket_counter++; 
  uart_write_bytes(get_uart_num(),head,header_size);
  free(head);
}

void RS485::_callback_task(void *arg)
{
  RS485 *self = (RS485 *)arg;
  while(1)
    {
       //Gelen datanın hazır olmasını bekle 
       xEventGroupClearBits(self->RSEvent, Xb9);       
       xEventGroupWaitBits(self->RSEvent,Xb9,pdFALSE,pdFALSE,(TickType_t)portMAX_DELAY);
       if (self->callback!=NULL) self->callback(self->paket_buffer,self->paket_sender,TR_SERIAL, self->paket_response);
    }
  vTaskDelete(NULL);  
}

void RS485::_sender_task(void *arg)
{
   RS485 *self = (RS485 *)arg;
   Data_t *data ;

   while(1)
   {
      if(xQueueReceive(self->s_queue, &data, (TickType_t)portMAX_DELAY)) 
      {
         xEventGroupClearBits(self->RSEvent, Xb0 | Xb1 | Xb2 | Xb3 | Xb4 | Xb5 | Xb6);
         //printf("GIDECEK DATA : %s\n",data->data);
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
          uint8_t error = 0;
          for (int i=0;i<paket_sayisi;i++)
          {
              //Paket oluşturuluyor
              head->current_pk = i+1;
              uint16_t start = i*PAKET_SIZE;
              char *bff = (char *) malloc(PK_LONG);
              memset(bff,0,PK_LONG);
              uint8_t uzn = PAKET_SIZE;
              if (start+PAKET_SIZE>size) uzn=size-(start);
              head->data_len = uzn;
              memcpy(bff,head,header_size);
              memcpy(bff+header_size+1,data->data+start,uzn); 
              //Gonderilecek paket header ile birlikte bff içinde
              //printf("Paket %d %d:%d\n", head->id,head->total_pk,head->current_pk);
              //Paket 3 kez gondermeye çalışılacak  
              bool sended = true;
              uint8_t send_counter=0;
              
              while(sended)
              {   
                  send_counter++;
                  uart_flush(self->get_uart_num());
                  uart_wait_tx_done(self->get_uart_num(), RS485_READ_TIMEOUT); 
                  uint8_t s = uart_write_bytes(self->get_uart_num(), bff, header_size + head->data_len+1);
                  uart_wait_tx_done(self->get_uart_num(), RS485_READ_TIMEOUT); 
                  uart_write_bytes(self->get_uart_num(), "##", 2); 
                  uart_wait_tx_done(self->get_uart_num(), RS485_READ_TIMEOUT);
                  bool collision = false; 
                  if (self->OE_PIN!=255) uart_get_collision_flag(self->get_uart_num(), &collision); 
                  if (collision) error=1; //Collesion var
                  if (s!=header_size + head->data_len+1) error=2; //gonderilemedi
                  if (error==0 && head->receiver==RS485_BROADCAST) sended=false;        
                  if (error==0 && head->receiver!=RS485_BROADCAST)
                    {
                       //ack beklenecek
                       //printf("ACK BEKLIYOR\n");
                       Paket_t *pk ;
                       //uint8_t ww = uxQueueSpacesAvailable(self->p_queue);
                       //printf("queue Boslugu %d\n",ww);
                      
                       if(xQueueReceive(self->p_queue, &pk, 1000 / portTICK_PERIOD_MS)) 
                        {
                          xQueueReset(self->p_queue);
                          //printf("ACK %d %d %d:%d\n", pk->header->flag.paket_type,pk->header->id,pk->header->total_pk,pk->header->current_pk);
                          if(pk->header->flag.paket_type==PAKET_ACK) 
                           {
                            //printf("ACK %d=%d %d:%d\n", pk->header->id,head->id,pk->header->current_pk,head->current_pk);
                            //if (pk->header->id==head->id && pk->header->current_pk==head->current_pk) sended = false;
                            sended = false;
                            free(pk->header);pk->header=NULL; 
                            free(pk->data);pk->data=NULL;
                            free(pk);pk=NULL;
                           }

                        } else ESP_LOGE(RS485_ERR,"ACK TIMEOUT");
                        
                    }
                  if(send_counter>3)
                  {
                    sended=false;
                    error=3; //3 kez denendi no ack
                  }
              } //While sended
              if (error>0)
                {
                  if (error==1) xEventGroupSetBits(self->RSEvent, Xb1);
                  if (error==2) xEventGroupSetBits(self->RSEvent, Xb2); 
                  if (error==3) xEventGroupSetBits(self->RSEvent, Xb3); 
                  break;
                } 
              free(bff); 
          }//For paket sayısı
         if (error==0) xEventGroupSetBits(self->RSEvent, Xb0);
         free(data->data);data->data=NULL;
         free(data);data=NULL;
         xEventGroupSetBits(self->RSEvent, Xb6); 
      } //queue
   } //While
   vTaskDelete(NULL);
}

//----------------------------------------------------------
return_type_t RS485::_Sender(Data_t *param)
{  
    return_type_t ret = RET_OK;
    xQueueReset(p_queue);
    xQueueSend( s_queue, ( void * ) &param, ( TickType_t ) 0 );
    EventBits_t uxBits = xEventGroupWaitBits(RSEvent,Xb6,pdFALSE,pdFALSE, 1000 / portTICK_PERIOD_MS );
    if( ( uxBits & Xb0 ) != 0 ) ret = RET_OK;
    if( ( uxBits & Xb3 ) != 0 ) ret = RET_NO_ACK;
    if( ( uxBits & Xb2 ) != 0 ) ret = RET_HARDWARE_ERROR; 
    if( ( uxBits & Xb1 ) != 0 ) ret = RET_COLLESION; 

    //Xb5i bekle timeoutta response yoktur 
    //xEventGroupClearBits(RSEvent, Xb5 );  
    //uxBits = xEventGroupWaitBits(RSEvent,Xb5,pdFALSE,pdFALSE, 100 / portTICK_PERIOD_MS );
    //if ((uxBits&Xb5)!=0) uxBits = xEventGroupWaitBits(RSEvent,Xb4,pdFALSE,pdFALSE, 500 / portTICK_PERIOD_MS );
    //printf("ux %d RET %d\n-------------\n",uxBits, ret);   
  return ret;    
}

return_type_t RS485::Sender(const char *data, uint8_t receiver, bool response) 
{
    return_type_t ret = RET_OK;
    if (!response)
       if (wait) return RET_BUSY;
    wait = true;
    //printf("--------------------\n     BASLADI\n-------------------\n");
    Mode = SENDER;
    Data_t *param = (Data_t *) malloc(sizeof(Data_t));
    param->data = (uint8_t *)malloc(strlen((char*)data)+1);
    strcpy((char*)param->data,(char *)data);
    param->receiver = receiver;
    param->self = (RS485 *)this;
    param->paket_type = PAKET_NORMAL;
    if (response) param->paket_type = PAKET_RESPONSE;
    ret = _Sender(param); 
    //500ms fin gelmesini bekler gelmezse normale doner   
   xEventGroupClearBits(RSEvent, Xb5 ); 
   if (!response) xEventGroupWaitBits(RSEvent,Xb5,pdFALSE,pdFALSE, 500 / portTICK_PERIOD_MS );
   Mode = RECEIVER;
   wait = false;
   //printf("--------------------\n     BITTI\n-------------------\n");
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
void RS485::pong_start(uint8_t dev, ping_reset_callback_t cb, int ld)
{
    pong_reset_callback = cb;
    pong_device = dev;
    PING_LED = (gpio_num_t) ld;
    esp_timer_create_args_t arg = {};
    arg.callback = &pong_timer_callback;
    arg.arg = (void *) this;
    arg.name = "ptim2";
    ESP_ERROR_CHECK(esp_timer_create(&arg, &pong_tim)); 
    pong_timer_start();
}
void RS485::pong_timer_start(void)
{
    ESP_ERROR_CHECK(esp_timer_start_periodic(pong_tim, PONG_TIMEOUT));
}
void RS485::pong_timer_stop(void)
{
    if (esp_timer_is_active(pong_tim)) esp_timer_stop(pong_tim);
}
void RS485::pong_timer_restart(void)
{
   pong_timer_stop();
   pong_timer_start();
}
void RS485::pong_timer_callback(void *arg)
{
  RS485 *mthis = (RS485 *) arg; 
  mthis->pong_error_counter++;
  if (mthis->pong_error_counter>5) {
    if (mthis->pong_reset_callback!=NULL) mthis->pong_reset_callback(mthis->pong_error_counter);
  }
}
//----------------------------------------------------------
bool RS485::ping(uint8_t dev)
{
     bool ret = true;
     RS485_header_t *hd = (RS485_header_t *)malloc(header_size);
     hd->flag.paket_type = PAKET_PING;
     hd->sender = get_device_id();
     hd->receiver = dev;
     hd->id = ping_counter++;
     hd->total_pk = 1;
     hd->current_pk = 1;
     hd->data_len = 0;
     xEventGroupClearBits(RSEvent, Xb8);
     int hh = uart_write_bytes(get_uart_num(),hd,header_size);
     if (hh!=header_size) ret = false;
     if (ret)
       { 
          EventBits_t uxBits = xEventGroupWaitBits(
                    RSEvent,  
                    Xb8, 
                    pdFALSE,        
                    pdFALSE,       
                    150 / portTICK_PERIOD_MS );
          if( ( uxBits & Xb8 ) == 0 ) ret=false;
       }
       free(hd);
      return ret;
}

void RS485::ping_timer_start(void)
{
    ESP_ERROR_CHECK(esp_timer_start_periodic(ping_tim, PING_TIMEOUT));
}
void RS485::ping_timer_stop(void)
{
    if (esp_timer_is_active(ping_tim)) esp_timer_stop(ping_tim);
}
void RS485::ping_timer_restart(void)
{
   ping_timer_stop();
   ping_timer_start();
}
void RS485::ping_start(uint8_t dev, ping_reset_callback_t cb, int ld)
{
        ping_device = dev;
        ping_reset_callback = cb;
        PING_LED = (gpio_num_t) ld;
        esp_timer_create_args_t arg = {};
        arg.callback = &ping_timer_callback;
        arg.arg = (void *) this;
        arg.name = "ptim0";
        ESP_ERROR_CHECK(esp_timer_create(&arg, &ping_tim)); 
        ping_timer_start();
}

void RS485::ping_timer_callback(void *arg)
{
   RS485 *mthis = (RS485 *) arg; 
   if (mthis->PING_LED!=-1) gpio_set_level(mthis->PING_LED, 1); 
   if (mthis->ping(mthis->ping_device))
    {
         mthis->ping_error_counter = 0;
    } else {
        if (++mthis->ping_error_counter>5) 
        {
          if (mthis->ping_reset_callback!=NULL) mthis->ping_reset_callback(mthis->ping_error_counter);
          //mthis->ping_error_counter = 0;
        }
       }
     if (mthis->PING_LED!=-1) gpio_set_level(mthis->PING_LED, 0); 
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
                    uart_read_bytes(mthis->get_uart_num(), dtmp, event.size, portMAX_DELAY); 
                    dtmp[event.size]=0;
                    if (event.size>=header_size)
                     {
                      RS485_header_t *hd = (RS485_header_t *) malloc(header_size);
                      memcpy(hd,dtmp,header_size);
                      if(mthis->ping_device>0) 
                        if(hd->sender==mthis->ping_device) mthis->ping_timer_restart();

                      //printf("%d %d %d %d:%d\n",mthis->Mode,hd->flag.paket_type,hd->id, hd->total_pk,hd->current_pk);
                      if (mthis->Mode==SENDER)
                        {
                            if(hd->flag.paket_type==PAKET_ACK)
                              {
                                //printf("%d %d %d:%d\n",hd->flag.paket_type,hd->id, hd->total_pk,hd->current_pk);
                                Paket_t *pk = (Paket_t *)malloc(sizeof(Paket_t));
                                RS485_header_t *hd0 = (RS485_header_t *) malloc(header_size);
                                memcpy(hd0,hd,header_size);
                                char *bf = (char *)malloc(hd->data_len+2);
                                memcpy(bf,dtmp+header_size+1,hd->data_len);
                                bf[hd->data_len] = 0;
                                pk->header = hd0;
                                pk->data = (uint8_t *)bf;
                                //printf("ack\n");
                                if (pk->header->id>0)
                                   xQueueSend( mthis->p_queue, ( void * ) &pk, ( TickType_t ) 0 );
                              }  
                            if(hd->flag.paket_type==PAKET_FIN) 
                              {
                                xEventGroupSetBits(mthis->RSEvent, Xb5);
                              } 
                        }
                      if (hd->flag.paket_type==PAKET_PING && hd->receiver==mthis->get_device_id())
                        {
                            if (mthis->PING_LED!=-1) gpio_set_level(mthis->PING_LED, 1);
                            if (mthis->pong_device==hd->sender) mthis->pong_timer_restart();
                            RS485_header_t *hd0 = (RS485_header_t *)malloc(header_size);
                            hd0->flag.paket_type = PAKET_PONG;
                            hd0->sender = hd->receiver;
                            hd0->receiver = hd->sender;
                            hd0->id = hd->id;
                            hd0->total_pk = 1;
                            hd0->current_pk = 1;
                            hd0->data_len = 0;
                            uart_write_bytes(mthis->get_uart_num(),hd0,header_size);
                            if (mthis->PING_LED!=-1) gpio_set_level(mthis->PING_LED, 0);
                        }  
                      if (hd->flag.paket_type==PAKET_PONG) xEventGroupSetBits(mthis->RSEvent, Xb8);
                                                
                      free(hd);
                    }          
                    //ESP_LOGI(RS485_TAG,"   >> %d %s",event.size,dtmp);
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
                        //--- Header ayrılıyor ---------
                        RS485_header_t *hd = (RS485_header_t *) malloc(header_size);
                        memcpy(hd,dtmp,header_size);
                        if (hd->receiver == mthis->get_device_id() || hd->receiver==RS485_BROADCAST)
                        {    
                          //if(hd->flag.paket_type==PAKET_RESPONSE) xEventGroupSetBits(mthis->RSEvent, Xb5);                                        
                          //--- data ayrılıyor ---------
                          char *bf = (char *)malloc(hd->data_len+2);
                          memcpy(bf,dtmp+header_size+1,hd->data_len);
                          bf[hd->data_len] = 0;
                          //------ack gidecek ---------
                          if (hd->receiver!=RS485_BROADCAST)
                            {
                              vTaskDelay(50 / portTICK_PERIOD_MS );
                              //printf("Alici %d Gonderici %d \n",hd->sender,mthis->get_device_id());
                              RS485_header_t *hd0 = (RS485_header_t *)malloc(header_size);
                              memcpy(hd0,hd,header_size);
                              hd0->flag.paket_type = PAKET_ACK;
                              hd0->receiver = hd->sender;
                              hd0->sender = mthis->get_device_id();
                              uart_write_bytes(mthis->get_uart_num(),hd0,header_size);
                              uart_wait_tx_done(mthis->get_uart_num(), RS485_READ_TIMEOUT); 
                              free(hd0);
                            }
                            //printf("%s\n",bf);
                           if (mthis->paket_decode(dtmp))
                              {
                                mthis->paket_sender = hd->sender;
                                if (hd->flag.paket_type==PAKET_RESPONSE) mthis->paket_response = true; else mthis->paket_response=false;
                                xEventGroupSetBits(mthis->RSEvent, Xb9);
                              } 
                          
                          free(bf);
                        }
                       free(hd);     
                      }  
                        uint8_t pat[mthis->get_uart_num() + 1];
                        memset(pat, 0, sizeof(pat));
                        uart_read_bytes(mthis->get_uart_num(), pat, PATTERN_CHR_NUM, 200 / portTICK_PERIOD_MS);
                    }                    
                }
                break;
              default:
                break;
            }
        }
    }
    free(dtmp);
    dtmp = NULL;
    vTaskDelete(NULL);
}

void RS485::initialize(RS485_config_t *cfg, uart_transmisyon_callback_t cb, int baud)
{
    device_id = cfg->dev_num;
    callback = cb; 
    uart_number = cfg->uart_num;
    uart_config_t uart_config = {};
        uart_config.baud_rate = baud;
        uart_config.data_bits = UART_DATA_8_BITS;
        uart_config.parity = UART_PARITY_DISABLE;
        uart_config.stop_bits = UART_STOP_BITS_1;
        uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
        uart_config.rx_flow_ctrl_thresh = 122;
    ESP_ERROR_CHECK(uart_driver_install(uart_number, BUF_SIZE * 2, BUF_SIZE * 2, 20, &u_queue, 0));
    ESP_ERROR_CHECK(uart_param_config(uart_number, &uart_config));
    OE_PIN = cfg->oe_pin;
    if (cfg->oe_pin!=255) {
      ESP_ERROR_CHECK(uart_set_pin(uart_number, cfg->tx_pin, cfg->rx_pin, cfg->oe_pin, UART_PIN_NO_CHANGE));
      ESP_ERROR_CHECK(uart_set_mode(uart_number, UART_MODE_RS485_HALF_DUPLEX));  //UART_MODE_RS485_COLLISION_DETECT  UART_MODE_RS485_HALF_DUPLEX
    } else {
      ESP_ERROR_CHECK(uart_set_pin(uart_number, cfg->tx_pin, cfg->rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
      ESP_ERROR_CHECK(uart_set_mode(uart_number, UART_MODE_UART));
    }

    ESP_ERROR_CHECK(uart_set_rx_timeout(uart_number, RS485_READ_TIMEOUT));
    uart_enable_pattern_det_baud_intr(uart_number, '#', PATTERN_CHR_NUM, 9, 0, 0);
    uart_pattern_queue_reset(uart_number, 20);
    RSEvent = xEventGroupCreate();
    xEventGroupClearBits(RSEvent, Xb0 | Xb1 | Xb2 | Xb3);
    s_queue = xQueueCreate( 5, sizeof( Data_t *) );
    p_queue = xQueueCreate( 5, sizeof( Paket_t *) );
    
    xTaskCreate(_sender_task, "sendtask", 4096, (void *) this, 1, &SenderTask);
    xTaskCreate(_event_task, "uart_event_task", 4096, (void *)this, 12, &ReceiverTask);
    xTaskCreate(_callback_task, "callbacktask", 4096, (void *) this, 1, NULL);
} 
