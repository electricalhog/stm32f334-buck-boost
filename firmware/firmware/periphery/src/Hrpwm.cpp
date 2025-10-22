
/********************************************************************************
 * Include 
 ********************************************************************************/

#include "Hrpwm.h"
#include <stdint.h>

/********************************************************************************
 * Class HRPWM
 ********************************************************************************/

#define HRTIM_TIMERINDEX_TIMER_A  1
#define HRTIM_TIMERINDEX_TIMER_B  2

void Hrpwm::Init() {

    Hrpwm::InitGpio();

    // Enable HRTIM1 clock (on APB2 for G4)
    RCC->APB2ENR |= RCC_APB2ENR_HRTIM1EN;
    __DSB(); // Ensure the clock is enabled before accessing HRTIM1

    // Reset HRTIM1 (optional, but recommended)
    RCC->APB2RSTR |= RCC_APB2RSTR_HRTIM1RST;
    RCC->APB2RSTR &= ~RCC_APB2RSTR_HRTIM1RST;

    // Calibrate DLL (Delay Locked Loop)
    HRTIM1->sCommonRegs.DLLCR |= HRTIM_DLLCR_CAL | HRTIM_DLLCR_CALEN;
    while ((HRTIM1->sCommonRegs.ISR & HRTIM_ISR_DLLRDY) == 0);

    // Set period and compare for Timer A (Master Timer is index 0, Timer A is 1, Timer B is 2)
    HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_A].PERxR = Hrpwm::periodHrpwm;
    HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_A].CMP1xR = 0;
    HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_B].PERxR = Hrpwm::periodHrpwm;
    HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_B].CMP1xR = 0;

    // Dead-time configuration for Timer A and B
    HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_A].OUTxR |= HRTIM_OUTR_DTEN;
    HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_A].DTxR  |= (3 << HRTIM_DTR_DTPRSC_Pos);
    HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_A].DTxR  |= (5 << HRTIM_DTR_DTR_Pos) | (5 << HRTIM_DTR_DTF_Pos);
    HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_A].DTxR  |= HRTIM_DTR_DTFSLK | HRTIM_DTR_DTRSLK;

    HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_B].OUTxR |= HRTIM_OUTR_DTEN;
    HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_B].DTxR  |= (3 << HRTIM_DTR_DTPRSC_Pos);
    HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_B].DTxR  |= (5 << HRTIM_DTR_DTR_Pos) | (5 << HRTIM_DTR_DTF_Pos);
    HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_B].DTxR  |= HRTIM_DTR_DTFSLK | HRTIM_DTR_DTRSLK;

    // Set and reset events for PWM
    HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_A].SETx1R |= HRTIM_SET1R_PER;
    HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_A].RSTx1R |= HRTIM_RST1R_CMP1;
    HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_B].SETx1R |= HRTIM_SET1R_PER;
    HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_B].RSTx1R |= HRTIM_RST1R_CMP1;

    // Continuous mode
    HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_A].TIMxCR |= HRTIM_TIMCR_CONT;
    HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_B].TIMxCR |= HRTIM_TIMCR_CONT;

    // Enable outputs (TA1 and TB1)
    HRTIM1->sCommonRegs.OENR |= 
        HRTIM_OENR_TA1OEN  |  // PA8
        HRTIM_OENR_TA2OEN |  // PA9
        HRTIM_OENR_TB1OEN  |  // PA10
        HRTIM_OENR_TB2OEN;   // PA11
    // Set main output polarity (active high) and complementary (active low)
    HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_A].OUTxR &= ~(1 << 0); // POL1 = 0
    HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_A].OUTxR |=  (1 << 1); // NPOL1 = 1
    
    HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_B].OUTxR &= ~(1 << 0); // POL1 = 0
    HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_B].OUTxR |=  (1 << 1); // NPOL1 = 1
    uint32_t oenr = HRTIM1->sCommonRegs.OENR;

    // Set master timer period and enable
    HRTIM1->sMasterRegs.MPER = Hrpwm::periodHrpwm;
    HRTIM1->sMasterRegs.MCR |= HRTIM_MCR_MCEN | HRTIM_MCR_TACEN | HRTIM_MCR_TBCEN;
};

void Hrpwm::SetDuty(Channel channel, uint16_t duty) {
    if (channel == Hrpwm::Channel::boost) { HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_A].CMP1xR = Hrpwm::periodHrpwm - duty; }
    if (channel == Hrpwm::Channel::buck)  { HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_B].CMP1xR = duty; }
};

 void Hrpwm::InitGpio() {
    // Configure PA8 (HRTIM1_CH_A1), PA9 (HRTIM1_CH_A1N), PA10 (HRTIM1_CH_B1), PA11 (HRTIM1_CH_B1N) as alternate function 13 (AF13)
    Gpio::Init<8,9,10,11>(GPIOA, Gpio::Mode::outputAF, Gpio::Type::PP, Gpio::Speed::veryHigh, Gpio::Pupd::pullDown, Gpio::AF::af13);
    // Configure PA4 and PA6 as outputs for enables signals
    Gpio::Init<4,6>(GPIOA, Gpio::Mode::output, Gpio::Type::PP, Gpio::Speed::low, Gpio::Pupd::noPull);
 }

void Hrpwm::SentEnable(Status input_enable, Status output_enable) {
    if (input_enable == Hrpwm::Status::enable) { Gpio::Set<4>(GPIOA); }
    else if (input_enable == Hrpwm::Status::disable) {Gpio::Reset<4>(GPIOA); }
    if (output_enable == Hrpwm::Status::enable) { Gpio::Set<6>(GPIOA); }
    else if (output_enable == Hrpwm::Status::disable) { Gpio::Reset<6>(GPIOA); }
}