#include "stm32f10x.h"

void timer2_init(void)
{
		GPIO_InitTypeDef 					GPIO_InitStructure;
		TIM_TimeBaseInitTypeDef		TIM_TimeBaseStructure;
		TIM_OCInitTypeDef  				TIM_OCInitStructure;
			
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB,ENABLE);
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
		GPIO_Init(GPIOA, &GPIO_InitStructure);
	
		TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; 
		TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; 
		TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure); 
		
	
		/* PWM1 Mode configuration: Channel */
		TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;	
		TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;	
		TIM_OCInitStructure.TIM_Pulse = 0;									
		TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;			

		TIM_OC3Init(TIM2, &TIM_OCInitStructure);
		TIM_OC3PreloadConfig(TIM2, TIM_OCPreload_Enable);	
			
		TIM_Cmd(TIM2, ENABLE);
}
