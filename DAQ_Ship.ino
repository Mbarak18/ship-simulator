 
//SECTION A: Declare Structures

 #include <Ethernet.h>
 #include <SPI.h>
 #include <string.h>
 #include <Statistic.h>
 Statistic tunervoltage;
 #define Vessel_ID 0 
 
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
byte ip[] = { 10, 10, 10, 15 };        //IP Address of Arduino PLC
byte server[] = { 10, 10, 10,1};     // IP Adress of Vessel Work Station
EthernetClient client;

struct Pushbuttons {                  //for pushbuttons
                    int sigID;
                    int sigNO;
                    };   
                           
struct twowaysw   {                  //for non-latch two-way switch
                   int sigID;
                   int sigNO;
                   bool swflag;
                    };

struct fourwaysw  {                  //for latching 4-way switch
                    int sigID[4];
                    int sigNO;  
                    int currPosition;
                    };
fourwaysw currswitch;
struct DAQ_AI      {
                    int sigID;
                    int sigNO;
                    int currPosition;
                    float m,c,Supvoltage; 
                    int iteration;      //averaged readings
                    bool outputType;    //high = absolute, low = delta
                    };

struct DAQ_DO       {
                      int sigNO;
                      int ActivityType;
                     };
struct All_Lamps     {
                       int sigID;
                       int sigNO;
                      };


struct DO_Input   {
                   int DO_Instr;
                   int DO_Param;
                    };

char nextchar, tableIndex = 0;
int currval= 0,buf_size, Input_data[10];
void get_input ();

int DI_List[20],
    DI_Kounter;
//int testlamp;
//int testkount = 0; 
     
     
//SECTION B: Prepare database for signals
        
 Pushbuttons Pb[] = {
                     {I0_1,3},       // BRIDGE DESK ANN Ack/Reset PB    
                    //{I0_5,37},      // Chart Room Selector Switch:IN
                     //{I0_6,38},      // Chart Room Selector Switch:OUT
                     {I1_10,27},     // NFU TILLER Selector Switch:STARBORD
                     {I1_11,28},     // BOW THRUSTER Selector Switch:PORT
                     {I1_12,30},     // BOW THRUSTER Selector Switch:STARBORD
                     {I1_9,25},      // NFU TILLER Selector Switch:PORT
                     {I2_1,33},      // Display Unit 6-inch Selector Switch: PREV
                     {I2_10,36},     // Display Unit 15-inch Selector Switch: NEXT 
                     {I2_2,34},      // Display Unit 6-inch Selector Switch: NEXT 
                     {I2_3,35},      // Display Unit 15-inch Selector Switch: PREV
                     
                      };
                      

                 int Pb_Size = sizeof (Pb)/ sizeof (Pushbuttons);
 twowaysw dpdt[] = {
                    {I0_0,1,HIGH}, //BRIDGE DESK Test Lamp Off PB
                    {I0_7,11,HIGH},//Ship Bell with NO/NC RelaySelector Switch: OFF
                    {I0_4,9,HIGH}, //Ship Horn with NO/NC relay Selector Switch:OFF
                    {I0_2,5,HIGH}, //Ship Lamp Selector Switch :  OFF   
                    {I0_3,7,HIGH}, //Anchor Selector Switch :  DOWN   
                    {I2_0,31,HIGH},//Lubricator Selector Switch :  NORMAL   
                      };
                   int dpdt_size = sizeof (dpdt)/sizeof(twowaysw);
 
 fourwaysw latching [] = {
                             {{I0_12, I1_0, I1_1, I1_2},17,0}, //System Mode  Selector Switch :  OFF   
                             {{I0_8, I0_9, I0_10, I0_11},13,0}, //Generator   Selector Switch :  OFF   
                             {{I1_3, I1_4, I1_7, I1_8},21,0}, //FU Control Mode  Selector Switch :  OFF   
                       };
 
 int latching_size = sizeof (latching)/ sizeof (fourwaysw); 

 int LAMPLIST[50];

All_Lamps DO[] =    {
                      {Q0_0,2},        // BRIDGE DESK Power On Process LED
                      {Q0_1,12},       // BRIDGE Engine Mode Panel: ME READY TO START  Process LED
                      {Q0_2,13},       // BRIDGE Engine Mode Panel: SLOW TURNING Process LED
                      {Q0_3,14},       // BRIDGE Engine Mode Panel: START VALVE IN SERVICE Process LED
                      {Q0_4,15},       // BRIDGE Engine Mode Panel: LUBRICATING SETTING Process LED
                      {Q0_5,16},       // BRIDGE Engine Mode Panel: REVERSE AHEAD Process LED
                      {Q0_6,17},       // BRIDGE Engine Mode Panel: REVERSE ASTERN Process LED
                      {Q0_7,18},       // BRIDGE Engine Mode Panel: GEAR ENGAGED Process LED
                      {Q1_0,19},       // BRIDGE Engine Mode Panel: GEAR DISENGAGED Process LED
                      {Q1_1,20},       // BRIDGE Engine Mode Panel: AUX. BLOWER OFF Process LED
                      {Q1_2,21},       // BOW THRUSTER DIR:PORT Process LED
                      {Q1_3,22},       // BRIDGE THRUSTER DIR: STARBOARD Process LED
                      {Q1_4,23},       // BRIDGE NFU Tiller Direction:PORT Process LED
                      {Q1_5,24},       // BRIDGE NFU TIller Direction: STARBOARD Process LED
                      {Q1_6,25},       // BRIDGE Alarm Panel: ME TRIPPED Process LED
                      {Q1_7,26},       // BRIDGE Alarm Panel: ME ALARM Process LED
                      {Q2_0,27},       // BRIDGE Alarm Panel: ME OVER SPEED Process LED
                      {Q2_1,28},       // BRIDGE Alarm Panel: ME OVER LOAD Process LED
                      {Q2_2,29},       // BRIDGE Alarm Panel: SYSTEM 1 LOW PRESS Process LED
                      {Q2_3,30},       // BRIDGE Alarm Panel: SYSTEM 2 LOW PRESS Process LED
                      {Q2_5,31},       // BRIDGE Alarm Panel: ENGINE SLOWDOWN Process LED
                      //{Q2_5,32},       // BRIDGE Alarm Panel: POWER FAILURE Process LED
                      {Q2_6,33},       // BRIDGE Alarm Panel: FUEL LEVEL LOW Process LED
                      };
            
int DO_size = sizeof (DO)/ sizeof(DO[0]);

int Volt_Val[10];

DAQ_AI Tuner[] = {
                     {I2_7,1,0,0.093,0.33,5.0,2,HIGH}, // Throttle  
               {I2_8,2,0,5.3444,-415,4.81, 10, HIGH}, // handwheel MVO1
                  // {I2_8,2,0,5.02,-215.15,4.88,10,HIGH}, // handwheel MVO2
                    }; 
int Tuner_Kounter = sizeof(Tuner)/sizeof(Tuner[0]);

DAQ_AI Voltage_Feedback = {I2_9,3,0,0,0};
DAQ_DO ActiveDO [50];
int DO_Kounter = 0;
int  DAQ_Position(DAQ_AI);
int i,j,k,l,b,d;              //global variables
bool flag,blinkflag=false;
void DAQ_OUT(int,int);
void DAQ_PB();
void DAQ_DPDT();
void DAQ_SW();
void DAQ_DO(); 
void DAQ_Tuners();
int arraySearch(int);
int arraySearch( int sigNo)
{ int y = 0;
 
  do
   {
   }
    while(ActiveDO[y++].sigNO!=sigNo&&y<DO_Kounter);
if (y > DO_Kounter)
    return -99;
else return y-1;
}

 void get_Input()
 {
  tableIndex = 0;
  do
  {
    if(nextchar== ',')
    {
      Input_data[tableIndex++] = currval;
      currval = 0;
    }
    else currval = currval*10+ nextchar - 48;
    nextchar = client.read();  
      }
      while (client.available()&&nextchar>-1);
    }

void DAQ_OUT(int DAQ_Type, int sigNO)
{
char str_each[20],str_each1[20];
char char_out[50];
itoa(sigNO,str_each,10);
buf_size = 20;  
itoa(buf_size+1000,char_out,10); 
itoa (DAQ_Type, str_each1, 10);

 strcat(char_out,str_each1);
 strcat(char_out,",");
 strcat(char_out,str_each);
 strcat(char_out,",   ");
client.write(char_out,15);
 /*testlamp = DO[testkount++%DO_size].sigNO;
 digitalWrite(testlamp,HIGH);
 delay(500);
digitalWrite(testlamp,LOW);
*/
}
//SECTION E4: LAMP CONTROL
void lampTestStart();
void lampTestStop();
 
DO_Input LAMP;


/*void get_do_command();
  Pushbuttons PbInterrupt[] = {
      {I0_5,11},
    };*/
void LAMP_ONOFF(int,int);
void auto_connect()
{
do
{ 
digitalWrite(Q0_0,HIGH);
delay(200);
digitalWrite(Q0_0,LOW);
client.connect(server,6000);
delay(200);
}

while(!client.connected());
}

void setup()
{
  Ethernet.begin(mac, ip);
 for (i=0;i<DO_size;i++)
     LAMPLIST[DO[i].sigNO] = DO[i].sigID;

// DAQ_DPDT();
 for (i=0; i<dpdt_size; i++)
     {
     // if (dpdt[i].swflag)
         DAQ_OUT(1100,dpdt[i].sigNO);
         delay(50);
     }
     
 //DAQ_SW();

 for (i=0; i<latching_size; i++)
     {
     // if (latching[i].currPosition==0)
         DAQ_OUT(1100,latching[i].sigNO);
         delay(50);
     }
}
void loop () 

{
if (!client.connected())
 auto_connect();
 

DAQ_PB();
DAQ_DPDT();
DAQ_SW();
DAQ_Tuners();
DAQ_DO();
delay(1000); 
  }

  //SECTION E1: PushButton
  void DAQ_PB()  
  {
 for (i=0; i<Pb_Size; i++)
    {
  delay(10);   
  bool flag = digitalRead(Pb[i].sigID);
  if (flag == true)
  {
   DAQ_OUT(1100,Pb[i].sigNO);
    }   
  }
  }

 //SECTION E2: DPDT
  void DAQ_DPDT() 

  {
    for (i=0; i<dpdt_size; i++)
  {
    flag = digitalRead(dpdt[i].sigID);
    if (flag != dpdt[i].swflag)
    {
     dpdt[i].swflag = flag;
     DAQ_OUT(1100,dpdt[i].sigNO+!flag);
    }
  }  
  }     

   //SECTION E3: LATCHING SWITCHES
   
   void DAQ_SW()
     {
      for (i=0;i<latching_size;i++)
      {
        currswitch= latching[i];
        flag= digitalRead(currswitch.sigID[currswitch.currPosition]);
        
        if (!flag)
        {
          for ( k=0;k<4;k++)
          {
          if (k !=currswitch.currPosition)
           {
            flag= digitalRead(currswitch.sigID[k]);
            
            if (flag)
            {
              latching[i].currPosition= k;
               DAQ_OUT(1100,currswitch.sigNO+k);
              return;
              }
           }
        }
      }
     }
     }
    
//SECTION E4: Get Throttle and Wheel Position
int mode(int Volt_read[],int kounter){
  int maxValue = 0, maxCount = 0,b,d;

  for (b=0;  b<kounter; ++b){
    int count = 0;

     for (d=0;d<kounter; ++d){
      if(Volt_read[b] ==Volt_read[d])
      ++count;
     }
     if (count>maxCount){
      maxCount = count;
      maxValue = Volt_Val[b];
     }
  }
  return maxValue;
}
int DAQ_Position(DAQ_AI Potentiometer)
   {
int Volt_In = 0;
     for (int ii=0; ii<Potentiometer.iteration;ii++)
     { Volt_Val[ii] = (int)(analogRead(Potentiometer.sigID));
//Volt_In +=(int)(analogRead(Potentiometer.sigID));
delay(50);
   }
//int newPosition = (int)((Potentiometer.m * (float) Volt_In/Voltage_Feedback.m)+Potentiometer.c);
//int newPosition = Volt_In/(Potentiometer.iteration);
 Volt_In = mode(Volt_Val, Potentiometer.iteration);
float voltage_ratio = (float) Volt_In*100.0f / Voltage_Feedback.m;
//voltage_ratio = voltage_ratio*Potentiometer.m+Potentiometer.c;
//int newPosition = (int)voltage_ratio;
int newPosition =(int) (voltage_ratio* Potentiometer.m + Potentiometer.c);
    if ( newPosition!=Potentiometer.currPosition)
       {
        //newPosition = (int) Voltage_Feedback.m*1;
        //if (Potentiometer.outputType)
            DAQ_OUT(1200+Potentiometer.sigNO,newPosition);
         //else   DAQ_OUT(1200+Potentiometer.sigNO,newPosition-Potentiometer.currPosition);
     //delay(200);
      }

      return newPosition;
    }

  void DAQ_Tuners()
   {
   Voltage_Feedback.m = analogRead(Voltage_Feedback.sigID);// store reference voltage here
   for (i=0; i< Tuner_Kounter; i++)
               Tuner[i].currPosition = DAQ_Position(Tuner[i]);
    }

   
 // SECTION E5: LAMPS
 //Temporary generation of Test Data

 void get_do_command()
 {
 nextchar = client.read();

 if (nextchar>-1) 
  {
    get_Input();
 
   LAMP.DO_Instr = Input_data[0];
   LAMP.DO_Param = Input_data[1];
 }
 
 else LAMP.DO_Instr = 0;
 
 }

 void LAMP_ONOFF(int index,int ACTION)
    {
      /*Action is as follows
       0 = OFF
       1 = ON
       2 = Test Lamp
       3 = Blink
       starts
       4 = Blink Stops
       5 = Blink Once 
        */

    if (ACTION >0)
      {
        ActiveDO[DO_Kounter].sigNO=index;
        ActiveDO[DO_Kounter++].ActivityType=ACTION;
        digitalWrite(LAMPLIST[index],HIGH);
       }
    else   
        ActiveDO[arraySearch(index)].ActivityType= 0;
         
      }  

 void lampTestStart()
 {
  
  
  for (j=0;j<DO_size;j++)
     
     {
      l=DO[j].sigNO;
     
      int m =arraySearch(l);
      
     // DAQ_OUT(1440+j, m);
     // DAQ_OUT(1640+j, ActiveDO[m].ActivityType); 
     
      if ( m > -1 && ActiveDO[m].ActivityType == 1)
         {
         }
         else 
             {
              
              LAMP_ONOFF(l,2); 
              delay(50);   
             }
      
     }
 }

void lampTestStop()

{
 DAQ_OUT(1421,DO_Kounter);
 for (j=0;j<DO_Kounter;j++)
     {
      if (ActiveDO[j].ActivityType==2)
       ActiveDO[j].ActivityType=0;
     } 
 }
 
 void DAQ_DO()
  {
 
   get_do_command();
   switch(LAMP.DO_Instr)
      {
  case 1205://switch Lamp ON
       LAMP_ONOFF(LAMP.DO_Param,1);
      break;

  case 1206: //Switch OFF/ON
      LAMP_ONOFF(LAMP.DO_Param/100,0);
      LAMP_ONOFF(LAMP.DO_Param%100,1);
      break;

  case 1207: //Blink once
      LAMP_ONOFF(LAMP.DO_Param,4);
      break;

  case 1208: //Test LamP
       DAQ_OUT(1420,LAMP.DO_Param);
        
         if (LAMP.DO_Param)
             lampTestStart();
        else lampTestStop();
     
        break;

  case 1210:  //Blink Continuous
             LAMP_ONOFF(LAMP.DO_Param,3);
             break;
   case 1215:  //switch Lamp OFF
            LAMP_ONOFF(LAMP.DO_Param,0);
            break;

  default:break;
      }
 
 
 //Handle Blink and other Clean- Ups

 blinkflag = !blinkflag;

 for(j=0;j<DO_Kounter;j++)
 {int DO_pin = LAMPLIST[ActiveDO[j].sigNO];
 switch (ActiveDO[j].ActivityType)
 {case 0:  //Remove from Register
  
      digitalWrite(DO_pin,LOW);
  
      ActiveDO[j--]= ActiveDO[DO_Kounter--];  
      break;
  
 case 3: //Blink Lamp
     digitalWrite(DO_pin,blinkflag);
    break;

 case 4: //Queue for Removal
     ActiveDO[j].ActivityType = 0;
     break;

 default: break;
    }
    }
    }
 
