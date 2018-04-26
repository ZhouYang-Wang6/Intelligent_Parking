#include "stm32f10x.h"
#include "rc522.h"
#include "gpio.h"
#include "pwm.h"
#include <string.h>
#include <stdio.h>

//初始化参数
u8 CT[2];	//类型码
u8 SN[4];	//序列码
u8 status = 0;
u8 buf3[16];
u8 KEY[6]={0xff,0xff,0xff,0xff,0xff,0xff};
u8 RFID_R[16];		//存放读RFID
u8 RFID_W[] = {"*wangzhouyang*"};	//存放写RFID
u8 s=0x02;
u8 zigbee_flag = 0;
u8 user_flag = 0;
u16 fac_us;
u16 fac_ms;

//延迟程序
void delay_us(u32 nus)	//微秒延时
{
		u32 i=0,j=0;
		for(i=0; i<nus; i++)
		{
			for(j=0; j<72; j++)
			{
				__NOP();
			}
		}
}

void delay_ms(u32 nms)	//毫秒延时
{
		u32 i=0;
		for(i=0; i<nms; i++)
		{
			delay_us(1000);
		}

}
//蜂鸣器响声控制
void ring(u32 rnum)
{
		switch (rnum)
		{
		case 0:
			GPIO_ResetBits(GPIOA,GPIO_Pin_4);
			delay_ms(50);
			GPIO_SetBits(GPIOA,GPIO_Pin_4);
			break;
		case 1:
			GPIO_ResetBits(GPIOA,GPIO_Pin_4);
			delay_ms(50);
			GPIO_SetBits(GPIOA,GPIO_Pin_4);
			delay_ms(50);
			GPIO_ResetBits(GPIOA,GPIO_Pin_4);
			delay_ms(50);
			GPIO_SetBits(GPIOA,GPIO_Pin_4);
			break;
		case 2:
			GPIO_ResetBits(GPIOA,GPIO_Pin_4);
			delay_ms(200);
			GPIO_SetBits(GPIOA,GPIO_Pin_4);
			break;
		}
}
//检查用户
u8 user_check()
{
	u8 check_i = 0;
	for(check_i = 0; check_i < sizeof(RFID_W); check_i++)
	{
		if(RFID_R[check_i] != RFID_W[check_i])
		{
			return 0;
		}
	}
	return 1;
}



void Judge_close(void)
{	
	//寻卡
	status = PcdRequest(PICC_REQALL,CT);
	if(status==MI_OK)
	{
		status = MI_ERR;
	}

	//防冲撞
	status = PcdAnticoll(SN);
	if(status==MI_OK)
	{
		status = MI_ERR;
	}

	//选卡
	status = PcdSelect(SN);
	if(status==MI_OK)
	{
		status=MI_ERR;
	}

	//验证
	status =PcdAuthState(0x60,0x03,KEY,SN);
	if(status==MI_OK)
	{
		status=MI_ERR;
	}
	//写卡
//	status = PcdWrite(s, RFID_W);
//	if (status == MI_OK)
//	{
//		status=MI_ERR;
//	}

	//读卡
	status=PcdRead(s,RFID_R);
	if(status==MI_OK)
	{
		status=MI_ERR;
		//有空位且是验证用户
		if((user_check() == 1) && ((GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_8)==1) || (GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_9)==1) || (GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_10)==1)))
		{
			ring(0);
			TIM_SetCompare3(TIM2,750);
			delay_ms(500);
			TIM_SetCompare3(TIM2,250);
			if(user_flag == 1)
			{
			}

			user_flag = 0;
			memset(RFID_R,0,16);
		}
		//非验证用户
		else if(user_check() == 0)
		{
			user_flag = 1;
			ring(2);
		}
		//无空位或两者皆不是
		else if(((GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_8)==1) || (GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_9)==1) || (GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_10)==1))==0)
		{
			user_flag = 1;
			ring(1);
		}
	}
	
}


int main(void)
{
	GPIOA_init();	//初始化IO
	pwm_configuration();	//初始化舵机
	TIM_SetCompare3(TIM2,250);	//舵机回正

	delay_ms(30);
	InitRc522();
	PcdReset();
	PcdAntennaOff();
	delay_ms(30);
	PcdAntennaOn();
	M500PcdConfigISOType('A');
	delay_ms(30);
	while(1)
	{
		if(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_8)==0)
			GPIO_ResetBits(GPIOB,GPIO_Pin_6);
		else
			GPIO_SetBits(GPIOB,GPIO_Pin_6);
		
		if(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_9)==0)
			GPIO_ResetBits(GPIOB,GPIO_Pin_7);
		else
			GPIO_SetBits(GPIOB,GPIO_Pin_7);
		
		if(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_10)==0)
			GPIO_ResetBits(GPIOB,GPIO_Pin_8);
		else
			GPIO_SetBits(GPIOB,GPIO_Pin_8);
		Judge_close();
		PcdHalt();

	}
}
