/********************************************************************************
 * Include 
 ********************************************************************************/
#include "stdint.h"
#include "Clock.h"
#include "stm32g4xx.h" // Make sure this is included for PWR registers

/********************************************************************************
 * Class Clock
 ********************************************************************************/

void Clock::Init (void) {
    // Enable power interface clock
    RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;
    // Set Range 1 boost mode for 170MHz operation
    PWR->CR5 &= ~PWR_CR5_R1MODE;
    uint32_t rcc_cr = RCC->CR;
    // Enable HSE (external crystal)
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY));

    // Configure Flash latency for 170MHz (see RM0440, Table 16)
    FLASH->ACR = (FLASH->ACR & ~FLASH_ACR_LATENCY) | FLASH_ACR_LATENCY_4WS;

    // Disable PLL before configuring
    RCC->CR &= ~RCC_CR_PLLON;
    while (RCC->CR & RCC_CR_PLLRDY);

    // Configure PLL: PLLSRC = HSE, PLLM = 6, PLLN = 85, PLLR = 2 (170MHz SYSCLK)
    // PLL input = 24MHz / 6 = 4MHz
    // VCO = 4MHz * 85 = 340MHz
    // PLLR output = 340MHz / 2 = 170MHz

    RCC->PLLCFGR = (
        (RCC_PLLCFGR_PLLSRC_HSE)  // HSE as PLL source: 24mhz
        | (0b0101 << RCC_PLLCFGR_PLLM_Pos) // PLLM = 6: 4mhz
        | (85 << RCC_PLLCFGR_PLLN_Pos) // PLLN = 85: 340mhz
        | (0b00 << RCC_PLLCFGR_PLLR_Pos) // PLLR = 2: 170mhz
        | (RCC_PLLCFGR_PLLREN) // Enable PLLR output
    );

    // Enable PLL
    RCC->CR |= RCC_CR_PLLON;
    while ((RCC->CR & RCC_CR_PLLRDY) == 0) {}

    // Select PLL as system clock
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) {}
}

void Clock::EnableMCO (Status status) {
    if (status == Clock::Status::enable) {
        Gpio::Init<8>(GPIOA, Gpio::Mode::outputAF, Gpio::Type::PP, Gpio::Speed::veryHigh, Gpio::Pupd::noPull, Gpio::AF::af0);
        // MCO on PA8, select SYSCLK as source, no prescaler
        RCC->CFGR = (RCC->CFGR & ~(RCC_CFGR_MCOSEL | RCC_CFGR_MCOPRE)) | RCC_CFGR_MCOSEL_0;
    }
    if (status == Clock::Status::disable) {
        Gpio::Init<8>(GPIOA, Gpio::Mode::input);
    }
}