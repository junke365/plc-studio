/**
 * plc_adc.h - ADC/DAC 硬件抽象层接口
 *
 * 提供统一的模数/数模转换接口
 * 支持可配置分辨率、采样时间、参考电压
 */

#ifndef PLC_ADC_H
#define PLC_ADC_H

#include "plc_platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========== 常量定义 ========== */

#ifndef PLC_ADC_MAX_CHANNELS
  #ifdef PLATFORM_STM32
    #define PLC_ADC_MAX_CHANNELS  16
  #elif defined(PLATFORM_ESP32)
    #define PLC_ADC_MAX_CHANNELS  20
  #else
    #define PLC_ADC_MAX_CHANNELS  64
  #endif
#endif

#ifndef PLC_DAC_MAX_CHANNELS
  #ifdef PLATFORM_STM32
    #define PLC_DAC_MAX_CHANNELS  2
  #elif defined(PLATFORM_ESP32)
    #define PLC_DAC_MAX_CHANNELS  2
  #else
    #define PLC_DAC_MAX_CHANNELS  16
  #endif
#endif

/* ========== 枚举定义 ========== */

/** ADC 分辨率 */
typedef enum {
  PLC_ADC_RES_12BIT = 12,  /**< 12 位分辨率 (0-4095) */
  PLC_ADC_RES_14BIT = 14,  /**< 14 位分辨率 (0-16383) */
  PLC_ADC_RES_16BIT = 16   /**< 16 位分辨率 (0-65535) */
} PlcAdcResolution;

/** ADC 采样时间 */
typedef enum {
  PLC_ADC_SAMPLE_1_5   = 0,   /**< 1.5 个时钟周期 */
  PLC_ADC_SAMPLE_7_5   = 1,   /**< 7.5 个时钟周期 */
  PLC_ADC_SAMPLE_13_5  = 2,   /**< 13.5 个时钟周期 */
  PLC_ADC_SAMPLE_28_5  = 3,   /**< 28.5 个时钟周期 */
  PLC_ADC_SAMPLE_41_5  = 4,   /**< 41.5 个时钟周期 */
  PLC_ADC_SAMPLE_55_5  = 5,   /**< 55.5 个时钟周期 */
  PLC_ADC_SAMPLE_71_5  = 6,   /**< 71.5 个时钟周期 */
  PLC_ADC_SAMPLE_239_5 = 7    /**< 239.5 个时钟周期 */
} PlcAdcSampleTime;

/* ========== 结构体定义 ========== */

/** ADC 通道配置 */
typedef struct {
  uint8_t            channel;      /**< 通道号 (0-based) */
  PlcAdcResolution   resolution;   /**< ADC 分辨率 */
  PlcAdcSampleTime   sample_time;  /**< 采样时间 */
  uint32_t           vref_mv;      /**< 参考电压 (毫伏, 如 3300) */
} PlcAdcConfig;

/** ADC 通道运行状态 */
typedef struct {
  bool      initialized;     /**< 是否已初始化 */
  uint32_t  last_raw;        /**< 最近一次原始值 */
  uint32_t  last_mv;         /**< 最近一次毫伏值 */
  uint32_t  read_count;      /**< 读取次数 */
} PlcAdcChannelState;

/** DAC 通道配置 */
typedef struct {
  uint8_t            channel;       /**< 通道号 (0-based) */
  PlcAdcResolution   resolution;    /**< DAC 分辨率 */
  uint32_t           output_range_mv; /**< 输出范围 (毫伏, 如 3300) */
} PlcDacConfig;

/* ========== ADC 函数声明 ========== */

/**
 * 初始化 ADC 子系统
 * @return 0 成功, 负值为错误码
 */
int plc_adc_init(void);

/**
 * 配置 ADC 通道
 * @param config 通道配置参数
 * @return 0 成功, 负值为错误码
 */
int plc_adc_channel_config(const PlcAdcConfig* config);

/**
 * 读取 ADC 原始值
 * @param channel 通道号
 * @param raw     输出参数: 原始 ADC 值
 * @return 0 成功, 负值为错误码
 */
int plc_adc_read(uint8_t channel, uint32_t* raw);

/**
 * 读取 ADC 电压值（毫伏）
 * @param channel 通道号
 * @param mv      输出参数: 电压值 (毫伏)
 * @return 0 成功, 负值为错误码
 */
int plc_adc_read_mv(uint8_t channel, uint32_t* mv);

/**
 * 启动 ADC 校准
 * @return 0 成功, 负值为错误码
 */
int plc_adc_calibrate(void);

/**
 * 获取 ADC 通道状态
 * @param channel 通道号
 * @param state   输出参数: 通道状态
 * @return 0 成功, 负值为错误码
 */
int plc_adc_get_state(uint8_t channel, PlcAdcChannelState* state);

/* ========== DAC 函数声明 ========== */

/**
 * 初始化 DAC 子系统
 * @return 0 成功, 负值为错误码
 */
int plc_dac_init(void);

/**
 * 配置 DAC 通道
 * @param config 通道配置参数
 * @return 0 成功, 负值为错误码
 */
int plc_dac_channel_config(const PlcDacConfig* config);

/**
 * 写入 DAC 原始值
 * @param channel 通道号
 * @param raw     原始 DAC 值
 * @return 0 成功, 负值为错误码
 */
int plc_dac_write(uint8_t channel, uint32_t raw);

/**
 * 写入 DAC 电压值（毫伏）
 * @param channel 通道号
 * @param mv      电压值 (毫伏)
 * @return 0 成功, 负值为错误码
 */
int plc_dac_write_mv(uint8_t channel, uint32_t mv);

#ifdef __cplusplus
}
#endif

#endif /* PLC_ADC_H */
