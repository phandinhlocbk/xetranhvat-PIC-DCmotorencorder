#include <18F4550.h>
#include <STDDEF.H>
////////////
#DEVICE ADC=10
#FUSES NOWDT 
#FUSES HSPLL      
#FUSES MCLR      
#FUSES NOPROTECT       
#FUSES NOLVP       
#FUSES NODEBUG       
#FUSES USBDIV       
#FUSES PLL5       
#FUSES CPUDIV1       
#FUSES VREGEN  
#FUSES HS, NOPUT, NOBROWNOUT, NOCPD, NOWRT
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <stdlib.h>
#use delay(clock=20000000)
#use rs232(uart1,baud=38400,parity=N,Stop=1,bits=8,stream=COM3,errors)//Khai bao su dung uart cung
#include <string.h> 
////////////////////
#use FAST_IO(A)
#use FAST_IO(D)

#byte PORTA =  0xf80
#byte PORTB =  0xf81
#byte PORTC =  0xf82
#byte PORTD =  0xf83
#byte PORTE =  0xf84
#BIT RC1 = PORTC.1
#BIT RC2 = PORTC.2
#BIT RE0 = PORTE.0
#BIT RE2 = PORTE.2
////////////////
#define nguong_right 1000 //nguong PWM
#define nguong_left 1000  
#define time_step 50535  // 0.005 sencond  5535 //
#define CHUVI_LEFT 250// chu vi
#define CHUVI_RIGHT 250
#define RADIUS 200   //DO RONG XE/2
/////////////////
#define DIR_RIGHT RE0
#define DIR_LEFT RE2
#define B RC2 // left ccp2
#define A RC1 // right cpp1
void chay();
void pid_right();
void pid_left();
signed int32 countright=0,position_right=0,position_set_right = 0,trunggian=0,count_left;
signed int32 countleft=0,position_left=0,position_set_left = 0,trunggian1=0;
signed int32 e_sum_r=0, e_del_r=0, e1_r=0, e2_r=0;
signed int32 e_sum_l=0, e_del_l=0, e1_l=0, e2_l=0;
float Kp_r=100, Kd_r=9, Ki_r=0.0001;
float Kp_l=100, Kd_l=9, Ki_l=0.0001;
float angle_current = 45;
float x_cur = 0, y_cur = 0, x_cur_old = 0, y_cur_old = 0;
int1  copid_right = 1, copid_left = 1 ,codoctinhieu=0,donePID = 0,done_Timer1 = 0;
/////////////////////////////
char  ss,huong;
char data_receive[50] = "";
int8 index = 0,i;
float value,value_cmd,anpha;
//////////////////////////////
void dichuyen(char huong_move, float value);
void huongdichuyen();
void pid_robot();
void xy_vitri(float khoangcach);
//////////////////////////////

#INT_RDA
void RDA_isr()
{
char value[20];
ss=getc();
      if (ss=='A')// begining symbol
      {
      index=0;
      *data_receive="";
      }
      else if (ss=='C')
      {
      data_receive[index++]='\0';
      huong = data_receive[0];
     // putc(huong);
      strncpy(value, data_receive + 1, strlen(data_receive) - 1);
      value_cmd=atof(value);    
     // printf("%f",value_cmd);//he 10
      codoctinhieu= 1;
      for (i=0;i<19;i++)
      {
      value[i]=NULL;
      }
      }
      else data_receive[index++] = ss; 
}
///////////////////////////////
#INT_EXT1
void EXT1_isr()
{
  disable_interrupts(int_ext1);
   if (input_state(PIN_D1))//qua chieu duong //A-RB1 B-RD1 day vang la B trang la A 
     { 
     countright++;
     if ((huong=='T')||(huong=='F')) count_left++;// de cho no khi 2 banh cung chay
     }
   else {
   countright--;
   if ((huong=='T')||(huong=='F')) count_left++;// de cho no khi 2 banh cung chay
   }
   position_right=countright;
 enable_interrupts(int_ext1);
}
#INT_EXT2
void EXT2_isr()
{
   disable_interrupts(int_ext2);
   if (input_state(PIN_D2))
   {    countleft++;   }
   else   {  countleft--;  }
   position_left=countleft;
   enable_interrupts(int_ext2);
}
#INT_TIMER1
void TIMER1_isr()
{      done_Timer1 = 1;
       set_timer1(50535);
}
/**************************************/
void main(){

       set_tris_D(0xFF);
       set_tris_A(0);
       set_tris_B(0xFF);
     //  set_tris_C(0b10000000);
       set_tris_E(0);   
        set_tris_C(0);
        setup_adc_ports(NO_ANALOGS);
       setup_timer_1(T1_INTERNAL | T1_DIV_BY_1 );
       set_timer1(50535); 
       setup_timer_2(T2_DIV_BY_4,255,1);  //f_pwm = 2.9 Khz, T_pwm = 4.(PR2+1).Tosc.Pre-scale
       output_low(PIN_C1); // Set CCP2 output low 
       output_low(PIN_C2); // Set CCP1 output low 
      
       setup_ccp1(CCP_PWM);    //khoi tao bo PWM1
       setup_ccp2(CCP_PWM);    //khoi tao bo PWM2
       set_pwm1_duty(0);      //PIN_C2, Right motor
       set_pwm2_duty(0);      //PIN_C1, Left motor
      
       enable_interrupts(INT_RDA); 
       enable_interrupts(int_ext1);
       enable_interrupts(int_ext2);
       ext_int_edge(1, H_TO_L );
       ext_int_edge(2, H_TO_L );
       enable_interrupts(INT_TIMER1);   
       enable_interrupts(GLOBAL);   
       A=1;
       B=1;
            output_low(pin_A0);
while(true)
{
   if (done_Timer1 == 1)
   {
      dichuyen(huong,value_cmd);
    pid_robot();
     done_Timer1 =0;    
   }
}
}
void pid_robot()
{
      pid_right();
      pid_left();
      if ((copid_right)&&(copid_left)&&(!donePID))
      {
      
       if (huong=='R')
      {
        angle_current =angle_current - value_cmd;
        huong = "";
       }
       else if (huong=='L')
        {
             angle_current  =angle_current + value_cmd;
            huong = "";
        }
        else if (huong=='F')
        {
              xy_vitri(value_cmd);
               huong = "";
        }
        else if (huong=='B')
        {
             xy_vitri(-value_cmd);
               huong = "";
        }
    if (angle_current >= 360.0) angle_current =angle_current -360.0;
    if (angle_current < 0 ) angle_current =angle_current + 360.0;
    x_cur= (int32 )x_cur;
    y_cur= (int32)y_cur;
   angle_current= (int32) angle_current; 
  printf("X%fY%fZ%fBX%fY%fZ%fB", x_cur, y_cur, angle_current,x_cur, y_cur, angle_current);
    donePID = 1;       
     }
}
void xy_vitri(float khoangcach)
{
 x_cur = x_cur_old + khoangcach*cos(angle_current*PI/180.0);
 y_cur = y_cur_old + khoangcach*sin(angle_current*PI/180.0);
 x_cur_old = x_cur;
 y_cur_old = y_cur; 
}
void dichuyen(char huong_move, float value)
{
  if (codoctinhieu==1)
      {
      copid_right=0;
      copid_left=0;
      donePID = 0;
      
      countright = 0;
      countleft = 0;
      position_left = 0;
      position_right = 0;
      codoctinhieu=0;
       switch (huong_move)
      {
      case 'T':
      {
         position_set_left  = (unsigned int32)(100.0*value/CHUVI_LEFT);
         position_set_right = (unsigned int32)(100.0*value/CHUVI_RIGHT);
         count_left=0;
         break;
      }
      case 'S':
      {
          position_set_left  = (unsigned int32)(100.0*value/CHUVI_LEFT);
          position_set_right = (unsigned int32)(100.0*value/CHUVI_RIGHT);
          break;
      }
      case 'F':
      {
          position_set_left  = (unsigned int32)(100.0*value/CHUVI_LEFT);
          position_set_right = (unsigned int32)(100.0*value/CHUVI_RIGHT);  
          count_left=0;
          break;
      }
      case 'B':
      {
       position_set_left  =  (unsigned int32)(-100.0*value/CHUVI_LEFT);
        position_set_right =  (unsigned int32)(-100.0*value/CHUVI_RIGHT);
        break;
      }
      case 'R':
      {
       anpha = - value*10*3.1415/180.0;   
       position_set_left  =  (unsigned int32)(-100.0*(anpha*RADIUS)/CHUVI_LEFT);
       position_set_right =  (unsigned int32)(100.0*(anpha*RADIUS)/CHUVI_RIGHT);
      
        break;
      }
      case 'L':
      {
        anpha = 10*value*3.1415/180.0;     
        position_set_left  =  (unsigned int32)(-100.0*(anpha*RADIUS)/CHUVI_LEFT);//so xung doc encorder
        position_set_right =  (unsigned int32)(100.0*(anpha*RADIUS)/CHUVI_RIGHT);
        break;         
     
      }
       default:  break;
      }
      }
      
}
void pid_left()
{

  B=1;
    signed int16 temp_kp = 0;
  signed int16 temp_ki = 0;
  signed int16 temp_kd = 0;
  signed int16 pw_duty_Left_Temp = 0;
  e2_l= position_set_left - position_left;
  e_sum_l+=e2_l;
  e_del_l=e2_l-e1_l;
  e1_l=e2_l;
   temp_kp = (signed int16)((float)kp_l*e2_l);
   temp_ki = (signed int16)((float)ki_l*e_sum_l);
   temp_kd = (signed int16)((float)kd_l*e_del_l);
  
   pw_duty_Left_Temp = (signed int16)(temp_kp + temp_ki + temp_kd);
   if ( pw_duty_Left_Temp > nguong_left)
   {
   pw_duty_Left_Temp=nguong_left;
   }
   if (pw_duty_Left_Temp < - nguong_left)
   {
   pw_duty_Left_Temp=-nguong_left;
   }
   if ( pw_duty_Left_Temp>0)
   {
   output_high(pin_A0);
   DIR_LEFT=1;
    set_pwm2_duty(pw_duty_Left_Temp);
   }
   else 
   {
    DIR_LEFT=0;
    trunggian1=abs(pw_duty_Left_Temp);
    set_pwm2_duty( trunggian1);   
   } 
   if ((huong=='L')||(huong=='R'))
      {
      if((e2_l>-100)&&(e2_l<100))
   {
   B=0;
    DIR_LEFT=0;
    set_pwm2_duty( 0);
    copid_left=1;
   } 
   else 
   {
   copid_left=0;
   }
      }
      ////////////
    else
    {
   if((e2_l>-40)&&(e2_l<40))
   {
    DIR_LEFT=0;
    B=0;
    set_pwm2_duty( 0);
    copid_left=1;
   } 
   else 
   {
   copid_left=0;
   }
    }
 
}
void pid_right()
{

A=1;
  signed int16 temp_kp = 0;
  signed int16 temp_ki = 0;
  signed int16 temp_kd = 0;
  signed int16 pw_duty_Right_Temp = 0;

  e2_r= position_set_right- position_right;
  e_sum_r+=e2_r;
  e_del_r=e2_r-e1_r;
  e1_r=e2_r;
   temp_kp = (signed int16)((float)kp_r*e2_r);
   temp_ki = (signed int16)((float)ki_r*e_sum_r);
   temp_kd = (signed int16)((float)kd_r*e_del_r);
  
   pw_duty_Right_Temp = (signed int16)( temp_kp + temp_ki + temp_kd);
   if ( pw_duty_Right_Temp > nguong_right)
   {
   pw_duty_Right_Temp=nguong_right;
   }
   if (pw_duty_Right_Temp < -nguong_right)
   {
   pw_duty_Right_Temp=-nguong_right;
   }
   if ( pw_duty_Right_Temp>0)
   {
   DIR_RIGHT=1;
    set_pwm1_duty(pw_duty_Right_Temp);
   }
   else 
   {
    DIR_RIGHT=0;
    trunggian=abs(pw_duty_Right_Temp);
    set_pwm1_duty( trunggian);
   } 
   if ( ((huong=='L')||(huong=='R')))
   {
    if((e2_r>-100)&&(e2_r<100))
   {
       DIR_RIGHT=0;
       A=0;
      set_pwm1_duty( 0);
      copid_right=1;
   }
   else 
   {
      copid_right=0;
   }
   }
   else
   {
   if((e2_r>-40)&&(e2_r<40))
   {
       DIR_RIGHT=0;
       A=0;
      set_pwm1_duty( 0);
      copid_right=1;
   }
   else 
   {
      copid_right=0;
   }
   }
}
/*
void chay()
{
A=1;
//B=1;
//output_high(pin_E1);
DIR_RIGHT=1;
//DIR_LEFT=1;
  //set_pwm2_duty(255); 
  set_pwm1_duty(255);
  delay_ms(2000);
 // output_low(pin_E1);
 DIR_RIGHT=0;
 //DIR_LEFT=0;
 // set_pwm2_duty(255);
  set_pwm1_duty(255);
  delay_ms(2000);
}*/
