#include <debug.h>
#include <macros.h>
#include <escape.h>
#include "stm32f7xx.h"

#if !defined  (HSE_VALUE) 
  #define HSE_VALUE    ((uint32_t)19200000) /*!< Default value of the External oscillator in Hz */
#endif /* HSE_VALUE */

#if !defined  (HSI_VALUE)
  #define HSI_VALUE    ((uint32_t)16000000) /*!< Value of the Internal oscillator in Hz*/
#endif /* HSI_VALUE */

// Vector table base address
extern const uint32_t g_pfnVectors[];

  /* This variable is updated in three ways:
      1) by calling CMSIS function SystemCoreClockUpdate()
      2) by calling HAL API function HAL_RCC_GetHCLKFreq()
      3) each time HAL_RCC_ClockConfig() is called to configure the system clock frequency 
         Note: If you use this function to configure the system clock; then there
               is no need to call the 2 first functions listed above, since SystemCoreClock
               variable is updated automatically.
  */
  uint32_t SystemCoreClock = 16000000;
  const uint8_t AHBPrescTable[16] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 6, 7, 8, 9};
  const uint8_t APBPrescTable[8] = {0, 0, 0, 0, 1, 2, 3, 4};

/**
  * @brief  Setup the microcontroller system
  *         Initialize the Embedded Flash Interface, the PLL and update the 
  *         SystemFrequency variable.
  * @param  None
  * @retval None
  */
void SystemInit(void)
{
  /* FPU settings ------------------------------------------------------------*/
  #if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
    SCB->CPACR |= ((3UL << 10*2)|(3UL << 11*2));  /* set CP10 and CP11 Full Access */
  #endif
  /* Reset the RCC clock configuration to the default reset state ------------*/
  /* Set HSION bit */
  RCC->CR |= (uint32_t)0x00000001;

  /* Reset CFGR register */
  RCC->CFGR = 0x00000000;

  /* Reset HSEON, CSSON and PLLON bits */
  RCC->CR &= (uint32_t)0xFEF6FFFF;

  /* Reset PLLCFGR register */
  RCC->PLLCFGR = 0x24003010;

  /* Reset HSEBYP bit */
  RCC->CR &= (uint32_t)0xFFFBFFFF;

  /* Disable all interrupts */
  RCC->CIR = 0x00000000;

  /* Configure the Vector Table location add offset address ------------------*/
  SCB->VTOR = (uint32_t)g_pfnVectors;
}

/**
   * @brief  Update SystemCoreClock variable according to Clock Register Values.
  *         The SystemCoreClock variable contains the core clock (HCLK), it can
  *         be used by the user application to setup the SysTick timer or configure
  *         other parameters.
  *           
  * @note   Each time the core clock (HCLK) changes, this function must be called
  *         to update SystemCoreClock variable value. Otherwise, any configuration
  *         based on this variable will be incorrect.         
  *     
  * @note   - The system frequency computed by this function is not the real 
  *           frequency in the chip. It is calculated based on the predefined 
  *           constant and the selected clock source:
  *             
  *           - If SYSCLK source is HSI, SystemCoreClock will contain the HSI_VALUE(*)
  *                                              
  *           - If SYSCLK source is HSE, SystemCoreClock will contain the HSE_VALUE(**)
  *                          
  *           - If SYSCLK source is PLL, SystemCoreClock will contain the HSE_VALUE(**) 
  *             or HSI_VALUE(*) multiplied/divided by the PLL factors.
  *         
  *         (*) HSI_VALUE is a constant defined in stm32f7xx_hal_conf.h file (default value
  *             16 MHz) but the real value may vary depending on the variations
  *             in voltage and temperature.   
  *    
  *         (**) HSE_VALUE is a constant defined in stm32f7xx_hal_conf.h file (default value
  *              25 MHz), user has to ensure that HSE_VALUE is same as the real
  *              frequency of the crystal used. Otherwise, this function may
  *              have wrong result.
  *                
  *         - The result of this function could be not correct when using fractional
  *           value for HSE crystal.
  *     
  * @param  None
  * @retval None
  */
void SystemCoreClockUpdate(void)
{
  uint32_t tmp = 0, pllvco = 0, pllp = 2, pllsource = 0, pllm = 2;
  
  /* Get SYSCLK source -------------------------------------------------------*/
  tmp = RCC->CFGR & RCC_CFGR_SWS;

  switch (tmp)
  {
    case 0x00:  /* HSI used as system clock source */
      SystemCoreClock = HSI_VALUE;
      break;
    case 0x04:  /* HSE used as system clock source */
      SystemCoreClock = HSE_VALUE;
      break;
    case 0x08:  /* PLL used as system clock source */

      /* PLL_VCO = (HSE_VALUE or HSI_VALUE / PLL_M) * PLL_N
         SYSCLK = PLL_VCO / PLL_P
         */    
      pllsource = (RCC->PLLCFGR & RCC_PLLCFGR_PLLSRC) >> 22;
      pllm = RCC->PLLCFGR & RCC_PLLCFGR_PLLM;
      
      if (pllsource != 0)
      {
        /* HSE used as PLL clock source */
        pllvco = (HSE_VALUE / pllm) * ((RCC->PLLCFGR & RCC_PLLCFGR_PLLN) >> 6);
      }
      else
      {
        /* HSI used as PLL clock source */
        pllvco = (HSI_VALUE / pllm) * ((RCC->PLLCFGR & RCC_PLLCFGR_PLLN) >> 6);      
      }

      pllp = (((RCC->PLLCFGR & RCC_PLLCFGR_PLLP) >>16) + 1 ) *2;
      SystemCoreClock = pllvco/pllp;
      break;
    default:
      SystemCoreClock = HSI_VALUE;
      break;
  }
  /* Compute HCLK frequency --------------------------------------------------*/
  /* Get HCLK prescaler */
  tmp = AHBPrescTable[((RCC->CFGR & RCC_CFGR_HPRE) >> 4)];
  /* HCLK frequency */
  SystemCoreClock >>= tmp;
}

/**
 * @brief  This is the code that gets called when the processor receives an
 *         unexpected interrupt.  This simply enters an infinite loop, preserving
 *         the system state for examination by a debugger.
 * @param  None
 * @retval None
*/
void debug_handler()
{
	uint32_t ipsr = __get_IPSR();
#ifdef DEBUG
#ifndef BOOTLOADER
	SCB_Type scb = *SCB;
	NVIC_Type nvic = *NVIC;
	printf(ESC_ERROR "\nUnexpected interrupt: %lu\n", ipsr);
#endif
#else
	printf(ESC_ERROR "\nUnexpected fault: %lu\n\n", ipsr);
#endif

	fflush(stdout);
#ifdef DEBUG
	SCB_CleanInvalidateDCache();
	__BKPT(0);
#endif
	NVIC_SystemReset();
}

#ifdef DEBUG
#ifndef BOOTLOADER
void HardFault_Handler()
{
	SCB_Type scb = *SCB;
	printf(ESC_ERROR "\nHard fault: 0x%08lx\n", scb.HFSR);
	printf("...\t%s%s%s\n",
	       scb.HFSR & SCB_HFSR_DEBUGEVT_Msk ? "Debug " : "",
	       scb.HFSR & SCB_HFSR_FORCED_Msk ? "Forced " : "",
	       scb.HFSR & SCB_HFSR_VECTTBL_Msk ? "Vector " : "");
	uint8_t mfsr = FIELD(scb.CFSR, SCB_CFSR_MEMFAULTSR);
	if (mfsr & 0x80) {	// Memory manage fault valid
		printf("Memory manage fault: 0x%02x\n", mfsr);
		printf("...\t%s%s%s%s\n",
		       mfsr & 0x10 ? "Entry " : "",
		       mfsr & 0x08 ? "Return " : "",
		       mfsr & 0x02 ? "Data " : "",
		       mfsr & 0x01 ? "Instruction " : "");
		printf("...\tAddress: 0x%08lx\n", scb.MMFAR);
	}
	uint8_t bfsr = FIELD(scb.CFSR, SCB_CFSR_BUSFAULTSR);
	if (bfsr & 0x80) {	// Bus fault valid
		printf("Bus fault: 0x%02x\n", bfsr);
		printf("...\t%s%s%s%s%s\n",
		       bfsr & 0x10 ? "Entry " : "",
		       bfsr & 0x08 ? "Return " : "",
		       bfsr & 0x04 ? "Imprecise " : "",
		       bfsr & 0x02 ? "Precise " : "",
		       bfsr & 0x01 ? "Instruction " : "");
		if (bfsr & 0x02)
			printf("...\tPrecise: 0x%08lx\n", scb.BFAR);
	}
	uint16_t ufsr = FIELD(scb.CFSR, SCB_CFSR_USGFAULTSR);
	if (ufsr) {	// Usage fault
		printf("Usage fault: 0x%04x\n", ufsr);
		printf("...\t%s%s%s%s%s%s\n",
		       ufsr & 0x0200 ? "Divide " : "",
		       ufsr & 0x0100 ? "Unaligned " : "",
		       ufsr & 0x0008 ? "Coprocessor " : "",
		       ufsr & 0x0004 ? "INVPC " : "",
		       ufsr & 0x0002 ? "INVSTATE " : "",
		       ufsr & 0x0001 ? "UNDEFINSTR " : "");
	}

	fflush(stdout);
	SCB_CleanInvalidateDCache();
	__BKPT(0);
	NVIC_SystemReset();
}
#endif
#endif
