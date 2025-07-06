/********************************************************************************
 * Include 
 ********************************************************************************/

#include "Adc.h"

/********************************************************************************
 * Variables
 ********************************************************************************/

uint16_t Adc::inputVoltage [Adc::sizeBuffer];
uint16_t Adc::inputCurrent [Adc::sizeBuffer];
uint16_t Adc::outputVoltage [Adc::sizeBuffer];
uint16_t Adc::outputCurrent [Adc::sizeBuffer];

uint8_t Adc::step = 0;

bool Adc::Status::stopInputVoltage = false;
bool Adc::Status::stopInputCurrent = false;
bool Adc::Status::stopOutputVoltage = false;
bool Adc::Status::stopOutputCurrent = false;

/********************************************************************************
 * Class ADC
 ********************************************************************************/

void Adc::Init() {
    // Enable ADC12 clock on STM32G4 (AHB2ENR)
    RCC->AHB2ENR |= RCC_AHB2ENR_ADC12EN;
    RCC->CCIPR |= RCC_CCIPR_ADC12SEL_1; // select SYSCLK as peripheral clock for ADC12


    Adc::GpioInit();
    Adc::InitTimerEvent();
    Adc::StartCallibrationAdc();

    // Configure injected sequence: 4 conversions, trigger on TIM6 TRGO, rising edge, channels IN1, IN2, IN3, IN4
    ADC1->JSQR = (3 << ADC_JSQR_JL_Pos) |           // JL: 4 conversions (JL = 3 means 4 conversions)
                 (0b01101 << ADC_JSQR_JEXTSEL_Pos) |     // JEXTSEL: TIM6_TRGO (see RM0440 Table 100)
                 (1 << ADC_JSQR_JEXTEN_Pos) |       // JEXTEN: 0 = disabled, 1 = rising edge
                 (1 << ADC_JSQR_JSQ1_Pos) |         // JSQ1: IN1
                 (2 << ADC_JSQR_JSQ2_Pos) |         // JSQ2: IN2
                 (3 << ADC_JSQR_JSQ3_Pos) |         // JSQ3: IN3
                 (4 << ADC_JSQR_JSQ4_Pos);          // JSQ4: IN4

    ADC1->IER |= ADC_IER_JEOSIE;            // Interrupt enable for injected end of sequence
    NVIC_EnableIRQ(ADC1_2_IRQn);            // Enable interrupt ADC1 and ADC2

    ADC1->CR |= ADC_CR_ADEN;                // Enable ADC1
    while(!(ADC1->ISR & ADC_ISR_ADRDY));    // Wait until ADC1 is ready

    ADC1->CR |= ADC_CR_JADSTART;            // Start injected conversion
}

void Adc::GpioInit() {
    // Configure PA0, PA1, PA2, PA3, PA5 as analog (for ADC1 IN1-IN4)
    Gpio::Init<0,1,2,3>(GPIOA, Gpio::Mode::analog);
}

void Adc::StartCallibrationAdc() {
    // Enable ADC as per RM0440, Section 21.4.6
    ADC1->CR &= ~ADC_CR_DEEPPWD;           // Clear DEEPPWD bit to exit deep power-down mode3
    ADC1->CR |= ADC_CR_ADVREGEN;            // Set ADVREGEN bit to enable voltage regulator
    for (volatile int i = 0; i < 1000; ++i); // Short delay for regulator startup

    ADC1->CR &= ~ADC_CR_ADCALDIF;           // Single-ended calibration
    ADC1->CR |= ADC_CR_ADCAL;               // Start calibration
    while (ADC1->CR & ADC_CR_ADCAL);        // Wait for calibration to finish
}

void Adc::InitTimerEvent() {
    RCC->APB1ENR1 |= RCC_APB1ENR1_TIM6EN;   // Enable TIM6 clock (APB1ENR1 for G4)
    TIM6->PSC = 1-1;
    TIM6->ARR = 36;
    TIM6->CR2 |= TIM_CR2_MMS_1;             // TRGO on update event
    TIM6->CR1  |= TIM_CR1_CEN;
}

/********************************************************************************
 * ADC handler
 ********************************************************************************/

extern "C" void ADC1_2_IRQHandler(void) {
    ADC1->ISR |= ADC_ISR_JEOS;  
 
    if (!Adc::Status::stopInputVoltage) { Adc::inputVoltage[Adc::step] = ADC1->JDR1; }
    if (!Adc::Status::stopInputCurrent) { Adc::inputCurrent[Adc::step] = ADC1->JDR2; }
    if (!Adc::Status::stopOutputCurrent) { Adc::outputCurrent[Adc::step] = ADC1->JDR3; }
    if (!Adc::Status::stopOutputVoltage) { Adc::outputVoltage[Adc::step] = ADC1->JDR4; }

    if (Adc::step >= Adc::sizeBuffer) { Adc::step = 0; } 
    Adc::step++;
}