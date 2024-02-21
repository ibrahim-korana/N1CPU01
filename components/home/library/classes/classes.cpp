


#include "classes.h"

Base_Port::Base_Port() 
{
    name=(char*)calloc(1,20);
    strcpy(name,"NO_NAME");
   // func_callback = NULL;
};

//arg üzerinden port, usr üzerinden fonksiyon gelir
void Base_Port::inportscall(void *arg, void *usr)
{
    //printf("classes inport action\n");

   //button_handle_t *btn = (button_handle_t *) arg;
   Base_Port *prt = (Base_Port *) usr;
   //button_event_t evt = iot_button_get_event(btn);

   if (prt->port_action_callback!=NULL)
    {
        port_action_t rt = {};
        rt.action_type = ACTION_INPUT;
        rt.port = arg;
        prt->port_action_callback(prt->base_function,rt);
   }
} 
void Base_Port::outportscall(void *arg, void *usr)
{
   //Base_Function *fnc = (Base_Function *) usr;
   //arg da button var
   Base_Port *prt = (Base_Port *) usr;
   if (prt->port_action_callback!=NULL)
   {
    port_action_t rt = {};
    rt.action_type = ACTION_OUTPUT;
    rt.port = arg;
    prt->port_action_callback(prt->base_function,rt);
   }
} 

void Base_Port::set_active(bool dr)
{
    bool notok = true; 
    if ((type==PORT_FIRE || type==PORT_GAS || type==PORT_EMERGENCY || type==PORT_WATER) && !dr) notok=false;
    if (notok)
      {
        active = dr;
        if (outport!=NULL) iot_out_set_disable(outport,!active);
        if (inport!=NULL) iot_button_set_disable(inport,!active);
      }
}

void Base_Port::set_active_admin(bool dr)
{
        active = dr;
        if (outport!=NULL) iot_out_set_disable(outport,!active);
        if (inport!=NULL) iot_button_set_disable(inport,!active);
}

void Base_Port::set_user_active(bool dr)
{
    user_active = dr;
}

bool Base_Port::set_status(bool st)
{
    if (active)
    {
        if (type==PORT_OUTPORT)
        {
            status = st;
            if (outport!=NULL) {
                status=iot_out_set_level(outport,status);
                outportscall((void *) outport, iot_out_get_user_data(outport)); 
            }    
        }
    }
    return status;  
}

bool Base_Port::get_hardware_status(void)
{
    if (type==PORT_OUTPORT)
      {
        status=iot_out_get_level(outport);
        return status;
      }
    return false;  
}

bool Base_Port::set_toggle(void)
{
    if (active)
    {
        if (type==PORT_OUTPORT) 
          {
             if (outport!=NULL) status=iot_out_set_toggle(outport);
          }
    }
    return status;
}

bool Base_Port::set_port_type(port_type_t tp, void *usr_data)
{
    type = tp;
    if (!virtual_port)
    { 
        if (tp==PORT_OUTPORT) {
            if (outport!=NULL) {
                          iot_out_register_cb(outport, &outportscall, (void *)this);
                               }
            return true;
        } else {
            if (inport!=NULL) {
                if (tp==PORT_INPORT || tp==PORT_SENSOR || tp==PORT_FIRE || tp==PORT_WATER || tp==PORT_GAS || tp==PORT_EMERGENCY ) {
                     iot_button_register_cb(inport, BUTTON_PRESS_DOWN, &inportscall, (void *)this);
                     iot_button_register_cb(inport, BUTTON_PRESS_UP, &inportscall, (void *)this);
                     iot_button_register_cb(inport, BUTTON_LONG_PRESS_START, &inportscall, (void *)this);
                                     }
                if (tp==PORT_INPULS) {
                     iot_button_register_cb(inport, BUTTON_PRESS_UP, &inportscall, (void *)this);
                     iot_button_register_cb(inport, BUTTON_LONG_PRESS_START, &inportscall, (void *)this);
                                     }                     
            }
            return true;
        }
    }
    return false;
}

//================================================================

