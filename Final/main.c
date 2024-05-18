#include <REG52.H>
#include"pic.c"
#include <intrins.h>
#define LCD_DATA P2
#define button_delay 150  
#define button_acceleration 65  
#define GAME_LOCATION 30

#define TRACK_WIDTH  4  
#define LEFT_BOUNDARY 32   
#define RIGHT_BOUNDARY 96  

sbit change = P3^4;
sbit OK = P3^5;
sbit up = P3^2;
sbit down = P3^0;
sbit left = P3^1;
sbit right = P3^3;
sbit speaker=P3^6; 

sbit LCD_RS=P1^0;
sbit LCD_RW=P1^1;
sbit LCD_E=P1^2;
sbit LCD_CS2=P1^4;		
sbit LCD_CS1=P1^3;		
sbit LCD_RST=P3^7;

unsigned int up_reg=button_delay;       
unsigned int down_reg=button_delay;
unsigned int left_reg=button_delay;
unsigned int right_reg=button_delay;
unsigned int button_a_reg=button_delay;
unsigned int button_b_reg=button_delay;
unsigned int right_acceleration=0;
unsigned int left_acceleration=0;

unsigned int idata Box_Ram[19];
unsigned char box_down_reg;
unsigned char time0_reg;
unsigned char next_mode;
unsigned char next_shape;
unsigned int destroy_row_num=0;
unsigned char speed_num=0;
bit game_over_flag;
bit pause_game_flag;

struct
{
	unsigned char mode;
	unsigned char shape;
	unsigned char x;
	unsigned char y;
	unsigned int box;
}s_box0,s_box;

void LCD_check_busy()
{
	unsigned char temp;
	LCD_RS=0;
	LCD_RW=1;
	do
	{
		LCD_DATA=0xff;
		LCD_E=1;
		temp=LCD_DATA;
		LCD_E=0;
	}while((temp&0x80)==0x80);		
}

void LCD_W_code(unsigned char tpcode, bit cs)
{
	LCD_RS=0;
	LCD_RW=0;
	LCD_CS2=~cs;
	LCD_CS1=cs;
	LCD_DATA=tpcode;
	LCD_E=1;
	_nop_();
	LCD_E=0;
}

void LCD_W_data(unsigned char tpdata, bit cs)
{
	LCD_check_busy();
	LCD_RS=1;
	LCD_RW=0;
	LCD_CS2=~cs;
	LCD_CS1=cs;	
	LCD_DATA=tpdata;
	LCD_E=1;	
	_nop_();
	LCD_E=0;
}

void LCD_initialize()
{
	LCD_RST=0;
	_nop_();
	_nop_();
	LCD_RST=1;
	LCD_W_code(0x3f,0);		
	LCD_W_code(0xc0,0);		
	LCD_W_code(0xb8,0);		
	LCD_W_code(0x40,0);		
	LCD_W_code(0x3f,1);
	LCD_W_code(0xc0,1);	
	LCD_W_code(0xb8,1);
	LCD_W_code(0x40,1);
}

void LCD_clear()
{
	unsigned char i,j;
	for(j=0;j<8;j++)
	{
		LCD_W_code(0xb8+j,0);
		LCD_W_code(0x40,0);
		LCD_W_code(0xb8+j,1);
		LCD_W_code(0x40,1);
		for(i=0;i<64;i++)
		{	
			LCD_W_data(0x00,0);	
			LCD_W_data(0x00,1);
		}
	}
}

void LCD_display_word(unsigned char word[], unsigned int length, unsigned char x, unsigned char y)
{
	unsigned char i;
	for(i=0;i<length;i++)
	{
		LCD_W_code(0xb8+x,0);
		LCD_W_code(0xb8+x,1);
		if(y+i<64)
		{
			LCD_W_code(0x40+y+i,0);	
			LCD_W_data(word[i],0);
		}
		else
		{
			LCD_W_code(y+i,1);	
			LCD_W_data(word[i],1);
		}
	}
}

void LCD_full_draw(unsigned char word[])
{
	unsigned char i,j;
	for(i=0;i<8;i++)
	{
		LCD_W_code(0xb8+i,0);
		LCD_W_code(0x40,0);	
		for(j=0;j<64;j++)
		{
			LCD_W_data(word[i*128+j],0);
		}
		LCD_W_code(0xb8+i,1);
		LCD_W_code(0x40,1);	
		for(j=0;j<64;j++)
		{
			LCD_W_data(word[i*128+64+j],1);
		}			
	}
}

void LCD_display_byte(unsigned char x, unsigned char y, unsigned char tpdata)
{
	if(x<64)
	{
		LCD_W_code(0xb8+y,0);
		LCD_W_code(0x40+x,0);
		LCD_W_data(tpdata,0);	
	}
	else
	{
		LCD_W_code(0xb8+y,1);
		LCD_W_code(x,1);
		LCD_W_data(tpdata,1);	
	}
} 

void LCD_draw(unsigned char word[])
{
  unsigned char i,j;
  for(i=0;i<8;i++)
  {
    LCD_W_code(0xb8+i,1);
	LCD_W_code(0x40+20,1);
	for(j=0;j<44;j++)
	{
	  LCD_W_data(word[i*44+j],1);
	}
  }
}

void display_basic()
{
	unsigned char i;
	for(i=0;i<8;i++)
	{
		LCD_display_byte(GAME_LOCATION,i,0xff);
		LCD_display_byte(GAME_LOCATION+41,i,0xff);
	}
}

void refurbish_display()
{
	unsigned char i,j,tpdata;
	for(i=0;i<8;i++)
	{
		for(j=0;j<10;j++)
		{
			tpdata=0x00;
			if(  (Box_Ram[2*i]>>(12-j))&0x0001==1  )
			{
				tpdata=0x0f;
			}
			if(  (Box_Ram[2*i+1]>>(12-j))&0x0001==1  )
			{
				tpdata|=0xf0;
			}
			LCD_display_byte(GAME_LOCATION+1+j*4,i,tpdata);
			LCD_display_byte(GAME_LOCATION+2+j*4,i,0xbb&tpdata);
			LCD_display_byte(GAME_LOCATION+3+j*4,i,0xdd&tpdata);
			LCD_display_byte(GAME_LOCATION+4+j*4,i,tpdata);
		}
	}
}

unsigned char basic_button()
{
	unsigned char tpflag=0;
	if(OK==0)
	{
	    if(button_b_reg<button_delay*8)
		{
		  button_b_reg++;
		}
		else
		{
		  button_b_reg=0;
		  tpflag=6;
		}
	}
	else
	{
   	     button_b_reg=button_delay*8;
	}
	if(down==0)
	{
		if(down_reg<button_delay)
		{
			down_reg++;
		}
		else
		{
			down_reg=0;
			tpflag=1;
		}		
	}
	else
	{
		down_reg=button_delay;
	}
	if(up==0)
	{
		if(up_reg<button_delay)
		{
			up_reg++;
		}
		else
		{
			up_reg=0;
			tpflag=2;
		}		
	}
	else
	{
		up_reg=button_delay;
	}
	if(change==0)
	{
		if(button_a_reg<button_delay)
		{
			button_a_reg++;
		}
		else
		{
			button_a_reg=0;
			tpflag=3;
		}		
	}
	else
	{
		button_a_reg=button_delay;
	}
	if(left==0)
	{
		if(left_reg<(button_delay))
		{
			left_reg++;
		}
		else
		{
			left_reg=left_acceleration*button_acceleration;
			if(left_acceleration<2)left_acceleration++;
			tpflag=4;
		}		
	}
	else
	{
		left_acceleration=0;
		left_reg=button_delay;
	}
	if(right==0)
	{
		if(right_reg<(button_delay))
		{
			right_reg++;
		}
		else
		{
			right_reg=right_acceleration*button_acceleration;
			if(right_acceleration<2)right_acceleration++;
			tpflag=5;
		}		
	}
	else
	{
		right_acceleration=0;
		right_reg=button_delay;
	}
	return(tpflag);
}

bit check_cover(unsigned char tpx,unsigned char tpy,unsigned int tpbox)
{
	unsigned char i;
	bit tpflag=1;
	unsigned int temp;
	temp=s_box.box;
	for(i=0;i<4;i++)
	{
		Box_Ram[3-i+s_box.y]&=(~((temp&0x000f)<<(9-s_box.x))); 
		temp=temp>>4;
	}
	temp=tpbox;
	for(i=0;i<4;i++)
	{
		if((((temp&0x000f)<<(9-tpx))&Box_Ram[3-i+tpy])!=0x0000)
		{
			tpflag=0;
		}
		temp=temp>>4;
	}
	temp=s_box.box;
	for(i=0;i<4;i++)
	{
		Box_Ram[3-i+s_box.y]|=((temp&0x000f)<<(9-s_box.x));
		temp=temp>>4;
	}
	return(tpflag);
}

unsigned int box0_read_data(unsigned char tpmode,unsigned char tpshape)
{
	unsigned int tpbox;
   (void)tpmode;

    tpbox = 0x4E4E; 
    return tpbox;

}

unsigned int box_read_data(unsigned char tpmode,unsigned char tpshape)
{
	unsigned int tpbox;
   (void)tpmode;

    tpbox = 0xE4E4;
    return tpbox;

}

void box0_load()
{
	s_box0.box=box0_read_data(s_box0.mode,s_box0.shape);
}

void box_load()
{
	s_box.box=box_read_data(s_box.mode,s_box.shape);
}

void box0_to_Box_Ram(unsigned char tpx,unsigned char tpy,unsigned int tpbox)
{
	unsigned char i;
	unsigned int temp;
	temp=tpbox;
	for(i=0;i<4;i++)
	{
		Box_Ram[3-i+tpy]=Box_Ram[3-i+tpy]&(~((temp&0x000f)<<(9-tpx))); 
		temp=temp>>4;
	}
	temp=s_box0.box;
	for(i=0;i<4;i++)
	{
		Box_Ram[3-i+s_box0.y]=((temp&0x000f)<<(9-s_box0.x))|Box_Ram[3-i+s_box0.y];
		temp=temp>>4;
	}
}

void box_to_Box_Ram(unsigned char tpx,unsigned char tpy,unsigned int tpbox)
{
	unsigned char i;
	unsigned int temp;
	temp=tpbox;
	for(i=0;i<4;i++)
	{
		Box_Ram[3-i+tpy]=Box_Ram[3-i+tpy]&(~((temp&0x000f)<<(9-tpx))); 
		temp=temp>>4;
	}
	temp=s_box.box;
	for(i=0;i<4;i++)
	{
		Box_Ram[3-i+s_box.y]=((temp&0x000f)<<(9-s_box.x))|Box_Ram[3-i+s_box.y];
		temp=temp>>4;
	}
}

void box_to_Box_Ram_clean(unsigned char tpx,unsigned char tpy,unsigned int tpbox)
{
	unsigned char i;
	unsigned int temp;
	temp=tpbox;
	for(i=0;i<4;i++)
	{
		Box_Ram[3-i+tpy]=Box_Ram[3-i+tpy]&(~((temp&0x000f)<<(9-tpx))); 
		temp=temp>>4;
	}
}

void show_num(unsigned char x,
					  unsigned char y,
					  unsigned char tpdata)
{
	unsigned char i;
	for(i=0;i<4;i++)
	{
		LCD_display_byte(x+i,y,num_data[tpdata*4+i]);	
	}
} 
void show_speed_num(unsigned char x,unsigned char y)
{
	show_num(x,y,speed_num);
}
//��ʾ�÷ֺ���
void show_score_num(unsigned char x,unsigned char y)
{
	show_num(x,y,destroy_row_num/10000);
	show_num(x+=5,y,(destroy_row_num%10000)/1000);
	show_num(x+=5,y,(destroy_row_num%1000)/100);
	show_num(x+=5,y,(destroy_row_num%100)/10);
	show_num(x+=5,y,destroy_row_num%10);
}
//���к���
void destroy_row()
{
	unsigned char i,j=0;
	unsigned char tpflag[4]={0,0,0,0};
	for(i=0;i<16;i++)
	{
		if((Box_Ram[i]&0x3ffc)==0x3ffc)
		{
			tpflag[j]=i+1;
			destroy_row_num++;
			if(destroy_row_num%30==0&&speed_num!=9)
			{
				speed_num++;
				show_speed_num(13,4);
			}
			j++;
			if(j==4)
			{
				break;
			}
		}
	}
	for(j=0;j<4;j++)
	{
		if(tpflag[j]!=0)
		{
			for(i=tpflag[j]-1;i>0;i--)
			{
			Box_Ram[i]=Box_Ram[i-1];
			Box_Ram[0]=0x2004;
			}
		}
	}
	show_score_num(3,1);
}
void show_next_box()
{
	unsigned char i,tpdata;
	unsigned int temp;
	temp=box_read_data(next_mode,next_shape);
	for(i=0;i<4;i++)
	{
		tpdata=0x00;
		if(  ((temp>>(15-i))&0x0001)==1  )
		{
			tpdata=0x0f;
		}
		if(  ((temp>>(11-i))&0x0001)==1  )
		{
			tpdata|=0xf0;
		}
		LCD_display_byte(7+i*4,6,tpdata);
		LCD_display_byte(8+i*4,6,0xbb&tpdata);
		LCD_display_byte(9+i*4,6,0xdd&tpdata);
		LCD_display_byte(10+i*4,6,tpdata);	
		tpdata=0x00;
		if(  ((temp>>(7-i))&0x0001)==1  )
		{
			tpdata=0x0f;
		}
		if(  ((temp>>(3-i))&0x0001)==1  )
		{
			tpdata|=0xf0;
		}
		LCD_display_byte(7+i*4,7,tpdata);
		LCD_display_byte(8+i*4,7,0xbb&tpdata);
		LCD_display_byte(9+i*4,7,0xdd&tpdata);
		LCD_display_byte(10+i*4,7,tpdata);		
	}
}
void box0_build()
{
	s_box0.x=3;
	s_box0.y=12;
}
void box_build()
{
	s_box.x=TL0%5;
	s_box.y=0;
	show_next_box();
}
void game_button()
{
    switch(basic_button())
    {
		 case 2: if(1)
                {
                    EA=0;
                    speaker=0;
                    if(s_box0.shape==3&&check_cover(s_box0.x,s_box0.y,box_read_data(s_box0.mode,0)))
                    {
							
                        s_box0.shape=0;
                        box0_load();
                        box0_to_Box_Ram(s_box0.x,s_box0.y,box_read_data(s_box0.mode,3));
                    }
                    else if(check_cover(s_box0.x,s_box.y,box_read_data(s_box0.mode,0)))
                    {	if(check_cover(s_box0.x,s_box0.y,box_read_data(s_box0.mode,s_box0.shape+1)))
							{	
								s_box0.shape++;
								box0_load();
								box0_to_Box_Ram(s_box0.x,s_box0.y,box_read_data(s_box0.mode,s_box0.shape-1));
							}
                     }
                    EA=1;
                    speaker=1;
					}break;
		case 1: if(1)
        {
            speaker=0;
            if(check_cover(s_box0.x,s_box0.y+1,s_box0.box))
            {
                s_box0.y++;
                box0_to_Box_Ram(s_box0.x,s_box0.y-1,s_box0.box);
            }
            speaker=1;
            }break;
		case 4: if(1)
        {
            speaker=0;
            if(check_cover(s_box0.x-1,s_box0.y,s_box0.box))
            {
                s_box0.x--;
                box0_to_Box_Ram(s_box0.x+1,s_box0.y,s_box0.box);
            }
            speaker=1;
        }break;
		case 5: if(1)
                {
                    speaker=0;
                    if(check_cover(s_box0.x+1,s_box0.y,s_box0.box))
                    {
                        s_box0.x++;
                        box0_to_Box_Ram(s_box0.x-1,s_box0.y,s_box0.box);
                    }
						speaker=1;
					}break;
        case 3: 
            speaker=0;
            speaker=0;
            if(check_cover(s_box0.x,s_box0.y-1,s_box0.box))
            {
                s_box0.y--;
                box0_to_Box_Ram(s_box0.x,s_box0.y+1,s_box0.box);
            }
            speaker=1;
            break;
        default:;
    }	
}
bit check_game_over()
{
	unsigned char i;
	bit tpflag=0;
	unsigned int temp;
	temp=s_box.box;
	for(i=0;i<4;i++)
	{
		if((((temp&0x000f)<<(9-s_box.x))&Box_Ram[3-i+s_box.y])!=0x0000)
		{
			tpflag=1;
		}
		temp=temp>>4;
	}
	return(tpflag);
}
void game_execute()
{
	if(box_down_reg<20-(speed_num<<1))
	{				  
		box_down_reg++;
	}
	else
	{
		box_down_reg=0;
		if(check_cover(s_box.x,s_box.y+1,s_box.box))
		{
			s_box.y++;
			box_to_Box_Ram(s_box.x,s_box.y-1,s_box.box);
			box0_to_Box_Ram(s_box0.x,s_box0.y-1,s_box0.box);
		}
		else if(s_box.y>11)
		{
			box_to_Box_Ram_clean(s_box.x,s_box.y,s_box.box);
			destroy_row_num++;
			show_score_num(3,1);	
			box_build();
			box_load();
			game_over_flag=check_game_over();
			box_to_Box_Ram(s_box.x,s_box.y,s_box.box);
			box0_to_Box_Ram(s_box0.x,s_box0.y-1,s_box0.box);
			box_down_reg=(20-(speed_num<<1)-1);	
		}
		else	
		{
            destroy_row();
			// box_build();
			// box_load();
			game_over_flag=1;
		}
	}



	
}
void select_speed()
{
	unsigned char i;
	bit tpflag=1;
	LCD_clear();
	for(i=0;i<128;i++)
	{
		LCD_display_byte(i,0,0xff);
		LCD_display_byte(i,7,0xff);
	}
	LCD_display_byte(60,4,0x7f);
	LCD_display_byte(59,4,0x3e);
	LCD_display_byte(58,4,0x1c);
	LCD_display_byte(57,4,0x08);
	LCD_display_byte(67,4,0x7f);
	LCD_display_byte(68,4,0x3e);
	LCD_display_byte(69,4,0x1c);
	LCD_display_byte(70,4,0x08);
	LCD_display_word(speed_data,24,3,52);

	show_speed_num(62,4);
	while(tpflag)
	{
		switch(basic_button())
		{
			case 4: if(speed_num!=0)
					{
						speaker=0;
						speed_num--;
						show_speed_num(62,4);
						speaker=1;
					}
					while(left==0);
					break;
			case 5: if(speed_num!=9)
					{
					    speaker=0;
						speed_num++;
						show_speed_num(62,4);
						speaker=1;
					}
					while(right==0);
					break;
			case 6: tpflag=0;
		 	        speaker=0;
					while(OK==0);
					speaker=1;
					break;
			default:;
		}
	}
}
void game_start_show()
{
	bit tpflag=1;
	LCD_full_draw(start_pic);
	while(tpflag)
	{
		switch(basic_button())
		{
			case 6: tpflag=0;
			        speaker=0;
					while(OK==0);
					speaker=1;
					break;
			default:;
		}
	}
}

void game_initialize()
{
	box_down_reg=0;
	next_mode=6;
	next_shape=2;
	destroy_row_num=0;
	game_over_flag=0;
	pause_game_flag=0;

	LCD_clear();

	time0_reg=0;
	display_basic();

	LCD_display_word(score_data,24,0,3);
	LCD_display_word(speed_data,24,3,3);
	show_score_num(3,1);
	show_speed_num(13,4);

}
void time0_initialize()
{
	TMOD=0x03;
	TR0=1; 
	ET0=1; 
	EA=1; 
}
void Tetris_main()
{
	unsigned char i;
	for(i=0;i<19;i++)
	{
		Box_Ram[i]=Box_Ram_data[i];
	};
	LCD_draw(mpic);

	game_over_flag=0;
	box0_build();
    box_build();
	box0_load();
	box_load();
//	next_box();
	box0_to_Box_Ram(s_box0.x,s_box0.y,s_box0.box);
	box_to_Box_Ram(s_box.x,s_box.y,s_box.box);
	box_down_reg=(20-(speed_num<<1)-1);
	time0_initialize();

	while(!game_over_flag)
	{
		game_button();
	}
	EA=0;
}
void game_over_show()
{
	unsigned char i;
	bit tpflag=1;
	LCD_full_draw(over_pic);
	while(change==0);
	while(tpflag)
	{
		switch(basic_button())
		{
			case 6: tpflag=0;
			        speaker=0;
					while(OK==0);
					speaker=1;
					break;
			default:;
		}
	}
	LCD_clear();
	for(i=0;i<128;i++)
	{
		LCD_display_byte(i,0,0xff);
		LCD_display_byte(i,7,0xff);
	}
	LCD_display_word(score_data,24,3,52);
	show_score_num(52,4);
	tpflag=1;
	while(tpflag)
	{
		switch(basic_button())
		{
			case 6: tpflag=0;
			        speaker=0;
					while(OK==0);
					speaker=1;
					break;
			default:;
		}
	}
}




void main()
{
	LCD_initialize();
	LCD_clear();
	while(1)
	{
		game_start_show();
		select_speed();
		game_initialize();
		Tetris_main();
		game_over_show();	
	}	
}

void timer0() interrupt 1
{
	TH0=0x00;
	TL0=0x00;
	if(time0_reg<10)
	{				  
		time0_reg++;
	}
	else
	{
		time0_reg=0;
		if(pause_game_flag==0)
		{
			game_execute();
			refurbish_display();
		}
	}
}