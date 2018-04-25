#include "stm32f10x.h"
void GPIOA_init(void)
{
								GPIO_InitTypeDef GPIO_InitStructure;
								RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);

								GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 || GPIO_Pin_1 || GPIO_Pin_2;
								GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
								GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
								GPIO_Init(GPIOA, &GPIO_InitStructure);

								GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
								GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
								GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
								GPIO_Init(GPIOA, &GPIO_InitStructure);
								GPIO_SetBits(GPIOA,GPIO_Pin_4);

}
