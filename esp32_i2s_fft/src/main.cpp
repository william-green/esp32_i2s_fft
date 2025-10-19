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
#define I2S_SAMPLE_RATE   16000
#define I2S_BUFFER_SIZE   512

#define LED_PIN 2
#define NUM_LEDS 64
#define LED_COLS 8
#define LED_ROWS 8
const int columnBase[LED_COLS] = {0, 8, 16, 24, 32, 40, 48, 56};
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

//tuning parameters
#define BRIGHTNESS 10
#define MIN_THRESHOLD 0.2f
#define MAX_THRESHOLD 2.0f


float vReal[I2S_BUFFER_SIZE];
float vImag[I2S_BUFFER_SIZE];
//float fft_output[I2S_BUFFER_SIZE];
float fft_magnitude_output[I2S_BUFFER_SIZE / 2];
fft_config_t *fft_instance;//global pointer to the fft object

void flash_leds(float* fft_magnitude_output){
    int bin_size = (I2S_BUFFER_SIZE / 2) / LED_COLS; // how many fft bins per led column
    float bin_magnitudes[LED_COLS] = {0};
    float max_magnitude = 0.0f;
    float coeff = BRIGHTNESS / 255.0f;

    //Serial.println("bin size (magnitude entries per led bin): " + String(bin_size));

    for(int i = 0; i < LED_COLS; i++){
        float col_mag_sum = 0.0f;
        for(int j = 0; j < bin_size; j++){
            int bin_index = i * bin_size + j;
            col_mag_sum += fft_magnitude_output[bin_index];
        }
        bin_magnitudes[i] = col_mag_sum;
        if(bin_magnitudes[i] > max_magnitude){
            max_magnitude = bin_magnitudes[i];
        }
    }

    //normalize the values
    for(int i = 0; i < LED_COLS; i++){
        bin_magnitudes[i] /= max_magnitude; // normalize to 0.0 - 1.0
        if(bin_magnitudes[i] < MIN_THRESHOLD){
            bin_magnitudes[i] = 0.0f;
        }
        //bin_magnitudes[i] -= MIN_THRESHOLD;
        //if(bin_magnitudes[i] < MIN_THRESHOLD){
         //   bin_magnitudes[i] = 0.0f;
        //}
    }

    int led_rows_lit[LED_COLS] = {0};
    for(int i = 0; i < LED_COLS; i++){
        led_rows_lit[i] = (int)(bin_magnitudes[i] * LED_ROWS + 0.5f);
        if(led_rows_lit[i] > LED_ROWS){
            led_rows_lit[i] = LED_ROWS;
        }
        //Serial.println("Column " + String(i) + " magnitude: " + String
    }

    for(int i = 0; i < LED_COLS; i++){
        //compute LED addressing
        if(i % 2 == 0){
            //even column, bottom to top
            for(int j = 0; j < LED_ROWS; j++){
                int led_index = columnBase[i] + j;
                if(j < led_rows_lit[i]){
                    strip.setPixelColor(led_index, strip.Color(coeff*255, coeff*255, coeff*255));
                } else {
                    strip.setPixelColor(led_index, strip.Color(0, 0, 0));
                }
            }
        } else {
            //odd column, top to bottom
            for(int j = 0; j < LED_ROWS; j++){
                int led_index = columnBase[i] + (LED_ROWS - 1 - j);
                if(j < led_rows_lit[i]){
                    strip.setPixelColor(led_index, strip.Color(coeff*255, coeff*255, coeff*255));
                } else {
                    strip.setPixelColor(led_index, strip.Color(0, 0, 0));
                }
            }
        }
        //strip.setPixelColor(columnBase[i], strip.Color(255, 255, 255));
    }
    /*
    strip.setPixelColor(0, strip.Color(255, 255, 255));
    strip.setPixelColor(15, strip.Color(255, 255, 255));
    strip.setPixelColor(16, strip.Color(255, 255, 255));
    strip.setPixelColor(31, strip.Color(255, 255, 255));
    strip.setPixelColor(32, strip.Color(255, 255, 255));
    strip.setPixelColor(47, strip.Color(255, 255, 255));
    strip.setPixelColor(48, strip.Color(255, 255, 255));
    strip.setPixelColor(63, strip.Color(255, 255, 255));*/
    strip.show();
    /*for (int i = 0; i < NUM_LEDS; i++) {
        delay(500);
        if(i > 0){
            strip.setPixelColor(i-1, strip.Color(0, 0, 0));
        }
        strip.setPixelColor(i, strip.Color(10, 10, 10));
        strip.show();
        delay(500);
  }*/
  //strip.show();
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
  fft_instance = fft_init(I2S_BUFFER_SIZE, FFT_REAL, FFT_FORWARD, vReal, vImag);//fft_output);


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

    float mean = 0.0f;
  for(int i = 0; i < I2S_BUFFER_SIZE; i++){
    //shift to 24 bit and normalize
    int32_t sample24 = buffer[i] >> 8;
    //because int32_t is signed, normalization by dividing by (2^24)/2 = 8388608
    vReal[i] = sample24 / 8388608.0; // normalize to -1.0..1.0
    vImag[i] = 0.0f;
    mean += vReal[i];
  }

  mean /= I2S_BUFFER_SIZE;
  //Serial.println(mean);

  for(int i = 0; i < I2S_BUFFER_SIZE; i++){
    vReal[i] -= mean;
  }

  fft_execute(fft_instance);

//process the fft results
//break the fft output into real and imaginary parts
//calculate magnitude for each frequency bin
//
    for (int i = 0; i < I2S_BUFFER_SIZE / 2; i++) {
        float real = vReal[i];
        float imag = vImag[i];
    //float real = fft_output[2 * i];       // Real part
    //float imag = fft_output[2 * i + 1];   // Imag part
    fft_magnitude_output[i] = sqrtf(real * real + imag * imag);
    
    //float magnitude = sqrtf(real * real + imag * imag);

    // Optional: compute frequency for reference
    float freq = (float)i * I2S_SAMPLE_RATE / I2S_BUFFER_SIZE;

    //Serial.print(freq, 1);
    //Serial.print(" Hz: ");
    //Serial.println(fft_magnitude_output[i], 6);
    }

    //Serial.println();

    flash_leds(fft_magnitude_output);
    //delay(1);

  //Serial.println(vReal[0]);
  //Serial.println(fft_output[15]);
}
