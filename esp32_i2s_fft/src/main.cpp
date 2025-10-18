#include <Arduino.h>
#include <driver/i2s.h>
#include <Adafruit_NeoPixel.h>
//#include <fft.h>

extern "C"{
    #include "fft.h"
}

#define I2S_WS 15
#define I2S_SD 32
#define I2S_SCK 14

#define LED_PIN 2
#define NUM_LEDS 64
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

#define I2S_SAMPLE_RATE   16000
#define I2S_BUFFER_SIZE   512

float vReal[I2S_BUFFER_SIZE];
float vImag[I2S_BUFFER_SIZE];
float fft_output[I2S_BUFFER_SIZE];
fft_config_t *fft_instance;//global pointer to the fft object

void flash_leds(){
    for (int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, strip.Color(10, 10, 10)); // Green
  }
  strip.show();
}

void setup() {
  // put your setup code here, to run once:
  //int result = myFunction(2, 3);

  Serial.begin(115200);

  Serial.println("testing");

  //configure i2s
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX), //configure to receive data
    .sample_rate = I2S_SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT, //mono audio. Selection depends on whether the select pin is high or set to gnd.
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = 0,
    .dma_buf_count = 4,
    .dma_buf_len = I2S_BUFFER_SIZE
  };

  //pin map
  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_SD
  };

  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pin_config);

  Serial.println("I2S configured");

  //configure fft object
  //fft = fft_init(I2S_BUFFER_SIZE, FFT_REAL, FFT_FORWARD, vReal, vImag);
  fft_instance = fft_init(I2S_BUFFER_SIZE, FFT_REAL, FFT_FORWARD, vReal, fft_output);


  Serial.println("FFT configured");

  strip.begin();
  strip.show();
}

void loop() {
  delay(10);
  //Serial.println("alive");
  // put your main code here, to run repeatedly:
  int32_t buffer[I2S_BUFFER_SIZE];
  size_t bytesRead;
  i2s_read(I2S_NUM_0, &buffer, sizeof(buffer), &bytesRead, portMAX_DELAY);

  //float vReal[I2S_BUFFER_SIZE];
  //float vImag[I2S_BUFFER_SIZE];
  for(int i = 0; i < I2S_BUFFER_SIZE; i++){
    //shift to 24 bit and normalize
    int32_t sample24 = buffer[i] >> 8;
    //because int32_t is signed, normalization by dividing by (2^24)/2 = 8388608
    vReal[i] = sample24 / 8388608.0; // normalize to -1.0..1.0
    vImag[i] = 0.0f;
  }

  fft_execute(fft_instance);

//process the fft results
//break the fft output into real and imaginary parts
//calculate magnitude for each frequency bin
//
    for (int i = 0; i < I2S_BUFFER_SIZE / 2; i++) {
    float real = fft_output[2 * i];       // Real part
    float imag = fft_output[2 * i + 1];   // Imag part
    float magnitude = sqrtf(real * real + imag * imag);

    // Optional: compute frequency for reference
    float freq = (float)i * I2S_SAMPLE_RATE / I2S_BUFFER_SIZE;

    Serial.print(freq, 1);
    Serial.print(" Hz: ");
    Serial.println(magnitude, 6);
    }

    Serial.println();

    flash_leds();
    delay(10000);

  //Serial.println(vReal[0]);
  //Serial.println(fft_output[15]);
}
