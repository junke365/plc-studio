/**
 * plc_io.c - I/O 管理模块实现
 *
 * 管理数字量/模拟量输入输出通道
 * 支持缩放因子和偏移量变换
 */

#include "plc_io.h"
#include <string.h>

void plc_io_init(PlcIoConfig* config) {
  if (config == NULL) return;
  memset(config, 0, sizeof(PlcIoConfig));
  config->initialized = true;
}

int plc_io_register(PlcIoConfig* config, IoType type, const char* name,
                    const char* var_name, uint32_t physical_addr) {
  if (config == NULL || name == NULL) return -1;

  /* 检查通道数是否已满 */
  if (config->channel_count >= PLC_MAX_IO_CHANNELS) return -1;

  uint16_t id = config->channel_count;
  PlcIoChannel* ch = &config->channels[id];

  ch->channel_id = id;
  ch->type = type;
  ch->name = name;
  ch->var_name = var_name;
  ch->var = NULL; /* 绑定阶段设置 */
  ch->physical_addr = physical_addr;
  ch->raw_value = 0;
  ch->scaled_value = 0;
  ch->scale_factor = 1.0f;
  ch->offset = 0;
  ch->min_value = 0;
  ch->max_value = 0;
  ch->status = IO_STATUS_OK;
  ch->enabled = true;

  config->channel_count++;

  return (int)id;
}

int plc_io_bind(PlcIoConfig* config, uint16_t channel_id, PlcVarTable* var_table) {
  if (config == NULL || var_table == NULL) return -1;
  if (channel_id >= config->channel_count) return -1;

  PlcIoChannel* ch = &config->channels[channel_id];
  if (ch->var_name == NULL) return -1;

  /* 在变量表中查找绑定的变量 */
  ch->var = plc_var_find(var_table, ch->var_name);
  if (ch->var == NULL) return -1;

  return 0;
}

void plc_io_read_inputs(PlcIoConfig* config) {
  if (config == NULL) return;

  for (uint16_t i = 0; i < config->channel_count; i++) {
    PlcIoChannel* ch = &config->channels[i];

    /* 只处理输入通道 */
    if (!ch->enabled) continue;
    if (ch->type != IO_TYPE_DI && ch->type != IO_TYPE_AI &&
        ch->type != IO_TYPE_ENCODER && ch->type != IO_TYPE_COUNTER) {
      continue;
    }

    /* 从硬件读取原始值 */
    ch->raw_value = plc_hal_read_input(ch->physical_addr, ch->type);

    /* 应用缩放因子和偏移量 */
    ch->scaled_value = (int32_t)((float)ch->raw_value * ch->scale_factor) + ch->offset;

    /* 范围检查 */
    if (ch->min_value != 0 || ch->max_value != 0) {
      if (ch->scaled_value < ch->min_value) {
        ch->scaled_value = ch->min_value;
        ch->status |= IO_STATUS_UNDERFLOW;
      } else if (ch->scaled_value > ch->max_value) {
        ch->scaled_value = ch->max_value;
        ch->status |= IO_STATUS_OVERFLOW;
      }
    }

    /* 写入绑定的变量 */
    if (ch->var != NULL && ch->var->data != NULL) {
      /* 根据变量类型写入 */
      switch (ch->var->type) {
        case VAR_TYPE_BOOL: {
          plc_bool val = (ch->scaled_value != 0) ? true : false;
          memcpy(ch->var->data, &val, sizeof(plc_bool));
          break;
        }
        case VAR_TYPE_INT:
        case VAR_TYPE_UINT: {
          plc_int val = (plc_int)ch->scaled_value;
          memcpy(ch->var->data, &val, sizeof(plc_int));
          break;
        }
        case VAR_TYPE_DINT:
        case VAR_TYPE_UDINT: {
          plc_dint val = (plc_dint)ch->scaled_value;
          memcpy(ch->var->data, &val, sizeof(plc_dint));
          break;
        }
        case VAR_TYPE_REAL: {
          plc_real val = (plc_real)ch->scaled_value;
          memcpy(ch->var->data, &val, sizeof(plc_real));
          break;
        }
        default: {
          /* 通用处理：直接拷贝 */
          plc_dint val = (plc_dint)ch->scaled_value;
          uint32_t copy_size = ch->var->size < sizeof(plc_dint)
                               ? ch->var->size : sizeof(plc_dint);
          memcpy(ch->var->data, &val, copy_size);
          break;
        }
      }
    }
  }
}

void plc_io_write_outputs(PlcIoConfig* config) {
  if (config == NULL) return;

  for (uint16_t i = 0; i < config->channel_count; i++) {
    PlcIoChannel* ch = &config->channels[i];

    /* 只处理输出通道 */
    if (!ch->enabled) continue;
    if (ch->type != IO_TYPE_DO && ch->type != IO_TYPE_AO &&
        ch->type != IO_TYPE_PWM) {
      continue;
    }

    /* 从绑定的变量读取值 */
    if (ch->var != NULL && ch->var->data != NULL) {
      plc_dint raw_val = 0;

      switch (ch->var->type) {
        case VAR_TYPE_BOOL: {
          plc_bool val = false;
          memcpy(&val, ch->var->data, sizeof(plc_bool));
          raw_val = val ? 1 : 0;
          break;
        }
        case VAR_TYPE_INT:
        case VAR_TYPE_UINT: {
          plc_int val = 0;
          memcpy(&val, ch->var->data, sizeof(plc_int));
          raw_val = (plc_dint)val;
          break;
        }
        case VAR_TYPE_DINT:
        case VAR_TYPE_UDINT: {
          memcpy(&raw_val, ch->var->data, sizeof(plc_dint));
          break;
        }
        case VAR_TYPE_REAL: {
          plc_real val = 0.0f;
          memcpy(&val, ch->var->data, sizeof(plc_real));
          raw_val = (plc_dint)val;
          break;
        }
        default: {
          uint32_t copy_size = ch->var->size < sizeof(plc_dint)
                               ? ch->var->size : sizeof(plc_dint);
          memcpy(&raw_val, ch->var->data, copy_size);
          break;
        }
      }

      /* 反向应用缩放（先减偏移，再除缩放因子） */
      if (ch->scale_factor != 0.0f) {
        ch->raw_value = (int32_t)(((float)(raw_val - ch->offset)) / ch->scale_factor);
      } else {
        ch->raw_value = raw_val;
      }

      ch->scaled_value = raw_val;

      /* 范围检查 */
      if (ch->min_value != 0 || ch->max_value != 0) {
        if (ch->raw_value < ch->min_value) {
          ch->raw_value = ch->min_value;
          ch->status |= IO_STATUS_UNDERFLOW;
        } else if (ch->raw_value > ch->max_value) {
          ch->raw_value = ch->max_value;
          ch->status |= IO_STATUS_OVERFLOW;
        }
      }

      /* 写入硬件 */
      plc_hal_write_output(ch->physical_addr, ch->type, ch->raw_value);
    }
  }
}

int32_t plc_io_read_channel(PlcIoConfig* config, uint16_t channel_id) {
  if (config == NULL || channel_id >= config->channel_count) return 0;
  return config->channels[channel_id].scaled_value;
}

void plc_io_write_channel(PlcIoConfig* config, uint16_t channel_id, int32_t value) {
  if (config == NULL || channel_id >= config->channel_count) return;

  PlcIoChannel* ch = &config->channels[channel_id];
  ch->scaled_value = value;

  /* 写入绑定的变量 */
  if (ch->var != NULL && ch->var->data != NULL) {
    plc_dint val = (plc_dint)value;
    memcpy(ch->var->data, &val, ch->var->size < sizeof(plc_dint)
           ? ch->var->size : sizeof(plc_dint));
  }
}

void plc_io_set_scale(PlcIoConfig* config, uint16_t channel_id,
                      float factor, int32_t offset) {
  if (config == NULL || channel_id >= config->channel_count) return;

  PlcIoChannel* ch = &config->channels[channel_id];
  ch->scale_factor = factor;
  ch->offset = offset;
}
