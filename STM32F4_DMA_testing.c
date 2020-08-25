#include <stm32f4xx_ll_bus.h>
#include <stm32f4xx_ll_gpio.h>
#include <stm32f4xx_ll_utils.h>
#include <stm32f4xx_ll_pwr.h>
#include <stm32f4xx_ll_rcc.h>
#include <stm32f4xx_ll_system.h>
#include <stm32f4xx_ll_cortex.h>
#include <stm32f4xx_ll_usart.h>
#include <stm32f4xx_ll_fmc.h>
#include <stm32f4xx_ll_spi.h>
#include <stm32f4xx_ll_dma.h>
#include "stdarg.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"




//SDRAM-FMC
#define SDRAM_MODEREG_BURST_LENGTH_1             ((uint16_t)0x0000)
#define SDRAM_MODEREG_BURST_LENGTH_2             ((uint16_t)0x0001)
#define SDRAM_MODEREG_BURST_LENGTH_4             ((uint16_t)0x0002)
#define SDRAM_MODEREG_BURST_LENGTH_8             ((uint16_t)0x0004)
#define SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL      ((uint16_t)0x0000)
#define SDRAM_MODEREG_BURST_TYPE_INTERLEAVED     ((uint16_t)0x0008)
#define SDRAM_MODEREG_CAS_LATENCY_2              ((uint16_t)0x0020)
#define SDRAM_MODEREG_CAS_LATENCY_3              ((uint16_t)0x0030)
#define SDRAM_MODEREG_OPERATING_MODE_STANDARD    ((uint16_t)0x0000)
#define SDRAM_MODEREG_WRITEBURST_MODE_PROGRAMMED ((uint16_t)0x0000) 
#define SDRAM_MODEREG_WRITEBURST_MODE_SINGLE     ((uint16_t)0x0200)  

#define EXTERNAL_SDRAM_SIZE				0x400000
#define EXTERNAL_SDRAM_BANK_ADDR		((uint32_t)0xD0000000)
#define EXTERNAL_SDRAM_CAS_LATENCY		FMC_SDRAM_CAS_LATENCY_3
#define SDCLOCK_PERIOD					FMC_SDRAM_CLOCK_PERIOD_2 
#define EXTERNAL_SDRAM_READBURST		FMC_SDRAM_RBURST_DISABLE 
#define external_start_addr       *(__IO uint16_t*)(EXTERNAL_SDRAM_BANK_ADDR )

//DMA
void initDMA(void);

//FMC
void clear_SDRAM(uint16_t value);
void memCheck(void);
void fmc_cmdStruct_config(FMC_SDRAM_CommandTypeDef *sdramCMD);
void SDRAM_InitSequence(void);
void init_SDRAM(void);
void init_SDRAM_GPIO(void);

//CL
void setClockTo180(void);
void CL_printMsg_init_Default(void);
void CL_printMsg(char *msg, ...);
void initLed(void);
void blinkLed(uint16_t times, uint16_t del);
uint16_t internal_buff[100]; 

int main(void)
{
	setClockTo180();
	CL_printMsg_init_Default();
	CL_printMsg("SystemCoreClock : %d \n", SystemCoreClock);
	
	init_SDRAM(); 
	FMC_SDRAM_WriteProtection_Disable(FMC_Bank5_6, FMC_SDRAM_BANK2);
//	clear_SDRAM(0x001F);
	for (int counter = 0; counter < 100; counter++)
	{
		CL_printMsg("%d\n", *(__IO uint16_t*)(EXTERNAL_SDRAM_BANK_ADDR + 2*counter));
		
	}
	
	for (int counter = 0; counter < 100; counter++)
	{
		internal_buff[counter] =counter;
	}
	for (int counter = 0; counter < 100; counter++)
	{
		CL_printMsg("%d\n", internal_buff[counter]);
	}
	CL_printMsg("Internal Buff init done\n");
	
	initDMA();
	LL_DMA_EnableStream(DMA1, LL_DMA_STREAM_0);
	CL_printMsg("SDRAM DMA transfer done\n");
	for (int counter = 0; counter < 100; counter++)
	{
		CL_printMsg("%d\n", *(__IO uint16_t*)(EXTERNAL_SDRAM_BANK_ADDR + 2*counter));
		
	}
	
	
	for (;;)
	{
	
	}
}
void initDMA(void)
{
	RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;
	LL_DMA_InitTypeDef dma;
	LL_DMA_StructInit(&dma);
	dma.Direction = LL_DMA_DIRECTION_MEMORY_TO_MEMORY;
	dma.PeriphOrM2MSrcAddress	= (uint32_t)&internal_buff;
	dma.PeriphOrM2MSrcDataSize	= LL_DMA_MDATAALIGN_HALFWORD; 
	dma.PeriphOrM2MSrcIncMode	= LL_DMA_PERIPH_INCREMENT;
	dma.MemoryOrM2MDstAddress	= EXTERNAL_SDRAM_BANK_ADDR;
	dma.MemoryOrM2MDstDataSize	= LL_DMA_MDATAALIGN_HALFWORD;
	dma.MemoryOrM2MDstIncMode	= LL_DMA_MEMORY_INCREMENT;	
	dma.NbData = 100;
	LL_DMA_Init(DMA1,LL_DMA_STREAM_0, &dma);
}

void clear_SDRAM(uint16_t value)
{
	for (int counter = 0x00; counter < EXTERNAL_SDRAM_SIZE; counter++)
	{
		*(__IO uint16_t*)(EXTERNAL_SDRAM_BANK_ADDR + counter) = (uint16_t)value;
	}
}//--------------------------------------------------------------------------------
void memCheck(void)
{
	uint8_t ubWritedata_8b = 0x3C, ubReaddata_8b = 0;  
	uint16_t uhWritedata_16b = 0x1E5A, uhReaddata_16b = 0;  
	uint32_t uwReadwritestatus = 0;
	uint32_t counter = 0x0;
	CL_printMsg("Mem clear initialized \n");
	for (counter = 0x00; counter < EXTERNAL_SDRAM_SIZE; counter++)
	{
		*(__IO uint8_t*)(EXTERNAL_SDRAM_BANK_ADDR + counter) = (uint8_t)0x0;
	}
	CL_printMsg("Mem clear done\n");
	
	
	//--------------| 8 bit memory writes 	
	
		CL_printMsg("Mem 8bit  write  start\n");
	for (counter = 0; counter < EXTERNAL_SDRAM_SIZE; counter++)
	{
		*(__IO uint8_t*)(EXTERNAL_SDRAM_BANK_ADDR + counter) = (uint8_t)(ubWritedata_8b + counter);
	}
	CL_printMsg("Mem 8bit  write  done\n");
	
	
	counter = 0;
	uwReadwritestatus = 0;
	CL_printMsg("mem read and check start\n");
	while ((counter < EXTERNAL_SDRAM_SIZE) && (uwReadwritestatus == 0))
	{
		ubReaddata_8b = *(__IO uint8_t*)(EXTERNAL_SDRAM_BANK_ADDR + counter);
		if (ubReaddata_8b != (uint8_t)(ubWritedata_8b + counter))
		{
			uwReadwritestatus = 1;
			CL_printMsg("8bit mem write NOT ok!\n");
			          
		}
		else
		{
			
		}
		counter++;
	} 
	CL_printMsg("8bit mem write ok!\n");
	
	//--------------| 16 bit memory writes 
		//clear 
			CL_printMsg("Mem clear initialized \n");
	for (counter = 0x00; counter < EXTERNAL_SDRAM_SIZE; counter++)
	{
		*(__IO uint16_t*)(EXTERNAL_SDRAM_BANK_ADDR + 2*counter) = (uint16_t)0x00;
	}
	CL_printMsg("Mem clear done\n");
	/* Write data value to all SDRAM memory */
	CL_printMsg("Mem 16bit  write  start\n");
	for (counter = 0; counter < EXTERNAL_SDRAM_SIZE; counter++)
	{
		*(__IO uint16_t*)(EXTERNAL_SDRAM_BANK_ADDR + 2*counter) = (uint16_t)(uhWritedata_16b + counter);
	}
	CL_printMsg("Mem 16bit  write  done\n");
	/* Read back SDRAM memory and check content correctness*/
	counter = 0;
	uwReadwritestatus = 0;
	CL_printMsg("mem read and check start\n");
	while ((counter < EXTERNAL_SDRAM_SIZE) && (uwReadwritestatus == 0))
	{
		uhReaddata_16b = *(__IO uint16_t*)(EXTERNAL_SDRAM_BANK_ADDR + 2*counter);
		if (uhReaddata_16b != (uint16_t)(uhWritedata_16b + counter))
		{
			uwReadwritestatus = 1;
			CL_printMsg("16bit mem write NOT ok!\n");
		}
		else
		{
			
		}
		counter++;
	}
	CL_printMsg("16bit mem write ok!\n");
}//--------------------------------------------------------------------------------
void fmc_cmdStruct_config(FMC_SDRAM_CommandTypeDef *sdramCMD)
{
	uint32_t temp = 0;
	temp = (uint32_t)(sdramCMD->CommandMode |
                      sdramCMD->CommandTarget |
                     (((sdramCMD->AutoRefreshNumber) - 1) << 5) |
                     ((sdramCMD->ModeRegisterDefinition) << 9));
  
	FMC_Bank5_6->SDCMR = temp;
}//--------------------------------------------------------------------------------
void SDRAM_InitSequence(void)
{
	uint32_t temp = 0;
	FMC_SDRAM_CommandTypeDef sdramCMD;
	sdramCMD.CommandMode	= 	FMC_SDRAM_CMD_CLK_ENABLE;
	sdramCMD.CommandTarget	=	FMC_SDRAM_CMD_TARGET_BANK2;
	sdramCMD.AutoRefreshNumber =	1;
	sdramCMD.ModeRegisterDefinition = 0;
	
	while (__FMC_SDRAM_GET_FLAG(FMC_Bank5_6, FMC_SDRAM_FLAG_BUSY) !=  0) ;
	fmc_cmdStruct_config(&sdramCMD);
	
	LL_mDelay(10);
	/*_____________________________________________________________________________________*/
	sdramCMD.CommandMode	= 	FMC_SDRAM_CMD_PALL;
	sdramCMD.CommandTarget	=	FMC_SDRAM_CMD_TARGET_BANK2;  //unchanged from previous
	sdramCMD.AutoRefreshNumber =	1;
	sdramCMD.ModeRegisterDefinition = 0;
	while (__FMC_SDRAM_GET_FLAG(FMC_Bank5_6, FMC_SDRAM_FLAG_BUSY) !=  0) ;
	fmc_cmdStruct_config(&sdramCMD);
	/*_____________________________________________________________________________________*/
	
	sdramCMD.CommandMode	= 	FMC_SDRAM_CMD_AUTOREFRESH_MODE;
	sdramCMD.CommandTarget	=	FMC_SDRAM_CMD_TARGET_BANK2;
	sdramCMD.AutoRefreshNumber =	4;
	sdramCMD.ModeRegisterDefinition = 0;
	
	while (__FMC_SDRAM_GET_FLAG(FMC_Bank5_6, FMC_SDRAM_FLAG_BUSY) !=  0) ;
	fmc_cmdStruct_config(&sdramCMD);
	while (__FMC_SDRAM_GET_FLAG(FMC_Bank5_6, FMC_SDRAM_FLAG_BUSY) !=  0) ;
	fmc_cmdStruct_config(&sdramCMD);
	/*_____________________________________________________________________________________*/
	temp = (uint32_t)SDRAM_MODEREG_BURST_LENGTH_2 | SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL | SDRAM_MODEREG_CAS_LATENCY_3 | SDRAM_MODEREG_OPERATING_MODE_STANDARD | SDRAM_MODEREG_WRITEBURST_MODE_SINGLE;
	
	sdramCMD.CommandMode	= 	FMC_SDRAM_CMD_LOAD_MODE;
	sdramCMD.CommandTarget	=	FMC_SDRAM_CMD_TARGET_BANK2;
	sdramCMD.AutoRefreshNumber =	1;
	sdramCMD.ModeRegisterDefinition = temp;
	
	while (__FMC_SDRAM_GET_FLAG(FMC_Bank5_6, FMC_SDRAM_FLAG_BUSY) !=  0) ;
	fmc_cmdStruct_config(&sdramCMD);
	/*_____________________________________________________________________________________*/
	FMC_SDRAM_ProgramRefreshRate(FMC_Bank5_6, 1386);
	while (__FMC_SDRAM_GET_FLAG(FMC_Bank5_6, FMC_SDRAM_FLAG_BUSY) !=  0) ;
	
	
}//--------------------------------------------------------------------------------
void init_SDRAM(void)
{
	init_SDRAM_GPIO();
	
	RCC->AHB3ENR |= RCC_AHB3ENR_FMCEN;
	
	
	FMC_SDRAM_TimingTypeDef		sdramTim;	
	sdramTim.LoadToActiveDelay		= 2;
	sdramTim.ExitSelfRefreshDelay	= 7;
	sdramTim.SelfRefreshTime		= 4;
	sdramTim.RowCycleDelay			= 7;
	sdramTim.WriteRecoveryTime		= 2;
	sdramTim.RPDelay				= 2;
	sdramTim.RCDDelay				= 2;
	
	FMC_SDRAM_InitTypeDef		sdramInit;
	sdramInit.SDBank			 = FMC_SDRAM_BANK2;
	sdramInit.ColumnBitsNumber	 = FMC_SDRAM_COLUMN_BITS_NUM_8;
	sdramInit.RowBitsNumber		 = FMC_SDRAM_ROW_BITS_NUM_12;
	sdramInit.MemoryDataWidth	 = FMC_SDRAM_MEM_BUS_WIDTH_16;
	sdramInit.InternalBankNumber = FMC_SDRAM_INTERN_BANKS_NUM_4;
	sdramInit.CASLatency		 = FMC_SDRAM_CAS_LATENCY_3;
	sdramInit.WriteProtection	 = FMC_SDRAM_WRITE_PROTECTION_DISABLE;
	sdramInit.SDClockPeriod      = FMC_SDRAM_CLOCK_PERIOD_2;
	sdramInit.ReadBurst			 = FMC_SDRAM_RBURST_DISABLE;
	sdramInit.ReadPipeDelay		 = FMC_SDRAM_RPIPE_DELAY_1;
	FMC_SDRAM_Init(FMC_Bank5_6, &sdramInit);
	FMC_SDRAM_Timing_Init(FMC_Bank5_6, &sdramTim, FMC_SDRAM_BANK2);
	
	SDRAM_InitSequence();
	
}//--------------------------------------------------------------------------------
void init_SDRAM_GPIO(void)
{
	
  
	/* Enable GPIOs clock GPIO B C D E F G */
	RCC->AHB1ENR |=  RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN | RCC_AHB1ENR_GPIODEN | RCC_AHB1ENR_GPIOEEN | RCC_AHB1ENR_GPIOFEN | RCC_AHB1ENR_GPIOGEN;
                            
	/*-- GPIOs Configuration -----------------------------------------------------*/
	/*
	 +-------------------+--------------------+--------------------+--------------------+
	 +                       SDRAM pins assignment                                      +
	 +-------------------+--------------------+--------------------+--------------------+
	 | PD0  <-> FMC_D2   | PE0  <-> FMC_NBL0  | PF0  <-> FMC_A0    | PG0  <-> FMC_A10   |
	 | PD1  <-> FMC_D3   | PE1  <-> FMC_NBL1  | PF1  <-> FMC_A1    | PG1  <-> FMC_A11   |
	 | PD8  <-> FMC_D13  | PE7  <-> FMC_D4    | PF2  <-> FMC_A2    | PG8  <-> FMC_SDCLK |
	 | PD9  <-> FMC_D14  | PE8  <-> FMC_D5    | PF3  <-> FMC_A3    | PG15 <-> FMC_NCAS  |
	 | PD10 <-> FMC_D15  | PE9  <-> FMC_D6    | PF4  <-> FMC_A4    |--------------------+ 
	 | PD14 <-> FMC_D0   | PE10 <-> FMC_D7    | PF5  <-> FMC_A5    |   
	 | PD15 <-> FMC_D1   | PE11 <-> FMC_D8    | PF11 <-> FMC_NRAS  | 
	 +-------------------| PE12 <-> FMC_D9    | PF12 <-> FMC_A6    | 
	                     | PE13 <-> FMC_D10   | PF13 <-> FMC_A7    |    
	                     | PE14 <-> FMC_D11   | PF14 <-> FMC_A8    |
	                     | PE15 <-> FMC_D12   | PF15 <-> FMC_A9    |
	                    +-------------------+--------------------+--------------------+
	                    | PB5 <-> FMC_SDCKE1| 
	                    | PB6 <-> FMC_SDNE1 | 
	                    | PC0 <-> FMC_SDNWE |
	                    +-------------------+  
  
	                    */
  
	                     
	LL_GPIO_InitTypeDef gpio;
	LL_GPIO_StructInit(&gpio);
	
	gpio.Mode  = LL_GPIO_MODE_ALTERNATE;
	gpio.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
	gpio.Alternate = LL_GPIO_AF_12;


  
	gpio.Pin = LL_GPIO_PIN_5  | LL_GPIO_PIN_6;      
	LL_GPIO_Init(GPIOB, &gpio);  	
  
	gpio.Pin = LL_GPIO_PIN_0;      
	LL_GPIO_Init(GPIOC, &gpio);  


	gpio.Pin = LL_GPIO_PIN_0 | LL_GPIO_PIN_1  | LL_GPIO_PIN_8 | LL_GPIO_PIN_9 | LL_GPIO_PIN_10 | LL_GPIO_PIN_14 | LL_GPIO_PIN_15;
	LL_GPIO_Init(GPIOD, &gpio);


	gpio.Pin = LL_GPIO_PIN_0  | LL_GPIO_PIN_1  | LL_GPIO_PIN_7 | LL_GPIO_PIN_8  | LL_GPIO_PIN_9  | LL_GPIO_PIN_10 | LL_GPIO_PIN_11 | LL_GPIO_PIN_12 | LL_GPIO_PIN_13 |  LL_GPIO_PIN_14 | LL_GPIO_PIN_15;
	LL_GPIO_Init(GPIOE, &gpio);



	gpio.Pin = LL_GPIO_PIN_0  | LL_GPIO_PIN_1 | LL_GPIO_PIN_2 | LL_GPIO_PIN_3  | LL_GPIO_PIN_4 | LL_GPIO_PIN_5 | LL_GPIO_PIN_11 | LL_GPIO_PIN_12 | LL_GPIO_PIN_13 | LL_GPIO_PIN_14 | LL_GPIO_PIN_15;      
	LL_GPIO_Init(GPIOF, &gpio);

  

	gpio.Pin = LL_GPIO_PIN_0 | LL_GPIO_PIN_1 | LL_GPIO_PIN_4 | LL_GPIO_PIN_5 | LL_GPIO_PIN_8 | LL_GPIO_PIN_15;
	LL_GPIO_Init(GPIOG, &gpio);
}//--------------------------------------------------------------------------------
void setClockTo180(void)
{
	LL_FLASH_SetLatency(LL_FLASH_LATENCY_5);

	if (LL_FLASH_GetLatency() != LL_FLASH_LATENCY_5)
	{
		//	Error_Handler();  
	}
	
	
	LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE1);
	LL_PWR_EnableOverDriveMode();
	LL_RCC_HSE_Enable();

	/* Wait till HSE is ready */
	while (LL_RCC_HSE_IsReady() != 1)
	{
    
	}
	LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSE, LL_RCC_PLLM_DIV_4, 180, LL_RCC_PLLP_DIV_2);
	LL_RCC_PLL_Enable();

	/* Wait till PLL is ready */
	while (LL_RCC_PLL_IsReady() != 1)
	{
    
	}
	
	
	LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
	LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_4);
	LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_2);
	LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);

	/* Wait till System clock is ready */
	while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL)
	{
  
	}
	LL_Init1msTick(180000000);
	LL_SYSTICK_SetClkSource(LL_SYSTICK_CLKSOURCE_HCLK);
	LL_SetSystemCoreClock(180000000);
	LL_RCC_SetTIMPrescaler(LL_RCC_TIM_PRESCALER_TWICE);
}//--------------------------------------------------------------------------------
void CL_printMsg_init_Default(void)
{

	LL_USART_InitTypeDef USART_InitStruct = { 0 };

	LL_GPIO_InitTypeDef GPIO_InitStruct = { 0 };

	/* Peripheral clock enable */
	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_USART1);
  
	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
	/**USART1 GPIO Configuration  
	PA9   ------> USART1_TX
	PA10   ------> USART1_RX 
	*/
	LL_GPIO_StructInit(&GPIO_InitStruct);
	GPIO_InitStruct.Pin = LL_GPIO_PIN_9;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
	GPIO_InitStruct.Alternate = LL_GPIO_AF_7;
	LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/* USER CODE BEGIN USART1_Init 1 */

	/* USER CODE END USART1_Init 1 */
	LL_USART_StructInit(&USART_InitStruct);
	USART_InitStruct.BaudRate = 115200;
	USART_InitStruct.TransferDirection = LL_USART_DIRECTION_TX;
	LL_USART_Init(USART1, &USART_InitStruct);
	LL_USART_ConfigAsyncMode(USART1);
	USART1->BRR = 0x30D;
	LL_USART_Enable(USART1);
}//--------------------------------------------------------------------------------
void CL_printMsg(char *msg, ...)
{	
	char buff[80];	
	va_list args;
	va_start(args, msg);
	vsprintf(buff, msg, args);
		
	for (int i = 0; i < strlen(buff); i++)
	{		
		USART1->DR = buff[i];
		while (!(USART1->SR & USART_SR_TXE)) ;
	}		
		
	while (!(USART1->SR & USART_SR_TC)) ;		
}//--------------------------------------------------------------------------------
void initLed(void)
{	
	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOG);
	LL_GPIO_InitTypeDef gpio;
	LL_GPIO_StructInit(&gpio);
	gpio.Mode = LL_GPIO_MODE_OUTPUT;
	gpio.Pin = LL_GPIO_PIN_14 | LL_GPIO_PIN_13;
	LL_GPIO_Init(GPIOG, &gpio);


}//--------------------------------------------------------------------------------
void blinkLed(uint16_t times, uint16_t del)
{
	for (int i = 0; i < times; i++)
	{
		LL_GPIO_SetOutputPin(GPIOG, LL_GPIO_PIN_14);
		LL_mDelay(del);
		LL_GPIO_ResetOutputPin(GPIOG, LL_GPIO_PIN_13);
		LL_mDelay(del);
		LL_GPIO_SetOutputPin(GPIOG, LL_GPIO_PIN_13);
		LL_mDelay(del);
		LL_GPIO_ResetOutputPin(GPIOG, LL_GPIO_PIN_14);
		LL_mDelay(del);
	}
}
