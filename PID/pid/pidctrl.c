

 
/**




**/

#include "stm32f10x.h"
/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"
#include "pidctrl.h"
#include "ds18b20.h" 
#include "main.h"
#ifndef ABS
#define ABS(a)	   (((a) < 0) ? -(a) : (a))
#endif

#define WINDUP_ON(_pid)         (_pid->features & PID_ENABLE_WINDUP)
#define DEBUG_ON(_pid)          (_pid->features & PID_DEBUG)
#define SAT_MIN_ON(_pid)        (_pid->features & PID_OUTPUT_SAT_MIN)
#define SAT_MAX_ON(_pid)        (_pid->features & PID_OUTPUT_SAT_MAX)
#define HIST_ON(_pid)           (_pid->features & PID_INPUT_HIST)


#define PID_CREATTRM_INTERVAL  	(250*4)  /*250*2 ms ---  pid once that again and again*/

#ifdef USING_PID





/*****external SSR ctrl module*********/
 
 uint8_t Interrupt_Extern=0;
 uint16_t Adj_Power_Time=0;
 uint16_t Power_Adj=0;


 void initBta16TMER()
 {
 //ʹ����һ��100US�Ķ�ʱ����
 //��ʼ��
 uint16_t 	usPrescalerValue;
	NVIC_InitTypeDef NVIC_InitStructure;
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	//====================================ʱ�ӳ�ʼ��===========================
	//ʹ�ܶ�ʱ��2ʱ��
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
	//====================================��ʱ����ʼ��===========================
	//��ʱ��ʱ�������˵��
	//HCLKΪ72MHz��APB1����2��ƵΪ36MHz
	//TIM2��ʱ�ӱ�Ƶ��Ϊ72MHz��Ӳ���Զ���Ƶ,�ﵽ���
	//TIM2�ķ�Ƶϵ��Ϊ3599��ʱ���Ƶ��Ϊ72 / (1 + Prescaler) = 100KHz,��׼Ϊ10us
	//TIM������ֵΪusTim1Timerout50u	
	usPrescalerValue = (uint16_t) (72000000 / 25000) - 1;
	//Ԥװ��ʹ��
	TIM_ARRPreloadConfig(TIM4, ENABLE);
	//====================================�жϳ�ʼ��===========================
	//����NVIC���ȼ�����ΪGroup2��0-3��ռʽ���ȼ���0-2����Ӧʽ���ȼ�
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_3);
	NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	//�������жϱ�־λ
	TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
	//��ʱ��2����жϹر�
	TIM_ITConfig(TIM4, TIM_IT_Update, DISABLE);
	//��ʱ��3����
	TIM_Cmd(TIM4, DISABLE);

	TIM_TimeBaseStructure.TIM_Prescaler = usPrescalerValue;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseStructure.TIM_Period = (uint16_t)( 1);//100us
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);

	TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
	TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);
	TIM_SetCounter(TIM4, 0);
	TIM_Cmd(TIM4, DISABLE);
 
 }
 void TIM4_IRQHandler(void)
{
	
	if (TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET)
	{
		
		TIM_ClearFlag(TIM4, TIM_FLAG_Update);	     //���жϱ��
		TIM_ClearITPendingBit(TIM4, TIM_IT_Update);	 //�����ʱ��TIM3����жϱ�־λ

	    Adj_Power_Time++;//increase time 
  	   AutoRunPowerAdjTask();
	}
	
}	
void EXTI4_IRQHandler(void)
{
     
	 	Interrupt_Extern=1;
		Adj_Power_Time=0;
    /* Clear the Key Button EXTI line pending bit */
    EXTI_ClearITPendingBit(EXTI_Line4);

   
}
void InitzeroGPIO()
 {
 
//DrvGPIO_SetIntCallback(pfP0P1Callback, pfP2P3P4Callback);
//DrvGPIO_EnableInt(E_PORT2, E_PIN7, E_IO_RISING, E_MODE_EDGE);
	   //��������Ϊ�ⲿ�ж��½�����Ч

	 NVIC_InitTypeDef NVIC_InitStructure;
	 GPIO_InitTypeDef GPIO_InitStructure;
     EXTI_InitTypeDef EXTI_InitStructure;

	  /* Enable GPIOC and GPIOB clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

   RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);
  
  /* Enable AFIO clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

 	/* Enable the EXTI3 Interrupt */
    NVIC_InitStructure.NVIC_IRQChannel = EXTI4_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

	/* configure PB4 as SSR ctrl pin */

		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOE, &GPIO_InitStructure);

	  GPIO_SetBits(GPIOE,GPIO_Pin_2);
	  GPIO_ResetBits(GPIOE,GPIO_Pin_2);
	  	/* configure PB3 as external interrupt */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

     GPIO_EXTILineConfig(GPIO_PortSourceGPIOC, GPIO_PinSource4);
	 
	    /* Configure  EXTI Line to generate an interrupt on falling edge */
    EXTI_InitStructure.EXTI_Line = EXTI_Line4;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

	/* Clear the Key Button EXTI line pending bit */
	EXTI_ClearITPendingBit(EXTI_Line4); 
	  
 }


/*
���ù��ʰٷֱ�

 Power ��Χ��0-100
*/
void Set_Power(char Power)
{



	 Power_Adj=100-Power;



}

void Trdelay()
{

	  uint32_t i;
	  for(i=500;i;i--);


}
 void Trigger_SSR_Task()
 {

	 

 	// Delay_init(72);
	 GPIO_ResetBits(GPIOE,GPIO_Pin_2);	//
//	 GPIO_SetBits(GPIOE,GPIO_Pin_2);
	// Delay_us(30);
	  Trdelay();
  //  GPIO_ResetBits(GPIOE,GPIO_Pin_2);
//	 Delay_us(30);
	 GPIO_SetBits(GPIOE,GPIO_Pin_2);
	 // DrvGPIO_ClrBit(E_PORT4, E_PIN3); 
	 // DrvSYS_Delay(20);// 20us
	  //DrvGPIO_SetBit(E_PORT4, E_PIN3); 
	 
				   
 }

 /*----------Power Task Auto  Run---------------------------------------------------------------------*/
 void AutoRunPowerAdjTask()
 {
		   if(Interrupt_Extern==1)
		 {	
				 if( Adj_Power_Time>=Power_Adj)
				   {
				   
				   	Trigger_SSR_Task();		 
		 		   				 
				   	 
					 Interrupt_Extern=0;

				   }
		 		  	
			
		 }	


 }


 void InitSSR(void)
 {
 	InitzeroGPIO();
 
   initBta16TMER();
 }

 /*start of PID*/

 extern float Temp_pid[1];
 uint32_t PidCreatTrm=0;

 	PID_t pid;
	float result,kj;
	



extern void DS1820main(void); 		
//PID

void init_PID(void)
{
	/*��ʼ��PID�ṹ*/
		/*
	  	 ��������=1��
		 ��������=0��
		 ΢������=0��

	   */	
   	 pid_init(&pid, 15, 0, 0);
	  /*ʹ�ܻ��߽���PID����Ĺ���*/	
   //pid_enable_feature(&pid, , 0)		
	/*Ŀ��ֵ�趨*/		 
     pid_set(&pid,40);

}
void PidThread(uint32_t Time)
{  
     float temppid;
     			 
if (Time - PidCreatTrm >= PID_CREATTRM_INTERVAL)
  {
        PidCreatTrm =  Time;
	 
	   DS1820main();//�ɼ���ǰ�¶�  
	    
	temppid =  Temp_pid[0]/100;

	result  =  pid_calculate(&pid,temppid, 1) ;
		
		   if((result<100)||(result>0))/*0-100����*/
		   {
		   	  Set_Power(result);
		   }

		  /*if(Power_Adj==100)
			   Power_Adj=0;
			   else
			   Power_Adj++;
		  	  */
		TIM_Cmd(TIM4, ENABLE);

  }
	
	
}


void pid_enable_feature(PID_t *pid, unsigned int feature, float value)
{
    pid->features |= feature;

    switch (feature) {
        case PID_ENABLE_WINDUP:
            /* integral windup is in absolute output units, so scale to input units */
            pid->intmax = ABS(value / pid->ki);
            break;
        case PID_DEBUG:
            break;
        case PID_OUTPUT_SAT_MIN:
            pid->sat_min = value;
            break;
        case PID_OUTPUT_SAT_MAX:
            pid->sat_max = value;
            break;
        case PID_INPUT_HIST:
            break;
    }
}

/**
 *
 * @param pid
 * @param kp
 * @param ki
 * @param kd
 */
void pid_init(PID_t *pid, float kp, float ki, float kd)
{
	pid->kp = kp;
	pid->ki = ki;
	pid->kd = kd;

	pid->sp = 0;
	pid->error_previous = 0;
	pid->integral = 0;

    pid->features = 0;

    if (DEBUG_ON(pid))
        printf("setpoint,value,P,I,D,error,i_total,int_windup\n");
}

void pid_set(PID_t *pid, float sp)
{
	pid->sp = sp;
	pid->error_previous = 0;
	pid->integral = 0;
}

/**
 *
 * @param pid
 * @param val  NOW VALUE
 * @param dt   
 * @return
 */
float pid_calculate(PID_t *pid, float val, float dt)
{
	float i,d, error, total;

	error = pid->sp - val;
	i = pid->integral + (error * dt);
	d = (error - pid->error_previous) / dt;

    total = (error * pid->kp) + (i * pid->ki) + (d * pid->kd);

    if (DEBUG_ON(pid))
        printf("%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%d\n", 
                pid->sp,val,
                (error * pid->kp), (i * pid->ki), (d * pid->kd),
                error, pid->integral, ABS(i) == pid->intmax);

    if ( WINDUP_ON(pid) ) {
        if ( i < 0 )
            i = ( i < -pid->intmax ? -pid->intmax : i );
        else
   		    i = ( i < pid->intmax ? i : pid->intmax );
    }
    pid->integral = i;

    if ( SAT_MIN_ON(pid) && (total < pid->sat_min) )
        return pid->sat_min;
    if ( SAT_MAX_ON(pid) && (total > pid->sat_max) )
        return pid->sat_max;

	pid->error_previous = error;
	return total;
}


 /*end of PID*/



 #endif

