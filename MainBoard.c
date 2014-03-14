//**************************************************************************************
//單鍵密碼鎖主板程式
//編寫日期:2007/10/03
//採用AVR ATMEGA8515
//功能:接收密碼鎖副板利用UART傳過來的密碼,然後和EEPROM裡面的密碼比較看是否相同
// 相同的話就啟動RELAY.
// 裡面可以利用4*4鍵盤按B設定密碼,按A輸入密碼確定是否正確
//程式內容:程式分為(1)LCD16*2的驅動
// (2)4*4鍵盤的掃描輸入
// (3)EEPROM的讀寫
//**************************************************************************************
#include “lcd1602.h"
#define SetBit(bit) (1<<(bit)) //定義SetBit巨集指令
#define ClearBit(bit) ~(1<<(bit)) //定義ClearBit巨集指令
#define check(adr,bit)(adr&(1<
//鐵門開啟開關
#define Switch_door_open PA7
//鐵門解鎖開關
#define Switch_door_unlock PA6
unsigned char G_lcd_show_start_index; //LCD16*2 移動式字慕的字段啟始位置
unsigned char *G_lcd_show_str = " press A to keyin password;or press B to set password";
unsigned char G_select_reg; //判斷按下那個4*4按鑑的暫存器
unsigned char G_keyin_number_reg[10]; //用來存放輸入密碼的暫存器
unsigned char G_now_keyin_number_count; //現在輸入的密碼的計數值
//**************************************************************************************
//——————————————————————
// 主程式
//——————————————————————
void main(void){

//——————————————————————
// PORT的初始設定
//——————————————————————
PORTC = 0xff; //LCD16*2的DATA BUS
DDRC = 0xff; //設為輸出

PORTE = 0xff; //LCD16*2的E,RW,RS接腳
DDRE = 0×00; //設為輸入

PORTA = 0xff;
DDRA = 0×00; //設為輸入
DDRA |= (1< DDRA |= (1<
PORTD = 0xff;
DDRD = 0×00; //設為輸入

//SFIOR &= ClearBit(PUD); // 提升電阻on
//——————————————————————
// 中斷設定
//——————————————————————
	SREG |= (1<<7); //Global interrupt enable set
//——————————————————————
// 延遲一下
//——————————————————————
	delay_ms(100);
//——————————————————————
// LCD16*2的初始設定
//——————————————————————
	LCD_init();
//——————————————————————
// USART的初始設定
//——————————————————————
	USART_Init(9600);
//——————————————————————
// 主迴圈
//——————————————————————
while(1)
{
	//————————————————————
	// 此迴圈將4*4鍵盤按下的按鑑A和B轉成下面程式作為判斷的依據
	//————————————————————
	while(1)
	{
		//————————————————–
		// LCD16*2的畫面一直更新的副程式
		//————————————————–
		Lcd_show_sub();
		//————————————————–
	
		if(Keyboard_value_sub() == 10) //代表4*4鍵盤按A,選擇輸入程式
		{
			G_select_reg = 1;
			break;
		}
		if(Keyboard_value_sub() == 11) //代表4*4鑑盤按B,進入密碼設定程式
		{
			G_select_reg = 2;
			break;
		}
	}
//————————————————————
//————————————————————
// 根據Select_reg的值判斷進入那個副程式
//————————————————————

if(G_select_reg == 1)
{
	G_now_keyin_number_count = 0; //初始
	//————————————————–
	// 判斷輸入密碼是否正確的副程式
	//————————————————–
	Check_password_sub();
	//————————————————–
}

if(G_select_reg == 2) //設定密碼模式
{
	//————————————————–
	// 設定密碼到EEPROM的副程式
	//————————————————–
	Set_password_sub();
}

//————————————————————–
	delay_ms(250);
	delay_ms(250);
	delay_ms(250);
	delay_ms(250);
	delay_ms(250);
	delay_ms(250);
	delay_ms(250);
	delay_ms(250);
	}
}
//**************************************************************************************
//——————————————————————
// LCD16*2的畫面一直更新的副程式
//——————————————————————
void Lcd_show_sub()
{
	char i;

	LCD_write_str(0,0,"Welcome To Use!!");
	delay_ms(250);
	delay_ms(250);

	LCD_clear();

	for(i = G_lcd_show_start_index ;i <= 15 + G_lcd_show_start_index ;i++ ) //*G_lcd_show_str指標的內容值一個一個往左移
	{
		LCD_write_char(i – G_lcd_show_start_index,1,*(G_lcd_show_str + i));
	}

	G_lcd_show_start_index++; //整個字串往左移一格

	if(G_lcd_show_start_index == 60) //到達60清除為0重新開始
	{
		G_lcd_show_start_index = 0;
	}
}
//**************************************************************************************
//——————————————————————
//判斷輸入密碼是否正確的副程式
//——————————————————————
void Check_password_sub(void)
{
	char i;
	unsigned char Keyboard_read_num_reg;

	//————————————————————
	//一個小迴圈一直把鍵盤輸入的值讀出來,並判斷按鍵盤那個鍵
	//————————————————————
	
	while(1)
	{
		delay_ms(200);
		LCD_clear();
		LCD_write_str(0,0,"Enter(A)\Clear(B)");

		if(G_now_keyin_number_count >= 1) //假如有輸入數字則開始把輸入的字顯示出來
		{
			for(i = 0 ;i <= G_now_keyin_number_count-1 ;i++ )
			{
				LCD_write_char(i ,1 ,48 + G_keyin_number_reg[i]);
			}
		}

		Keyboard_read_num_reg = Keyboard_value_sub(); //讀鍵盤的輸入值

		if(Keyboard_read_num_reg >=0 && Keyboard_read_num_reg <= 9) //判斷鍵盤輸入的值是否為0~9
		{
			if(G_now_keyin_number_count < 10) //判斷密碼輸入數目是否達到10
			{
				G_keyin_number_reg[G_now_keyin_number_count] = Keyboard_read_num_reg;
				G_now_keyin_number_count++;
			}
		}

		if(Keyboard_read_num_reg == 10) //代表鍵盤按A,表示輸入結束並把輸入的值丟到判斷副程式去判斷是否正確
		{
			if(Check_eeprom_password_sub() == 1) //判斷密碼如果完全相同擇輸出check ok!!
			{
				LCD_clear();
				LCD_write_str(0,0,"check OK!!");
				//Open_door_sub();
			}
			else
			{
				LCD_clear();
				LCD_write_str(0,0,"check error!!");
			}

			break;
		}

		if(Keyboard_read_num_reg == 11) //代表鍵盤按B,清除輸入密碼重新輸入
		{
			G_now_keyin_number_count = 0;
			for(i = 0 ; i<= 9 ;i++ )
				G_keyin_number_reg[i]=";
		}
	}
}
//**************************************************************************************
//——————————————————————
// 把輸入的密碼和EEPROM存的密碼作判斷的函數
// 回傳:1代表全部正確,0代表有錯誤
//——————————————————————
unsigned char Check_eeprom_password_sub(void)
{
	char i;
	unsigned char Check_flag;
	if(G_now_keyin_number_count > 0)
	{
		G_now_keyin_number_count–; //先減1

		Check_flag = 1;
		
		for ( i = 0; i <= G_now_keyin_number_count; i++) // 判斷是否輸入和密碼全部相同
		{
			if(EEPROM_read(i) != G_keyin_number_reg[i])
			{
				Check_flag = 0;
			}
		}
	}
	else
	{
		Check_flag = 0;
	}

	return Check_flag;
}
//**************************************************************************************
//——————————————————————
// 設定新的密碼到EEPROM的副程式
//——————————————————————
void Set_password_sub(void)
{
	unsigned char Keyboard_read_num_reg;
	char i;
	unsigned char Eeprom_write_count_reg = 0;

	while(1)
	{
		delay_ms(200);
		LCD_clear();
		LCD_write_str(0,0,"Enter(A) for ok");

		if(Eeprom_write_count_reg >= 1)
		{
			//————————————————————-
			//LCD顯示出已輸入密碼的迴圈
			//————————————————————-
			for( i = 0; i <= Eeprom_write_count_reg – 1; i++)
			{
				LCD_write_char(i,1,48+EEPROM_read(i));
			}
			//————————————————————-
		}

		Keyboard_read_num_reg = Keyboard_value_sub();

		if(Keyboard_read_num_reg >=0 && Keyboard_read_num_reg <= 9) //判斷輸入是否為0~9
		{
			if(Eeprom_write_count_reg < 10) //判斷輸入數目是否達到10
			{
				EEPROM_write(Eeprom_write_count_reg ,Keyboard_read_num_reg);
				Eeprom_write_count_reg++;
			}
		}

		if(Keyboard_read_num_reg == 10) //鍵盤按A
		{
			LCD_clear();
			LCD_write_str(0,0,"password set ok!!");
			break;
		}
	}
}
//**************************************************************************************
//——————————————————————
//鐵門解鎖並開啟
//——————————————————————
void Open_door_sub()
{
	PORTA &= ~(1< delay_ms(200);
	delay_ms(200);
	delay_ms(200);
	delay_ms(200);
	delay_ms(200);
	PORTA |= (1< delay_ms(200);
	delay_ms(200);
	delay_ms(200);
	PORTA &= ~(1< delay_ms(200);
	delay_ms(200);
	delay_ms(200);
	delay_ms(200);
	delay_ms(200);
	PORTA |= (1< 
}
//——————————————————————