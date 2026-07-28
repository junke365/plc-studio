/**
 * hmi.generator.ts - HMI 代码生成器
 *
 * 将 HMIDesigner 的 forms 数据生成 C 代码，
 * 编译为 Windows EXE (Win32 GDI) 或 ESP32 HEX (LVGL)
 */

// ==================== 类型定义 ====================

interface CanvasElement {
  type: string;
  x: number;
  y: number;
  w: number;
  h: number;
  id: string;
  label?: string;
  icon?: string;
  text?: string;
  fontSize?: number;
  fontWeight?: string;
  textAlign?: string;
  bgColor?: string;
  fgColor?: string;
  borderColor?: string;
  borderWidth?: number;
  borderRadius?: number;
  opacity?: number;
  iconSize?: number;
  min?: number;
  max?: number;
  step?: number;
  value?: number;
  options?: string;
  placeholder?: string;
  checked?: boolean;
  disabled?: boolean;
  readonly?: boolean;
  unit?: string;
  bindingVar?: string;
  bindingProtocol?: string;
  refreshRate?: string;
  scaleMin?: number;
  scaleMax?: number;
}

interface FormDef {
  id: string;
  name: string;
  width: number;
  height: number;
  bgColor: string;
  elements: CanvasElement[];
}

interface HmiProject {
  forms: FormDef[];
}

// ==================== 工具函数 ====================

function hexColorToU32(hex?: string): string {
  if (!hex) return '0xFFFFFFFF';
  let c = hex.replace('#', '');
  if (c.length === 3) c = c[0] + c[0] + c[1] + c[1] + c[2] + c[2];
  if (c.length === 6) c = 'FF' + c;
  return '0x' + c.toUpperCase();
}

function escapeCString(s: string): string {
  return s
    .replace(/\\/g, '\\\\')
    .replace(/"/g, '\\"')
    .replace(/\n/g, '\\n')
    .replace(/\r/g, '\\r')
    .replace(/\t/g, '\\t');
}

function toCName(name: string): string {
  return name
    .replace(/[^a-zA-Z0-9\u4e00-\u9fff_]/g, '_')
    .replace(/^(\d)/, '_$1');
}

// 编辑器类型 → C 运行时控件类型映射
const TYPE_MAP: Record<string, string> = {
  label:        'PLC_HMI_WIDGET_LABEL',
  textblock:    'PLC_HMI_WIDGET_LABEL',
  tooltip:      'PLC_HMI_WIDGET_LABEL',
  richtext:     'PLC_HMI_WIDGET_LABEL',
  datetime:     'PLC_HMI_WIDGET_LABEL',
  button:       'PLC_HMI_WIDGET_BUTTON',
  switch:       'PLC_HMI_WIDGET_SWITCH',
  slider:       'PLC_HMI_WIDGET_SLIDER',
  gauge:        'PLC_HMI_WIDGET_GAUGE',
  gauge2:       'PLC_HMI_WIDGET_GAUGE',
  value:        'PLC_HMI_WIDGET_VALUE_DISPLAY',
  numberinput:  'PLC_HMI_WIDGET_VALUE_DISPLAY',
  numberdisplay:'PLC_HMI_WIDGET_VALUE_DISPLAY',
  'progress-text': 'PLC_HMI_WIDGET_VALUE_DISPLAY',
  bar:          'PLC_HMI_WIDGET_BAR',
  bar2:         'PLC_HMI_WIDGET_BAR',
  progress:     'PLC_HMI_WIDGET_PROGRESS_BAR',
  progressbar:  'PLC_HMI_WIDGET_PROGRESS_BAR',
  trend:        'PLC_HMI_WIDGET_TREND_CHART',
  rectangle:    'PLC_HMI_WIDGET_RECTANGLE',
  circle:       'PLC_HMI_WIDGET_CIRCLE',
  led:          'PLC_HMI_WIDGET_CIRCLE',
};

function mapType(editorType: string): string {
  return TYPE_MAP[editorType] || 'PLC_HMI_WIDGET_LABEL';
}

// ==================== 生成器主函数 ====================

export class HmiGenerator {
  /**
   * 生成 HMI 屏幕初始化 C 代码
   */
  generateInit(project: HmiProject): string {
    const lines: string[] = [];
    const { forms } = project;

    lines.push('#include "plc_hmi.h"');
    lines.push('#include "plc_hmi_widget.h"');
    lines.push('#include <string.h>');
    lines.push('');

    // 生成每个屏幕的初始化函数
    for (let fi = 0; fi < forms.length; fi++) {
      const form = forms[fi];
      const fnName = `plc_hmi_screen_${fi}_init`;

      lines.push(`static void ${fnName}(void)`);
      lines.push('{');

      // 屏幕背景矩形
      lines.push(`  /* ${form.name} - 背景 */`);
      lines.push('  {');
      lines.push(`    uint16_t id = plc_hmi_widget_create(PLC_HMI_WIDGET_RECTANGLE, 0, 0, ${form.width}, ${form.height});`);
      lines.push(`    plc_hmi_widget_set_prop(id, "bg_color", "${form.bgColor.replace('#', '')}");`);
      lines.push('  }');
      lines.push('');

      // 遍历元素生成控件
      for (let ei = 0; ei < form.elements.length; ei++) {
        const el = form.elements[ei];
        const cType = mapType(el.type);
        const cName = toCName(el.label || el.text || el.type) + `_${ei}`;

        lines.push(`  /* ${el.label || el.text || el.type} */`);
        lines.push('  {');
        lines.push(`    uint16_t id = plc_hmi_widget_create(${cType}, ${el.x}, ${el.y}, ${el.w}, ${el.h});`);

        // 通用属性
        if (el.text !== undefined) {
          lines.push(`    plc_hmi_widget_set_prop(id, "text", "${escapeCString(el.text)}");`);
        }
        if (el.fontSize !== undefined) {
          const fontSize = Math.max(1, Math.round(el.fontSize / 8));
          lines.push(`    plc_hmi_widget_set_prop(id, "font_size", "${fontSize}");`);
        }
        if (el.fgColor) {
          lines.push(`    plc_hmi_widget_set_prop(id, "color", "${el.fgColor.replace('#', '')}");`);
        }
        if (el.bgColor) {
          lines.push(`    plc_hmi_widget_set_prop(id, "bg_color", "${el.bgColor.replace('#', '')}");`);
        }
        if (el.borderColor) {
          lines.push(`    plc_hmi_widget_set_prop(id, "border_color", "${el.borderColor.replace('#', '')}");`);
        }
        if (el.borderWidth !== undefined) {
          lines.push(`    plc_hmi_widget_set_prop(id, "border_width", "${el.borderWidth}");`);
        }
        if (el.value !== undefined) {
          lines.push(`    plc_hmi_widget_set_prop(id, "value", "${el.value}");`);
        }
        if (el.min !== undefined) {
          lines.push(`    plc_hmi_widget_set_prop(id, "min", "${el.min}");`);
        }
        if (el.max !== undefined) {
          lines.push(`    plc_hmi_widget_set_prop(id, "max", "${el.max}");`);
        }
        if (el.checked !== undefined) {
          lines.push(`    plc_hmi_widget_set_prop(id, "value", "${el.checked ? 1 : 0}");`);
        }
        if (el.unit) {
          lines.push(`    plc_hmi_widget_set_prop(id, "unit", "${escapeCString(el.unit)}");`);
        }
        if (el.scaleMin !== undefined) {
          lines.push(`    plc_hmi_widget_set_prop(id, "min", "${el.scaleMin}");`);
        }
        if (el.scaleMax !== undefined) {
          lines.push(`    plc_hmi_widget_set_prop(id, "max", "${el.scaleMax}");`);
        }

        lines.push('  }');
        lines.push('');
      }

      lines.push('}');
      lines.push('');
    }

    // 生成主初始化函数
    lines.push('/**');
    lines.push(' * HMI 屏幕初始化 - 由编辑器自动生成');
    lines.push(' */');
    lines.push('void plc_hmi_screens_init(void)');
    lines.push('{');
    lines.push('  plc_hmi_widget_init();');
    lines.push('');
    for (let fi = 0; fi < forms.length; fi++) {
      lines.push(`  plc_hmi_screen_${fi}_init();`);
    }
    lines.push('}');
    lines.push('');

    return lines.join('\n');
  }

  /**
   * 生成 HMI 屏幕更新 C 代码
   * 将 PLC 变量表的值更新到控件属性
   */
  generateUpdate(project: HmiProject): string {
    const lines: string[] = [];
    const { forms } = project;

    lines.push('#include "plc_hmi.h"');
    lines.push('#include "plc_hmi_widget.h"');
    lines.push('#include <stdio.h>');
    lines.push('');

    lines.push('/**');
    lines.push(' * HMI 屏幕更新 - 由编辑器自动生成');
    lines.push(' * 从 PLC 变量表读取值并更新控件');
    lines.push(' * @param var_table PLC变量表指针');
    lines.push(' * @param var_table_size 变量表大小(字节)');
    lines.push(' */');
    lines.push('void plc_hmi_screens_update(void* var_table, uint32_t var_table_size)');
    lines.push('{');
    lines.push('  (void)var_table;');
    lines.push('  (void)var_table_size;');

    // 收集有 bindingVar 的元素
    let boundCount = 0;
    for (const form of forms) {
      for (const el of form.elements) {
        if (el.bindingVar) {
          boundCount++;
          const cName = `val_${toCName(el.bindingVar)}`;
          lines.push('');
          lines.push(`  /* ${el.label || el.text || el.type} -> ${el.bindingVar} */`);
          lines.push('  {');
          lines.push(`    int32_t ${cName} = 0;`);
          lines.push(`    /* TODO: 从 var_table 读取 ${el.bindingVar} */`);
          lines.push('    {');
          lines.push('      char buf[32];');
          lines.push(`      snprintf(buf, sizeof(buf), "%d", ${cName});`);
          // 找到对应控件 ID - 用元素索引
          const ei = form.elements.indexOf(el);
          lines.push(`      plc_hmi_widget_set_prop(${ei}, "value", buf);`);
          lines.push('    }');
          lines.push('  }');
        }
      }
    }

    if (boundCount === 0) {
      lines.push('');
      lines.push('  /* 无绑定变量 */');
    }

    lines.push('}');
    lines.push('');

    return lines.join('\n');
  }

  /**
   * 生成完整的 HMI C 代码文件
   */
  generate(project: HmiProject): string {
    const initCode = this.generateInit(project);
    const updateCode = this.generateUpdate(project);
    return initCode + '\n' + updateCode;
  }
}
