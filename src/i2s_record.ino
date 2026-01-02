#include "driver/i2s_std.h"

#ifndef DEBUG
#define DEBUG true
#define DebugPrint(x); if (DEBUG) { Serial.print(x); }
#define DebugPrintln(x); if (DEBUG) { Serial.println(x); }
#endif

#define I2S_WS 22
#define I2S_SD 35
#define I2S_SCK 33

#define SAMPLE_RATE 16000
#define BITS_PER_SAMPLE 8
#define GAIN_BOOSTER_I2S 10

i2s_std_config_t std_cfg = {
  .clk_cfg = {
    .sample_rate_hz = SAMPLE_RATE,
    .clk_src = I2S_CLK_SRC_DEFAULT,
    .mclk_multiple = I2S_MCLK_MULTIPLE_256,
  },
  .slot_cfg = {
    .data_bit_width = I2S_DATA_BIT_WIDTH_16BIT,
    .slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO,
    .slot_mode = I2S_SLOT_MODE_MONO,
    .slot_mask = I2S_STD_SLOT_RIGHT,
    .ws_width = I2S_DATA_BIT_WIDTH_16BIT,
    .ws_pol = false,
    .bit_shift = true,
    .msb_right = false,
  },
  .gpio_cfg = {
    .mclk = I2S_GPIO_UNUSED,
    .bclk = (gpio_num_t)I2S_SCK,
    .ws = (gpio_num_t)I2S_WS,
    .dout = I2S_GPIO_UNUSED,
    .din = (gpio_num_t)I2S_SD,
    .invert_flags = {
      .mclk_inv = false,
      .bclk_inv = false,
      .ws_inv = false,
    },
  },
};

i2s_chan_handle_t rx_handle;

struct WAV_HEADER {
  char riff[4] = { 'R', 'I', 'F', 'F' };
  long flength = 0;
  char wave[4] = { 'W', 'A', 'V', 'E' };
  char fmt[4] = { 'f', 'm', 't', ' ' };
  long chunk_size = 16;
  short format_tag = 1;
  short num_chans = 1;
  long srate = SAMPLE_RATE;
  long bytes_per_sec = SAMPLE_RATE * (BITS_PER_SAMPLE / 8);
  short bytes_per_samp = (BITS_PER_SAMPLE / 8);
  short bits_per_samp = BITS_PER_SAMPLE;
  char dat[4] = { 'd', 'a', 't', 'a' };
  long dlength = 0;
} myWAV_Header;

bool flg_is_recording = false;
bool flg_I2S_initialized = false;

bool I2S_Record_Init() {
  i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
  i2s_new_channel(&chan_cfg, NULL, &rx_handle);
  i2s_channel_init_std_mode(rx_handle, &std_cfg);
  i2s_channel_enable(rx_handle);
  flg_I2S_initialized = true;
  return true;
}

bool Record_Start(String audio_filename) {
  if (!flg_I2S_initialized) return false;

  if (!flg_is_recording) {
    flg_is_recording = true;
    if (SD.exists(audio_filename)) SD.remove(audio_filename);
    File audio_file = SD.open(audio_filename, FILE_WRITE);
    audio_file.write((uint8_t*)&myWAV_Header, 44);
    audio_file.close();
    return true;
  }

  int16_t audio_buffer[1500];
  uint8_t audio_buffer_8bit[1500];
  size_t bytes_read = 0;

  i2s_channel_read(rx_handle, audio_buffer, sizeof(audio_buffer), &bytes_read, portMAX_DELAY);

  if (GAIN_BOOSTER_I2S > 1 && GAIN_BOOSTER_I2S <= 64) {
    for (int i = 0; i < bytes_read / 2; i++) {
      audio_buffer[i] *= GAIN_BOOSTER_I2S;
    }
  }

  if (BITS_PER_SAMPLE == 8) {
    for (int i = 0; i < bytes_read / 2; i++) {
      audio_buffer_8bit[i] = (uint8_t)(((audio_buffer[i] + 32768) >> 8) & 0xFF);
    }
  }

  File audio_file = SD.open(audio_filename, FILE_APPEND);
  if (!audio_file) return false;

  if (BITS_PER_SAMPLE == 16)
    audio_file.write((uint8_t*)audio_buffer, bytes_read);
  else
    audio_file.write((uint8_t*)audio_buffer_8bit, bytes_read / 2);

  audio_file.close();
  return true;
}

bool Record_Available(String audio_filename, float* audiolength_sec) {
  if (!flg_is_recording || !flg_I2S_initialized) return false;

  File audio_file = SD.open(audio_filename, "r+");
  long filesize = audio_file.size();
  audio_file.seek(0);
  myWAV_Header.flength = filesize;
  myWAV_Header.dlength = filesize - 8;
  audio_file.write((uint8_t*)&myWAV_Header, 44);
  audio_file.close();

  flg_is_recording = false;
  *audiolength_sec = (float)(filesize - 44) / (SAMPLE_RATE * BITS_PER_SAMPLE / 8);
  return true;
}
