/**
 * plc_adc.c - ADC/DAC 硬件抽象层实现
 *
 * 使用通道配置数组存储各通道参数
 * 电压计算公式: mv = raw * vref_mv / (1 << resolution)
 * DAC 反向计算: raw = mv * (1 << resolution) / output_range_mv
 */

#include "plc_adc.h"
#include <string.h>

/* ========== 内部数据结构 ========== */

/** ADC 通道运行状态 */
typedef struct {
  PlcAdcConfig  config;
  bool          configured;
  uint32_t      last_raw;
  uint32_t      last_mv;
  uint32_t      read_count;
} AdcChannelEntry;

/** DAC 通道运行状态 */
typedef struct {
  PlcDacConfig  config;
  bool          configured;
  uint32_t      last_raw;
} DacChannelEntry;

/* ========== 静态变量 ========== */

static AdcChannelEntry s_adc_channels[PLC_ADC_MAX_CHANNELS];
static DacChannelEntry s_dac_channels[PLC_DAC_MAX_CHANNELS];
static uint8_t s_adc_init_done = 0;
static uint8_t s_dac_init_done = 0;

/* ========== ADC 内部辅助 ========== */

/** 根据原始值和配置计算毫伏值 */
static uint32_t adc_calc_mv(uint32_t raw, const PlcAdcConfig* cfg) {
  uint32_t max_val = (1u << cfg->resolution) - 1;
  if (max_val == 0) max_val = 1;
  return (uint32_t)((uint64_t)raw * cfg->vref_mv / max_val);
}

/** 模拟 ADC 读取 (平台相关) */
static int adc_hw_read(uint8_t channel, uint32_t* raw) {
  (void)channel;
  /* 模拟读取返回 0; 实际平台替换为寄存器读取 */
  *raw = 0;
  return 0;
}

/** 模拟 DAC 写入 */
static int dac_hw_write(uint8_t channel, uint32_t raw) {
  (void)channel;
  (void)raw;
  return 0;
}

/* ========== ADC 公共接口 ========== */

int plc_adc_init(void) {
  memset(s_adc_channels, 0, sizeof(s_adc_channels));
  s_adc_init_done = 1;
  return 0;
}

int plc_adc_channel_config(const PlcAdcConfig* config) {
  if (config == NULL || config->channel >= PLC_ADC_MAX_CHANNELS) {
    return -1;
  }
  uint8_t ch = config->channel;
  s_adc_channels[ch].config = *config;
  s_adc_channels[ch].configured = true;
  s_adc_channels[ch].last_raw = 0;
  s_adc_channels[ch].last_mv = 0;
  return 0;
}

int plc_adc_read(uint8_t channel, uint32_t* raw) {
  if (channel >= PLC_ADC_MAX_CHANNELS || raw == NULL) {
    return -1;
  }
  if (!s_adc_channels[channel].configured) {
    return -2;
  }

  int ret = adc_hw_read(channel, raw);
  if (ret != 0) {
    return ret;
  }

  s_adc_channels[channel].last_raw = *raw;
  s_adc_channels[channel].last_mv =
      adc_calc_mv(*raw, &s_adc_channels[channel].config);
  s_adc_channels[channel].read_count++;
  return 0;
}

int plc_adc_read_mv(uint8_t channel, uint32_t* mv) {
  if (channel >= PLC_ADC_MAX_CHANNELS || mv == NULL) {
    return -1;
  }
  if (!s_adc_channels[channel].configured) {
    return -2;
  }

  uint32_t raw = 0;
  int ret = plc_adc_read(channel, &raw);
  if (ret != 0) {
    return ret;
  }
  *mv = s_adc_channels[channel].last_mv;
  return 0;
}

int plc_adc_calibrate(void) {
  /* 校准: 清除所有通道的累计偏移 */
  for (uint8_t i = 0; i < PLC_ADC_MAX_CHANNELS; i++) {
    s_adc_channels[i].last_raw = 0;
    s_adc_channels[i].last_mv = 0;
  }
  return 0;
}

int plc_adc_get_state(uint8_t channel, PlcAdcChannelState* state) {
  if (channel >= PLC_ADC_MAX_CHANNELS || state == NULL) {
    return -1;
  }
  state->initialized = s_adc_channels[channel].configured;
  state->last_raw = s_adc_channels[channel].last_raw;
  state->last_mv = s_adc_channels[channel].last_mv;
  state->read_count = s_adc_channels[channel].read_count;
  return 0;
}

/* ========== DAC 公共接口 ========== */

int plc_dac_init(void) {
  memset(s_dac_channels, 0, sizeof(s_dac_channels));
  s_dac_init_done = 1;
  return 0;
}

int plc_dac_channel_config(const PlcDacConfig* config) {
  if (config == NULL || config->channel >= PLC_DAC_MAX_CHANNELS) {
    return -1;
  }
  uint8_t ch = config->channel;
  s_dac_channels[ch].config = *config;
  s_dac_channels[ch].configured = true;
  s_dac_channels[ch].last_raw = 0;
  return 0;
}

int plc_dac_write(uint8_t channel, uint32_t raw) {
  if (channel >= PLC_DAC_MAX_CHANNELS) {
    return -1;
  }
  if (!s_dac_channels[channel].configured) {
    return -2;
  }

  int ret = dac_hw_write(channel, raw);
  if (ret != 0) {
    return ret;
  }

  s_dac_channels[channel].last_raw = raw;
  return 0;
}

int plc_dac_write_mv(uint8_t channel, uint32_t mv) {
  if (channel >= PLC_DAC_MAX_CHANNELS) {
    return -1;
  }
  if (!s_dac_channels[channel].configured) {
    return -2;
  }

  PlcDacConfig* cfg = &s_dac_channels[channel].config;
  uint32_t max_val = (1u << cfg->resolution) - 1;
  if (cfg->output_range_mv == 0) {
    return -3;
  }

  uint32_t raw = (uint32_t)((uint64_t)mv * max_val / cfg->output_range_mv);
  if (raw > max_val) {
    raw = max_val;
  }

  return plc_dac_write(channel, raw);
}
