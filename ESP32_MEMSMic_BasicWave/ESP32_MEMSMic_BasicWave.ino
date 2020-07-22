#include <WiFi.h>
#include "driver/i2s.h"

// I2S setting
#define I2S_SampleRate  44100
i2s_config_t i2s_config = {
      mode: (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
      sample_rate: I2S_SampleRate,
      bits_per_sample: I2S_BITS_PER_SAMPLE_32BIT,
      channel_format: I2S_CHANNEL_FMT_ONLY_LEFT,
      communication_format: (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_LSB),
      intr_alloc_flags: ESP_INTR_FLAG_LEVEL1,
      dma_buf_count: 8,
      dma_buf_len: 8
};
   
i2s_pin_config_t pin_config = {
    .bck_io_num = 21, //this is BCK pin
    .ws_io_num = 25, // this is LRCK pin
    .data_out_num = I2S_PIN_NO_CHANGE, // this is DATA output pin
    .data_in_num = 19   //DATA IN
};
const int i2s_num=0;
int retStat = 0;
int32_t sampleIn=0, avgGain=0, avgGainFilter=0, peaks[2]={60000,-60000}, avgSampleIn=0;
int32_t currentRead=0;
unsigned short int numOfBlanks=1000;
int returnResult=0;
const double samplingFrequency = I2S_SampleRate;
unsigned int sampling_period_us;
unsigned long microseconds;

void setup()
{
  sampling_period_us = round(1000000*(1.0/samplingFrequency));
  Serial.begin(115200); 
  Serial.printf("\r\n\r\n\r\n");
    
  pinMode(19, INPUT);
  returnResult = i2s_driver_install((i2s_port_t)i2s_num, &i2s_config, 0, NULL);
  Serial.printf("\r\nDriver Install Result\t%d", returnResult);
  returnResult = i2s_set_pin((i2s_port_t)i2s_num, &pin_config);
  Serial.printf("\r\nSET PIN Result\t%d", returnResult);
  returnResult = i2s_start((i2s_port_t)i2s_num);
  Serial.printf("\r\nI2S Start Result\t%d", returnResult);
  Serial.print("\r\nBegin Level detect...");
  Serial.printf("\r\n\tRead\t%d\tsamples to level out...", numOfBlanks);
  for(retStat=0; retStat<numOfBlanks; retStat++)
  {
    i2s_pop_sample((i2s_port_t)i2s_num, (char*)&sampleIn, portMAX_DELAY);
    delay(1);
  }
  Serial.printf("\r\n\tRead\t%d\tsamples to get avg...", numOfBlanks);
  for(retStat=0; retStat<numOfBlanks; retStat++)
  {
    i2s_pop_sample((i2s_port_t)i2s_num, (char*)&sampleIn, portMAX_DELAY);
    sampleIn>>=14;
    avgGain += sampleIn;
    delay(1);
  }
  avgGain = avgGain/numOfBlanks;
  
  //Grab filtered gain
  Serial.printf("\r\n\tRead\t%d\tsamples to get avg filter gain...", numOfBlanks);
  for(retStat=0; retStat<numOfBlanks; retStat++)
  {
    i2s_pop_sample((i2s_port_t)i2s_num, (char*)&sampleIn, portMAX_DELAY);
    sampleIn>>=14;
    avgGainFilter += avgGain-sampleIn;
    delay(1);
  }
  avgGainFilter = avgGainFilter/numOfBlanks;
  //set peaks
  peaks[0]=-avgGain;
  peaks[1]=avgGain;

  Serial.printf("\r\nAVG Gain=\t%i\tAVG Filter\t%i\tMaxGain\t%i\tMinGain\t%i", avgGain, avgGainFilter, peaks[0], peaks[1]);
  delay(1000);
}

void loop()
{
  // Output basic waveform output
  Serial.printf("\r\n%d", readMic(10, avgGain, avgGainFilter));
  currentRead = readMic(10, avgGain, avgGainFilter);
  if(currentRead>avgGainFilter || currentRead<-avgGainFilter)
  {
    Serial.print(-12000);
    Serial.print(" , ");
    Serial.print(currentRead);
    Serial.print(" , ");
    Serial.print(12000);
    Serial.println("");
  }
  else
  {
    Serial.printf("\r\n0");
  }
  yield();
}

int32_t readMic(unsigned short int numberOfSamples, int32_t gain, int32_t fGain)
{
  unsigned short int samps=numberOfSamples;
  int32_t inValues=0;
  
  for(samps; samps>0; samps--)
  {
    i2s_pop_sample((i2s_port_t)i2s_num, (char*)&sampleIn, portMAX_DELAY);
    sampleIn>>=14;
    inValues += (gain-sampleIn)-fGain;
    yield();
  }
  inValues = inValues/numberOfSamples;
  return inValues;
}

int32_t readMicOnce(int32_t gain, int32_t fGain)
{
  i2s_pop_sample((i2s_port_t)i2s_num, (char*)&sampleIn, portMAX_DELAY);
  sampleIn>>=14;
  return  (gain-sampleIn)-fGain;
}
