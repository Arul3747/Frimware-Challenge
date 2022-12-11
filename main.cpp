#include <stdio.h>
#include <stdlib.h>

#define Idle        0
#define CC          1
#define CV          2

#define start charging 201
float Iref, Vref, Imin, Vfeed; //receiving  from the bms 
bool Enable_command = false;
int State_charger; // initializing it to IDLE state
int Charging_status;
int Battery_connected;

typedef struct {
uint8_t Data[8];
uint16_t Length;
uint32_t ID;
} CAN_msg_typedef;

CAN_msg_typedef Can_tx;
CAN_msg_typedef Can_rx;
void CAN_write(CAN_msg_typedef *msg);
bool CAN_read(CAN_msg_typedef *msg); //return true if there is received msg
uint32_t time_ms;
void Initialization(void)
{
     Iref= Vref= Imin = Vfeed = 0 ; //receiving these values from the bms through input can message
    Enable_command = false;
    State_charger = 0; // initializing it to IDLE state
    Charging_status = 0; // not charging
    Battery_connected = 0; //battery not connected
}

void control_routine(void)
{
    CAN_read_handle();
    
    if (State_charger == 0 && Enable_command == false)
    {
        State_charger = 0; //stay in Idle state
    }
    if (Enable_command == true)
    {
        if (State_charger == 0 )
        {
            State_charger = 1; //change the state of the charger to CC
        }
        if(State_charger == 1 && Vfeed == Vref)
        {
            State_charger = 2; // change the state of charger to CV
        }
        if (State_charger ==2 && Iref == Imin)
        {
            Enable_command = false;
            State_charger = 0; // back to IDLE
        }
    }
    time_ms++; //assume INT frequency is 1kHz, for timing purpose
}
void main_state_machine(void)
{
    if (Enable_command == false)
    {
        State_charger = 0;
    }
    
    if (Enable_command == true)
    {
        if (Charging_status == 0) //if charging stops go to idle mode
        {
            Enable_command = false;
            State_Charger = 0;
        }
            
        else if (Charging_status == 1)
        {
            //start to read the incoming messages
                CAN_read_handler();
            //send outgoing messages every 200ms 
            CAN_write(0x20110101); // charging 
            CAN_write(0x7012); //operational mode
        }
    }
}
void CAN_write_handler(void)
{
    if (time_ms <= 200)
    {
        if (Battery_connected == 1) // can message
        {
            if (Charging_status == 0)
                CAN_write(0x20110100);//not charging
            else 
                CAN_write(0x20110101);//charging
        }
    }
    //send heartbeat message every 1000ms
    if (time_ms <= 1000)
    {
        if (Enable_command == false)
            CAN_write(0x7010);
        else if (Enable_command == true && Charging_status == 0)
            CAN_write(0x7011);
        else if (Enable_command == true && Charging_status == 1)
            CAN_write(0x7012);
    }
    
    time_ms = 0; //clearing the clock 
}
void CAN_read_handler(void)
{
    //reading incoming message with CAN ID 0x181 to get Iref,Vref
    if (CAN_read == true)
    {
        // bytes 0 & 1 give Voltage feedback
        Vfeed = (float) CAN_msg[0] | CAN_msg[1]; 
        //bytes 2 & 3 give current feedback
        Ifeed = (float)CAN_msg[2] | CAN_msg[3];
        //charging status 
        Charging_status = (int) CAN_msg[4];
    }
    else
        //stop message
}
void network_management(void)
{
     if (Enable_command == false)
     {
            CAN_write(0x7011); // pre-operational 
     }
     
     if(Enable_command == true)
     {
         if (Charging_status == 1)
         {
             if(time_ms >=5000 && CAN_read == false)
             {
                 //stop the heart beat messages
             }
         }
     }
    
}
void main(void)
{
    Initialization();
    PieVectTable.EPWM1_INT = &control_routine;
    while(true){
    main_state_machine();
    network_management();
}

