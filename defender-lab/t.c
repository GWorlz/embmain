/********************************************************************
Copyright 2010-2017 K.C. Wang, <kwang@eecs.wsu.edu>
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
********************************************************************/

#include "defines.h"
#include "string.c"
#include <stddef.h>
//#include <stdio.h>

struct sprite {
   int x,y;
   char *p;
   int direction; // use this to track defender direction
   int enabled;   // modify this when sprite is hit to remove
   int oldstartRow;
   int oldstartCol;
};

int spriteMove = 0;
char *tab = "0123456789ABCDEF";
int color;

#include "timer.c"

#include "interrupts.c"
#include "kbd.c"
#include "uart.c"
#include "vid.c"
extern char _binary_lander_bmp_start;
extern char _binary_ship_left_bmp_start;
extern char _binary_ship_right_bmp_start;
extern char _binary_shoot_bmp_start;


void copy_vectors(void) {
    extern u32 vectors_start;
    extern u32 vectors_end;
    u32 *vectors_src = &vectors_start;
    u32 *vectors_dst = (u32 *)0;

    while(vectors_src < &vectors_end)
       *vectors_dst++ = *vectors_src++;
}
int kprintf(char *fmt, ...);
void timer_handler();
void vid_handler();

void uart0_handler()
{
  uart_handler(&uart[0]);
}
void uart1_handler()
{
  uart_handler(&uart[1]);
}

// IRQ interrupts handler entry point
//void __attribute__((interrupt)) IRQ_handler()
// timer0 base=0x101E2000; timer1 base=0x101E2020
// timer3 base=0x101E3000; timer1 base=0x101E3020
// currentValueReg=0x04
TIMER *tp[4];

void IRQ_handler()
{
    int vicstatus, sicstatus;
    int ustatus, kstatus;

    // read VIC SIV status registers to find out which interrupt
    vicstatus = VIC_STATUS;
    sicstatus = SIC_STATUS;  
    //kprintf("vicstatus=%x sicstatus=%x\n", vicstatus, sicstatus);
    // VIC status BITs: timer0,1=4, uart0=13, uart1=14, SIC=31: KBD at 3
    /**************
    if (vicstatus & 0x0010){   // timer0,1=bit4
      if (*(tp[0]->base+TVALUE)==0) // timer 0
         timer_handler(0);
      if (*(tp[1]->base+TVALUE)==0)
         timer_handler(1);
    }
    if (vicstatus & 0x0020){   // timer2,3=bit5
       if(*(tp[2]->base+TVALUE)==0)
         timer_handler(2);
       if (*(tp[3]->base+TVALUE)==0)
         timer_handler(3);
    }
    if (vicstatus & 0x80000000){
       if (sicstatus & 0x08){
          kbd_handler();
       }
    }
    *********************/
    /******************
    if (vicstatus & (1<<4)){   // timer0,1=bit4
      if (*(tp[0]->base+TVALUE)==0) // timer 0
         timer_handler(0);
      if (*(tp[1]->base+TVALUE)==0)
         timer_handler(1);
    }
    if (vicstatus & (1<<5)){   // timer2,3=bit5
       if(*(tp[2]->base+TVALUE)==0)
         timer_handler(2);
       if (*(tp[3]->base+TVALUE)==0)
         timer_handler(3);
    }
    *********************/
    if (vicstatus & (1<<4)){   // timer0,1=bit4
       timer_handler(0);
    }   
    if (vicstatus & (1<<12)){   // bit 12: uart0 
         uart0_handler();
    }
    if (vicstatus & (1<<13)){   // bit 13: uart1
         uart1_handler();
    }
    if (vicstatus & (1<<16)){   // bit 16: video
         vid_handler();
        VIC_CLEAR &= ~(1<<16);    
    }
    if (vicstatus & (1<<31)){
      if (sicstatus & (1<<3)){
       //   kbd_handler();
       }
    }
VIC_CLEAR =0;
}
extern int oldstartR;
extern int oldstartC;
extern int replacePix;
extern int buff[16][16];
struct sprite sprites[4];
#define Width  80
#define Height 60
#define defender_size 16   // size of defender
int defender_direction = 0;
char *defender_sprite;

int fire;

Draw_all(){
char *p;
int i=0,j=0;

for (int x=0; x<640*480; x++)
    fb[x] = 0x00000000;    // clean screen; all pixels are BLACK
 TIMER *t = &timer[0];
for (i=0; i<8; i++){
       kpchar(t->clock[i], 1, 70+i);
    }


   
    if(sprites[0].enabled){  
        if(sprites[0].direction == 0){
	      sprites[0].x -= 16;	} 
         else{
	     sprites[0].x += 16;	}
	 if(sprites[0].x < 0 || sprites[0].x > 590){
              sprites[0].enabled = 0;
              }
        for(i = 2; i <4; i++){    // collision checking
        
             if (sprites[0].x <= (sprites[i].x + 32) &&
              (sprites[0].x + 10) >= sprites[i].x  &&
              sprites[0].y <= (sprites[i].y + 27 ) &&
              (sprites[0].y + 50) >= sprites[i].y) {
              // collision detected

	      sprites[i].enabled = 0;
	      sprites[0].enabled = 0;
	  break;
            }
          }
          }
    
      
      if(sprites[0].enabled){
        show_bmp(sprites[0].p, sprites[0].y, sprites[0].x);
      }
       
      for(i = 1; i < 4; i++){
        if(sprites[i].enabled){
          show_bmp(sprites[i].p , sprites[i].y,sprites[i].x);      
        }
        if (i>1){
           sprites[i].x+=10; 
          }
        }
      
fbmodified =1;
*(volatile unsigned int *)(0x1012001C) |= 0xc; // enable video interrupts
}

int counter;
int main()
{
   int i,j; 
   char line[128], string[32]; 
   UART *up;
  
  int x=80;
   int y=0;
   sprites[0].x = 80;
   sprites[0].y = 0;
   sprites[0].p = &_binary_shoot_bmp_start;
   sprites[1].x = 80;
   sprites[1].y = 0;
   sprites[1].p = &_binary_ship_right_bmp_start;
   sprites[1].direction = 1;
   sprites[1].enabled = 1;
   sprites[2].x = 200;
   sprites[2].y = 200;
   sprites[2].p = &_binary_lander_bmp_start;
   sprites[2].enabled = 1;
   sprites[3].x = 200;
   sprites[3].y = 100;
   sprites[3].p = &_binary_lander_bmp_start;
   sprites[3].enabled = 1;

   char * p;   
   color = YELLOW;
   row = col = 0; 
   

   /* enable timer0,1, uart0,1 SIC interrupts */
  VIC_INTENABLE |= (1<<4);  // timer0,1 at bit4 
  // VIC_INTENABLE |= (1<<5);  // timer2,3 at bit5 

   VIC_INTENABLE |= (1<<12); // UART0 at 12
  // VIC_INTENABLE |= (1<<13); // UART1 at 13

   VIC_INTENABLE |= (1<<16); // LCD at 16
 
   UART0_IMSC = 1<<4;  // enable UART0 RXIM interrupt
   UART1_IMSC = 1<<4;  // enable UART1 RXIM interrupt

  //VIC_INTENABLE |= 1<<31;   // SIC to VIC's IRQ31

   /* enable KBD IRQ */
   SIC_ENSET = 1<<3;  // KBD int=3 on SIC
   SIC_PICENSET = 1<<3;  // KBD int=3 on SIC
fbuf_init();
   kprintf("C3.3 start: Interrupt-driven drivers for Timer KBD UART\n");
   timer_init();

   /***********
   for (i=0; i<4; i++){
      tp[i] = &timer[i];
      timer_start(i);
   }
   ************/
   timer_start(0);
   kbd_init();

   uart_init();
   up = &uart[0]; 


   // p = &_binary_pacman_bmp_start;
   //show_bmp(p, 0, 80,buff,replacePix,&oldstartR,&oldstartC);
 
  // while(1);
   
   /*
   while(1){
      color = CYAN;
      kprintf("Enter a line from KBD\n");
      kgets(line);
   }
   */
   unlock();
 kprintf("\nenter KBD here\n");
uuprints("from UART0 here\n\r");
int move=0;
int key;
   while(1){
//uprintf("enable %x\n",*(volatile unsigned int *)(0x1012001C));
    if (vidhandler){
//     uprintf("got vidhandler %x counter %d\n",*(volatile unsigned int *)(0x1012001C),counter);
      vidhandler=0;
      counter++;
}

     move=0;
			
     if (upeek(up)){
     key=ugetc(up);
     switch(key){
     case 'd': // Right
		 if (sprites[1].x < 568){
			 sprites[1].x += 16;
			 sprites[1].p = &_binary_ship_right_bmp_start;
			 sprites[1].direction = 1;
			
			 
		 }
		 move = 1;
        break;
		 /* TODO 
other keys w,e and f to move defender around */
                case 's':   // to move down
                 if ( sprites[1].y < 445){
                    sprites[1].y+=16;
                    }
                 move =1;
                break;	      
		case 'w':   // to move up
         if ( sprites[1].y> 0){
            sprites[1].y-=16;
            }
         move =1;
        break;
        case 'e': //left

		 if(sprites[1].x > 0){
			 sprites[1].x -= 16;
			 sprites[1].p =     &_binary_ship_left_bmp_start;			 
			 sprites[1].direction = 0;
			 			 		
		 }
		 move = 1;
		break;
		

        case 'f':
	  /* TODO firing */
	  if(sprites[0].enabled == 0 ){
	  show_bmp(sprites[0].p, sprites[0].y, sprites[0].x);
	  }
			if(sprites[1].direction == 1){
			
				  sprites[0].x = sprites[1].x + 16;
				  sprites[0].direction = 1;
			 }
			else if(sprites[1].direction == 0){
			        
				sprites[0].x = sprites[1].x - 16;
				sprites[0].direction = 0;
			}
			sprites[0].y = sprites[1].y;
			sprites[0].p = &_binary_shoot_bmp_start;
		        sprites[0].enabled = 1;	
    
  
		        int *temp_fb = fb;
		        
		        fb = fb1;
	  show_bmp(sprites[0].p, sprites[0].y, sprites[0].x);
	  fb = fb2;
	   show_bmp(sprites[0].p, sprites[0].y, sprites[0].x);
	   
	   fb = temp_fb;
	  fbmodified = 1;
	  		
		break;
		
  
      default:
        move=0;
      }
}
      if (move || spriteMove){
         // switch_buffer();
          Draw_all();
          spriteMove=0;
        }
      
   
   }
}
