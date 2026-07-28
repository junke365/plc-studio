<template>
  <div class="library-panel">
    <div class="panel-header">
      <span class="panel-title">库管理器</span>
      <span class="lib-count">{{ totalCount }} 项</span>
    </div>
    <div class="search-area">
      <div class="search-input">
        <span class="material-symbols-outlined">search</span>
        <input v-model="searchText" placeholder="搜索库函数..." />
      </div>
    </div>
    <div class="library-scroll custom-scrollbar">
      <!-- PLCopen Part 1: 基础功能 -->
      <div class="lib-section" v-show="showSection('part1')">
        <div class="section-title-bar" @click="toggleSection('part1')">
          <span
            class="material-symbols-outlined section-arrow"
            :class="{ collapsed: !sectionExpanded.part1 }"
            >expand_more</span
          >
          <span class="lib-section-title">PLCopen Part 1 — 基础功能</span>
        </div>
        <div v-show="sectionExpanded.part1">
          <div class="sub-section" v-show="showSubSection('part1', 'typeConv')">
            <div class="sub-title">类型转换</div>
            <div class="lib-grid">
              <div
                v-for="item in filterItems(typeConversion)"
                :key="item.name"
                class="lib-item"
                draggable="true"
                @dragstart="handleDragStart($event, item)"
              >
                <span class="material-symbols-outlined lib-icon">{{
                  item.icon
                }}</span>
                <span class="lib-name">{{ item.name }}</span>
              </div>
            </div>
          </div>

          <div class="sub-section" v-show="showSubSection('part1', 'math')">
            <div class="sub-title">数学函数</div>
            <div class="lib-grid">
              <div
                v-for="item in filterItems(mathFunctions)"
                :key="item.name"
                class="lib-item"
                draggable="true"
                @dragstart="handleDragStart($event, item)"
              >
                <span class="material-symbols-outlined lib-icon">{{
                  item.icon
                }}</span>
                <span class="lib-name">{{ item.name }}</span>
              </div>
            </div>
          </div>

          <div
            class="sub-section"
            v-show="showSubSection('part1', 'arithmetic')"
          >
            <div class="sub-title">算术运算</div>
            <div class="lib-grid">
              <div
                v-for="item in filterItems(arithmeticOps)"
                :key="item.name"
                class="lib-item"
                draggable="true"
                @dragstart="handleDragStart($event, item)"
              >
                <span class="material-symbols-outlined lib-icon">{{
                  item.icon
                }}</span>
                <span class="lib-name">{{ item.name }}</span>
              </div>
            </div>
          </div>

          <div
            class="sub-section"
            v-show="showSubSection('part1', 'comparison')"
          >
            <div class="sub-title">比较运算</div>
            <div class="lib-grid">
              <div
                v-for="item in filterItems(comparisonOps)"
                :key="item.name"
                class="lib-item"
                draggable="true"
                @dragstart="handleDragStart($event, item)"
              >
                <span class="material-symbols-outlined lib-icon">{{
                  item.icon
                }}</span>
                <span class="lib-name">{{ item.name }}</span>
              </div>
            </div>
          </div>

          <div
            class="sub-section"
            v-show="showSubSection('part1', 'selection')"
          >
            <div class="sub-title">选择运算</div>
            <div class="lib-grid">
              <div
                v-for="item in filterItems(selectionOps)"
                :key="item.name"
                class="lib-item"
                draggable="true"
                @dragstart="handleDragStart($event, item)"
              >
                <span class="material-symbols-outlined lib-icon">{{
                  item.icon
                }}</span>
                <span class="lib-name">{{ item.name }}</span>
              </div>
            </div>
          </div>

          <div class="sub-section" v-show="showSubSection('part1', 'bitwise')">
            <div class="sub-title">位运算</div>
            <div class="lib-grid">
              <div
                v-for="item in filterItems(bitwiseOps)"
                :key="item.name"
                class="lib-item"
                draggable="true"
                @dragstart="handleDragStart($event, item)"
              >
                <span class="material-symbols-outlined lib-icon">{{
                  item.icon
                }}</span>
                <span class="lib-name">{{ item.name }}</span>
              </div>
            </div>
          </div>
        </div>
      </div>

      <!-- PLCopen Part 2: 功能块 -->
      <div class="lib-section" v-show="showSection('part2')">
        <div class="section-title-bar" @click="toggleSection('part2')">
          <span
            class="material-symbols-outlined section-arrow"
            :class="{ collapsed: !sectionExpanded.part2 }"
            >expand_more</span
          >
          <span class="lib-section-title">PLCopen Part 2 — 功能块</span>
        </div>
        <div v-show="sectionExpanded.part2">
          <div class="sub-section" v-show="showSubSection('part2', 'timer')">
            <div class="sub-title">定时器</div>
            <div class="lib-grid">
              <div
                v-for="item in filterItems(timerFBs)"
                :key="item.name"
                class="lib-item"
                draggable="true"
                @dragstart="handleDragStart($event, item)"
              >
                <span class="material-symbols-outlined lib-icon">{{
                  item.icon
                }}</span>
                <span class="lib-name">{{ item.name }}</span>
                <span class="lib-desc">{{ item.desc }}</span>
              </div>
            </div>
          </div>

          <div class="sub-section" v-show="showSubSection('part2', 'counter')">
            <div class="sub-title">计数器</div>
            <div class="lib-grid">
              <div
                v-for="item in filterItems(counterFBs)"
                :key="item.name"
                class="lib-item"
                draggable="true"
                @dragstart="handleDragStart($event, item)"
              >
                <span class="material-symbols-outlined lib-icon">{{
                  item.icon
                }}</span>
                <span class="lib-name">{{ item.name }}</span>
                <span class="lib-desc">{{ item.desc }}</span>
              </div>
            </div>
          </div>

          <div class="sub-section" v-show="showSubSection('part2', 'edge')">
            <div class="sub-title">边沿检测</div>
            <div class="lib-grid">
              <div
                v-for="item in filterItems(edgeFBs)"
                :key="item.name"
                class="lib-item"
                draggable="true"
                @dragstart="handleDragStart($event, item)"
              >
                <span class="material-symbols-outlined lib-icon">{{
                  item.icon
                }}</span>
                <span class="lib-name">{{ item.name }}</span>
                <span class="lib-desc">{{ item.desc }}</span>
              </div>
            </div>
          </div>

          <div class="sub-section" v-show="showSubSection('part2', 'trigger')">
            <div class="sub-title">触发器 / 锁存器</div>
            <div class="lib-grid">
              <div
                v-for="item in filterItems(triggerFBs)"
                :key="item.name"
                class="lib-item"
                draggable="true"
                @dragstart="handleDragStart($event, item)"
              >
                <span class="material-symbols-outlined lib-icon">{{
                  item.icon
                }}</span>
                <span class="lib-name">{{ item.name }}</span>
                <span class="lib-desc">{{ item.desc }}</span>
              </div>
            </div>
          </div>
        </div>
      </div>

      <!-- PLCopen Part 3: 运动控制 -->
      <div class="lib-section" v-show="showSection('part3')">
        <div class="section-title-bar" @click="toggleSection('part3')">
          <span
            class="material-symbols-outlined section-arrow"
            :class="{ collapsed: !sectionExpanded.part3 }"
            >expand_more</span
          >
          <span class="lib-section-title">PLCopen Part 3 — 运动控制</span>
        </div>
        <div v-show="sectionExpanded.part3">
          <div class="sub-section" v-show="showSubSection('part3', 'basic')">
            <div class="sub-title">基本控制</div>
            <div class="lib-grid">
              <div
                v-for="item in filterItems(motionBasic)"
                :key="item.name"
                class="lib-item"
                draggable="true"
                @dragstart="handleDragStart($event, item)"
              >
                <span class="material-symbols-outlined lib-icon mc-icon">{{
                  item.icon
                }}</span>
                <span class="lib-name">{{ item.name }}</span>
                <span class="lib-desc">{{ item.desc }}</span>
              </div>
            </div>
          </div>

          <div class="sub-section" v-show="showSubSection('part3', 'move')">
            <div class="sub-title">运动指令</div>
            <div class="lib-grid">
              <div
                v-for="item in filterItems(motionMove)"
                :key="item.name"
                class="lib-item"
                draggable="true"
                @dragstart="handleDragStart($event, item)"
              >
                <span class="material-symbols-outlined lib-icon mc-icon">{{
                  item.icon
                }}</span>
                <span class="lib-name">{{ item.name }}</span>
                <span class="lib-desc">{{ item.desc }}</span>
              </div>
            </div>
          </div>

          <div class="sub-section" v-show="showSubSection('part3', 'read')">
            <div class="sub-title">读取功能块</div>
            <div class="lib-grid">
              <div
                v-for="item in filterItems(motionRead)"
                :key="item.name"
                class="lib-item"
                draggable="true"
                @dragstart="handleDragStart($event, item)"
              >
                <span class="material-symbols-outlined lib-icon mc-icon">{{
                  item.icon
                }}</span>
                <span class="lib-name">{{ item.name }}</span>
                <span class="lib-desc">{{ item.desc }}</span>
              </div>
            </div>
          </div>

          <div class="sub-section" v-show="showSubSection('part3', 'group')">
            <div class="sub-title">轴组操作</div>
            <div class="lib-grid">
              <div
                v-for="item in filterItems(motionGroup)"
                :key="item.name"
                class="lib-item"
                draggable="true"
                @dragstart="handleDragStart($event, item)"
              >
                <span class="material-symbols-outlined lib-icon mc-icon">{{
                  item.icon
                }}</span>
                <span class="lib-name">{{ item.name }}</span>
                <span class="lib-desc">{{ item.desc }}</span>
              </div>
            </div>
          </div>
        </div>
      </div>

      <!-- PLCopen Part 4: 凸轮 -->
      <div class="lib-section" v-show="showSection('part4')">
        <div class="section-title-bar" @click="toggleSection('part4')">
          <span
            class="material-symbols-outlined section-arrow"
            :class="{ collapsed: !sectionExpanded.part4 }"
            >expand_more</span
          >
          <span class="lib-section-title">PLCopen Part 4 — 电子凸轮</span>
        </div>
        <div v-show="sectionExpanded.part4">
          <div class="sub-section" v-show="showSubSection('part4', 'cam')">
            <div class="sub-title">凸轮功能块</div>
            <div class="lib-grid">
              <div
                v-for="item in filterItems(camFBs)"
                :key="item.name"
                class="lib-item"
                draggable="true"
                @dragstart="handleDragStart($event, item)"
              >
                <span class="material-symbols-outlined lib-icon">{{
                  item.icon
                }}</span>
                <span class="lib-name">{{ item.name }}</span>
                <span class="lib-desc">{{ item.desc }}</span>
              </div>
            </div>
          </div>
        </div>
      </div>

      <!-- PLCopen Part 5: 数据传输 -->
      <div class="lib-section" v-show="showSection('part5')">
        <div class="section-title-bar" @click="toggleSection('part5')">
          <span
            class="material-symbols-outlined section-arrow"
            :class="{ collapsed: !sectionExpanded.part5 }"
            >expand_more</span
          >
          <span class="lib-section-title">PLCopen Part 5 — 数据传输</span>
        </div>
        <div v-show="sectionExpanded.part5">
          <div class="sub-section" v-show="showSubSection('part5', 'data')">
            <div class="sub-title">数据传输功能块</div>
            <div class="lib-grid">
              <div
                v-for="item in filterItems(dataTransferFBs)"
                :key="item.name"
                class="lib-item"
                draggable="true"
                @dragstart="handleDragStart($event, item)"
              >
                <span class="material-symbols-outlined lib-icon">{{
                  item.icon
                }}</span>
                <span class="lib-name">{{ item.name }}</span>
                <span class="lib-desc">{{ item.desc }}</span>
              </div>
            </div>
          </div>
        </div>
      </div>

      <!-- 第三方库 -->
      <div class="lib-section" v-show="showSection('thirdparty')">
        <div class="section-title-bar" @click="toggleSection('thirdparty')">
          <span
            class="material-symbols-outlined section-arrow"
            :class="{ collapsed: !sectionExpanded.thirdparty }"
            >expand_more</span
          >
          <span class="lib-section-title">第三方库</span>
        </div>
        <div v-show="sectionExpanded.thirdparty">
          <div
            class="sub-section"
            v-show="showSubSection('thirdparty', 'comm')"
          >
            <div class="sub-title">通讯协议</div>
            <div class="lib-grid">
              <div
                v-for="item in filterItems(commLibs)"
                :key="item.name"
                class="lib-item"
                draggable="true"
                @dragstart="handleDragStart($event, item)"
              >
                <span class="material-symbols-outlined lib-icon">{{
                  item.icon
                }}</span>
                <span class="lib-name">{{ item.name }}</span>
                <span class="lib-desc">{{ item.desc }}</span>
              </div>
            </div>
          </div>
          <div
            class="sub-section"
            v-show="showSubSection('thirdparty', 'util')"
          >
            <div class="sub-title">实用工具</div>
            <div class="lib-grid">
              <div
                v-for="item in filterItems(utilLibs)"
                :key="item.name"
                class="lib-item"
                draggable="true"
                @dragstart="handleDragStart($event, item)"
              >
                <span class="material-symbols-outlined lib-icon">{{
                  item.icon
                }}</span>
                <span class="lib-name">{{ item.name }}</span>
                <span class="lib-desc">{{ item.desc }}</span>
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed } from "vue";

const searchText = ref("");

interface LibraryItem {
  name: string;
  type: string;
  icon: string;
  category: string;
  part: string;
  desc?: string;
}

// ========== PLCopen Part 1: 基础功能 ==========

const typeConversion: LibraryItem[] = [
  {
    name: "BOOL_TO_INT",
    type: "function",
    icon: "swap_horiz",
    category: "类型转换",
    part: "part1",
  },
  {
    name: "BOOL_TO_DINT",
    type: "function",
    icon: "swap_horiz",
    category: "类型转换",
    part: "part1",
  },
  {
    name: "BOOL_TO_REAL",
    type: "function",
    icon: "swap_horiz",
    category: "类型转换",
    part: "part1",
  },
  {
    name: "INT_TO_BOOL",
    type: "function",
    icon: "swap_horiz",
    category: "类型转换",
    part: "part1",
  },
  {
    name: "INT_TO_DINT",
    type: "function",
    icon: "swap_horiz",
    category: "类型转换",
    part: "part1",
  },
  {
    name: "INT_TO_REAL",
    type: "function",
    icon: "swap_horiz",
    category: "类型转换",
    part: "part1",
  },
  {
    name: "INT_TO_STRING",
    type: "function",
    icon: "swap_horiz",
    category: "类型转换",
    part: "part1",
  },
  {
    name: "DINT_TO_BOOL",
    type: "function",
    icon: "swap_horiz",
    category: "类型转换",
    part: "part1",
  },
  {
    name: "DINT_TO_INT",
    type: "function",
    icon: "swap_horiz",
    category: "类型转换",
    part: "part1",
  },
  {
    name: "DINT_TO_REAL",
    type: "function",
    icon: "swap_horiz",
    category: "类型转换",
    part: "part1",
  },
  {
    name: "DINT_TO_STRING",
    type: "function",
    icon: "swap_horiz",
    category: "类型转换",
    part: "part1",
  },
  {
    name: "REAL_TO_BOOL",
    type: "function",
    icon: "swap_horiz",
    category: "类型转换",
    part: "part1",
  },
  {
    name: "REAL_TO_INT",
    type: "function",
    icon: "swap_horiz",
    category: "类型转换",
    part: "part1",
  },
  {
    name: "REAL_TO_DINT",
    type: "function",
    icon: "swap_horiz",
    category: "类型转换",
    part: "part1",
  },
  {
    name: "REAL_TO_STRING",
    type: "function",
    icon: "swap_horiz",
    category: "类型转换",
    part: "part1",
  },
  {
    name: "STRING_TO_INT",
    type: "function",
    icon: "swap_horiz",
    category: "类型转换",
    part: "part1",
  },
  {
    name: "STRING_TO_REAL",
    type: "function",
    icon: "swap_horiz",
    category: "类型转换",
    part: "part1",
  },
  {
    name: "STRING_TO_TIME",
    type: "function",
    icon: "swap_horiz",
    category: "类型转换",
    part: "part1",
  },
  {
    name: "TIME_TO_STRING",
    type: "function",
    icon: "swap_horiz",
    category: "类型转换",
    part: "part1",
  },
  {
    name: "BYTE_TO_INT",
    type: "function",
    icon: "swap_horiz",
    category: "类型转换",
    part: "part1",
  },
  {
    name: "WORD_TO_INT",
    type: "function",
    icon: "swap_horiz",
    category: "类型转换",
    part: "part1",
  },
  {
    name: "DWORD_TO_DINT",
    type: "function",
    icon: "swap_horiz",
    category: "类型转换",
    part: "part1",
  },
];

const mathFunctions: LibraryItem[] = [
  {
    name: "ABS",
    type: "function",
    icon: "functions",
    category: "数学函数",
    part: "part1",
    desc: "绝对值",
  },
  {
    name: "SQRT",
    type: "function",
    icon: "functions",
    category: "数学函数",
    part: "part1",
    desc: "平方根",
  },
  {
    name: "LN",
    type: "function",
    icon: "functions",
    category: "数学函数",
    part: "part1",
    desc: "自然对数",
  },
  {
    name: "LOG",
    type: "function",
    icon: "functions",
    category: "数学函数",
    part: "part1",
    desc: "常用对数",
  },
  {
    name: "EXP",
    type: "function",
    icon: "functions",
    category: "数学函数",
    part: "part1",
    desc: "指数",
  },
  {
    name: "SIN",
    type: "function",
    icon: "functions",
    category: "数学函数",
    part: "part1",
    desc: "正弦",
  },
  {
    name: "COS",
    type: "function",
    icon: "functions",
    category: "数学函数",
    part: "part1",
    desc: "余弦",
  },
  {
    name: "TAN",
    type: "function",
    icon: "functions",
    category: "数学函数",
    part: "part1",
    desc: "正切",
  },
  {
    name: "ASIN",
    type: "function",
    icon: "functions",
    category: "数学函数",
    part: "part1",
    desc: "反正弦",
  },
  {
    name: "ACOS",
    type: "function",
    icon: "functions",
    category: "数学函数",
    part: "part1",
    desc: "反余弦",
  },
  {
    name: "ATAN",
    type: "function",
    icon: "functions",
    category: "数学函数",
    part: "part1",
    desc: "反正切",
  },
];

const arithmeticOps: LibraryItem[] = [
  {
    name: "ADD",
    type: "function",
    icon: "add",
    category: "算术运算",
    part: "part1",
    desc: "加法",
  },
  {
    name: "SUB",
    type: "function",
    icon: "remove",
    category: "算术运算",
    part: "part1",
    desc: "减法",
  },
  {
    name: "MUL",
    type: "function",
    icon: "close",
    category: "算术运算",
    part: "part1",
    desc: "乘法",
  },
  {
    name: "DIV",
    type: "function",
    icon: "percent",
    category: "算术运算",
    part: "part1",
    desc: "除法",
  },
  {
    name: "MOD",
    type: "function",
    icon: "percent",
    category: "算术运算",
    part: "part1",
    desc: "取模",
  },
  {
    name: "EXPT",
    type: "function",
    icon: "functions",
    category: "算术运算",
    part: "part1",
    desc: "指数运算",
  },
  {
    name: "NEG",
    type: "function",
    icon: "remove_circle",
    category: "算术运算",
    part: "part1",
    desc: "取负",
  },
];

const comparisonOps: LibraryItem[] = [
  {
    name: "GT",
    type: "function",
    icon: "more_than",
    category: "比较运算",
    part: "part1",
    desc: "大于 >",
  },
  {
    name: "GE",
    type: "function",
    icon: "more_or_equal",
    category: "比较运算",
    part: "part1",
    desc: "大于等于 >=",
  },
  {
    name: "EQ",
    type: "function",
    icon: "equal",
    category: "比较运算",
    part: "part1",
    desc: "等于 =",
  },
  {
    name: "LE",
    type: "function",
    icon: "less_or_equal",
    category: "比较运算",
    part: "part1",
    desc: "小于等于 <=",
  },
  {
    name: "LT",
    type: "function",
    icon: "less_than",
    category: "比较运算",
    part: "part1",
    desc: "小于 <",
  },
  {
    name: "NE",
    type: "function",
    icon: "not_equal",
    category: "比较运算",
    part: "part1",
    desc: "不等于 <>",
  },
];

const selectionOps: LibraryItem[] = [
  {
    name: "SEL",
    type: "function",
    icon: "call_split",
    category: "选择运算",
    part: "part1",
    desc: "二选一",
  },
  {
    name: "MAX",
    type: "function",
    icon: "arrow_upward",
    category: "选择运算",
    part: "part1",
    desc: "最大值",
  },
  {
    name: "MIN",
    type: "function",
    icon: "arrow_downward",
    category: "选择运算",
    part: "part1",
    desc: "最小值",
  },
  {
    name: "LIMIT",
    type: "function",
    icon: "vertical_align_center",
    category: "选择运算",
    part: "part1",
    desc: "限幅",
  },
  {
    name: "MUX",
    type: "function",
    icon: "filter_list",
    category: "选择运算",
    part: "part1",
    desc: "多路选择",
  },
];

const bitwiseOps: LibraryItem[] = [
  {
    name: "SHL",
    type: "function",
    icon: "arrow_forward",
    category: "位运算",
    part: "part1",
    desc: "左移",
  },
  {
    name: "SHR",
    type: "function",
    icon: "arrow_back",
    category: "位运算",
    part: "part1",
    desc: "右移",
  },
  {
    name: "ROL",
    type: "function",
    icon: "rotate_right",
    category: "位运算",
    part: "part1",
    desc: "循环左移",
  },
  {
    name: "ROR",
    type: "function",
    icon: "rotate_left",
    category: "位运算",
    part: "part1",
    desc: "循环右移",
  },
  {
    name: "AND",
    type: "function",
    icon: "token",
    category: "位运算",
    part: "part1",
    desc: "按位与",
  },
  {
    name: "OR",
    type: "function",
    icon: "token",
    category: "位运算",
    part: "part1",
    desc: "按位或",
  },
  {
    name: "XOR",
    type: "function",
    icon: "token",
    category: "位运算",
    part: "part1",
    desc: "按位异或",
  },
  {
    name: "NOT",
    type: "function",
    icon: "not_interested",
    category: "位运算",
    part: "part1",
    desc: "按位取反",
  },
];

// ========== PLCopen Part 2: 功能块 ==========

const timerFBs: LibraryItem[] = [
  {
    name: "TON",
    type: "functionBlock",
    icon: "timer",
    category: "定时器",
    part: "part2",
    desc: "接通延时",
  },
  {
    name: "TOF",
    type: "functionBlock",
    icon: "timer_off",
    category: "定时器",
    part: "part2",
    desc: "断开延时",
  },
  {
    name: "TP",
    type: "functionBlock",
    icon: "timer",
    category: "定时器",
    part: "part2",
    desc: "脉冲定时",
  },
  {
    name: "TONR",
    type: "functionBlock",
    icon: "timer",
    category: "定时器",
    part: "part2",
    desc: "带记忆接通延时",
  },
  {
    name: "TIME_EXTRACT",
    type: "function",
    icon: "schedule",
    category: "定时器",
    part: "part2",
    desc: "提取时间分量",
  },
  {
    name: "TIME_ADD",
    type: "function",
    icon: "schedule",
    category: "定时器",
    part: "part2",
    desc: "时间加法",
  },
  {
    name: "TIME_SUB",
    type: "function",
    icon: "schedule",
    category: "定时器",
    part: "part2",
    desc: "时间减法",
  },
];

const counterFBs: LibraryItem[] = [
  {
    name: "CTU",
    type: "functionBlock",
    icon: "counter",
    category: "计数器",
    part: "part2",
    desc: "加计数",
  },
  {
    name: "CTD",
    type: "functionBlock",
    icon: "counter",
    category: "计数器",
    part: "part2",
    desc: "减计数",
  },
  {
    name: "CTUD",
    type: "functionBlock",
    icon: "counter",
    category: "计数器",
    part: "part2",
    desc: "加减计数",
  },
  {
    name: "CTUD_DINT",
    type: "functionBlock",
    icon: "counter",
    category: "计数器",
    part: "part2",
    desc: "32位加减计数",
  },
];

const edgeFBs: LibraryItem[] = [
  {
    name: "R_TRIG",
    type: "functionBlock",
    icon: "arrow_upward",
    category: "边沿检测",
    part: "part2",
    desc: "上升沿检测",
  },
  {
    name: "F_TRIG",
    type: "functionBlock",
    icon: "arrow_downward",
    category: "边沿检测",
    part: "part2",
    desc: "下降沿检测",
  },
];

const triggerFBs: LibraryItem[] = [
  {
    name: "SR",
    type: "functionBlock",
    icon: "toggle_on",
    category: "触发器",
    part: "part2",
    desc: "优先置位锁存",
  },
  {
    name: "RS",
    type: "functionBlock",
    icon: "toggle_on",
    category: "触发器",
    part: "part2",
    desc: "优先复位锁存",
  },
  {
    name: "SEMA",
    type: "functionBlock",
    icon: "lock",
    category: "触发器",
    part: "part2",
    desc: "信号量",
  },
];

// ========== PLCopen Part 3: 运动控制 ==========

const motionBasic: LibraryItem[] = [
  {
    name: "MC_Power",
    type: "functionBlock",
    icon: "power_settings_new",
    category: "基本控制",
    part: "part3",
    desc: "轴使能",
  },
  {
    name: "MC_Home",
    type: "functionBlock",
    icon: "home",
    category: "基本控制",
    part: "part3",
    desc: "回原点",
  },
  {
    name: "MC_Stop",
    type: "functionBlock",
    icon: "stop_circle",
    category: "基本控制",
    part: "part3",
    desc: "轴停止",
  },
  {
    name: "MC_Halt",
    type: "functionBlock",
    icon: "pause_circle",
    category: "基本控制",
    part: "part3",
    desc: "轴暂停",
  },
  {
    name: "MC_Reset",
    type: "functionBlock",
    icon: "refresh",
    category: "基本控制",
    part: "part3",
    desc: "故障复位",
  },
  {
    name: "MC_ReInitialize",
    type: "functionBlock",
    icon: "restart_alt",
    category: "基本控制",
    part: "part3",
    desc: "重新初始化",
  },
];

const motionMove: LibraryItem[] = [
  {
    name: "MC_MoveAbsolute",
    type: "functionBlock",
    icon: "my_location",
    category: "运动指令",
    part: "part3",
    desc: "绝对定位",
  },
  {
    name: "MC_MoveRelative",
    type: "functionBlock",
    icon: "open_with",
    category: "运动指令",
    part: "part3",
    desc: "相对定位",
  },
  {
    name: "MC_MoveVelocity",
    type: "functionBlock",
    icon: "speed",
    category: "运动指令",
    part: "part3",
    desc: "速度控制",
  },
  {
    name: "MC_MoveAdditive",
    type: "functionBlock",
    icon: "add_circle",
    category: "运动指令",
    part: "part3",
    desc: "叠加运动",
  },
  {
    name: "MC_MoveCircular",
    type: "functionBlock",
    icon: "radio_button_unchecked",
    category: "运动指令",
    part: "part3",
    desc: "圆弧插补",
  },
  {
    name: "MC_MoveLinear",
    type: "functionBlock",
    icon: "straight",
    category: "运动指令",
    part: "part3",
    desc: "直线插补",
  },
  {
    name: "MC_GearIn",
    type: "functionBlock",
    icon: "settings",
    category: "运动指令",
    part: "part3",
    desc: "电子齿轮",
  },
  {
    name: "MC_GearInPos",
    type: "functionBlock",
    icon: "settings",
    category: "运动指令",
    part: "part3",
    desc: "位置同步齿轮",
  },
];

const motionRead: LibraryItem[] = [
  {
    name: "MC_ReadParameter",
    type: "functionBlock",
    icon: "read_more",
    category: "读取功能块",
    part: "part3",
    desc: "读取参数",
  },
  {
    name: "MC_ReadStatus",
    type: "functionBlock",
    icon: "info",
    category: "读取功能块",
    part: "part3",
    desc: "读取状态",
  },
  {
    name: "MC_ReadActualPosition",
    type: "functionBlock",
    icon: "pin_drop",
    category: "读取功能块",
    part: "part3",
    desc: "读取实际位置",
  },
  {
    name: "MC_ReadActualVelocity",
    type: "functionBlock",
    icon: "speed",
    category: "读取功能块",
    part: "part3",
    desc: "读取实际速度",
  },
  {
    name: "MC_ReadActualAcceleration",
    type: "functionBlock",
    icon: "speed",
    category: "读取功能块",
    part: "part3",
    desc: "读取实际加速度",
  },
  {
    name: "MC_ReadAxisError",
    type: "functionBlock",
    icon: "error",
    category: "读取功能块",
    part: "part3",
    desc: "读取轴错误",
  },
  {
    name: "MC_ReadMotionState",
    type: "functionBlock",
    icon: "analytics",
    category: "读取功能块",
    part: "part3",
    desc: "读取运动状态",
  },
];

const motionGroup: LibraryItem[] = [
  {
    name: "MC_GroupEnable",
    type: "functionBlock",
    icon: "group",
    category: "轴组操作",
    part: "part3",
    desc: "使能轴组",
  },
  {
    name: "MC_GroupDisable",
    type: "functionBlock",
    icon: "group",
    category: "轴组操作",
    part: "part3",
    desc: "禁用轴组",
  },
  {
    name: "MC_GroupStop",
    type: "functionBlock",
    icon: "group",
    category: "轴组操作",
    part: "part3",
    desc: "停止轴组",
  },
  {
    name: "MC_GroupHome",
    type: "functionBlock",
    icon: "group",
    category: "轴组操作",
    part: "part3",
    desc: "轴组回原",
  },
  {
    name: "MC_GroupReadStatus",
    type: "functionBlock",
    icon: "group",
    category: "轴组操作",
    part: "part3",
    desc: "读取轴组状态",
  },
];

// ========== PLCopen Part 4: 电子凸轮 ==========

const camFBs: LibraryItem[] = [
  {
    name: "MC_CamIn",
    type: "functionBlock",
    icon: "settings_input_component",
    category: "凸轮功能块",
    part: "part4",
    desc: "凸轮啮合",
  },
  {
    name: "MC_CamOut",
    type: "functionBlock",
    icon: "settings_input_component",
    category: "凸轮功能块",
    part: "part4",
    desc: "凸轮脱开",
  },
  {
    name: "MC_CamDefine",
    type: "functionBlock",
    icon: "edit_note",
    category: "凸轮功能块",
    part: "part4",
    desc: "凸轮表定义",
  },
  {
    name: "MC_CamTableSelect",
    type: "functionBlock",
    icon: "list",
    category: "凸轮功能块",
    part: "part4",
    desc: "凸轮表选择",
  },
  {
    name: "MC_Phasing",
    type: "functionBlock",
    icon: "phase_agent",
    category: "凸轮功能块",
    part: "part4",
    desc: "相位偏移",
  },
];

// ========== PLCopen Part 5: 数据传输 ==========

const dataTransferFBs: LibraryItem[] = [
  {
    name: "MC_ReadAxesGroup",
    type: "functionBlock",
    icon: "download",
    category: "数据传输",
    part: "part5",
    desc: "读取轴组数据",
  },
  {
    name: "MC_WriteAxesGroup",
    type: "functionBlock",
    icon: "upload",
    category: "数据传输",
    part: "part5",
    desc: "写入轴组数据",
  },
  {
    name: "DATA_ACCESS",
    type: "functionBlock",
    icon: "database",
    category: "数据传输",
    part: "part5",
    desc: "数据访问",
  },
];

// ========== 第三方 / 扩展库 ==========

const commLibs: LibraryItem[] = [
  {
    name: "MODBUS_MASTER",
    type: "functionBlock",
    icon: "lan",
    category: "通讯",
    part: "thirdparty",
    desc: "Modbus 主站",
  },
  {
    name: "MODBUS_SLAVE",
    type: "functionBlock",
    icon: "lan",
    category: "通讯",
    part: "thirdparty",
    desc: "Modbus 从站",
  },
  {
    name: "MODBUS_TCP",
    type: "functionBlock",
    icon: "settings_ethernet",
    category: "通讯",
    part: "thirdparty",
    desc: "Modbus TCP",
  },
  {
    name: "MODBUS_RTU",
    type: "functionBlock",
    icon: "cable",
    category: "通讯",
    part: "thirdparty",
    desc: "Modbus RTU",
  },
  {
    name: "OPC_UA_CLIENT",
    type: "functionBlock",
    icon: "cloud",
    category: "通讯",
    part: "thirdparty",
    desc: "OPC UA 客户端",
  },
  {
    name: "OPC_UA_SERVER",
    type: "functionBlock",
    icon: "cloud",
    category: "通讯",
    part: "thirdparty",
    desc: "OPC UA 服务端",
  },
  {
    name: "ETHERCAT_MASTER",
    type: "functionBlock",
    icon: "settings_ethernet",
    category: "通讯",
    part: "thirdparty",
    desc: "EtherCAT 主站",
  },
  {
    name: "PROFINET_IO",
    type: "functionBlock",
    icon: "settings_ethernet",
    category: "通讯",
    part: "thirdparty",
    desc: "PROFINET IO",
  },
  {
    name: "ETHERNET_IP",
    type: "functionBlock",
    icon: "settings_ethernet",
    category: "通讯",
    part: "thirdparty",
    desc: "EtherNet/IP",
  },
  {
    name: "CANOPEN",
    type: "functionBlock",
    icon: "settings_ethernet",
    category: "通讯",
    part: "thirdparty",
    desc: "CANopen",
  },
  {
    name: "BACNET",
    type: "functionBlock",
    icon: "settings_ethernet",
    category: "通讯",
    part: "thirdparty",
    desc: "BACnet",
  },
  {
    name: "MQTT_CLIENT",
    type: "functionBlock",
    icon: "cloud_sync",
    category: "通讯",
    part: "thirdparty",
    desc: "MQTT 客户端",
  },
  {
    name: "TCP_CLIENT",
    type: "functionBlock",
    icon: "settings_ethernet",
    category: "通讯",
    part: "thirdparty",
    desc: "TCP 客户端",
  },
  {
    name: "TCP_SERVER",
    type: "functionBlock",
    icon: "settings_ethernet",
    category: "通讯",
    part: "thirdparty",
    desc: "TCP 服务端",
  },
  {
    name: "UDP_SOCKET",
    type: "functionBlock",
    icon: "settings_ethernet",
    category: "通讯",
    part: "thirdparty",
    desc: "UDP 套接字",
  },
  {
    name: "HTTP_CLIENT",
    type: "functionBlock",
    icon: "language",
    category: "通讯",
    part: "thirdparty",
    desc: "HTTP 客户端",
  },
  {
    name: "SMS_SEND",
    type: "functionBlock",
    icon: "sms",
    category: "通讯",
    part: "thirdparty",
    desc: "短信发送",
  },
];

const utilLibs: LibraryItem[] = [
  {
    name: "PID",
    type: "functionBlock",
    icon: "tune",
    category: "PID",
    part: "thirdparty",
    desc: "PID 控制器",
  },
  {
    name: "PID_TUNING",
    type: "functionBlock",
    icon: "tune",
    category: "PID",
    part: "thirdparty",
    desc: "PID 自整定",
  },
  {
    name: "RAMP",
    type: "functionBlock",
    icon: "trending_up",
    category: "信号",
    part: "thirdparty",
    desc: "斜坡函数",
  },
  {
    name: "MOVE_AVG",
    type: "functionBlock",
    icon: "show_chart",
    category: "信号",
    part: "thirdparty",
    desc: "移动平均",
  },
  {
    name: "FILTER",
    type: "functionBlock",
    icon: "filter_alt",
    category: "信号",
    part: "thirdparty",
    desc: "数字滤波",
  },
  {
    name: "DEBOUNCE",
    type: "functionBlock",
    icon: "toggle_on",
    category: "信号",
    part: "thirdparty",
    desc: "消抖处理",
  },
  {
    name: "ENCODER",
    type: "functionBlock",
    icon: "dialpad",
    category: "硬件",
    part: "thirdparty",
    desc: "编码器接口",
  },
  {
    name: "PWM",
    type: "functionBlock",
    icon: "graphic_eq",
    category: "硬件",
    part: "thirdparty",
    desc: "PWM 输出",
  },
  {
    name: "PULSE_GEN",
    type: "functionBlock",
    icon: "graphic_eq",
    category: "硬件",
    part: "thirdparty",
    desc: "脉冲发生器",
  },
  {
    name: "SDO_READ",
    type: "functionBlock",
    icon: "download",
    category: "CAN",
    part: "thirdparty",
    desc: "SDO 读取",
  },
  {
    name: "SDO_WRITE",
    type: "functionBlock",
    icon: "upload",
    category: "CAN",
    part: "thirdparty",
    desc: "SDO 写入",
  },
  {
    name: "PDO_MAP",
    type: "functionBlock",
    icon: "swap_horiz",
    category: "CAN",
    part: "thirdparty",
    desc: "PDO 映射",
  },
];

// ========== 合并和搜索 ==========

const allItems = computed(() => [
  ...typeConversion,
  ...mathFunctions,
  ...arithmeticOps,
  ...comparisonOps,
  ...selectionOps,
  ...bitwiseOps,
  ...timerFBs,
  ...counterFBs,
  ...edgeFBs,
  ...triggerFBs,
  ...motionBasic,
  ...motionMove,
  ...motionRead,
  ...motionGroup,
  ...camFBs,
  ...dataTransferFBs,
  ...commLibs,
  ...utilLibs,
]);

const totalCount = computed(() => allItems.value.length);

const sectionExpanded = ref({
  part1: true,
  part2: true,
  part3: false,
  part4: false,
  part5: false,
  thirdparty: false,
});

function toggleSection(key: keyof typeof sectionExpanded.value) {
  sectionExpanded.value[key] = !sectionExpanded.value[key];
}

function filterItems(items: LibraryItem[]): LibraryItem[] {
  if (!searchText.value) return items;
  const q = searchText.value.toLowerCase();
  return items.filter(
    (item) =>
      item.name.toLowerCase().includes(q) ||
      (item.desc && item.desc.toLowerCase().includes(q)) ||
      item.category.toLowerCase().includes(q),
  );
}

function showSection(section: string): boolean {
  if (!searchText.value) return true;
  return allItems.value.some(
    (item) =>
      item.part === section &&
      (item.name.toLowerCase().includes(searchText.value.toLowerCase()) ||
        (item.desc &&
          item.desc.toLowerCase().includes(searchText.value.toLowerCase()))),
  );
}

function showSubSection(section: string, category: string): boolean {
  if (!searchText.value) return true;
  return allItems.value.some(
    (item) =>
      item.part === section &&
      item.category.toLowerCase().includes(category.toLowerCase()) &&
      (item.name.toLowerCase().includes(searchText.value.toLowerCase()) ||
        (item.desc &&
          item.desc.toLowerCase().includes(searchText.value.toLowerCase()))),
  );
}

function handleDragStart(event: DragEvent, item: LibraryItem) {
  if (event.dataTransfer) {
    event.dataTransfer.setData("application/json", JSON.stringify(item));
  }
}
</script>

<style scoped>
.library-panel {
  height: 100%;
  display: flex;
  flex-direction: column;
}

.panel-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: var(--padding-sm);
  background: var(--surface-container-high);
  border-bottom: 1px solid var(--outline-variant);
  flex-shrink: 0;
}

.panel-title {
  font-size: 10px;
  font-weight: 700;
  letter-spacing: 0.05em;
  text-transform: uppercase;
  color: var(--on-surface);
}

.lib-count {
  font-size: 10px;
  color: var(--outline);
  font-family: "JetBrains Mono", monospace;
}

.search-area {
  padding: var(--padding-sm);
  flex-shrink: 0;
}

.search-input {
  display: flex;
  align-items: center;
  gap: 4px;
  background: var(--surface-dim);
  border: 1px solid var(--outline-variant);
  border-radius: var(--radius);
  padding: 0 var(--padding-sm);
}

.search-input .material-symbols-outlined {
  font-size: 16px;
  color: var(--on-surface-variant);
}

.search-input input {
  flex: 1;
  background: none;
  border: none;
  outline: none;
  color: var(--on-surface);
  font-size: 11px;
  font-family: "Inter", sans-serif;
  padding: 4px 0;
}

.library-scroll {
  flex: 1;
  overflow-y: auto;
  padding: 0 var(--padding-xs);
}

.lib-section {
  margin-bottom: 2px;
}

.section-title-bar {
  display: flex;
  align-items: center;
  gap: 2px;
  padding: 6px var(--padding-sm);
  cursor: pointer;
  border-radius: var(--radius);
  transition: background 0.15s;
}

.section-title-bar:hover {
  background: var(--surface-variant);
}

.section-arrow {
  font-size: 16px;
  color: var(--primary);
  transition: transform 0.15s;
}

.section-arrow.collapsed {
  transform: rotate(-90deg);
}

.lib-section-title {
  font-size: 10px;
  font-weight: 700;
  letter-spacing: 0.05em;
  text-transform: uppercase;
  color: var(--primary);
}

.sub-section {
  padding-left: 8px;
  margin-bottom: 4px;
}

.sub-title {
  padding: 2px var(--padding-sm);
  font-size: 10px;
  font-weight: 600;
  color: var(--outline);
}

.lib-grid {
  display: grid;
  grid-template-columns: 1fr;
  gap: 2px;
}

.lib-item {
  display: flex;
  align-items: center;
  gap: var(--padding-sm);
  padding: 4px var(--padding-sm);
  border: 1px solid var(--outline-variant);
  border-radius: var(--radius);
  cursor: grab;
  transition: background 0.15s;
  font-size: 11px;
  color: var(--on-surface);
}

.lib-item:hover {
  background: var(--surface-variant);
}

.lib-item:active {
  cursor: grabbing;
}

.lib-icon {
  font-size: 14px;
  color: var(--primary);
  flex-shrink: 0;
}

.lib-icon.mc-icon {
  color: var(--secondary);
}

.lib-name {
  flex: 1;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
  font-family: "JetBrains Mono", monospace;
  font-size: 11px;
}

.lib-desc {
  font-size: 10px;
  color: var(--outline);
  flex-shrink: 0;
}

.custom-scrollbar::-webkit-scrollbar {
  width: 4px;
}

.custom-scrollbar::-webkit-scrollbar-track {
  background: var(--surface-dim);
}

.custom-scrollbar::-webkit-scrollbar-thumb {
  background: #334155;
}
</style>
