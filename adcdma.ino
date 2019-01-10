//  https://forum.pjrc.com/threads/24963-Teensy-3-1-ADC-with-DMA
// T4 A0 ADC1 7
#include <DMAChannel.h>
#define PRREG(x) Serial.print(#x" 0x"); Serial.println(x,HEX)
#define AUDIO_BLOCK_SAMPLES 1024

DMAMEM static uint16_t analog_rx_buffer[AUDIO_BLOCK_SAMPLES];
DMAChannel dma(false);
uint16_t dc_average;

void setupADC(int pin)
{
  uint32_t i, sum = 0;

  // pin must be 0 to 13 (for A0 to A13)
  // or 14 to 23 for digital pin numbers A0-A9
  // or 34 to 37 corresponding to A10-A13
  if (pin > 23 && !(pin >= 34 && pin <= 37)) return;

  // Configure the ADC and run at least one software-triggered
  // conversion.  This completes the self calibration stuff and
  // leaves the ADC in a state that's mostly ready to use
  analogReadRes(10);
  // analogReference(INTERNAL); // range 0 to 1.2 volts
  //analogReference(DEFAULT); // range 0 to 3.3 volts
  analogReadAveraging(1);
  // Actually, do many normal reads, to start with a nice DC level
  uint32_t us = micros();
  for (i = 0; i < 1024; i++) {
    sum += analogRead(pin);
  }
  us = micros() - us;
  float t = us / 1024.;
  Serial.print(t); Serial.println(" us");
  dc_average = sum >> 10;
  Serial.println(dc_average);


  // enable the ADC for DMA
  ADC1_GC |= ADC_GC_DMAEN | ADC_GC_ADCO;

  PRREG(ADC1_GC);
  PRREG(ADC1_HC0);
  PRREG(ADC1_CFG);

  // set up a DMA channel to store the ADC data
  dma.begin(true); // Allocate the DMA channel first
  dma.source((uint16_t &) ADC1_R0);
  dma.destinationBuffer(analog_rx_buffer, sizeof(analog_rx_buffer));

  dma.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;
  dma.triggerAtHardwareEvent(DMAMUX_SOURCE_ADC1);
  dma.enable();
  dma.attachInterrupt(isr);

  ADC1_GC |= ADC_GC_ADACKEN;
}

volatile uint32_t ticks;
void isr(void)
{
  dma.clearInterrupt();
  ticks++;
}

void setup()
{
  Serial.begin(9600);
  while (!Serial);
  setupADC(14);  //A0
}

void loop()
{
  static int prev;
  Serial.println(ticks - prev);
  Serial.println(analog_rx_buffer[13]);
  prev = ticks;
  delay(2000);
}