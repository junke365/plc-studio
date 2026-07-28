<template>
  <div class="hmi-designer">
    <!-- 左侧面板 -->
    <aside class="hmi-toolbox">
      <!-- Tab 切换 -->
      <div class="toolbox-tabs">
        <button
          class="tab-btn"
          :class="{ active: toolboxTab === 'controls' }"
          @click="toolboxTab = 'controls'"
        >
          <span class="material-symbols-outlined" style="font-size: 14px"
            >widgets</span
          >
          控件库
        </button>
        <button
          class="tab-btn"
          :class="{ active: toolboxTab === 'assets' }"
          @click="toolboxTab = 'assets'"
        >
          <span class="material-symbols-outlined" style="font-size: 14px"
            >folder_open</span
          >
          资源浏览
        </button>
      </div>

      <!-- 控件库 Tab -->
      <template v-if="toolboxTab === 'controls'">
        <div class="toolbox-header">
          <input
            class="toolbox-search"
            v-model="toolboxSearch"
            placeholder="搜索控件..."
          />
        </div>
        <div class="toolbox-content custom-scrollbar">
          <div
            v-for="cat in filteredControlCategories"
            :key="cat.name"
            class="toolbox-section"
          >
            <button
              class="section-header-btn"
              @click="toggleToolboxSection(cat.name)"
            >
              <span
                class="material-symbols-outlined toggle-arrow"
                :class="{ collapsed: !expandedToolboxSections[cat.name] }"
                >expand_more</span
              >
              <span class="section-label">{{ cat.name }}</span>
              <span class="section-count">{{ cat.items.length }}</span>
            </button>
            <div
              v-show="expandedToolboxSections[cat.name] !== false"
              class="controls-grid"
            >
              <button
                v-for="ctrl in cat.items"
                :key="ctrl.type"
                class="control-btn"
                draggable="true"
                @dragstart="onDragStart($event, ctrl)"
                :title="ctrl.label"
              >
                <span class="material-symbols-outlined ctrl-icon">{{
                  ctrl.icon
                }}</span>
                <span class="ctrl-label">{{ ctrl.label }}</span>
              </button>
            </div>
          </div>

          <!-- 工业图库 -->
          <div class="toolbox-section">
            <button
              class="section-header-btn"
              @click="toggleToolboxSection('工业图库')"
            >
              <span
                class="material-symbols-outlined toggle-arrow"
                :class="{
                  collapsed: expandedToolboxSections['工业图库'] === false,
                }"
                >expand_more</span
              >
              <span class="section-label">工业图库</span>
              <span class="section-count">{{ industrialLibrary.length }}</span>
            </button>
            <div
              v-show="expandedToolboxSections['工业图库'] !== false"
              class="controls-grid"
            >
              <div
                v-for="item in industrialLibrary"
                :key="item.type"
                class="library-item"
                draggable="true"
                @dragstart="onDragStart($event, item)"
              >
                <span class="material-symbols-outlined lib-icon">{{
                  item.icon
                }}</span>
                <div class="lib-label">{{ item.label }}</div>
              </div>
            </div>
          </div>
        </div>
      </template>

      <!-- 资源浏览 Tab -->
      <template v-if="toolboxTab === 'assets'">
        <div class="toolbox-header assets-header">
          <span class="assets-title">窗体列表</span>
          <button class="icon-btn-sm" @click="addForm" title="新建窗体">
            <span class="material-symbols-outlined" style="font-size: 16px"
              >add</span
            >
          </button>
        </div>
        <div class="toolbox-content custom-scrollbar">
          <!-- 窗体列表 -->
          <div class="form-list">
            <div
              v-for="(form, fIdx) in forms"
              :key="form.id"
              class="form-item"
              :class="{ active: currentFormIndex === fIdx }"
              @click="switchForm(fIdx)"
            >
              <span class="material-symbols-outlined form-icon">web</span>
              <div class="form-info">
                <div class="form-name">{{ form.name }}</div>
                <div class="form-size">{{ form.width }}×{{ form.height }}</div>
              </div>
              <div class="form-actions">
                <button
                  class="icon-btn-xs"
                  @click.stop="renameForm(fIdx)"
                  title="重命名"
                >
                  <span
                    class="material-symbols-outlined"
                    style="font-size: 13px"
                    >edit</span
                  >
                </button>
                <button
                  class="icon-btn-xs"
                  @click.stop="removeForm(fIdx)"
                  title="删除"
                  :disabled="forms.length <= 1"
                >
                  <span
                    class="material-symbols-outlined"
                    style="font-size: 13px"
                    >delete</span
                  >
                </button>
              </div>
            </div>
          </div>

          <!-- 当前窗体控件树 -->
          <div
            class="controls-tree"
            v-if="forms[currentFormIndex]?.elements.length"
          >
            <div class="tree-header">
              <span class="tree-title">控件列表</span>
              <span class="tree-count">{{
                forms[currentFormIndex].elements.length
              }}</span>
            </div>
            <div
              v-for="(el, eIdx) in forms[currentFormIndex].elements"
              :key="el.id"
              class="tree-item"
              :class="{ active: selectedElement === eIdx }"
              @click="selectElement(eIdx)"
            >
              <span
                class="material-symbols-outlined tree-icon"
                :style="{ color: el.fgColor || '#94a3b8' }"
              >
                {{ getControlMeta(el.type)?.icon || "widgets" }}
              </span>
              <span class="tree-label">{{
                el.label || el.text || el.type
              }}</span>
              <button
                class="icon-btn-xs tree-del"
                @click.stop="deleteElement(eIdx)"
                title="删除控件"
              >
                <span class="material-symbols-outlined" style="font-size: 12px"
                  >close</span
                >
              </button>
            </div>
          </div>
          <div v-else class="empty-hint">
            <span
              class="material-symbols-outlined"
              style="font-size: 32px; opacity: 0.3"
              >inventory_2</span
            >
            <div>暂无控件</div>
          </div>
        </div>
      </template>
    </aside>

    <!-- 中间画布区 -->
    <main class="hmi-canvas-area">
      <div class="canvas-toolbar">
        <div class="toolbar-left">
          <div class="view-toggle">
            <button
              class="toggle-btn"
              :class="{ active: !isPreview }"
              @click="isPreview = false"
            >
              编辑器
            </button>
            <button
              class="toggle-btn"
              :class="{ active: isPreview }"
              @click="isPreview = true"
            >
              实时预览
            </button>
          </div>
          <div class="toolbar-divider" />
          <div class="zoom-controls">
            <button class="icon-btn" @click="zoomIn">
              <span class="material-symbols-outlined" style="font-size: 18px"
                >zoom_in</span
              >
            </button>
            <span class="zoom-value">{{ zoom }}%</span>
            <button class="icon-btn" @click="zoomOut">
              <span class="material-symbols-outlined" style="font-size: 18px"
                >zoom_out</span
              >
            </button>
          </div>
          <div class="toolbar-divider" />
          <span class="canvas-size-label"
            >{{ canvasWidth }}×{{ canvasHeight }}</span
          >
        </div>
        <div class="toolbar-right">
          <button class="save-btn">
            <span class="material-symbols-outlined" style="font-size: 16px"
              >save</span
            >
            保存
          </button>
          <button
            class="icon-btn"
            @click="showGrid = !showGrid"
            :class="{ active: showGrid }"
          >
            <span class="material-symbols-outlined" style="font-size: 18px"
              >grid_on</span
            >
          </button>
          <button class="icon-btn" @click="undo" title="撤销 Ctrl+Z">
            <span class="material-symbols-outlined" style="font-size: 18px"
              >undo</span
            >
          </button>
          <button class="icon-btn" @click="redo" title="重做 Ctrl+Y">
            <span class="material-symbols-outlined" style="font-size: 18px"
              >redo</span
            >
          </button>
        </div>
      </div>

      <!-- 画布 -->
      <div
        class="canvas-container custom-scrollbar"
        :class="{ 'canvas-grid': showGrid, 'preview-mode': isPreview }"
        @dragover.prevent
        @drop="onDrop"
      >
        <div
          class="canvas"
          :style="{
            transform: `scale(${zoom / 100})`,
            width: canvasWidth + 'px',
            height: canvasHeight + 'px',
          }"
          @mousedown="onCanvasBgMouseDown"
        >
          <div
            v-for="(el, idx) in canvasElements"
            :key="el.id"
            class="canvas-element"
            :class="{ selected: selectedElement === idx }"
            :style="{
              left: el.x + 'px',
              top: el.y + 'px',
              width: el.w + 'px',
              height: el.h + 'px',
            }"
            @mousedown.left="onElementMouseDown($event, idx)"
          >
            <!-- 渲染元素内容 -->
            <component
              :is="getRenderer(el.type)"
              :element="el"
              :isPreview="isPreview"
            />

            <!-- 缩放手柄 -->
            <template v-if="selectedElement === idx && !isPreview">
              <span
                class="resize-handle nw"
                @mousedown.stop="onResizeMouseDown($event, idx, 'nw')"
              />
              <span
                class="resize-handle ne"
                @mousedown.stop="onResizeMouseDown($event, idx, 'ne')"
              />
              <span
                class="resize-handle sw"
                @mousedown.stop="onResizeMouseDown($event, idx, 'sw')"
              />
              <span
                class="resize-handle se"
                @mousedown.stop="onResizeMouseDown($event, idx, 'se')"
              />
              <span
                class="resize-handle n"
                @mousedown.stop="onResizeMouseDown($event, idx, 'n')"
              />
              <span
                class="resize-handle s"
                @mousedown.stop="onResizeMouseDown($event, idx, 's')"
              />
              <span
                class="resize-handle e"
                @mousedown.stop="onResizeMouseDown($event, idx, 'e')"
              />
              <span
                class="resize-handle w"
                @mousedown.stop="onResizeMouseDown($event, idx, 'w')"
              />
            </template>
          </div>
        </div>
      </div>
    </main>

    <!-- 右侧属性编辑器 -->
    <aside class="hmi-inspector" :class="{ collapsed: !inspectorVisible }">
      <button
        class="inspector-toggle-btn"
        @click="inspectorVisible = !inspectorVisible"
        :title="inspectorVisible ? '隐藏属性面板' : '显示属性面板'"
      >
        <span class="material-symbols-outlined" style="font-size: 16px">
          {{ inspectorVisible ? "chevron_right" : "chevron_left" }}
        </span>
      </button>
      <template v-if="inspectorVisible">
        <div class="inspector-header">
          <h2 class="inspector-title">属性</h2>
        </div>

        <template
          v-if="selectedElement !== null && canvasElements[selectedElement]"
        >
          <div class="inspector-selection">
            <span class="material-symbols-outlined selection-icon">
              {{
                getControlMeta(canvasElements[selectedElement].type)?.icon ||
                "widgets"
              }}
            </span>
            <div>
              <div class="selection-name">
                {{
                  canvasElements[selectedElement].label ||
                  canvasElements[selectedElement].type
                }}
              </div>
              <div class="selection-type">
                {{
                  getControlMeta(canvasElements[selectedElement].type)
                    ?.category || "组件"
                }}
              </div>
            </div>
          </div>

          <div class="inspector-body custom-scrollbar">
            <!-- 标识符 -->
            <div class="inspector-section">
              <button class="section-toggle" @click="toggleSection('identity')">
                <span>位置与尺寸</span>
                <span class="material-symbols-outlined toggle-icon"
                  >expand_more</span
                >
              </button>
              <div v-show="expandedSections.identity" class="section-body">
                <div class="prop-row">
                  <div class="prop-field half">
                    <label class="prop-label">X</label>
                    <input
                      class="prop-input code-font"
                      type="number"
                      :value="canvasElements[selectedElement].x"
                      @change="updateProp('x', $event)"
                    />
                  </div>
                  <div class="prop-field half">
                    <label class="prop-label">Y</label>
                    <input
                      class="prop-input code-font"
                      type="number"
                      :value="canvasElements[selectedElement].y"
                      @change="updateProp('y', $event)"
                    />
                  </div>
                </div>
                <div class="prop-row">
                  <div class="prop-field half">
                    <label class="prop-label">宽</label>
                    <input
                      class="prop-input code-font"
                      type="number"
                      :value="canvasElements[selectedElement].w"
                      @change="updateProp('w', $event)"
                    />
                  </div>
                  <div class="prop-field half">
                    <label class="prop-label">高</label>
                    <input
                      class="prop-input code-font"
                      type="number"
                      :value="canvasElements[selectedElement].h"
                      @change="updateProp('h', $event)"
                    />
                  </div>
                </div>
                <div class="prop-field">
                  <label class="prop-label">ID</label>
                  <input
                    class="prop-input code-font"
                    :value="canvasElements[selectedElement].id"
                    readonly
                  />
                </div>
              </div>
            </div>

            <!-- 文本属性 -->
            <div class="inspector-section" v-if="hasProperty('text')">
              <button class="section-toggle" @click="toggleSection('text')">
                <span>文本</span>
                <span class="material-symbols-outlined toggle-icon"
                  >expand_more</span
                >
              </button>
              <div v-show="expandedSections.text" class="section-body">
                <div class="prop-field">
                  <label class="prop-label">显示文本</label>
                  <input
                    class="prop-input"
                    :value="canvasElements[selectedElement].text || ''"
                    @input="updateProp('text', $event)"
                  />
                </div>
                <div class="prop-field">
                  <label class="prop-label">字号</label>
                  <input
                    class="prop-input code-font"
                    type="number"
                    :value="canvasElements[selectedElement].fontSize || 14"
                    @change="updateProp('fontSize', $event)"
                  />
                </div>
                <div class="prop-row">
                  <div class="prop-field half">
                    <label class="prop-label">字体粗细</label>
                    <select
                      class="prop-input"
                      :value="
                        canvasElements[selectedElement].fontWeight || 'normal'
                      "
                      @change="updateProp('fontWeight', $event)"
                    >
                      <option value="normal">正常</option>
                      <option value="bold">粗体</option>
                    </select>
                  </div>
                  <div class="prop-field half">
                    <label class="prop-label">对齐</label>
                    <select
                      class="prop-input"
                      :value="
                        canvasElements[selectedElement].textAlign || 'center'
                      "
                      @change="updateProp('textAlign', $event)"
                    >
                      <option value="left">左对齐</option>
                      <option value="center">居中</option>
                      <option value="right">右对齐</option>
                    </select>
                  </div>
                </div>
              </div>
            </div>

            <!-- 颜色与样式 -->
            <div class="inspector-section">
              <button class="section-toggle" @click="toggleSection('style')">
                <span>颜色与样式</span>
                <span class="material-symbols-outlined toggle-icon"
                  >expand_more</span
                >
              </button>
              <div v-show="expandedSections.style" class="section-body">
                <div class="prop-field">
                  <label class="prop-label">背景色</label>
                  <div class="color-field">
                    <input
                      type="color"
                      class="color-picker"
                      :value="
                        canvasElements[selectedElement].bgColor || '#3b82f6'
                      "
                      @input="updateProp('bgColor', $event)"
                    />
                    <input
                      class="prop-input code-font"
                      :value="
                        canvasElements[selectedElement].bgColor || '#3b82f6'
                      "
                      @input="updateProp('bgColor', $event)"
                    />
                  </div>
                </div>
                <div class="prop-field">
                  <label class="prop-label">前景/文字色</label>
                  <div class="color-field">
                    <input
                      type="color"
                      class="color-picker"
                      :value="
                        canvasElements[selectedElement].fgColor || '#ffffff'
                      "
                      @input="updateProp('fgColor', $event)"
                    />
                    <input
                      class="prop-input code-font"
                      :value="
                        canvasElements[selectedElement].fgColor || '#ffffff'
                      "
                      @input="updateProp('fgColor', $event)"
                    />
                  </div>
                </div>
                <div class="prop-field">
                  <label class="prop-label">边框色</label>
                  <div class="color-field">
                    <input
                      type="color"
                      class="color-picker"
                      :value="
                        canvasElements[selectedElement].borderColor || '#475569'
                      "
                      @input="updateProp('borderColor', $event)"
                    />
                    <input
                      class="prop-input code-font"
                      :value="
                        canvasElements[selectedElement].borderColor || '#475569'
                      "
                      @input="updateProp('borderColor', $event)"
                    />
                  </div>
                </div>
                <div class="prop-field">
                  <label class="prop-label">边框宽度</label>
                  <input
                    class="prop-input code-font"
                    type="number"
                    min="0"
                    :value="canvasElements[selectedElement].borderWidth ?? 1"
                    @change="updateProp('borderWidth', $event)"
                  />
                </div>
                <div class="prop-field">
                  <label class="prop-label">圆角</label>
                  <input
                    class="prop-input code-font"
                    type="number"
                    min="0"
                    :value="canvasElements[selectedElement].borderRadius ?? 4"
                    @change="updateProp('borderRadius', $event)"
                  />
                </div>
                <div class="prop-field">
                  <label class="prop-label">透明度</label>
                  <input
                    type="range"
                    class="hmi-slider"
                    min="0"
                    max="100"
                    :value="canvasElements[selectedElement].opacity ?? 100"
                    @input="updateProp('opacity', $event)"
                  />
                </div>
              </div>
            </div>

            <!-- 图标属性 -->
            <div class="inspector-section" v-if="hasProperty('icon')">
              <button class="section-toggle" @click="toggleSection('icon')">
                <span>图标</span>
                <span class="material-symbols-outlined toggle-icon"
                  >expand_more</span
                >
              </button>
              <div v-show="expandedSections.icon" class="section-body">
                <div class="prop-field">
                  <label class="prop-label">图标名称</label>
                  <input
                    class="prop-input"
                    :value="canvasElements[selectedElement].icon || ''"
                    @input="updateProp('icon', $event)"
                    placeholder="Material Symbol 名称"
                  />
                </div>
                <div class="prop-field">
                  <label class="prop-label">图标大小</label>
                  <input
                    class="prop-input code-font"
                    type="number"
                    :value="canvasElements[selectedElement].iconSize || 24"
                    @change="updateProp('iconSize', $event)"
                  />
                </div>
              </div>
            </div>

            <!-- 交互属性 -->
            <div class="inspector-section" v-if="hasProperty('value')">
              <button
                class="section-toggle"
                @click="toggleSection('interaction')"
              >
                <span>值与交互</span>
                <span class="material-symbols-outlined toggle-icon"
                  >expand_more</span
                >
              </button>
              <div v-show="expandedSections.interaction" class="section-body">
                <div class="prop-field" v-if="hasProperty('min')">
                  <label class="prop-label">最小值</label>
                  <input
                    class="prop-input code-font"
                    type="number"
                    :value="canvasElements[selectedElement].min ?? 0"
                    @change="updateProp('min', $event)"
                  />
                </div>
                <div class="prop-field" v-if="hasProperty('max')">
                  <label class="prop-label">最大值</label>
                  <input
                    class="prop-input code-font"
                    type="number"
                    :value="canvasElements[selectedElement].max ?? 100"
                    @change="updateProp('max', $event)"
                  />
                </div>
                <div class="prop-field" v-if="hasProperty('step')">
                  <label class="prop-label">步长</label>
                  <input
                    class="prop-input code-font"
                    type="number"
                    :value="canvasElements[selectedElement].step ?? 1"
                    @change="updateProp('step', $event)"
                  />
                </div>
                <div class="prop-field" v-if="hasProperty('options')">
                  <label class="prop-label">选项（每行一个）</label>
                  <textarea
                    class="prop-input code-font"
                    rows="4"
                    :value="
                      canvasElements[selectedElement].options ||
                      '选项A\n选项B\n选项C'
                    "
                    @input="updateProp('options', $event)"
                  />
                </div>
                <div class="prop-field" v-if="hasProperty('placeholder')">
                  <label class="prop-label">占位文本</label>
                  <input
                    class="prop-input"
                    :value="canvasElements[selectedElement].placeholder || ''"
                    @input="updateProp('placeholder', $event)"
                  />
                </div>
                <div class="prop-field" v-if="hasProperty('checked')">
                  <label class="prop-label">默认选中</label>
                  <input
                    type="checkbox"
                    class="prop-checkbox"
                    :checked="canvasElements[selectedElement].checked"
                    @change="updateProp('checked', $event)"
                  />
                </div>
                <div class="prop-field" v-if="hasProperty('disabled')">
                  <label class="prop-label">禁用</label>
                  <input
                    type="checkbox"
                    class="prop-checkbox"
                    :checked="canvasElements[selectedElement].disabled"
                    @change="updateProp('disabled', $event)"
                  />
                </div>
                <div class="prop-field" v-if="hasProperty('readonly')">
                  <label class="prop-label">只读</label>
                  <input
                    type="checkbox"
                    class="prop-checkbox"
                    :checked="canvasElements[selectedElement].readonly"
                    @change="updateProp('readonly', $event)"
                  />
                </div>
                <div class="prop-field" v-if="hasProperty('unit')">
                  <label class="prop-label">单位</label>
                  <input
                    class="prop-input"
                    :value="canvasElements[selectedElement].unit || ''"
                    @input="updateProp('unit', $event)"
                    placeholder="如 °C, RPM, %"
                  />
                </div>
              </div>
            </div>

            <!-- PLC 数据绑定 -->
            <div class="inspector-section">
              <button
                class="section-toggle binding-toggle"
                @click="toggleSection('binding')"
              >
                <span>PLC 数据绑定</span>
                <span class="material-symbols-outlined toggle-icon">link</span>
              </button>
              <div v-show="expandedSections.binding" class="section-body">
                <div class="prop-field">
                  <label class="prop-label">映射变量</label>
                  <div class="binding-input">
                    <input
                      class="prop-input code-font binding-field"
                      :value="canvasElements[selectedElement].bindingVar || ''"
                      @input="updateProp('bindingVar', $event)"
                      placeholder="例: STATION_04.TANK_LEVEL"
                    />
                    <span class="material-symbols-outlined binding-list-icon"
                      >list</span
                    >
                  </div>
                </div>
                <div class="prop-field">
                  <label class="prop-label">协议类型</label>
                  <select
                    class="prop-input"
                    :value="
                      canvasElements[selectedElement].bindingProtocol ||
                      'Modbus TCP'
                    "
                    @change="updateProp('bindingProtocol', $event)"
                  >
                    <option>Modbus TCP</option>
                    <option>EtherCAT</option>
                    <option>Profinet</option>
                    <option>OPC UA</option>
                  </select>
                </div>
                <div class="prop-field">
                  <label class="prop-label">刷新频率</label>
                  <select
                    class="prop-input"
                    :value="
                      canvasElements[selectedElement].refreshRate || '500ms'
                    "
                    @change="updateProp('refreshRate', $event)"
                  >
                    <option>100ms</option>
                    <option>250ms</option>
                    <option>500ms</option>
                    <option>1000ms</option>
                  </select>
                </div>
                <div class="prop-field">
                  <label class="prop-label">缩放范围</label>
                  <div class="prop-row">
                    <input
                      class="prop-input code-font half"
                      type="number"
                      :value="canvasElements[selectedElement].scaleMin ?? 0"
                      @change="updateProp('scaleMin', $event)"
                      placeholder="输入最小"
                    />
                    <span class="prop-sep">→</span>
                    <input
                      class="prop-input code-font half"
                      type="number"
                      :value="canvasElements[selectedElement].scaleMax ?? 100"
                      @change="updateProp('scaleMax', $event)"
                      placeholder="输出最大"
                    />
                  </div>
                </div>
              </div>
            </div>
          </div>

          <div class="inspector-actions">
            <button class="action-btn danger" @click="deleteSelected">
              删除组件
            </button>
          </div>
        </template>

        <div v-else class="inspector-empty">
          <span class="material-symbols-outlined empty-icon">touch_app</span>
          <p>选择画布上的组件查看属性</p>
          <p class="hint-sub">从左侧拖入控件开始设计</p>
        </div>
      </template>
    </aside>
  </div>
</template>

<script setup lang="ts">
import {
  ref,
  reactive,
  computed,
  h,
  type Component,
  onBeforeUnmount,
} from "vue";
import type { EditorTab } from "@smart-plc/shared";

const props = defineProps<{ tab: EditorTab }>();

// ==================== 状态 ====================
const isPreview = ref(false);
const showGrid = ref(true);
const zoom = ref(100);
const selectedElement = ref<number | null>(null);
const toolboxSearch = ref("");
const toolboxTab = ref<"controls" | "assets">("controls");
const expandedToolboxSections = reactive<Record<string, boolean>>({});
const inspectorVisible = ref(true);

// ==================== 多窗体管理 ====================
interface FormDef {
  id: string;
  name: string;
  width: number;
  height: number;
  bgColor: string;
  elements: CanvasElement[];
}

function createFormId() {
  return `form_${Date.now()}_${Math.random().toString(36).substr(2, 5)}`;
}

const forms = ref<FormDef[]>([
  {
    id: createFormId(),
    name: "主窗体",
    width: 1024,
    height: 600,
    bgColor: "#1e293b",
    elements: [],
  },
]);
const currentFormIndex = ref(0);

const currentForm = computed(() => forms.value[currentFormIndex.value]);
const canvasElements = computed(() => currentForm.value?.elements ?? []);
const canvasWidth = computed(() => currentForm.value?.width ?? 1024);
const canvasHeight = computed(() => currentForm.value?.height ?? 600);

function addForm() {
  const name = `窗体${forms.value.length + 1}`;
  forms.value.push({
    id: createFormId(),
    name,
    width: 1024,
    height: 600,
    bgColor: "#1e293b",
    elements: [],
  });
  currentFormIndex.value = forms.value.length - 1;
}

function removeForm(idx: number) {
  if (forms.value.length <= 1) return;
  forms.value.splice(idx, 1);
  if (currentFormIndex.value >= forms.value.length) {
    currentFormIndex.value = forms.value.length - 1;
  }
}

function renameForm(idx: number) {
  const name = prompt("窗体名称:", forms.value[idx].name);
  if (name && name.trim()) forms.value[idx].name = name.trim();
}

function switchForm(idx: number) {
  currentFormIndex.value = idx;
  selectedElement.value = null;
}

function selectElement(eIdx: number) {
  selectedElement.value = eIdx;
}

function deleteElement(eIdx: number) {
  currentForm.value.elements.splice(eIdx, 1);
  if (selectedElement.value === eIdx) selectedElement.value = null;
  else if (selectedElement.value !== null && selectedElement.value > eIdx) {
    selectedElement.value--;
  }
}

function toggleToolboxSection(name: string) {
  expandedToolboxSections[name] = expandedToolboxSections[name] === false;
}

const expandedSections = reactive({
  identity: true,
  text: true,
  style: true,
  icon: false,
  interaction: true,
  binding: true,
});

// ==================== 控件定义 ====================
interface ControlDef {
  type: string;
  label: string;
  icon: string;
  category: string;
  defaults: Record<string, any>;
  properties: string[]; // 可编辑属性列表
}

const allControls: ControlDef[] = [
  // ===== 文本展示类 =====
  {
    type: "label",
    label: "静态文本",
    icon: "text_fields",
    category: "文本展示",
    defaults: { w: 120, h: 32, text: "Label", fontSize: 14 },
    properties: ["text", "icon"],
  },
  {
    type: "textblock",
    label: "多行文本",
    icon: "notes",
    category: "文本展示",
    defaults: { w: 160, h: 80, text: "多行文本内容\n第二行文本", fontSize: 12 },
    properties: ["text", "icon"],
  },
  {
    type: "tooltip",
    label: "悬浮提示",
    icon: "info",
    category: "文本展示",
    defaults: { w: 100, h: 32, text: "提示内容", icon: "info", iconSize: 16 },
    properties: ["text", "icon"],
  },
  {
    type: "richtext",
    label: "富文本",
    icon: "format_paragraph",
    category: "文本展示",
    defaults: { w: 160, h: 60, text: "富文本", fontSize: 14 },
    properties: ["text", "icon"],
  },
  {
    type: "datetime",
    label: "时间展示",
    icon: "schedule",
    category: "文本展示",
    defaults: { w: 160, h: 36, text: "2025-07-21 14:30:00", fontSize: 13 },
    properties: ["text", "icon"],
  },

  // ===== 基础输入 =====
  {
    type: "input",
    label: "单行输入",
    icon: "edit",
    category: "基础输入",
    defaults: {
      w: 180,
      h: 36,
      text: "",
      placeholder: "请输入...",
      fontSize: 13,
    },
    properties: ["text", "placeholder", "readonly", "disabled", "icon"],
  },
  {
    type: "textarea",
    label: "多行文本框",
    icon: "article",
    category: "基础输入",
    defaults: {
      w: 200,
      h: 100,
      text: "",
      placeholder: "请输入备注...",
      fontSize: 12,
    },
    properties: ["text", "placeholder", "readonly", "disabled", "icon"],
  },
  {
    type: "numberinput",
    label: "数字输入",
    icon: "pin",
    category: "基础输入",
    defaults: {
      w: 140,
      h: 36,
      text: "0",
      min: 0,
      max: 100,
      step: 1,
      unit: "",
      fontSize: 13,
    },
    properties: [
      "text",
      "min",
      "max",
      "step",
      "unit",
      "readonly",
      "disabled",
      "icon",
    ],
  },
  {
    type: "password",
    label: "密码框",
    icon: "lock",
    category: "基础输入",
    defaults: {
      w: 180,
      h: 36,
      text: "",
      placeholder: "请输入密码",
      fontSize: 13,
    },
    properties: ["text", "placeholder", "disabled", "icon"],
  },
  {
    type: "search",
    label: "搜索框",
    icon: "search",
    category: "基础输入",
    defaults: { w: 200, h: 36, text: "", placeholder: "搜索...", fontSize: 13 },
    properties: ["text", "placeholder", "disabled", "icon"],
  },

  // ===== 选择控件 =====
  {
    type: "select",
    label: "下拉选择",
    icon: "arrow_drop_down_circle",
    category: "选择控件",
    defaults: {
      w: 160,
      h: 36,
      text: "选项A",
      options: "选项A\n选项B\n选项C",
      fontSize: 13,
    },
    properties: ["text", "options", "disabled", "icon"],
  },
  {
    type: "radio",
    label: "单选框",
    icon: "radio_button_checked",
    category: "选择控件",
    defaults: {
      w: 200,
      h: 32,
      text: "选项A",
      options: "选项A\n选项B\n选项C",
      checked: false,
      fontSize: 12,
    },
    properties: ["text", "options", "disabled", "icon"],
  },
  {
    type: "checkbox",
    label: "复选框",
    icon: "check_box",
    category: "选择控件",
    defaults: { w: 140, h: 32, text: "复选", checked: false, fontSize: 12 },
    properties: ["text", "checked", "disabled", "icon"],
  },
  {
    type: "switch",
    label: "开关",
    icon: "toggle_on",
    category: "选择控件",
    defaults: { w: 80, h: 36, checked: false, fgColor: "#4ade80" },
    properties: ["checked", "disabled", "icon"],
  },
  {
    type: "cascader",
    label: "级联选择",
    icon: "account_tree",
    category: "选择控件",
    defaults: {
      w: 180,
      h: 36,
      text: "请选择",
      options: "省A/市1/区1\n省A/市2/区2\n省B/市3/区3",
      fontSize: 13,
    },
    properties: ["text", "options", "disabled", "icon"],
  },
  {
    type: "datepicker",
    label: "日期选择",
    icon: "calendar_month",
    category: "选择控件",
    defaults: { w: 160, h: 36, text: "2025-07-21", fontSize: 13 },
    properties: ["text", "disabled", "icon"],
  },
  {
    type: "daterange",
    label: "日期范围",
    icon: "date_range",
    category: "选择控件",
    defaults: { w: 220, h: 36, text: "2025-07-01 ~ 2025-07-21", fontSize: 12 },
    properties: ["text", "disabled", "icon"],
  },
  {
    type: "timepicker",
    label: "时间选择",
    icon: "schedule",
    category: "选择控件",
    defaults: { w: 130, h: 36, text: "14:30", fontSize: 13 },
    properties: ["text", "disabled", "icon"],
  },
  {
    type: "upload",
    label: "文件上传",
    icon: "upload_file",
    category: "选择控件",
    defaults: { w: 180, h: 48, text: "点击上传文件", fontSize: 12 },
    properties: ["text", "disabled", "icon"],
  },

  // ===== 按钮类 =====
  {
    type: "button",
    label: "普通按钮",
    icon: "smart_button",
    category: "按钮类",
    defaults: {
      w: 120,
      h: 40,
      text: "按钮",
      fontSize: 13,
      bgColor: "#475569",
      fgColor: "#ffffff",
    },
    properties: ["text", "icon", "disabled", "fontSize"],
  },
  {
    type: "primarybutton",
    label: "主按钮",
    icon: "ads_click",
    category: "按钮类",
    defaults: {
      w: 120,
      h: 40,
      text: "确认",
      fontSize: 13,
      bgColor: "#3b82f6",
      fgColor: "#ffffff",
    },
    properties: ["text", "icon", "disabled", "fontSize"],
  },
  {
    type: "iconbutton",
    label: "图标按钮",
    icon: "touch_app",
    category: "按钮类",
    defaults: {
      w: 40,
      h: 40,
      icon: "settings",
      iconSize: 20,
      bgColor: "#475569",
      fgColor: "#ffffff",
    },
    properties: ["icon", "iconSize", "disabled"],
  },
  {
    type: "linkbutton",
    label: "链接按钮",
    icon: "link",
    category: "按钮类",
    defaults: {
      w: 100,
      h: 32,
      text: "链接按钮",
      fontSize: 12,
      fgColor: "#60a5fa",
    },
    properties: ["text", "disabled", "fontSize"],
  },
  {
    type: "splitbutton",
    label: "下拉按钮",
    icon: "arrow_drop_down",
    category: "按钮类",
    defaults: {
      w: 140,
      h: 40,
      text: "操作",
      options: "导出\n打印\n删除",
      fontSize: 13,
      bgColor: "#475569",
      fgColor: "#ffffff",
    },
    properties: ["text", "options", "disabled", "icon"],
  },

  // ===== 滑块与进度 =====
  {
    type: "slider",
    label: "滑块",
    icon: "linear_scale",
    category: "滑块与进度",
    defaults: {
      w: 200,
      h: 40,
      min: 0,
      max: 100,
      step: 1,
      value: 50,
      bgColor: "#475569",
      fgColor: "#3b82f6",
    },
    properties: ["min", "max", "step", "value", "unit", "disabled", "icon"],
  },
  {
    type: "progress",
    label: "进度条",
    icon: "progress_activity",
    category: "滑块与进度",
    defaults: {
      w: 200,
      h: 24,
      min: 0,
      max: 100,
      value: 65,
      bgColor: "#1e293b",
      fgColor: "#3b82f6",
      text: "65%",
      fontSize: 11,
    },
    properties: ["min", "max", "value", "text", "icon"],
  },
  {
    type: "circleprogress",
    label: "环形进度",
    icon: "donut_large",
    category: "滑块与进度",
    defaults: {
      w: 80,
      h: 80,
      min: 0,
      max: 100,
      value: 72,
      fgColor: "#22c55e",
      bgColor: "#1e293b",
      text: "72%",
      fontSize: 14,
    },
    properties: ["min", "max", "value", "text", "icon"],
  },
  {
    type: "rate",
    label: "评分",
    icon: "star",
    category: "滑块与进度",
    defaults: {
      w: 160,
      h: 32,
      value: 3,
      max: 5,
      fgColor: "#fbbf24",
      fontSize: 18,
    },
    properties: ["min", "max", "value", "disabled", "icon"],
  },
  {
    type: "gauge",
    label: "仪表盘",
    icon: "speed",
    category: "滑块与进度",
    defaults: {
      w: 120,
      h: 120,
      min: 0,
      max: 100,
      value: 68,
      fgColor: "#22c55e",
      bgColor: "#1e293b",
      text: "68",
      unit: "%",
      fontSize: 16,
    },
    properties: ["min", "max", "value", "unit", "icon"],
  },

  // ===== 数据展示 =====
  {
    type: "table",
    label: "表格",
    icon: "table_chart",
    category: "数据展示",
    defaults: {
      w: 300,
      h: 180,
      text: "列A | 列B | 列C\n100 | 200 | 300\n400 | 500 | 600",
      fontSize: 11,
    },
    properties: ["text", "fontSize", "disabled", "icon"],
  },
  {
    type: "tree",
    label: "树形控件",
    icon: "account_tree",
    category: "数据展示",
    defaults: {
      w: 200,
      h: 180,
      text: "设备列表\n  设备1\n  设备2\n  设备3",
      fontSize: 12,
    },
    properties: ["text", "fontSize", "disabled", "icon"],
  },
  {
    type: "list",
    label: "列表",
    icon: "format_list_bulleted",
    category: "数据展示",
    defaults: { w: 200, h: 150, text: "项目 1\n项目 2\n项目 3", fontSize: 12 },
    properties: ["text", "fontSize", "disabled", "icon"],
  },
  {
    type: "tabs",
    label: "标签页",
    icon: "tab",
    category: "数据展示",
    defaults: {
      w: 280,
      h: 40,
      text: "标签1\n标签2\n标签3",
      fontSize: 12,
      fgColor: "#3b82f6",
    },
    properties: ["text", "options", "fontSize", "disabled", "icon"],
  },
  {
    type: "steps",
    label: "步骤条",
    icon: "timeline",
    category: "数据展示",
    defaults: {
      w: 360,
      h: 60,
      text: "步骤1\n步骤2\n步骤3",
      value: 2,
      max: 3,
      fontSize: 11,
      fgColor: "#3b82f6",
    },
    properties: ["text", "value", "max", "fontSize", "icon"],
  },
  {
    type: "timeline",
    label: "时间线",
    icon: "schedule",
    category: "数据展示",
    defaults: {
      w: 220,
      h: 160,
      text: "14:00 启动\n14:30 运行\n15:00 完成",
      fontSize: 11,
    },
    properties: ["text", "fontSize", "disabled", "icon"],
  },
  {
    type: "pagination",
    label: "分页",
    icon: "more_horiz",
    category: "数据展示",
    defaults: { w: 240, h: 36, value: 1, max: 10, fontSize: 12 },
    properties: ["value", "max", "disabled", "icon"],
  },

  // ===== 图表 =====
  {
    type: "linechart",
    label: "折线图",
    icon: "show_chart",
    category: "图表",
    defaults: {
      w: 300,
      h: 200,
      text: "折线图",
      fgColor: "#3b82f6",
      bgColor: "#0f172a",
      fontSize: 12,
    },
    properties: ["text", "fgColor", "bgColor", "disabled", "icon"],
  },
  {
    type: "barchart",
    label: "柱状图",
    icon: "bar_chart",
    category: "图表",
    defaults: {
      w: 300,
      h: 200,
      text: "柱状图",
      fgColor: "#8b5cf6",
      bgColor: "#0f172a",
      fontSize: 12,
    },
    properties: ["text", "fgColor", "bgColor", "disabled", "icon"],
  },
  {
    type: "piechart",
    label: "饼图",
    icon: "pie_chart",
    category: "图表",
    defaults: {
      w: 200,
      h: 200,
      text: "饼图",
      fgColor: "#f97316",
      bgColor: "#0f172a",
      fontSize: 12,
    },
    properties: ["text", "fgColor", "bgColor", "disabled", "icon"],
  },
  {
    type: "areachart",
    label: "面积图",
    icon: "area_chart",
    category: "图表",
    defaults: {
      w: 300,
      h: 200,
      text: "面积图",
      fgColor: "#22c55e",
      bgColor: "#0f172a",
      fontSize: 12,
    },
    properties: ["text", "fgColor", "bgColor", "disabled", "icon"],
  },

  // ===== 容器与反馈 =====
  {
    type: "card",
    label: "卡片",
    icon: "dashboard",
    category: "容器与反馈",
    defaults: {
      w: 240,
      h: 180,
      text: "卡片标题",
      fontSize: 14,
      bgColor: "#1e293b",
      borderColor: "#334155",
    },
    properties: ["text", "fontSize", "icon"],
  },
  {
    type: "divider",
    label: "分割线",
    icon: "horizontal_rule",
    category: "容器与反馈",
    defaults: { w: 200, h: 8, bgColor: "#334155" },
    properties: ["bgColor", "icon"],
  },
  {
    type: "alert",
    label: "提示信息",
    icon: "info",
    category: "容器与反馈",
    defaults: {
      w: 300,
      h: 48,
      text: "这是一条提示信息",
      bgColor: "#1e3a5f",
      fgColor: "#60a5fa",
      fontSize: 12,
      icon: "info",
    },
    properties: ["text", "bgColor", "fgColor", "fontSize", "icon"],
  },
  {
    type: "badge",
    label: "徽标",
    icon: "new_releases",
    category: "容器与反馈",
    defaults: {
      w: 48,
      h: 48,
      text: "5",
      bgColor: "#ef4444",
      fgColor: "#ffffff",
      fontSize: 11,
      icon: "notifications",
    },
    properties: ["text", "bgColor", "fgColor", "fontSize", "icon"],
  },
  {
    type: "avatar",
    label: "头像",
    icon: "account_circle",
    category: "容器与反馈",
    defaults: {
      w: 48,
      h: 48,
      text: "A",
      bgColor: "#6366f1",
      fgColor: "#ffffff",
      fontSize: 18,
    },
    properties: ["text", "bgColor", "fgColor", "fontSize", "icon"],
  },
  {
    type: "breadcrumb",
    label: "面包屑",
    icon: "chevron_right",
    category: "容器与反馈",
    defaults: {
      w: 280,
      h: 32,
      text: "首页 / 设备管理 / 设备列表",
      fontSize: 12,
    },
    properties: ["text", "fontSize", "icon"],
  },
  {
    type: "message",
    label: "轻提示",
    icon: "notifications_active",
    category: "容器与反馈",
    defaults: {
      w: 200,
      h: 40,
      text: "操作成功",
      bgColor: "#166534",
      fgColor: "#4ade80",
      fontSize: 12,
      icon: "check_circle",
    },
    properties: ["text", "bgColor", "fgColor", "fontSize", "icon"],
  },
  {
    type: "menu",
    label: "菜单",
    icon: "menu",
    category: "容器与反馈",
    defaults: {
      w: 200,
      h: 180,
      text: "菜单项1\n菜单项2\n菜单项3\n菜单项4",
      fontSize: 12,
      bgColor: "#1e293b",
    },
    properties: ["text", "fontSize", "disabled", "icon"],
  },

  // ===== 导航 =====
  {
    type: "scrollbar",
    label: "滚动条",
    icon: "swap_vert",
    category: "导航",
    defaults: { w: 16, h: 200, bgColor: "#334155", fgColor: "#64748b" },
    properties: ["bgColor", "fgColor", "icon"],
  },
];

const industrialLibrary = [
  {
    type: "tank",
    label: "储罐",
    icon: "propane_tank",
    category: "工业设备",
    defaults: { w: 80, h: 120, bgColor: "#1e293b", fgColor: "#60a5fa" },
    properties: ["value", "min", "max", "unit", "icon"],
  },
  {
    type: "pump",
    label: "电机/泵",
    icon: "cyclone",
    category: "工业设备",
    defaults: { w: 80, h: 80, bgColor: "#1e293b", fgColor: "#22c55e" },
    properties: ["checked", "disabled", "icon"],
  },
  {
    type: "valve",
    label: "阀门",
    icon: "valve",
    category: "工业设备",
    defaults: { w: 80, h: 60, bgColor: "#1e293b", fgColor: "#f97316" },
    properties: ["checked", "disabled", "icon"],
  },
  {
    type: "pipe",
    label: "管道",
    icon: "plumbing",
    category: "工业设备",
    defaults: { w: 200, h: 20, bgColor: "#334155", fgColor: "#64748b" },
    properties: ["bgColor", "fgColor", "icon"],
  },
  {
    type: "conveyor",
    label: "传送带",
    icon: "moving",
    category: "工业设备",
    defaults: { w: 200, h: 40, bgColor: "#1e293b", fgColor: "#a78bfa" },
    properties: ["checked", "disabled", "icon"],
  },
  {
    type: "agv",
    label: "AGV小车",
    icon: "local_shipping",
    category: "工业设备",
    defaults: { w: 80, h: 60, bgColor: "#1e293b", fgColor: "#fbbf24" },
    properties: ["value", "disabled", "icon"],
  },
  {
    type: "tower",
    label: "信号塔",
    icon: "cell_tower",
    category: "工业设备",
    defaults: { w: 60, h: 120, bgColor: "#1e293b", fgColor: "#ef4444" },
    properties: ["checked", "icon"],
  },
  {
    type: "solar",
    label: "太阳能板",
    icon: "solar_power",
    category: "工业设备",
    defaults: { w: 120, h: 80, bgColor: "#1e293b", fgColor: "#fbbf24" },
    properties: ["value", "icon"],
  },
];

// 所有控件合并
const allControlMap = new Map<string, ControlDef>();
[...allControls, ...industrialLibrary].forEach((c) =>
  allControlMap.set(c.type, c),
);

// 分类
const controlCategories = computed(() => {
  const cats: Record<string, ControlDef[]> = {};
  for (const c of allControls) {
    if (!cats[c.category]) cats[c.category] = [];
    cats[c.category].push(c);
  }
  return Object.entries(cats).map(([name, items]) => ({ name, items }));
});

const filteredControlCategories = computed(() => {
  if (!toolboxSearch.value) return controlCategories.value;
  const q = toolboxSearch.value.toLowerCase();
  return controlCategories.value
    .map((cat) => ({
      ...cat,
      items: cat.items.filter(
        (c) =>
          c.label.toLowerCase().includes(q) ||
          c.type.toLowerCase().includes(q) ||
          c.category.toLowerCase().includes(q),
      ),
    }))
    .filter((cat) => cat.items.length > 0);
});

// ==================== 元素管理 ====================
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

function getControlMeta(type: string): ControlDef | undefined {
  return allControlMap.get(type);
}

function hasProperty(prop: string): boolean {
  if (selectedElement.value === null) return false;
  const el = canvasElements.value[selectedElement.value];
  const meta = allControlMap.get(el.type);
  return meta?.properties.includes(prop) ?? false;
}

let dragType = "";
let dragIcon = "";
let dragDefaults: Record<string, any> = {};

function onDragStart(
  e: DragEvent,
  item: { type: string; icon: string; defaults?: Record<string, any> },
) {
  dragType = item.type;
  dragIcon = item.icon;
  dragDefaults = item.defaults || {};
  e.dataTransfer?.setData("text/plain", item.type);
}

function onDrop(e: DragEvent) {
  const rect = (e.currentTarget as HTMLElement).getBoundingClientRect();
  const scale = zoom.value / 100;
  const x = Math.round((e.clientX - rect.left) / scale / 20) * 20;
  const y = Math.round((e.clientY - rect.top) / scale / 20) * 20;

  const meta = allControlMap.get(dragType);
  const defaults = meta?.defaults || {};

  const el: CanvasElement = {
    type: dragType,
    x,
    y,
    w: defaults.w || 120,
    h: defaults.h || 60,
    id: `el_${Date.now()}_${Math.random().toString(36).substr(2, 5)}`,
    icon: defaults.icon || dragIcon,
    ...JSON.parse(JSON.stringify(defaults)),
  };
  currentForm.value.elements.push(el);
  selectedElement.value = currentForm.value.elements.length - 1;
  pushHistory();
}

function deleteSelected() {
  if (selectedElement.value !== null) {
    currentForm.value.elements.splice(selectedElement.value, 1);
    selectedElement.value = null;
    pushHistory();
  }
}

function updateProp(key: string, e: Event) {
  if (selectedElement.value === null) return;
  const el = canvasElements.value[selectedElement.value];
  const target = e.target as
    HTMLInputElement | HTMLSelectElement | HTMLTextAreaElement;
  const meta = allControlMap.get(el.type);

  if (key === "checked" || key === "disabled" || key === "readonly") {
    (el as any)[key] = (target as HTMLInputElement).checked;
  } else if (
    [
      "fontSize",
      "iconSize",
      "min",
      "max",
      "step",
      "value",
      "borderWidth",
      "borderRadius",
      "opacity",
      "x",
      "y",
      "w",
      "h",
      "scaleMin",
      "scaleMax",
    ].includes(key)
  ) {
    (el as any)[key] = Number(target.value);
  } else {
    (el as any)[key] = target.value;
  }
}

// ==================== 撤销/重做 ====================
const history: string[] = [];
const future: string[] = [];
let historyLock = false;

function pushHistory() {
  if (historyLock) return;
  history.push(JSON.stringify(forms.value));
  if (history.length > 50) history.shift();
  future.length = 0;
}

function undo() {
  if (history.length <= 1) return;
  historyLock = true;
  future.push(history.pop()!);
  forms.value = JSON.parse(history[history.length - 1]);
  historyLock = false;
}

function redo() {
  if (future.length === 0) return;
  historyLock = true;
  const state = future.pop()!;
  history.push(state);
  forms.value = JSON.parse(state);
  historyLock = false;
}

// ==================== 缩放 ====================
function zoomIn() {
  zoom.value = Math.min(200, zoom.value + 10);
}
function zoomOut() {
  zoom.value = Math.max(25, zoom.value - 10);
}

function toggleSection(key: keyof typeof expandedSections) {
  expandedSections[key] = !expandedSections[key];
}

// ==================== 拖拽移动 + 缩放手柄 ====================
type ResizeDir = "n" | "s" | "e" | "w" | "ne" | "nw" | "se" | "sw" | null;

let isDragging = false;
let isResizing = false;
let resizeDir: ResizeDir = null;
let startX = 0;
let startY = 0;
let startElX = 0;
let startElY = 0;
let startElW = 0;
let startElH = 0;
const MIN_SIZE = 20;
const GRID = 20;

function snapGrid(v: number): number {
  return Math.round(v / GRID) * GRID;
}

function onElementMouseDown(e: MouseEvent, idx: number) {
  if (isPreview.value) return;
  e.preventDefault();
  e.stopPropagation();
  selectedElement.value = idx;
  pushHistory();

  isDragging = true;
  const el = canvasElements.value[idx];
  startX = e.clientX;
  startY = e.clientY;
  startElX = el.x;
  startElY = el.y;

  const onMove = (me: MouseEvent) => {
    if (!isDragging) return;
    const scale = zoom.value / 100;
    const dx = (me.clientX - startX) / scale;
    const dy = (me.clientY - startY) / scale;
    el.x = snapGrid(startElX + dx);
    el.y = snapGrid(startElY + dy);
  };

  const onUp = () => {
    isDragging = false;
    document.removeEventListener("mousemove", onMove);
    document.removeEventListener("mouseup", onUp);
  };

  document.addEventListener("mousemove", onMove);
  document.addEventListener("mouseup", onUp);
}

function onResizeMouseDown(e: MouseEvent, idx: number, dir: ResizeDir) {
  if (isPreview.value) return;
  e.preventDefault();
  e.stopPropagation();

  isResizing = true;
  resizeDir = dir;
  const el = canvasElements.value[idx];
  startX = e.clientX;
  startY = e.clientY;
  startElX = el.x;
  startElY = el.y;
  startElW = el.w;
  startElH = el.h;

  const onMove = (me: MouseEvent) => {
    if (!isResizing || !resizeDir) return;
    const scale = zoom.value / 100;
    const dx = (me.clientX - startX) / scale;
    const dy = (me.clientY - startY) / scale;

    let newX = startElX;
    let newY = startElY;
    let newW = startElW;
    let newH = startElH;

    if (resizeDir.includes("e"))
      newW = Math.max(MIN_SIZE, snapGrid(startElW + dx));
    if (resizeDir.includes("w")) {
      newW = Math.max(MIN_SIZE, snapGrid(startElW - dx));
      newX = startElX + startElW - newW;
    }
    if (resizeDir.includes("s"))
      newH = Math.max(MIN_SIZE, snapGrid(startElH + dy));
    if (resizeDir.includes("n")) {
      newH = Math.max(MIN_SIZE, snapGrid(startElH - dy));
      newY = startElY + startElH - newH;
    }

    el.x = newX;
    el.y = newY;
    el.w = newW;
    el.h = newH;
  };

  const onUp = () => {
    isResizing = false;
    resizeDir = null;
    document.removeEventListener("mousemove", onMove);
    document.removeEventListener("mouseup", onUp);
  };

  document.addEventListener("mousemove", onMove);
  document.addEventListener("mouseup", onUp);
}

function onCanvasBgMouseDown() {
  selectedElement.value = null;
}

function onKeyDown(e: KeyboardEvent) {
  if (
    (e.key === "Delete" || e.key === "Backspace") &&
    selectedElement.value !== null
  ) {
    if (
      (e.target as HTMLElement).tagName === "INPUT" ||
      (e.target as HTMLElement).tagName === "TEXTAREA" ||
      (e.target as HTMLElement).tagName === "SELECT"
    )
      return;
    deleteSelected();
  }
  if (e.ctrlKey && e.key === "z") {
    e.preventDefault();
    undo();
  }
  if (e.ctrlKey && e.key === "y") {
    e.preventDefault();
    redo();
  }
}

onBeforeUnmount(() => {
  document.removeEventListener("keydown", onKeyDown);
});
document.addEventListener("keydown", onKeyDown);

// ==================== 渲染器 ====================
function getRenderer(type: string): Component {
  const rendererMap: Record<string, Component> = {
    label: {
      props: ["element", "isPreview"],
      setup(props: any) {
        return () =>
          h(
            "span",
            {
              class: "el-text",
              style: {
                fontSize: (props.element.fontSize || 14) + "px",
                fontWeight: props.element.fontWeight || "normal",
                textAlign: props.element.textAlign || "center",
                color: props.element.fgColor || "var(--on-surface)",
                width: "100%",
                display: "block",
                padding: "2px 6px",
                overflow: "hidden",
                textOverflow: "ellipsis",
                whiteSpace: "nowrap",
              },
            },
            props.element.text || "文本标签",
          );
      },
    },
    textblock: {
      props: ["element", "isPreview"],
      setup(props: any) {
        return () =>
          h(
            "div",
            {
              class: "el-text",
              style: {
                fontSize: (props.element.fontSize || 12) + "px",
                color: props.element.fgColor || "var(--on-surface)",
                width: "100%",
                height: "100%",
                padding: "4px 6px",
                overflow: "hidden",
                whiteSpace: "pre-wrap",
                wordBreak: "break-all",
                lineHeight: "1.4",
              },
            },
            props.element.text || "多行文本",
          );
      },
    },
    richtext: {
      props: ["element", "isPreview"],
      setup(props: any) {
        return () =>
          h(
            "div",
            {
              class: "el-text",
              style: {
                fontSize: (props.element.fontSize || 14) + "px",
                color: props.element.fgColor || "var(--on-surface)",
                width: "100%",
                height: "100%",
                padding: "4px 8px",
                overflow: "hidden",
                fontStyle: "italic",
              },
            },
            props.element.text || "富文本",
          );
      },
    },
    datetime: {
      props: ["element", "isPreview"],
      setup(props: any) {
        return () =>
          h(
            "div",
            {
              class: "el-text",
              style: {
                fontSize: (props.element.fontSize || 13) + "px",
                fontFamily: "'JetBrains Mono', monospace",
                color: props.element.fgColor || "#60a5fa",
                width: "100%",
                textAlign: "center",
                letterSpacing: "0.03em",
              },
            },
            [
              h(
                "span",
                {
                  class: "material-symbols-outlined",
                  style: {
                    fontSize: "14px",
                    verticalAlign: "middle",
                    marginRight: "4px",
                  },
                },
                "schedule",
              ),
              props.element.text || "2025-07-21 14:30:00",
            ],
          );
      },
    },
    tooltip: {
      props: ["element", "isPreview"],
      setup(props: any) {
        return () =>
          h(
            "div",
            {
              class: "el-tooltip",
              style: {
                display: "flex",
                alignItems: "center",
                justifyContent: "center",
                gap: "4px",
                fontSize: (props.element.fontSize || 12) + "px",
                color: props.element.fgColor || "#60a5fa",
                width: "100%",
                height: "100%",
              },
            },
            [
              h(
                "span",
                {
                  class: "material-symbols-outlined",
                  style: { fontSize: (props.element.iconSize || 16) + "px" },
                },
                props.element.icon || "info",
              ),
              h("span", {}, props.element.text || "提示"),
            ],
          );
      },
    },
    input: {
      props: ["element", "isPreview"],
      setup(props: any) {
        return () =>
          h("input", {
            class: "el-input",
            type: "text",
            value: props.element.text || "",
            placeholder: props.element.placeholder || "",
            readonly:
              props.element.readonly || props.isPreview
                ? undefined
                : "readonly",
            style: {
              fontSize: (props.element.fontSize || 13) + "px",
              color: props.element.fgColor || "var(--on-surface)",
              width: "100%",
              height: "100%",
              padding: "0 8px",
              background:
                props.element.bgColor || "var(--surface-container-highest)",
              border: "1px solid var(--outline-variant)",
              borderRadius: (props.element.borderRadius ?? 4) + "px",
              outline: "none",
            },
          });
      },
    },
    textarea: {
      props: ["element", "isPreview"],
      setup(props: any) {
        return () =>
          h("textarea", {
            class: "el-textarea",
            value: props.element.text || "",
            placeholder: props.element.placeholder || "",
            style: {
              fontSize: (props.element.fontSize || 12) + "px",
              color: props.element.fgColor || "var(--on-surface)",
              width: "100%",
              height: "100%",
              padding: "4px 8px",
              background:
                props.element.bgColor || "var(--surface-container-highest)",
              border: "1px solid var(--outline-variant)",
              borderRadius: (props.element.borderRadius ?? 4) + "px",
              outline: "none",
              resize: "none",
            },
          });
      },
    },
    numberinput: {
      props: ["element", "isPreview"],
      setup(props: any) {
        return () =>
          h(
            "div",
            {
              style: {
                display: "flex",
                alignItems: "center",
                width: "100%",
                height: "100%",
                gap: "2px",
              },
            },
            [
              h("input", {
                type: "number",
                value: props.element.text || "0",
                min: props.element.min,
                max: props.element.max,
                step: props.element.step,
                style: {
                  flex: "1",
                  fontSize: (props.element.fontSize || 13) + "px",
                  fontFamily: "'JetBrains Mono', monospace",
                  color: props.element.fgColor || "var(--on-surface)",
                  height: "100%",
                  padding: "0 6px",
                  background:
                    props.element.bgColor || "var(--surface-container-highest)",
                  border: "1px solid var(--outline-variant)",
                  borderRadius: (props.element.borderRadius ?? 4) + "px",
                  outline: "none",
                },
              }),
              props.element.unit
                ? h(
                    "span",
                    {
                      style: {
                        fontSize: "11px",
                        color: "var(--on-surface-variant)",
                        whiteSpace: "nowrap",
                      },
                    },
                    props.element.unit,
                  )
                : null,
            ],
          );
      },
    },
    password: {
      props: ["element", "isPreview"],
      setup(props: any) {
        return () =>
          h(
            "div",
            {
              style: {
                display: "flex",
                alignItems: "center",
                width: "100%",
                height: "100%",
                position: "relative",
              },
            },
            [
              h("input", {
                type: props.isPreview ? "password" : "text",
                value: props.element.text || "",
                placeholder: props.element.placeholder || "请输入密码",
                style: {
                  fontSize: (props.element.fontSize || 13) + "px",
                  color: props.element.fgColor || "var(--on-surface)",
                  width: "100%",
                  height: "100%",
                  padding: "0 28px 0 8px",
                  background:
                    props.element.bgColor || "var(--surface-container-highest)",
                  border: "1px solid var(--outline-variant)",
                  borderRadius: (props.element.borderRadius ?? 4) + "px",
                  outline: "none",
                },
              }),
              h(
                "span",
                {
                  class: "material-symbols-outlined",
                  style: {
                    position: "absolute",
                    right: "6px",
                    fontSize: "16px",
                    color: "var(--on-surface-variant)",
                  },
                },
                "lock",
              ),
            ],
          );
      },
    },
    search: {
      props: ["element", "isPreview"],
      setup(props: any) {
        return () =>
          h(
            "div",
            {
              style: {
                display: "flex",
                alignItems: "center",
                width: "100%",
                height: "100%",
                position: "relative",
              },
            },
            [
              h(
                "span",
                {
                  class: "material-symbols-outlined",
                  style: {
                    position: "absolute",
                    left: "6px",
                    fontSize: "16px",
                    color: "var(--on-surface-variant)",
                  },
                },
                "search",
              ),
              h("input", {
                type: "text",
                value: props.element.text || "",
                placeholder: props.element.placeholder || "搜索...",
                style: {
                  fontSize: (props.element.fontSize || 13) + "px",
                  color: props.element.fgColor || "var(--on-surface)",
                  width: "100%",
                  height: "100%",
                  padding: "0 24px 0 28px",
                  background:
                    props.element.bgColor || "var(--surface-container-highest)",
                  border: "1px solid var(--outline-variant)",
                  borderRadius: (props.element.borderRadius ?? 16) + "px",
                  outline: "none",
                },
              }),
            ],
          );
      },
    },
    select: {
      props: ["element", "isPreview"],
      setup(props: any) {
        return () => {
          const opts = (props.element.options || "选项A\n选项B\n选项C").split(
            "\n",
          );
          return h(
            "div",
            { style: { position: "relative", width: "100%", height: "100%" } },
            [
              h(
                "select",
                {
                  style: {
                    fontSize: (props.element.fontSize || 13) + "px",
                    color: props.element.fgColor || "var(--on-surface)",
                    width: "100%",
                    height: "100%",
                    padding: "0 24px 0 8px",
                    background:
                      props.element.bgColor ||
                      "var(--surface-container-highest)",
                    border: "1px solid var(--outline-variant)",
                    borderRadius: (props.element.borderRadius ?? 4) + "px",
                    outline: "none",
                    appearance: "none",
                  },
                },
                opts.map((o: string) =>
                  h("option", { value: o.trim() }, o.trim()),
                ),
              ),
              h(
                "span",
                {
                  class: "material-symbols-outlined",
                  style: {
                    position: "absolute",
                    right: "4px",
                    top: "50%",
                    transform: "translateY(-50%)",
                    fontSize: "18px",
                    color: "var(--on-surface-variant)",
                    pointerEvents: "none",
                  },
                },
                "expand_more",
              ),
            ],
          );
        };
      },
    },
    radio: {
      props: ["element", "isPreview"],
      setup(props: any) {
        return () => {
          const opts = (props.element.options || "选项A\n选项B\n选项C").split(
            "\n",
          );
          return h(
            "div",
            {
              style: {
                display: "flex",
                flexDirection: "column",
                gap: "4px",
                padding: "4px 8px",
                width: "100%",
                height: "100%",
                overflow: "hidden",
              },
            },
            opts.map((o: string, i: number) =>
              h(
                "label",
                {
                  style: {
                    display: "flex",
                    alignItems: "center",
                    gap: "6px",
                    fontSize: (props.element.fontSize || 12) + "px",
                    color: props.element.fgColor || "var(--on-surface)",
                    cursor: "pointer",
                  },
                },
                [
                  h("input", {
                    type: "radio",
                    name: props.element.id,
                    checked: i === 0,
                    style: { accentColor: props.element.fgColor || "#3b82f6" },
                  }),
                  h("span", {}, o.trim()),
                ],
              ),
            ),
          );
        };
      },
    },
    checkbox: {
      props: ["element", "isPreview"],
      setup(props: any) {
        return () =>
          h(
            "label",
            {
              style: {
                display: "flex",
                alignItems: "center",
                gap: "6px",
                padding: "0 8px",
                fontSize: (props.element.fontSize || 12) + "px",
                color: props.element.fgColor || "var(--on-surface)",
                cursor: "pointer",
                width: "100%",
                height: "100%",
              },
            },
            [
              h("input", {
                type: "checkbox",
                checked: props.element.checked || false,
                style: {
                  accentColor: props.element.fgColor || "#3b82f6",
                  width: "16px",
                  height: "16px",
                },
              }),
              h("span", {}, props.element.text || "复选"),
            ],
          );
      },
    },
    switch: {
      props: ["element", "isPreview"],
      setup(props: any) {
        return () => {
          const on = props.element.checked || false;
          return h(
            "div",
            {
              style: {
                width: "44px",
                height: "24px",
                borderRadius: "12px",
                cursor: "pointer",
                background: on ? props.element.fgColor || "#4ade80" : "#475569",
                position: "relative",
                transition: "background 0.2s",
                margin: "auto",
              },
            },
            [
              h("div", {
                style: {
                  width: "18px",
                  height: "18px",
                  borderRadius: "50%",
                  background: "#fff",
                  position: "absolute",
                  top: "3px",
                  left: on ? "23px" : "3px",
                  transition: "left 0.2s",
                  boxShadow: "0 1px 3px rgba(0,0,0,0.3)",
                },
              }),
            ],
          );
        };
      },
    },
    cascader: {
      props: ["element", "isPreview"],
      setup(props: any) {
        return () =>
          h(
            "div",
            {
              style: {
                display: "flex",
                alignItems: "center",
                width: "100%",
                height: "100%",
                position: "relative",
              },
            },
            [
              h("input", {
                type: "text",
                value: props.element.text || "请选择",
                readOnly: true,
                style: {
                  fontSize: (props.element.fontSize || 13) + "px",
                  color: "var(--on-surface)",
                  width: "100%",
                  height: "100%",
                  padding: "0 24px 0 8px",
                  background:
                    props.element.bgColor || "var(--surface-container-highest)",
                  border: "1px solid var(--outline-variant)",
                  borderRadius: (props.element.borderRadius ?? 4) + "px",
                  outline: "none",
                },
              }),
              h(
                "span",
                {
                  class: "material-symbols-outlined",
                  style: {
                    position: "absolute",
                    right: "4px",
                    fontSize: "18px",
                    color: "var(--on-surface-variant)",
                  },
                },
                "expand_more",
              ),
            ],
          );
      },
    },
    datepicker: {
      props: ["element", "isPreview"],
      setup(props: any) {
        return () =>
          h(
            "div",
            {
              style: {
                display: "flex",
                alignItems: "center",
                width: "100%",
                height: "100%",
                position: "relative",
              },
            },
            [
              h("input", {
                type: "text",
                value: props.element.text || "2025-07-21",
                style: {
                  fontSize: (props.element.fontSize || 13) + "px",
                  fontFamily: "'JetBrains Mono', monospace",
                  color: "var(--on-surface)",
                  width: "100%",
                  height: "100%",
                  padding: "0 24px 0 8px",
                  background:
                    props.element.bgColor || "var(--surface-container-highest)",
                  border: "1px solid var(--outline-variant)",
                  borderRadius: (props.element.borderRadius ?? 4) + "px",
                  outline: "none",
                },
              }),
              h(
                "span",
                {
                  class: "material-symbols-outlined",
                  style: {
                    position: "absolute",
                    right: "4px",
                    fontSize: "16px",
                    color: "var(--on-surface-variant)",
                  },
                },
                "calendar_month",
              ),
            ],
          );
      },
    },
    daterange: {
      props: ["element", "isPreview"],
      setup(props: any) {
        return () =>
          h(
            "div",
            {
              style: {
                display: "flex",
                alignItems: "center",
                width: "100%",
                height: "100%",
                position: "relative",
                gap: "4px",
              },
            },
            [
              h("input", {
                type: "text",
                value: props.element.text || "2025-07-01 ~ 2025-07-21",
                readOnly: true,
                style: {
                  fontSize: (props.element.fontSize || 12) + "px",
                  fontFamily: "'JetBrains Mono', monospace",
                  color: "var(--on-surface)",
                  width: "100%",
                  height: "100%",
                  padding: "0 24px 0 8px",
                  background:
                    props.element.bgColor || "var(--surface-container-highest)",
                  border: "1px solid var(--outline-variant)",
                  borderRadius: (props.element.borderRadius ?? 4) + "px",
                  outline: "none",
                },
              }),
              h(
                "span",
                {
                  class: "material-symbols-outlined",
                  style: {
                    position: "absolute",
                    right: "4px",
                    fontSize: "16px",
                    color: "var(--on-surface-variant)",
                  },
                },
                "date_range",
              ),
            ],
          );
      },
    },
    timepicker: {
      props: ["element", "isPreview"],
      setup(props: any) {
        return () =>
          h(
            "div",
            {
              style: {
                display: "flex",
                alignItems: "center",
                width: "100%",
                height: "100%",
                position: "relative",
              },
            },
            [
              h("input", {
                type: "text",
                value: props.element.text || "14:30",
                style: {
                  fontSize: (props.element.fontSize || 13) + "px",
                  fontFamily: "'JetBrains Mono', monospace",
                  color: "var(--on-surface)",
                  width: "100%",
                  height: "100%",
                  padding: "0 24px 0 8px",
                  background:
                    props.element.bgColor || "var(--surface-container-highest)",
                  border: "1px solid var(--outline-variant)",
                  borderRadius: (props.element.borderRadius ?? 4) + "px",
                  outline: "none",
                },
              }),
              h(
                "span",
                {
                  class: "material-symbols-outlined",
                  style: {
                    position: "absolute",
                    right: "4px",
                    fontSize: "16px",
                    color: "var(--on-surface-variant)",
                  },
                },
                "schedule",
              ),
            ],
          );
      },
    },
    upload: {
      props: ["element", "isPreview"],
      setup(props: any) {
        return () =>
          h(
            "div",
            {
              style: {
                display: "flex",
                flexDirection: "column",
                alignItems: "center",
                justifyContent: "center",
                gap: "4px",
                width: "100%",
                height: "100%",
                border: "2px dashed var(--outline-variant)",
                borderRadius: (props.element.borderRadius ?? 4) + "px",
                cursor: "pointer",
              },
            },
            [
              h(
                "span",
                {
                  class: "material-symbols-outlined",
                  style: {
                    fontSize: "20px",
                    color: "var(--on-surface-variant)",
                  },
                },
                "cloud_upload",
              ),
              h(
                "span",
                {
                  style: {
                    fontSize: (props.element.fontSize || 11) + "px",
                    color: "var(--on-surface-variant)",
                  },
                },
                props.element.text || "点击上传",
              ),
            ],
          );
      },
    },
    button: {
      props: ["element", "isPreview"],
      setup(props: any) {
        return () =>
          h(
            "button",
            {
              style: {
                fontSize: (props.element.fontSize || 13) + "px",
                fontWeight: "600",
                color: props.element.fgColor || "#fff",
                background: props.element.bgColor || "#475569",
                border: "none",
                borderRadius: (props.element.borderRadius ?? 4) + "px",
                width: "100%",
                height: "100%",
                cursor: "pointer",
                display: "flex",
                alignItems: "center",
                justifyContent: "center",
                gap: "4px",
              },
            },
            [
              props.element.icon
                ? h(
                    "span",
                    {
                      class: "material-symbols-outlined",
                      style: { fontSize: "16px" },
                    },
                    props.element.icon,
                  )
                : null,
              props.element.text || "按钮",
            ],
          );
      },
    },
    primarybutton: {
      props: ["element", "isPreview"],
      setup(props: any) {
        return () =>
          h(
            "button",
            {
              style: {
                fontSize: (props.element.fontSize || 13) + "px",
                fontWeight: "700",
                color: props.element.fgColor || "#fff",
                background: props.element.bgColor || "#3b82f6",
                border: "none",
                borderRadius: (props.element.borderRadius ?? 4) + "px",
                width: "100%",
                height: "100%",
                cursor: "pointer",
                display: "flex",
                alignItems: "center",
                justifyContent: "center",
                gap: "4px",
                boxShadow: "0 2px 8px rgba(59,130,246,0.3)",
              },
            },
            [
              props.element.icon
                ? h(
                    "span",
                    {
                      class: "material-symbols-outlined",
                      style: { fontSize: "16px" },
                    },
                    props.element.icon,
                  )
                : null,
              props.element.text || "确认",
            ],
          );
      },
    },
    iconbutton: {
      props: ["element", "isPreview"],
      setup(props: any) {
        return () =>
          h(
            "button",
            {
              style: {
                color: props.element.fgColor || "#fff",
                background: props.element.bgColor || "#475569",
                border: "none",
                borderRadius: "50%",
                width: "100%",
                height: "100%",
                cursor: "pointer",
                display: "flex",
                alignItems: "center",
                justifyContent: "center",
              },
            },
            [
              h(
                "span",
                {
                  class: "material-symbols-outlined",
                  style: { fontSize: (props.element.iconSize || 20) + "px" },
                },
                props.element.icon || "settings",
              ),
            ],
          );
      },
    },
    linkbutton: {
      props: ["element", "isPreview"],
      setup(props: any) {
        return () =>
          h(
            "button",
            {
              style: {
                fontSize: (props.element.fontSize || 12) + "px",
                fontWeight: "600",
                color: props.element.fgColor || "#60a5fa",
                background: "none",
                border: "none",
                width: "100%",
                height: "100%",
                cursor: "pointer",
                textDecoration: "underline",
              },
            },
            props.element.text || "链接按钮",
          );
      },
    },
    splitbutton: {
      props: ["element", "isPreview"],
      setup(props: any) {
        return () =>
          h(
            "div",
            { style: { display: "flex", width: "100%", height: "100%" } },
            [
              h(
                "button",
                {
                  style: {
                    flex: "1",
                    fontSize: (props.element.fontSize || 13) + "px",
                    fontWeight: "600",
                    color: props.element.fgColor || "#fff",
                    background: props.element.bgColor || "#475569",
                    border: "none",
                    borderRadius:
                      (props.element.borderRadius ?? 4) +
                      "px 0 0 " +
                      (props.element.borderRadius ?? 4) +
                      "px",
                    cursor: "pointer",
                  },
                },
                props.element.text || "操作",
              ),
              h(
                "button",
                {
                  style: {
                    width: "28px",
                    color: props.element.fgColor || "#fff",
                    background: props.element.bgColor || "#334155",
                    border: "none",
                    borderRadius:
                      "0 " +
                      (props.element.borderRadius ?? 4) +
                      "px " +
                      (props.element.borderRadius ?? 4) +
                      "px 0",
                    cursor: "pointer",
                    display: "flex",
                    alignItems: "center",
                    justifyContent: "center",
                  },
                },
                [
                  h(
                    "span",
                    {
                      class: "material-symbols-outlined",
                      style: { fontSize: "16px" },
                    },
                    "expand_more",
                  ),
                ],
              ),
            ],
          );
      },
    },
    slider: {
      props: ["element", "isPreview"],
      setup(props: any) {
        return () => {
          const val = props.element.value ?? 50;
          const min = props.element.min ?? 0;
          const max = props.element.max ?? 100;
          const pct = ((val - min) / (max - min)) * 100;
          return h(
            "div",
            {
              style: {
                display: "flex",
                flexDirection: "column",
                width: "100%",
                height: "100%",
                justifyContent: "center",
                padding: "0 8px",
              },
            },
            [
              h(
                "div",
                {
                  style: {
                    position: "relative",
                    height: "6px",
                    borderRadius: "3px",
                    background: props.element.bgColor || "#1e293b",
                  },
                },
                [
                  h("div", {
                    style: {
                      position: "absolute",
                      height: "100%",
                      borderRadius: "3px",
                      background: props.element.fgColor || "#3b82f6",
                      width: pct + "%",
                    },
                  }),
                  h("div", {
                    style: {
                      position: "absolute",
                      width: "14px",
                      height: "14px",
                      borderRadius: "50%",
                      background: "#fff",
                      border:
                        "2px solid " + (props.element.fgColor || "#3b82f6"),
                      top: "-4px",
                      left: `calc(${pct}% - 7px)`,
                      boxShadow: "0 1px 3px rgba(0,0,0,0.3)",
                    },
                  }),
                ],
              ),
              h(
                "div",
                {
                  style: {
                    display: "flex",
                    justifyContent: "space-between",
                    marginTop: "4px",
                    fontSize: "10px",
                    color: "var(--on-surface-variant)",
                    fontFamily: "'JetBrains Mono', monospace",
                  },
                },
                [
                  h("span", {}, String(min)),
                  h(
                    "span",
                    {
                      style: {
                        color: props.element.fgColor || "#3b82f6",
                        fontWeight: "600",
                      },
                    },
                    val + (props.element.unit || ""),
                  ),
                  h("span", {}, String(max)),
                ],
              ),
            ],
          );
        };
      },
    },
    progress: {
      props: ["element", "isPreview"],
      setup(props: any) {
        return () => {
          const val = props.element.value ?? 65;
          const min = props.element.min ?? 0;
          const max = props.element.max ?? 100;
          const pct = Math.max(
            0,
            Math.min(100, ((val - min) / (max - min)) * 100),
          );
          return h(
            "div",
            {
              style: {
                width: "100%",
                height: "100%",
                display: "flex",
                alignItems: "center",
                padding: "0 4px",
              },
            },
            [
              h(
                "div",
                {
                  style: {
                    flex: "1",
                    height: "8px",
                    borderRadius: "4px",
                    background: props.element.bgColor || "#1e293b",
                    overflow: "hidden",
                  },
                },
                [
                  h("div", {
                    style: {
                      height: "100%",
                      borderRadius: "4px",
                      background: props.element.fgColor || "#3b82f6",
                      width: pct + "%",
                      transition: "width 0.3s",
                    },
                  }),
                ],
              ),
              h(
                "span",
                {
                  style: {
                    marginLeft: "6px",
                    fontSize: (props.element.fontSize || 11) + "px",
                    fontFamily: "'JetBrains Mono', monospace",
                    color: props.element.fgColor || "#3b82f6",
                    fontWeight: "600",
                    whiteSpace: "nowrap",
                  },
                },
                props.element.text || pct + "%",
              ),
            ],
          );
        };
      },
    },
    circleprogress: {
      props: ["element", "isPreview"],
      setup(props: any) {
        return () => {
          const val = props.element.value ?? 72;
          const min = props.element.min ?? 0;
          const max = props.element.max ?? 100;
          const pct = Math.max(
            0,
            Math.min(100, ((val - min) / (max - min)) * 100),
          );
          const r = 34;
          const circ = 2 * Math.PI * r;
          const offset = circ - (pct / 100) * circ;
          return h(
            "svg",
            { width: "100%", height: "100%", viewBox: "0 0 80 80" },
            [
              h("circle", {
                cx: "40",
                cy: "40",
                r: String(r),
                fill: "none",
                stroke: props.element.bgColor || "#1e293b",
                "stroke-width": "6",
              }),
              h("circle", {
                cx: "40",
                cy: "40",
                r: String(r),
                fill: "none",
                stroke: props.element.fgColor || "#22c55e",
                "stroke-width": "6",
                "stroke-linecap": "round",
                "stroke-dasharray": String(circ),
                "stroke-dashoffset": String(offset),
                transform: "rotate(-90 40 40)",
                style: { transition: "stroke-dashoffset 0.3s" },
              }),
              h(
                "text",
                {
                  x: "40",
                  y: "40",
                  "text-anchor": "middle",
                  "dominant-baseline": "central",
                  fill: props.element.fgColor || "#22c55e",
                  "font-size": (props.element.fontSize || 14) + "px",
                  "font-weight": "700",
                  "font-family": "'JetBrains Mono', monospace",
                },
                props.element.text || pct + "%",
              ),
            ],
          );
        };
      },
    },
    rate: {
      props: ["element", "isPreview"],
      setup(props: any) {
        return () => {
          const val = props.element.value ?? 3;
          const max = props.element.max ?? 5;
          return h(
            "div",
            {
              style: {
                display: "flex",
                alignItems: "center",
                gap: "2px",
                padding: "0 4px",
              },
            },
            Array.from({ length: max }, (_, i) =>
              h(
                "span",
                {
                  class: "material-symbols-outlined",
                  style: {
                    fontSize: (props.element.fontSize || 18) + "px",
                    color:
                      i < val ? props.element.fgColor || "#fbbf24" : "#475569",
                  },
                },
                i < val ? "star" : "star_outline",
              ),
            ),
          );
        };
      },
    },
    gauge: {
      props: ["element", "isPreview"],
      setup(props: any) {
        return () => {
          const val = props.element.value ?? 68;
          const min = props.element.min ?? 0;
          const max = props.element.max ?? 100;
          const pct = Math.max(0, Math.min(1, (val - min) / (max - min)));
          const angle = -135 + pct * 270;
          return h(
            "svg",
            { width: "100%", height: "100%", viewBox: "0 0 120 120" },
            [
              h("circle", {
                cx: "60",
                cy: "60",
                r: "48",
                fill: props.element.bgColor || "#1e293b",
                stroke: "#334155",
                "stroke-width": "3",
              }),
              h("path", {
                d: "M 24 84 A 40 40 0 1 1 96 84",
                fill: "none",
                stroke: "#334155",
                "stroke-width": "6",
                "stroke-linecap": "round",
              }),
              h("path", {
                d: "M 24 84 A 40 40 0 1 1 96 84",
                fill: "none",
                stroke: props.element.fgColor || "#22c55e",
                "stroke-width": "6",
                "stroke-linecap": "round",
                "stroke-dasharray": "188",
                "stroke-dashoffset": String(188 - pct * 188),
              }),
              h("line", {
                x1: "60",
                y1: "60",
                x2: String(60 + 30 * Math.cos((angle * Math.PI) / 180)),
                y2: String(60 + 30 * Math.sin((angle * Math.PI) / 180)),
                stroke: "#fff",
                "stroke-width": "2",
                "stroke-linecap": "round",
              }),
              h("circle", { cx: "60", cy: "60", r: "4", fill: "#fff" }),
              h(
                "text",
                {
                  x: "60",
                  y: "80",
                  "text-anchor": "middle",
                  fill: "#fff",
                  "font-size": (props.element.fontSize || 16) + "px",
                  "font-weight": "700",
                  "font-family": "'JetBrains Mono', monospace",
                },
                String(val),
              ),
              h(
                "text",
                {
                  x: "60",
                  y: "96",
                  "text-anchor": "middle",
                  fill: "#94a3b8",
                  "font-size": "10px",
                },
                props.element.unit || "",
              ),
            ],
          );
        };
      },
    },
    table: {
      props: ["element", "isPreview"],
      setup(props: any) {
        return () => {
          const lines = (
            props.element.text || "列A | 列B | 列C\n100 | 200 | 300"
          ).split("\n");
          return h(
            "div",
            {
              style: {
                width: "100%",
                height: "100%",
                overflow: "auto",
                fontSize: (props.element.fontSize || 11) + "px",
                fontFamily: "'JetBrains Mono', monospace",
              },
            },
            [
              h(
                "table",
                { style: { width: "100%", borderCollapse: "collapse" } },
                lines.map((line: string, i: number) =>
                  h(
                    "tr",
                    {
                      style: {
                        borderBottom: "1px solid var(--outline-variant)",
                      },
                    },
                    line.split("|").map((cell: string) =>
                      h(
                        i === 0 ? "th" : "td",
                        {
                          style: {
                            padding: "3px 6px",
                            textAlign: "left",
                            fontWeight: i === 0 ? "600" : "normal",
                            color:
                              i === 0
                                ? "var(--on-surface)"
                                : "var(--on-surface-variant)",
                            background:
                              i === 0
                                ? "var(--surface-container)"
                                : "transparent",
                          },
                        },
                        cell.trim(),
                      ),
                    ),
                  ),
                ),
              ),
            ],
          );
        };
      },
    },
    tree: {
      props: ["element", "isPreview"],
      setup(props: any) {
        return () => {
          const lines = (
            props.element.text || "设备列表\n  设备1\n  设备2"
          ).split("\n");
          return h(
            "div",
            {
              style: {
                width: "100%",
                height: "100%",
                padding: "6px",
                fontSize: (props.element.fontSize || 12) + "px",
                color: "var(--on-surface)",
                overflow: "auto",
              },
            },
            lines.map((line: string) => {
              const depth = line.length - line.trimStart().length;
              return h(
                "div",
                {
                  style: {
                    paddingLeft: depth * 12 + "px",
                    padding: "2px 0 2px " + (depth * 12 + 6) + "px",
                    display: "flex",
                    alignItems: "center",
                    gap: "4px",
                  },
                },
                [
                  depth > 0
                    ? h(
                        "span",
                        {
                          class: "material-symbols-outlined",
                          style: {
                            fontSize: "14px",
                            color: "var(--on-surface-variant)",
                          },
                        },
                        "chevron_right",
                      )
                    : null,
                  h(
                    "span",
                    {
                      class: "material-symbols-outlined",
                      style: {
                        fontSize: "14px",
                        color:
                          depth > 0
                            ? "var(--on-surface-variant)"
                            : "var(--secondary)",
                      },
                    },
                    depth > 0 ? "description" : "folder",
                  ),
                  h("span", {}, line.trim()),
                ],
              );
            }),
          );
        };
      },
    },
    list: {
      props: ["element", "isPreview"],
      setup(props: any) {
        return () => {
          const items = (props.element.text || "项目 1\n项目 2\n项目 3").split(
            "\n",
          );
          return h(
            "div",
            {
              style: { width: "100%", height: "100%", overflow: "auto" },
            },
            items.map((item: string, i: number) =>
              h(
                "div",
                {
                  style: {
                    padding: "6px 8px",
                    borderBottom: "1px solid var(--outline-variant)",
                    fontSize: (props.element.fontSize || 12) + "px",
                    color: "var(--on-surface)",
                    display: "flex",
                    alignItems: "center",
                    gap: "6px",
                  },
                },
                [
                  h(
                    "span",
                    {
                      class: "material-symbols-outlined",
                      style: {
                        fontSize: "14px",
                        color: "var(--on-surface-variant)",
                      },
                    },
                    "chevron_right",
                  ),
                  item.trim(),
                ],
              ),
            ),
          );
        };
      },
    },
    tabs: {
      props: ["element", "isPreview"],
      setup(props: any) {
        return () => {
          const tabs = (props.element.text || "标签1\n标签2\n标签3").split(
            "\n",
          );
          return h(
            "div",
            {
              style: {
                display: "flex",
                width: "100%",
                height: "100%",
                borderBottom: "2px solid var(--outline-variant)",
              },
            },
            tabs.map((tab: string, i: number) =>
              h(
                "div",
                {
                  style: {
                    flex: "1",
                    display: "flex",
                    alignItems: "center",
                    justifyContent: "center",
                    fontSize: (props.element.fontSize || 12) + "px",
                    fontWeight: i === 0 ? "600" : "normal",
                    color:
                      i === 0
                        ? props.element.fgColor || "#3b82f6"
                        : "var(--on-surface-variant)",
                    borderBottom:
                      i === 0
                        ? "2px solid " + (props.element.fgColor || "#3b82f6")
                        : "2px solid transparent",
                    marginBottom: "-2px",
                    cursor: "pointer",
                  },
                },
                tab.trim(),
              ),
            ),
          );
        };
      },
    },
    steps: {
      props: ["element", "isPreview"],
      setup(props: any) {
        return () => {
          const steps = (props.element.text || "步骤1\n步骤2\n步骤3").split(
            "\n",
          );
          const current = props.element.value ?? 2;
          const fg = props.element.fgColor || "#3b82f6";
          return h(
            "div",
            {
              style: {
                display: "flex",
                alignItems: "center",
                width: "100%",
                height: "100%",
                padding: "0 8px",
              },
            },
            steps.map((step: string, i: number) =>
              h(
                "div",
                {
                  style: {
                    flex: "1",
                    display: "flex",
                    alignItems: "center",
                    gap: "4px",
                  },
                },
                [
                  h(
                    "div",
                    {
                      style: {
                        width: "22px",
                        height: "22px",
                        borderRadius: "50%",
                        background:
                          i + 1 <= current
                            ? fg
                            : "var(--surface-container-highest)",
                        color:
                          i + 1 <= current
                            ? "#fff"
                            : "var(--on-surface-variant)",
                        display: "flex",
                        alignItems: "center",
                        justifyContent: "center",
                        fontSize: "11px",
                        fontWeight: "600",
                        border:
                          i + 1 <= current
                            ? "none"
                            : "1px solid var(--outline-variant)",
                        flexShrink: "0",
                      },
                    },
                    String(i + 1),
                  ),
                  h(
                    "span",
                    {
                      style: {
                        fontSize: (props.element.fontSize || 11) + "px",
                        color:
                          i + 1 <= current ? fg : "var(--on-surface-variant)",
                        fontWeight: i + 1 === current ? "600" : "normal",
                        whiteSpace: "nowrap",
                      },
                    },
                    step.trim(),
                  ),
                  i < steps.length - 1
                    ? h("div", {
                        style: {
                          flex: "1",
                          height: "1px",
                          background:
                            i + 1 < current ? fg : "var(--outline-variant)",
                          minWidth: "8px",
                        },
                      })
                    : null,
                ],
              ),
            ),
          );
        };
      },
    },
    timeline: {
      props: ["element", "isPreview"],
      setup(props: any) {
        return () => {
          const items = (
            props.element.text || "14:00 启动\n14:30 运行\n15:00 完成"
          ).split("\n");
          return h(
            "div",
            {
              style: {
                width: "100%",
                height: "100%",
                padding: "4px 8px",
                overflow: "auto",
              },
            },
            items.map((item: string, i: number) =>
              h(
                "div",
                {
                  style: {
                    display: "flex",
                    gap: "8px",
                    marginBottom: i < items.length - 1 ? "8px" : "0",
                  },
                },
                [
                  h(
                    "div",
                    {
                      style: {
                        display: "flex",
                        flexDirection: "column",
                        alignItems: "center",
                        flexShrink: "0",
                      },
                    },
                    [
                      h("div", {
                        style: {
                          width: "8px",
                          height: "8px",
                          borderRadius: "50%",
                          background: props.element.fgColor || "#3b82f6",
                        },
                      }),
                      i < items.length - 1
                        ? h("div", {
                            style: {
                              width: "2px",
                              flex: "1",
                              background: "var(--outline-variant)",
                              minHeight: "12px",
                            },
                          })
                        : null,
                    ],
                  ),
                  h(
                    "span",
                    {
                      style: {
                        fontSize: (props.element.fontSize || 11) + "px",
                        color: "var(--on-surface)",
                        whiteSpace: "nowrap",
                      },
                    },
                    item.trim(),
                  ),
                ],
              ),
            ),
          );
        };
      },
    },
    pagination: {
      props: ["element", "isPreview"],
      setup(props: any) {
        return () => {
          const cur = props.element.value ?? 1;
          const total = props.element.max ?? 10;
          return h(
            "div",
            {
              style: {
                display: "flex",
                alignItems: "center",
                justifyContent: "center",
                gap: "2px",
                width: "100%",
                height: "100%",
              },
            },
            [
              h(
                "button",
                {
                  style: {
                    width: "24px",
                    height: "24px",
                    border: "1px solid var(--outline-variant)",
                    borderRadius: "3px",
                    background: "none",
                    cursor: "pointer",
                    fontSize: "12px",
                    color: "var(--on-surface-variant)",
                  },
                },
                "<",
              ),
              ...Array.from({ length: Math.min(total, 7) }, (_, i) =>
                h(
                  "button",
                  {
                    style: {
                      width: "24px",
                      height: "24px",
                      border: "none",
                      borderRadius: "3px",
                      background:
                        i + 1 === cur
                          ? props.element.fgColor || "#3b82f6"
                          : "none",
                      color:
                        i + 1 === cur ? "#fff" : "var(--on-surface-variant)",
                      cursor: "pointer",
                      fontSize: "11px",
                      fontWeight: i + 1 === cur ? "600" : "normal",
                    },
                  },
                  String(i + 1),
                ),
              ),
              h(
                "button",
                {
                  style: {
                    width: "24px",
                    height: "24px",
                    border: "1px solid var(--outline-variant)",
                    borderRadius: "3px",
                    background: "none",
                    cursor: "pointer",
                    fontSize: "12px",
                    color: "var(--on-surface-variant)",
                  },
                },
                ">",
              ),
            ],
          );
        };
      },
    },
    linechart: {
      props: ["element", "isPreview"],
      setup(props: any) {
        return () => {
          const fg = props.element.fgColor || "#3b82f6";
          return h(
            "div",
            {
              style: {
                width: "100%",
                height: "100%",
                display: "flex",
                flexDirection: "column",
                background: props.element.bgColor || "#0f172a",
                borderRadius: (props.element.borderRadius ?? 4) + "px",
                padding: "8px",
              },
            },
            [
              h(
                "div",
                {
                  style: {
                    fontSize: (props.element.fontSize || 12) + "px",
                    color: "#e2e8f0",
                    marginBottom: "4px",
                  },
                },
                props.element.text || "折线图",
              ),
              h(
                "svg",
                {
                  width: "100%",
                  height: "100%",
                  viewBox: "0 0 200 80",
                  preserveAspectRatio: "none",
                },
                [
                  h("polyline", {
                    points:
                      "0,60 30,40 60,50 90,20 120,30 150,10 180,25 200,15",
                    fill: "none",
                    stroke: fg,
                    "stroke-width": "2",
                  }),
                  h("polyline", {
                    points:
                      "0,60 30,40 60,50 90,20 120,30 150,10 180,25 200,15 200,80 0,80",
                    fill: fg,
                    "fill-opacity": "0.1",
                    stroke: "none",
                  }),
                ],
              ),
            ],
          );
        };
      },
    },
    barchart: {
      props: ["element", "isPreview"],
      setup(props: any) {
        return () => {
          const fg = props.element.fgColor || "#8b5cf6";
          const bars = [40, 65, 50, 80, 55, 70, 45];
          return h(
            "div",
            {
              style: {
                width: "100%",
                height: "100%",
                display: "flex",
                flexDirection: "column",
                background: props.element.bgColor || "#0f172a",
                borderRadius: (props.element.borderRadius ?? 4) + "px",
                padding: "8px",
              },
            },
            [
              h(
                "div",
                {
                  style: {
                    fontSize: (props.element.fontSize || 12) + "px",
                    color: "#e2e8f0",
                    marginBottom: "4px",
                  },
                },
                props.element.text || "柱状图",
              ),
              h(
                "svg",
                {
                  width: "100%",
                  height: "100%",
                  viewBox: "0 0 200 80",
                  preserveAspectRatio: "none",
                },
                bars.map((v, i) =>
                  h("rect", {
                    x: String(i * 28 + 4),
                    y: String(80 - v),
                    width: "20",
                    height: String(v),
                    fill: fg,
                    rx: "2",
                    "fill-opacity": "0.8",
                  }),
                ),
              ),
            ],
          );
        };
      },
    },
    piechart: {
      props: ["element", "isPreview"],
      setup(props: any) {
        return () => {
          const fg = props.element.fgColor || "#f97316";
          return h(
            "div",
            {
              style: {
                width: "100%",
                height: "100%",
                display: "flex",
                flexDirection: "column",
                alignItems: "center",
                background: props.element.bgColor || "#0f172a",
                borderRadius: (props.element.borderRadius ?? 4) + "px",
                padding: "8px",
              },
            },
            [
              h(
                "div",
                {
                  style: {
                    fontSize: (props.element.fontSize || 12) + "px",
                    color: "#e2e8f0",
                    marginBottom: "4px",
                  },
                },
                props.element.text || "饼图",
              ),
              h(
                "svg",
                { width: "80%", height: "100%", viewBox: "0 0 100 100" },
                [
                  h("circle", {
                    cx: "50",
                    cy: "50",
                    r: "40",
                    fill: "none",
                    stroke: fg,
                    "stroke-width": "20",
                    "stroke-dasharray": "75 176",
                    transform: "rotate(-90 50 50)",
                  }),
                  h("circle", {
                    cx: "50",
                    cy: "50",
                    r: "40",
                    fill: "none",
                    stroke: "#8b5cf6",
                    "stroke-width": "20",
                    "stroke-dasharray": "100 151",
                    "stroke-dashoffset": "-75",
                    transform: "rotate(-90 50 50)",
                  }),
                  h("circle", {
                    cx: "50",
                    cy: "50",
                    r: "40",
                    fill: "none",
                    stroke: "#22c55e",
                    "stroke-width": "20",
                    "stroke-dasharray": "75 176",
                    "stroke-dashoffset": "-175",
                    transform: "rotate(-90 50 50)",
                  }),
                ],
              ),
            ],
          );
        };
      },
    },
    areachart: {
      props: ["element", "isPreview"],
      setup(props: any) {
        return () => {
          const fg = props.element.fgColor || "#22c55e";
          return h(
            "div",
            {
              style: {
                width: "100%",
                height: "100%",
                display: "flex",
                flexDirection: "column",
                background: props.element.bgColor || "#0f172a",
                borderRadius: (props.element.borderRadius ?? 4) + "px",
                padding: "8px",
              },
            },
            [
              h(
                "div",
                {
                  style: {
                    fontSize: (props.element.fontSize || 12) + "px",
                    color: "#e2e8f0",
                    marginBottom: "4px",
                  },
                },
                props.element.text || "面积图",
              ),
              h(
                "svg",
                {
                  width: "100%",
                  height: "100%",
                  viewBox: "0 0 200 80",
                  preserveAspectRatio: "none",
                },
                [
                  h("polygon", {
                    points:
                      "0,70 20,55 40,60 60,40 80,45 100,25 120,35 140,15 160,20 180,10 200,20 200,80 0,80",
                    fill: fg,
                    "fill-opacity": "0.3",
                  }),
                  h("polyline", {
                    points:
                      "0,70 20,55 40,60 60,40 80,45 100,25 120,35 140,15 160,20 180,10 200,20",
                    fill: "none",
                    stroke: fg,
                    "stroke-width": "2",
                  }),
                ],
              ),
            ],
          );
        };
      },
    },
    card: {
      props: ["element", "isPreview"],
      setup(props: any) {
        return () =>
          h(
            "div",
            {
              style: {
                width: "100%",
                height: "100%",
                background: props.element.bgColor || "#1e293b",
                border: "1px solid " + (props.element.borderColor || "#334155"),
                borderRadius: (props.element.borderRadius ?? 8) + "px",
                padding: "12px",
                display: "flex",
                flexDirection: "column",
              },
            },
            [
              h(
                "div",
                {
                  style: {
                    fontSize: (props.element.fontSize || 14) + "px",
                    fontWeight: "600",
                    color: "var(--on-surface)",
                    borderBottom: "1px solid var(--outline-variant)",
                    paddingBottom: "8px",
                    marginBottom: "8px",
                  },
                },
                props.element.text || "卡片标题",
              ),
              h(
                "div",
                {
                  style: {
                    flex: "1",
                    color: "var(--on-surface-variant)",
                    fontSize: "12px",
                  },
                },
                "内容区域",
              ),
            ],
          );
      },
    },
    divider: {
      props: ["element", "isPreview"],
      setup(props: any) {
        return () =>
          h(
            "div",
            {
              style: {
                width: "100%",
                height: "100%",
                display: "flex",
                alignItems: "center",
              },
            },
            [
              h("div", {
                style: {
                  width: "100%",
                  height: "1px",
                  background: props.element.bgColor || "#334155",
                },
              }),
            ],
          );
      },
    },
    alert: {
      props: ["element", "isPreview"],
      setup(props: any) {
        return () =>
          h(
            "div",
            {
              style: {
                display: "flex",
                alignItems: "center",
                gap: "8px",
                width: "100%",
                height: "100%",
                background: props.element.bgColor || "#1e3a5f",
                borderRadius: (props.element.borderRadius ?? 4) + "px",
                padding: "0 12px",
              },
            },
            [
              h(
                "span",
                {
                  class: "material-symbols-outlined",
                  style: {
                    fontSize: "18px",
                    color: props.element.fgColor || "#60a5fa",
                  },
                },
                props.element.icon || "info",
              ),
              h(
                "span",
                {
                  style: {
                    fontSize: (props.element.fontSize || 12) + "px",
                    color: props.element.fgColor || "#60a5fa",
                  },
                },
                props.element.text || "提示信息",
              ),
            ],
          );
      },
    },
    badge: {
      props: ["element", "isPreview"],
      setup(props: any) {
        return () =>
          h(
            "div",
            {
              style: {
                width: "100%",
                height: "100%",
                position: "relative",
                display: "flex",
                alignItems: "center",
                justifyContent: "center",
              },
            },
            [
              h(
                "span",
                {
                  class: "material-symbols-outlined",
                  style: {
                    fontSize: (props.element.iconSize || 24) + "px",
                    color: "var(--on-surface-variant)",
                  },
                },
                props.element.icon || "notifications",
              ),
              props.element.text
                ? h(
                    "span",
                    {
                      style: {
                        position: "absolute",
                        top: "2px",
                        right: "2px",
                        minWidth: "16px",
                        height: "16px",
                        background: props.element.bgColor || "#ef4444",
                        color: props.element.fgColor || "#fff",
                        fontSize: (props.element.fontSize || 10) + "px",
                        fontWeight: "600",
                        borderRadius: "8px",
                        display: "flex",
                        alignItems: "center",
                        justifyContent: "center",
                        padding: "0 4px",
                      },
                    },
                    props.element.text,
                  )
                : null,
            ],
          );
      },
    },
    avatar: {
      props: ["element", "isPreview"],
      setup(props: any) {
        return () =>
          h(
            "div",
            {
              style: {
                width: "100%",
                height: "100%",
                borderRadius: "50%",
                background: props.element.bgColor || "#6366f1",
                color: props.element.fgColor || "#fff",
                display: "flex",
                alignItems: "center",
                justifyContent: "center",
                fontSize: (props.element.fontSize || 18) + "px",
                fontWeight: "600",
              },
            },
            props.element.text || "A",
          );
      },
    },
    breadcrumb: {
      props: ["element", "isPreview"],
      setup(props: any) {
        return () => {
          const parts = (props.element.text || "首页 / 设备管理").split("/");
          return h(
            "div",
            {
              style: {
                display: "flex",
                alignItems: "center",
                gap: "2px",
                fontSize: (props.element.fontSize || 12) + "px",
                color: "var(--on-surface-variant)",
                width: "100%",
                height: "100%",
                overflow: "hidden",
                whiteSpace: "nowrap",
              },
            },
            parts.map((p: string, i: number) =>
              h(
                "span",
                {
                  style: {
                    color:
                      i === parts.length - 1
                        ? "var(--primary)"
                        : "var(--on-surface-variant)",
                  },
                },
                [
                  i > 0
                    ? h(
                        "span",
                        { style: { margin: "0 4px", color: "var(--outline)" } },
                        "/",
                      )
                    : null,
                  p.trim(),
                ],
              ),
            ),
          );
        };
      },
    },
    message: {
      props: ["element", "isPreview"],
      setup(props: any) {
        return () =>
          h(
            "div",
            {
              style: {
                display: "flex",
                alignItems: "center",
                gap: "8px",
                width: "100%",
                height: "100%",
                background: props.element.bgColor || "#166534",
                borderRadius: (props.element.borderRadius ?? 4) + "px",
                padding: "0 12px",
              },
            },
            [
              h(
                "span",
                {
                  class: "material-symbols-outlined",
                  style: {
                    fontSize: "16px",
                    color: props.element.fgColor || "#4ade80",
                  },
                },
                props.element.icon || "check_circle",
              ),
              h(
                "span",
                {
                  style: {
                    fontSize: (props.element.fontSize || 12) + "px",
                    color: props.element.fgColor || "#4ade80",
                  },
                },
                props.element.text || "操作成功",
              ),
            ],
          );
      },
    },
    menu: {
      props: ["element", "isPreview"],
      setup(props: any) {
        return () => {
          const items = (
            props.element.text || "菜单项1\n菜单项2\n菜单项3"
          ).split("\n");
          return h(
            "div",
            {
              style: {
                width: "100%",
                height: "100%",
                background: props.element.bgColor || "#1e293b",
                borderRadius: (props.element.borderRadius ?? 4) + "px",
                padding: "4px 0",
                overflow: "auto",
              },
            },
            items.map((item: string, i: number) =>
              h(
                "div",
                {
                  style: {
                    padding: "6px 12px",
                    fontSize: (props.element.fontSize || 12) + "px",
                    color:
                      i === 0
                        ? props.element.fgColor || "#60a5fa"
                        : "var(--on-surface)",
                    background:
                      i === 0 ? "rgba(96,165,250,0.1)" : "transparent",
                    cursor: "pointer",
                  },
                },
                item.trim(),
              ),
            ),
          );
        };
      },
    },
    scrollbar: {
      props: ["element", "isPreview"],
      setup(props: any) {
        return () =>
          h(
            "div",
            {
              style: {
                width: "100%",
                height: "100%",
                background: props.element.bgColor || "#334155",
                borderRadius: "4px",
                position: "relative",
              },
            },
            [
              h("div", {
                style: {
                  width: "100%",
                  height: "40%",
                  background: props.element.fgColor || "#64748b",
                  borderRadius: "4px",
                  position: "absolute",
                  top: "10%",
                },
              }),
            ],
          );
      },
    },
    // 工业设备默认渲染
    tank: {
      props: ["element", "isPreview"],
      setup(props: any) {
        return () =>
          h(
            "div",
            {
              style: {
                width: "100%",
                height: "100%",
                display: "flex",
                flexDirection: "column",
                alignItems: "center",
                justifyContent: "flex-end",
              },
            },
            [
              h(
                "span",
                {
                  class: "material-symbols-outlined",
                  style: {
                    fontSize: "32px",
                    color: props.element.fgColor || "#60a5fa",
                  },
                },
                "propane_tank",
              ),
              props.element.unit
                ? h(
                    "span",
                    {
                      style: {
                        fontSize: "10px",
                        color: "var(--on-surface-variant)",
                      },
                    },
                    props.element.value + (props.element.unit || ""),
                  )
                : null,
            ],
          );
      },
    },
    pump: {
      props: ["element", "isPreview"],
      setup(props: any) {
        return () =>
          h(
            "div",
            {
              style: {
                width: "100%",
                height: "100%",
                display: "flex",
                alignItems: "center",
                justifyContent: "center",
              },
            },
            [
              h(
                "span",
                {
                  class: "material-symbols-outlined",
                  style: {
                    fontSize: "32px",
                    color: props.element.fgColor || "#22c55e",
                  },
                },
                "cyclone",
              ),
            ],
          );
      },
    },
    valve: {
      props: ["element", "isPreview"],
      setup(props: any) {
        return () =>
          h(
            "div",
            {
              style: {
                width: "100%",
                height: "100%",
                display: "flex",
                alignItems: "center",
                justifyContent: "center",
              },
            },
            [
              h(
                "span",
                {
                  class: "material-symbols-outlined",
                  style: {
                    fontSize: "28px",
                    color: props.element.fgColor || "#f97316",
                  },
                },
                "valve",
              ),
            ],
          );
      },
    },
    pipe: {
      props: ["element", "isPreview"],
      setup(props: any) {
        return () =>
          h(
            "div",
            {
              style: {
                width: "100%",
                height: "100%",
                display: "flex",
                alignItems: "center",
                justifyContent: "center",
              },
            },
            [
              h("div", {
                style: {
                  width: "100%",
                  height: "6px",
                  background: props.element.fgColor || "#64748b",
                  borderRadius: "3px",
                },
              }),
            ],
          );
      },
    },
    conveyor: {
      props: ["element", "isPreview"],
      setup(props: any) {
        return () =>
          h(
            "div",
            {
              style: {
                width: "100%",
                height: "100%",
                display: "flex",
                alignItems: "center",
                justifyContent: "center",
              },
            },
            [
              h(
                "span",
                {
                  class: "material-symbols-outlined",
                  style: {
                    fontSize: "24px",
                    color: props.element.fgColor || "#a78bfa",
                  },
                },
                "moving",
              ),
            ],
          );
      },
    },
    agv: {
      props: ["element", "isPreview"],
      setup(props: any) {
        return () =>
          h(
            "div",
            {
              style: {
                width: "100%",
                height: "100%",
                display: "flex",
                alignItems: "center",
                justifyContent: "center",
              },
            },
            [
              h(
                "span",
                {
                  class: "material-symbols-outlined",
                  style: {
                    fontSize: "28px",
                    color: props.element.fgColor || "#fbbf24",
                  },
                },
                "local_shipping",
              ),
            ],
          );
      },
    },
    tower: {
      props: ["element", "isPreview"],
      setup(props: any) {
        return () =>
          h(
            "div",
            {
              style: {
                width: "100%",
                height: "100%",
                display: "flex",
                alignItems: "center",
                justifyContent: "center",
              },
            },
            [
              h(
                "span",
                {
                  class: "material-symbols-outlined",
                  style: {
                    fontSize: "36px",
                    color: props.element.fgColor || "#ef4444",
                  },
                },
                "cell_tower",
              ),
            ],
          );
      },
    },
    solar: {
      props: ["element", "isPreview"],
      setup(props: any) {
        return () =>
          h(
            "div",
            {
              style: {
                width: "100%",
                height: "100%",
                display: "flex",
                alignItems: "center",
                justifyContent: "center",
              },
            },
            [
              h(
                "span",
                {
                  class: "material-symbols-outlined",
                  style: {
                    fontSize: "32px",
                    color: props.element.fgColor || "#fbbf24",
                  },
                },
                "solar_power",
              ),
            ],
          );
      },
    },
  };

  return (
    rendererMap[type] || {
      props: ["element", "isPreview"],
      setup(props: any) {
        return () =>
          h(
            "div",
            {
              style: {
                display: "flex",
                alignItems: "center",
                justifyContent: "center",
                width: "100%",
                height: "100%",
              },
            },
            [
              h(
                "span",
                {
                  class: "material-symbols-outlined",
                  style: {
                    fontSize: "24px",
                    color: "var(--on-surface-variant)",
                  },
                },
                props.element.icon || "widgets",
              ),
            ],
          );
      },
    }
  );
}
</script>

<style scoped>
.hmi-designer {
  width: 100%;
  height: 100%;
  display: flex;
  overflow: hidden;
}

/* ===== 左侧工具箱 ===== */
.hmi-toolbox {
  width: 240px;
  flex-shrink: 0;
  background: var(--surface-container-low);
  border-right: 1px solid var(--outline-variant);
  display: flex;
  flex-direction: column;
}

/* --- Tab 切换 --- */
.toolbox-tabs {
  display: flex;
  border-bottom: 1px solid var(--outline-variant);
}
.tab-btn {
  flex: 1;
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 4px;
  padding: 6px 0;
  background: none;
  border: none;
  border-bottom: 2px solid transparent;
  color: var(--on-surface-variant);
  font-size: 11px;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.12s;
}
.tab-btn:hover {
  color: var(--on-surface);
  background: var(--surface-variant);
}
.tab-btn.active {
  color: var(--primary);
  border-bottom-color: var(--primary);
}

.toolbox-header {
  padding: 8px;
  border-bottom: 1px solid var(--outline-variant);
}
.assets-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
}
.assets-title {
  font-size: 11px;
  font-weight: 700;
  color: var(--on-surface);
}
.toolbox-search {
  width: 100%;
  padding: 5px 8px;
  background: var(--surface-container-highest);
  border: 1px solid var(--outline-variant);
  border-radius: var(--radius);
  color: var(--on-surface);
  font-size: 11px;
  outline: none;
}
.toolbox-search:focus {
  border-color: var(--primary);
}
.toolbox-content {
  flex: 1;
  overflow-y: auto;
  padding: 6px;
  display: flex;
  flex-direction: column;
  gap: 12px;
}
.toolbox-section {
  display: flex;
  flex-direction: column;
  gap: 4px;
}

/* --- 可折叠分类头 --- */
.section-header-btn {
  display: flex;
  align-items: center;
  gap: 2px;
  width: 100%;
  padding: 4px 4px;
  background: none;
  border: none;
  cursor: pointer;
  border-radius: var(--radius);
  transition: background 0.1s;
}
.section-header-btn:hover {
  background: var(--surface-variant);
}
.toggle-arrow {
  font-size: 14px;
  color: var(--on-surface-variant);
  transition: transform 0.15s;
}
.toggle-arrow.collapsed {
  transform: rotate(-90deg);
}
.section-label {
  font-size: 9px;
  font-weight: 700;
  letter-spacing: 0.06em;
  text-transform: uppercase;
  color: var(--on-surface-variant);
  flex: 1;
  text-align: left;
}
.section-count {
  font-size: 9px;
  color: var(--outline);
  background: var(--surface-container-highest);
  padding: 0 4px;
  border-radius: 8px;
}
.section-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
}
.section-action {
  font-size: 14px;
  color: var(--on-surface-variant);
  cursor: pointer;
}
.controls-grid {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 3px;
}
.control-btn {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  padding: 6px 4px;
  background: var(--surface-variant);
  border: 1px solid transparent;
  border-radius: var(--radius);
  cursor: grab;
  gap: 3px;
  transition: all 0.12s;
  color: var(--on-surface);
}
.control-btn:hover {
  background: var(--surface-bright);
  border-color: var(--primary);
}
.control-btn:active {
  cursor: grabbing;
}
.ctrl-icon {
  font-size: 16px;
  color: var(--secondary);
}
.ctrl-label {
  font-size: 8px;
  font-weight: 700;
  letter-spacing: 0.03em;
  color: var(--on-surface);
  text-align: center;
  line-height: 1.2;
}
.library-item {
  aspect-ratio: 1;
  background: var(--surface-container-highest);
  border: 1px solid var(--outline-variant);
  border-radius: var(--radius);
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: grab;
  overflow: hidden;
  transition: background 0.12s;
}
.library-item:hover {
  background: var(--surface-bright);
  border-color: var(--primary);
}
.library-item:active {
  cursor: grabbing;
}
.lib-icon {
  font-size: 24px;
  color: var(--on-surface-variant);
}
.lib-label {
  position: absolute;
  bottom: 2px;
  left: 2px;
  font-size: 7px;
  font-weight: 700;
  opacity: 0;
  transition: opacity 0.12s;
  color: var(--on-surface);
}
.library-item:hover .lib-label {
  opacity: 1;
}

/* --- 资源浏览：窗体列表 --- */
.form-list {
  display: flex;
  flex-direction: column;
  gap: 2px;
}
.form-item {
  display: flex;
  align-items: center;
  gap: 6px;
  padding: 6px 8px;
  border-radius: var(--radius);
  cursor: pointer;
  transition: background 0.1s;
}
.form-item:hover {
  background: var(--surface-variant);
}
.form-item.active {
  background: var(--primary-container);
}
.form-icon {
  font-size: 16px;
  color: var(--on-surface-variant);
}
.form-item.active .form-icon {
  color: var(--primary);
}
.form-info {
  flex: 1;
  min-width: 0;
}
.form-name {
  font-size: 11px;
  font-weight: 600;
  color: var(--on-surface);
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}
.form-size {
  font-size: 9px;
  color: var(--outline);
  font-family: "JetBrains Mono", monospace;
}
.form-actions {
  display: flex;
  gap: 2px;
  opacity: 0;
  transition: opacity 0.12s;
}
.form-item:hover .form-actions {
  opacity: 1;
}

/* --- 资源浏览：控件树 --- */
.controls-tree {
  margin-top: 4px;
  border-top: 1px solid var(--outline-variant);
  padding-top: 6px;
}
.tree-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 2px 4px;
}
.tree-title {
  font-size: 9px;
  font-weight: 700;
  letter-spacing: 0.06em;
  text-transform: uppercase;
  color: var(--on-surface-variant);
}
.tree-count {
  font-size: 9px;
  color: var(--outline);
  background: var(--surface-container-highest);
  padding: 0 4px;
  border-radius: 8px;
}
.tree-item {
  display: flex;
  align-items: center;
  gap: 4px;
  padding: 3px 6px;
  border-radius: var(--radius);
  cursor: pointer;
  transition: background 0.1s;
}
.tree-item:hover {
  background: var(--surface-variant);
}
.tree-item.active {
  background: var(--primary-container);
}
.tree-icon {
  font-size: 13px;
}
.tree-label {
  flex: 1;
  font-size: 10px;
  color: var(--on-surface);
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}
.tree-item.active .tree-label {
  font-weight: 600;
}
.tree-del {
  opacity: 0;
  transition: opacity 0.1s;
}
.tree-item:hover .tree-del {
  opacity: 1;
}
.empty-hint {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 4px;
  padding: 20px 8px;
  color: var(--outline);
  font-size: 10px;
  text-align: center;
}

/* --- 通用小按钮 --- */
.icon-btn-sm {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 22px;
  height: 22px;
  border: none;
  border-radius: var(--radius);
  background: var(--primary-container);
  color: var(--primary);
  cursor: pointer;
  transition: all 0.1s;
}
.icon-btn-sm:hover {
  background: var(--primary);
  color: var(--on-primary);
}
.icon-btn-xs {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 18px;
  height: 18px;
  border: none;
  border-radius: 2px;
  background: none;
  color: var(--on-surface-variant);
  cursor: pointer;
  transition: all 0.1s;
}
.icon-btn-xs:hover {
  background: var(--surface-container-highest);
  color: var(--on-surface);
}

/* ===== 画布区 ===== */
.hmi-canvas-area {
  flex: 1;
  display: flex;
  flex-direction: column;
  overflow: hidden;
  min-width: 0;
}
.canvas-toolbar {
  display: flex;
  align-items: center;
  justify-content: space-between;
  height: 36px;
  padding: 0 8px;
  background: var(--surface-container-high);
  border-bottom: 1px solid var(--outline-variant);
  flex-shrink: 0;
}
.toolbar-left,
.toolbar-right {
  display: flex;
  align-items: center;
  gap: 6px;
}
.view-toggle {
  display: flex;
  background: var(--surface-container-low);
  border-radius: 2px;
  padding: 2px;
  gap: 2px;
}
.toggle-btn {
  padding: 2px 8px;
  font-size: 10px;
  border: none;
  border-radius: 2px;
  cursor: pointer;
  background: transparent;
  color: var(--on-surface-variant);
  transition: all 0.12s;
}
.toggle-btn:hover {
  color: var(--on-surface);
}
.toggle-btn.active {
  background: var(--surface-container-highest);
  color: var(--primary);
  font-weight: 600;
}
.toolbar-divider {
  width: 1px;
  height: 14px;
  background: var(--outline-variant);
}
.zoom-controls {
  display: flex;
  align-items: center;
  gap: 2px;
}
.icon-btn {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 26px;
  height: 26px;
  background: none;
  border: none;
  border-radius: var(--radius);
  color: var(--on-surface-variant);
  cursor: pointer;
  transition: background 0.12s;
}
.icon-btn:hover {
  background: var(--surface-variant);
  color: var(--on-surface);
}
.icon-btn.active {
  background: var(--primary-container);
  color: var(--on-primary-container);
}
.zoom-value {
  font-size: 10px;
  font-family: "JetBrains Mono", monospace;
  color: var(--on-surface-variant);
  min-width: 28px;
  text-align: center;
}
.canvas-size-label {
  font-size: 9px;
  font-family: "JetBrains Mono", monospace;
  color: var(--outline);
  padding: 2px 6px;
  background: var(--surface-container-low);
  border-radius: 2px;
}
.save-btn {
  display: flex;
  align-items: center;
  gap: 4px;
  padding: 3px 10px;
  background: none;
  border: 1px solid var(--secondary);
  border-radius: var(--radius);
  color: var(--secondary);
  font-size: 10px;
  font-weight: 700;
  letter-spacing: 0.04em;
  cursor: pointer;
  transition: background 0.12s;
}
.save-btn:hover {
  background: rgba(78, 222, 163, 0.1);
}

/* 画布 */
.canvas-container {
  flex: 1;
  overflow: auto;
  background: var(--surface-dim);
  display: flex;
  justify-content: center;
  padding: 24px;
}
.canvas-grid {
  background-image:
    linear-gradient(to right, var(--outline-variant) 1px, transparent 1px),
    linear-gradient(to bottom, var(--outline-variant) 1px, transparent 1px);
  background-size: 20px 20px;
}
.canvas {
  background: var(--surface-container-lowest);
  border: 1px solid var(--outline);
  box-shadow: 0 4px 24px rgba(0, 0, 0, 0.3);
  position: relative;
  transform-origin: center top;
}
.preview-mode {
  outline: 2px solid var(--secondary);
}
.canvas-element {
  position: absolute;
  border: 1px solid transparent;
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: move;
  transition: border-color 0.12s;
  overflow: hidden;
}
.canvas-element:hover {
  border-color: rgba(99, 102, 241, 0.4);
}
.canvas-element.selected {
  border-color: var(--primary);
  box-shadow: 0 0 0 1px var(--primary);
}

/* 缩放手柄 */
.resize-handle {
  position: absolute;
  width: 8px;
  height: 8px;
  background: var(--primary);
  border: 1px solid var(--on-primary);
  box-sizing: border-box;
  z-index: 10;
}
.resize-handle.nw {
  top: -4px;
  left: -4px;
  cursor: nw-resize;
}
.resize-handle.ne {
  top: -4px;
  right: -4px;
  cursor: ne-resize;
}
.resize-handle.sw {
  bottom: -4px;
  left: -4px;
  cursor: sw-resize;
}
.resize-handle.se {
  bottom: -4px;
  right: -4px;
  cursor: se-resize;
}
.resize-handle.n {
  top: -4px;
  left: 50%;
  margin-left: -4px;
  cursor: n-resize;
}
.resize-handle.s {
  bottom: -4px;
  left: 50%;
  margin-left: -4px;
  cursor: s-resize;
}
.resize-handle.e {
  right: -4px;
  top: 50%;
  margin-top: -4px;
  cursor: e-resize;
}
.resize-handle.w {
  left: -4px;
  top: 50%;
  margin-top: -4px;
  cursor: w-resize;
}

/* ===== 右侧属性面板 ===== */
.hmi-inspector {
  width: 260px;
  flex-shrink: 0;
  background: var(--surface-container);
  border-left: 1px solid var(--outline-variant);
  display: flex;
  flex-direction: column;
  position: relative;
  transition: width 0.2s;
}
.hmi-inspector.collapsed {
  width: 28px;
}
.inspector-toggle-btn {
  position: absolute;
  left: 0;
  top: 50%;
  transform: translate(-50%, -50%);
  z-index: 20;
  width: 20px;
  height: 32px;
  display: flex;
  align-items: center;
  justify-content: center;
  background: var(--surface-container-highest);
  border: 1px solid var(--outline-variant);
  border-radius: 0 var(--radius) var(--radius) 0;
  cursor: pointer;
  color: var(--on-surface-variant);
  transition: all 0.12s;
}
.inspector-toggle-btn:hover {
  background: var(--primary);
  color: var(--on-primary);
  border-color: var(--primary);
}
.inspector-header {
  padding: 8px 10px;
  border-bottom: 1px solid var(--outline-variant);
}
.inspector-title {
  font-size: 13px;
  font-weight: 700;
  color: var(--on-surface);
  margin: 0;
}
.inspector-selection {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 8px 10px;
  border-bottom: 1px solid var(--outline-variant);
}
.selection-icon {
  font-size: 14px;
  color: var(--primary);
}
.selection-name {
  font-size: 12px;
  font-weight: 500;
  color: var(--on-surface);
}
.selection-type {
  font-size: 9px;
  color: var(--outline);
}
.inspector-body {
  flex: 1;
  overflow-y: auto;
}
.inspector-section {
  border-bottom: 1px solid var(--outline-variant);
}
.section-toggle {
  width: 100%;
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 7px 10px;
  background: none;
  border: none;
  color: var(--on-surface);
  font-size: 11px;
  font-weight: 600;
  cursor: pointer;
}
.binding-toggle {
  color: var(--secondary);
}
.toggle-icon {
  font-size: 16px;
}
.section-body {
  padding: 0 10px 8px;
  display: flex;
  flex-direction: column;
  gap: 5px;
}
.prop-field {
  display: flex;
  flex-direction: column;
  gap: 2px;
}
.prop-label {
  font-size: 10px;
  color: var(--on-surface-variant);
}
.prop-input {
  width: 100%;
  padding: 2px 5px;
  height: 24px;
  background: var(--surface-container-highest);
  border: none;
  border-radius: var(--radius);
  color: var(--on-surface);
  font-size: 12px;
  outline: none;
  transition: box-shadow 0.12s;
}
.prop-input:focus {
  box-shadow: 0 0 0 1px var(--primary);
}
.prop-input.code-font {
  font-family: "JetBrains Mono", monospace;
  font-size: 10px;
}
.prop-checkbox {
  accent-color: var(--primary);
  width: 16px;
  height: 16px;
}
.prop-row {
  display: flex;
  gap: 6px;
  align-items: center;
}
.prop-field.half {
  flex: 1;
  min-width: 0;
}
.prop-sep {
  color: var(--outline);
  font-size: 10px;
}
textarea.prop-input {
  resize: vertical;
  min-height: 50px;
}
select.prop-input {
  cursor: pointer;
}
.color-field {
  display: flex;
  align-items: center;
  gap: 4px;
}
.color-picker {
  width: 20px;
  height: 20px;
  border: 1px solid var(--outline-variant);
  border-radius: 2px;
  cursor: pointer;
  padding: 0;
}
.color-field .prop-input {
  flex: 1;
}
.binding-field {
  color: var(--secondary);
}
.binding-field::placeholder {
  color: var(--outline);
}
.binding-input {
  position: relative;
  display: flex;
  align-items: center;
}
.binding-list-icon {
  position: absolute;
  right: 4px;
  font-size: 14px;
  color: var(--secondary);
  cursor: pointer;
}
.hmi-slider {
  width: 100%;
  -webkit-appearance: none;
  height: 4px;
  background: var(--surface-container);
  border-radius: 2px;
  outline: none;
}
.hmi-slider::-webkit-slider-thumb {
  -webkit-appearance: none;
  width: 12px;
  height: 12px;
  background: var(--primary);
  border-radius: 0;
  cursor: pointer;
}
.inspector-actions {
  padding: 8px 10px;
  border-top: 1px solid var(--outline-variant);
}
.action-btn {
  width: 100%;
  padding: 6px;
  font-size: 10px;
  font-weight: 700;
  letter-spacing: 0.04em;
  text-transform: uppercase;
  border-radius: var(--radius);
  cursor: pointer;
  border: none;
}
.action-btn.danger {
  background: var(--error-container);
  color: var(--on-error-container);
}
.action-btn.danger:hover {
  opacity: 0.9;
}
.inspector-empty {
  flex: 1;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 6px;
  color: var(--outline);
}
.empty-icon {
  font-size: 28px;
}
.inspector-empty p {
  font-size: 11px;
  margin: 0;
}
.hint-sub {
  font-size: 10px !important;
  opacity: 0.6;
}

/* 滚动条 */
.custom-scrollbar::-webkit-scrollbar {
  width: 4px;
  height: 4px;
}
.custom-scrollbar::-webkit-scrollbar-track {
  background: transparent;
}
.custom-scrollbar::-webkit-scrollbar-thumb {
  background: #334155;
  border-radius: 2px;
}
</style>
