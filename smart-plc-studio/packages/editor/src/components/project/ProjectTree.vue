<template>
  <div class="project-tree-panel">
    <div class="panel-top">
      <div class="panel-header">
        <span class="panel-title">设备管理器</span>
        <button class="btn-compile">全部编译</button>
      </div>
      <div class="tree-scroll">
        <!-- 设备树 -->
        <div
          class="tree-node"
          :class="{ active: rootExpanded }"
          @click="rootExpanded = !rootExpanded"
        >
          <span
            class="material-symbols-outlined chevron"
            :class="{ open: rootExpanded }"
            >chevron_right</span
          >
          <span
            class="material-symbols-outlined node-icon"
            style="color: var(--primary)"
            >dns</span
          >
          <span class="node-label">控制器 (CODESYS V3)</span>
        </div>
        <div v-if="rootExpanded" class="tree-children">
          <div class="tree-node" @click="plcExpanded = !plcExpanded">
            <span
              class="material-symbols-outlined chevron"
              :class="{ open: plcExpanded }"
              >chevron_right</span
            >
            <span class="material-symbols-outlined node-icon">terminal</span>
            <span class="node-label">PLC 逻辑</span>
          </div>
          <div v-if="plcExpanded" class="tree-children">
            <div
              class="tree-node active-highlight"
              @click="appExpanded = !appExpanded"
            >
              <span
                class="material-symbols-outlined chevron"
                :class="{ open: appExpanded }"
                >chevron_right</span
              >
              <span class="material-symbols-outlined node-icon"
                >folder_open</span
              >
              <span class="node-label">Application</span>
            </div>
            <div v-if="appExpanded" class="tree-children">
              <!-- POU 列表 -->
              <div
                v-for="pou in programs"
                :key="pou.name"
                class="tree-node pou-node"
                @click="openPou(pou)"
                @contextmenu.prevent="showPouMenu($event, pou)"
              >
                <span class="material-symbols-outlined node-icon">{{
                  getPouIcon(pou)
                }}</span>
                <span class="node-label"
                  >{{ pou.name }} ({{ getPouTypeLabel(pou) }})</span
                >
              </div>
              <!-- 新建 POU 按钮 -->
              <div class="tree-node add-node" @click="showNewPouDialog = true">
                <span class="material-symbols-outlined node-icon">add</span>
                <span class="node-label add-label">新建程序...</span>
              </div>

              <!-- 数据类型 -->
              <div
                class="tree-node"
                @click="dataTypesExpanded = !dataTypesExpanded"
              >
                <span
                  class="material-symbols-outlined chevron"
                  :class="{ open: dataTypesExpanded }"
                  >chevron_right</span
                >
                <span class="material-symbols-outlined node-icon"
                  >table_chart</span
                >
                <span class="node-label">数据类型</span>
              </div>
              <div v-if="dataTypesExpanded" class="tree-children">
                <div
                  v-for="dt in projectStore.dataTypes"
                  :key="dt.name"
                  class="tree-node"
                  @click="openDataType(dt)"
                >
                  <span class="material-symbols-outlined node-icon"
                    >description</span
                  >
                  <span class="node-label">{{ dt.name }}</span>
                </div>
                <div
                  class="tree-node add-node"
                  @click="showNewDataTypeDialog = true"
                >
                  <span class="material-symbols-outlined node-icon">add</span>
                  <span class="node-label add-label">新建数据类型...</span>
                </div>
              </div>

              <!-- 可视化窗口 -->
              <div class="tree-node" @click="openHmiEditor">
                <span class="material-symbols-outlined node-icon"
                  >grid_view</span
                >
                <span class="node-label">可视化窗口</span>
              </div>

              <!-- 任务配置 -->
              <div class="tree-node" @click="openTaskConfig">
                <span class="material-symbols-outlined node-icon"
                  >schedule</span
                >
                <span class="node-label">任务配置</span>
              </div>

              <!-- 运动学配置 -->
              <div class="tree-node" @click="kinExpanded = !kinExpanded">
                <span
                  class="material-symbols-outlined chevron"
                  :class="{ open: kinExpanded }"
                  >chevron_right</span
                >
                <span class="material-symbols-outlined node-icon"
                  >precision_manufacturing</span
                >
                <span class="node-label">运动学配置</span>
              </div>
              <div v-if="kinExpanded" class="tree-children">
                <div
                  v-for="(item, idx) in kinematicsItems"
                  :key="idx"
                  class="tree-node kin-node"
                  @click="openKinematics(item)"
                >
                  <span class="material-symbols-outlined node-icon">{{
                    item.kinType === "cnc" ? "settings_suggest" : "smart_toy"
                  }}</span>
                  <span class="node-label">{{ item.name }}</span>
                  <span class="node-lang-badge">{{
                    item.kinType === "cnc" ? "CNC" : "Robot"
                  }}</span>
                </div>
                <div
                  class="tree-node add-node"
                  @click="showNewKinDialog = true"
                >
                  <span class="material-symbols-outlined node-icon">add</span>
                  <span class="node-label add-label">新建运动学...</span>
                </div>
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>

    <!-- POU 树 -->
    <div class="panel-bottom">
      <div class="panel-header">
        <span class="panel-title">程序组织单元 (POU)</span>
      </div>
      <div class="tree-scroll">
        <div class="tree-node" @click="pouExpanded = !pouExpanded">
          <span
            class="material-symbols-outlined chevron"
            :class="{ open: pouExpanded }"
            >chevron_right</span
          >
          <span
            class="material-symbols-outlined node-icon"
            style="color: var(--tertiary)"
            >account_tree</span
          >
          <span class="node-label">工程设置</span>
        </div>
        <div v-if="pouExpanded" class="tree-children">
          <div
            v-for="pou in allPous"
            :key="pou.name"
            class="tree-node"
            @click="openPou(pou)"
          >
            <span class="material-symbols-outlined node-icon">functions</span>
            <span class="node-label">{{ pou.name }}</span>
            <span class="node-lang-badge">{{ getBodyLanguage(pou) }}</span>
          </div>
        </div>
      </div>
    </div>

    <!-- 新建 POU 对话框 -->
    <Teleport to="body">
      <div
        v-if="showNewPouDialog"
        class="dialog-overlay"
        @click.self="showNewPouDialog = false"
      >
        <div class="dialog">
          <div class="dialog-header">
            <span class="dialog-title">新建程序组织单元 (POU)</span>
            <button class="dialog-close" @click="showNewPouDialog = false">
              <span class="material-symbols-outlined">close</span>
            </button>
          </div>
          <div class="dialog-body">
            <div class="form-row">
              <label class="form-label">名称</label>
              <input
                v-model="newPouName"
                class="form-input"
                placeholder="例如: MyProgram"
                @keydown.enter="createPou"
                autofocus
              />
            </div>
            <div class="form-row">
              <label class="form-label">类型</label>
              <div class="radio-group">
                <label class="radio-item">
                  <input type="radio" v-model="newPouType" value="program" />
                  <span class="radio-label">程序 (Program)</span>
                </label>
                <label class="radio-item">
                  <input
                    type="radio"
                    v-model="newPouType"
                    value="functionBlock"
                  />
                  <span class="radio-label">功能块 (FunctionBlock)</span>
                </label>
                <label class="radio-item">
                  <input type="radio" v-model="newPouType" value="function" />
                  <span class="radio-label">函数 (Function)</span>
                </label>
              </div>
            </div>
            <div class="form-row">
              <label class="form-label">编程语言</label>
              <div class="radio-group lang-group">
                <label
                  class="radio-item lang-item"
                  :class="{ selected: newPouLang === 'LD' }"
                >
                  <input type="radio" v-model="newPouLang" value="LD" />
                  <div class="lang-card">
                    <span class="lang-icon">⏍</span>
                    <span class="lang-name">LD</span>
                    <span class="lang-desc">梯形图</span>
                  </div>
                </label>
                <label
                  class="radio-item lang-item"
                  :class="{ selected: newPouLang === 'ST' }"
                >
                  <input type="radio" v-model="newPouLang" value="ST" />
                  <div class="lang-card">
                    <span class="lang-icon">T</span>
                    <span class="lang-name">ST</span>
                    <span class="lang-desc">结构化文本</span>
                  </div>
                </label>
                <label
                  class="radio-item lang-item"
                  :class="{ selected: newPouLang === 'FBD' }"
                >
                  <input type="radio" v-model="newPouLang" value="FBD" />
                  <div class="lang-card">
                    <span class="lang-icon">▭</span>
                    <span class="lang-name">FBD</span>
                    <span class="lang-desc">功能块图</span>
                  </div>
                </label>
              </div>
            </div>
          </div>
          <div class="dialog-footer">
            <button class="btn btn-cancel" @click="showNewPouDialog = false">
              取消
            </button>
            <button
              class="btn btn-create"
              @click="createPou"
              :disabled="!newPouName.trim()"
            >
              创建
            </button>
          </div>
        </div>
      </div>
    </Teleport>

    <!-- 新建数据类型对话框 -->
    <Teleport to="body">
      <div
        v-if="showNewDataTypeDialog"
        class="dialog-overlay"
        @click.self="showNewDataTypeDialog = false"
      >
        <div class="dialog">
          <div class="dialog-header">
            <span class="dialog-title">新建数据类型</span>
            <button class="dialog-close" @click="showNewDataTypeDialog = false">
              <span class="material-symbols-outlined">close</span>
            </button>
          </div>
          <div class="dialog-body">
            <div class="form-row">
              <label class="form-label">名称</label>
              <input
                v-model="newDataTypeName"
                class="form-input"
                placeholder="例如: MyStruct"
                @keydown.enter="createDataType"
                autofocus
              />
            </div>
            <div class="form-row">
              <label class="form-label">基础类型</label>
              <select v-model="newDataTypeBase" class="form-select">
                <option value="BOOL">BOOL</option>
                <option value="INT">INT</option>
                <option value="DINT">DINT</option>
                <option value="REAL">REAL</option>
                <option value="LREAL">LREAL</option>
                <option value="STRING">STRING</option>
                <option value="TIME">TIME</option>
                <option value="BYTE">BYTE</option>
                <option value="WORD">WORD</option>
                <option value="DWORD">DWORD</option>
                <option value="ARRAY">ARRAY</option>
                <option value="STRUCT">STRUCT</option>
              </select>
            </div>
            <div class="form-row">
              <label class="form-label">注释</label>
              <input
                v-model="newDataTypeComment"
                class="form-input"
                placeholder="可选注释"
              />
            </div>
          </div>
          <div class="dialog-footer">
            <button
              class="btn btn-cancel"
              @click="showNewDataTypeDialog = false"
            >
              取消
            </button>
            <button
              class="btn btn-create"
              @click="createDataType"
              :disabled="!newDataTypeName.trim()"
            >
              创建
            </button>
          </div>
        </div>
      </div>
    </Teleport>

    <!-- 任务配置对话框 -->
    <Teleport to="body">
      <div
        v-if="showTaskConfigDialog"
        class="dialog-overlay"
        @click.self="showTaskConfigDialog = false"
      >
        <div class="dialog dialog-wide">
          <div class="dialog-header">
            <span class="dialog-title">任务配置</span>
            <button class="dialog-close" @click="showTaskConfigDialog = false">
              <span class="material-symbols-outlined">close</span>
            </button>
          </div>
          <div class="dialog-body">
            <div class="task-table">
              <div class="task-table-header">
                <span class="task-col-name">任务名称</span>
                <span class="task-col-interval">周期 (ms)</span>
                <span class="task-col-priority">优先级</span>
                <span class="task-col-type">类型</span>
                <span class="task-col-actions"></span>
              </div>
              <div
                v-for="(task, idx) in tasks"
                :key="idx"
                class="task-table-row"
              >
                <input
                  class="task-input task-col-name"
                  v-model="task.name"
                  placeholder="任务名"
                />
                <input
                  class="task-input task-col-interval"
                  v-model="task.interval"
                  placeholder="10"
                  type="number"
                />
                <input
                  class="task-input task-col-priority"
                  v-model.number="task.priority"
                  placeholder="1"
                  type="number"
                />
                <select class="task-select task-col-type" v-model="task.type">
                  <option value="cycle">循环执行</option>
                  <option value="event">事件触发</option>
                  <option value="init">初始化</option>
                </select>
                <button
                  class="task-del-btn task-col-actions"
                  @click="tasks.splice(idx, 1)"
                >
                  <span class="material-symbols-outlined">delete</span>
                </button>
              </div>
              <button class="task-add-btn" @click="addTask">
                <span class="material-symbols-outlined">add</span>
                添加任务
              </button>
            </div>
          </div>
          <div class="dialog-footer">
            <button
              class="btn btn-cancel"
              @click="showTaskConfigDialog = false"
            >
              取消
            </button>
            <button class="btn btn-create" @click="saveTaskConfig">保存</button>
          </div>
        </div>
      </div>
    </Teleport>

    <!-- 新建运动学对话框 -->
    <Teleport to="body">
      <div
        v-if="showNewKinDialog"
        class="dialog-overlay"
        @click.self="showNewKinDialog = false"
      >
        <div class="dialog">
          <div class="dialog-header">
            <span class="dialog-title">新建运动学配置</span>
            <button class="dialog-close" @click="showNewKinDialog = false">
              <span class="material-symbols-outlined">close</span>
            </button>
          </div>
          <div class="dialog-body">
            <div class="form-row">
              <label class="form-label">名称</label>
              <input
                v-model="newKinName"
                class="form-input"
                placeholder="例如: Machine_1"
                @keydown.enter="createKinematics"
                autofocus
              />
            </div>
            <div class="form-row">
              <label class="form-label">类型</label>
              <div class="kin-type-group">
                <div
                  class="kin-type-card"
                  :class="{ selected: newKinType === 'cnc' }"
                  @click="newKinType = 'cnc'"
                >
                  <span class="kin-type-icon">⚙</span>
                  <span class="kin-type-name">CNC</span>
                  <span class="kin-type-desc">G 代码解释器</span>
                  <span class="kin-type-detail"
                    >支持 G00/G01/G02/G03
                    等指令，适用于数控机床、雕刻机、激光切割等</span
                  >
                </div>
                <div
                  class="kin-type-card"
                  :class="{ selected: newKinType === 'robot' }"
                  @click="newKinType = 'robot'"
                >
                  <span class="kin-type-icon">🤖</span>
                  <span class="kin-type-name">Robot</span>
                  <span class="kin-type-desc">机器人运动指令解释器</span>
                  <span class="kin-type-detail"
                    >支持 MoveJ/MoveL/MoveC
                    等指令，适用于六轴机器人、SCARA、并联机器人等</span
                  >
                </div>
              </div>
            </div>
          </div>
          <div class="dialog-footer">
            <button class="btn btn-cancel" @click="showNewKinDialog = false">
              取消
            </button>
            <button
              class="btn btn-create"
              @click="createKinematics"
              :disabled="!newKinName.trim()"
            >
              创建
            </button>
          </div>
        </div>
      </div>
    </Teleport>

    <!-- 右键菜单 -->
    <Teleport to="body">
      <div
        v-if="contextMenu.visible"
        class="context-menu-overlay"
        @click="contextMenu.visible = false"
        @contextmenu.prevent="contextMenu.visible = false"
      >
        <div
          class="context-menu"
          :style="{ left: contextMenu.x + 'px', top: contextMenu.y + 'px' }"
        >
          <div v-if="contextMenu.type === 'pou'" class="context-menu-items">
            <div class="context-menu-item" @click="renamePou">
              <span class="material-symbols-outlined">edit</span>
              重命名
            </div>
            <div class="context-menu-divider" />
            <div class="context-menu-label">编程语言</div>
            <div
              class="context-menu-item"
              :class="{ 'menu-active': contextMenu.currentLang === 'LD' }"
              @click="switchPouLanguage('LD')"
            >
              <span class="material-symbols-outlined">{{
                contextMenu.currentLang === "LD"
                  ? "radio_button_checked"
                  : "radio_button_unchecked"
              }}</span>
              LD 梯形图
            </div>
            <div
              class="context-menu-item"
              :class="{ 'menu-active': contextMenu.currentLang === 'ST' }"
              @click="switchPouLanguage('ST')"
            >
              <span class="material-symbols-outlined">{{
                contextMenu.currentLang === "ST"
                  ? "radio_button_checked"
                  : "radio_button_unchecked"
              }}</span>
              ST 结构化文本
            </div>
            <div
              class="context-menu-item"
              :class="{ 'menu-active': contextMenu.currentLang === 'FBD' }"
              @click="switchPouLanguage('FBD')"
            >
              <span class="material-symbols-outlined">{{
                contextMenu.currentLang === "FBD"
                  ? "radio_button_checked"
                  : "radio_button_unchecked"
              }}</span>
              FBD 功能块图
            </div>
            <div class="context-menu-divider" />
            <div class="context-menu-item danger" @click="deletePou">
              <span class="material-symbols-outlined">delete</span>
              删除
            </div>
          </div>
          <div
            v-if="contextMenu.type === 'dataType'"
            class="context-menu-items"
          >
            <div class="context-menu-item danger" @click="deleteDataType">
              <span class="material-symbols-outlined">delete</span>
              删除
            </div>
          </div>
        </div>
      </div>
    </Teleport>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, reactive } from "vue";
import { useProjectStore } from "../../stores/project";
import { useEditorStore } from "../../stores/editor";
import { EditorLanguage, PouType } from "@smart-plc/shared";
import type { POU, DataType } from "@smart-plc/shared";

const projectStore = useProjectStore();
const editorStore = useEditorStore();

const rootExpanded = ref(true);
const plcExpanded = ref(true);
const appExpanded = ref(true);
const dataTypesExpanded = ref(false);
const pouExpanded = ref(true);
const kinExpanded = ref(false);

// 新建 POU 对话框
const showNewPouDialog = ref(false);
const newPouName = ref("");
const newPouType = ref<string>("program");
const newPouLang = ref<string>("LD");

// 新建数据类型对话框
const showNewDataTypeDialog = ref(false);
const newDataTypeName = ref("");
const newDataTypeBase = ref("BOOL");
const newDataTypeComment = ref("");

// 新建运动学对话框
const showNewKinDialog = ref(false);
const newKinName = ref("");
const newKinType = ref<"cnc" | "robot">("cnc");

// 运动学项目列表
interface KinItem {
  name: string;
  kinType: "cnc" | "robot";
}
const kinematicsItems = ref<KinItem[]>([]);

// 任务配置对话框
const showTaskConfigDialog = ref(false);
const tasks = ref<
  Array<{ name: string; interval: string; priority: number; type: string }>
>([{ name: "MainTask", interval: "10", priority: 1, type: "cycle" }]);

// 右键菜单
const contextMenu = reactive({
  visible: false,
  x: 0,
  y: 0,
  type: "" as "pou" | "dataType" | "",
  targetName: "",
  currentLang: "LD" as string,
});

const programs = computed(() => projectStore.getPouByType(PouType.Program));
const allPous = computed(() => [
  ...projectStore.getPouByType(PouType.Function),
  ...projectStore.getPouByType(PouType.FunctionBlock),
  ...projectStore.getPouByType(PouType.Program),
]);

function getPouIcon(pou: POU): string {
  switch (pou.pouType) {
    case PouType.Program:
      return "play_arrow";
    case PouType.FunctionBlock:
      return "widgets";
    case PouType.Function:
      return "function";
    default:
      return "description";
  }
}

function getPouTypeLabel(pou: POU): string {
  switch (pou.pouType) {
    case PouType.Program:
      return "PRG";
    case PouType.FunctionBlock:
      return "FB";
    case PouType.Function:
      return "FUN";
    default:
      return "";
  }
}

function getBodyLanguage(pou: POU): string {
  return (pou as any).bodyLanguage || "LD";
}

// ========== POU 操作 ==========

function openPou(pou: POU) {
  const lang = getBodyLanguage(pou);
  let editorLang = EditorLanguage.LD;
  if (lang === "ST") editorLang = EditorLanguage.ST;
  else if (lang === "FBD") editorLang = EditorLanguage.FBD;

  editorStore.openTab({
    id: pou.name,
    title: pou.name,
    language: editorLang,
    path: `pou/${pou.name}`,
    modified: false,
    content: pou.body,
    pouName: pou.name,
  });
}

function createPou() {
  const name = newPouName.value.trim();
  if (!name) return;

  // 检查重名
  if (projectStore.pous.some((p) => p.name === name)) {
    alert(`POU "${name}" 已存在`);
    return;
  }

  const pouType = newPouType.value as PouType;
  let body = "";

  if (newPouLang.value === "ST") {
    body = `PROGRAM ${name}\nVAR\nEND_VAR\n\nEND_PROGRAM`;
  } else if (newPouLang.value === "FBD") {
    body = JSON.stringify({ nodes: [], edges: [] });
  } else {
    // LD 默认内容
    body = JSON.stringify({ rungs: [] });
  }

  const pou: POU = {
    name,
    pouType,
    variables: {
      inputVars: [],
      outputVars: [],
      inOutVars: [],
      localVars: [],
      tempVars: [],
      globalVars: [],
      externalVars: [],
      accessVars: [],
    },
    body,
    bodyLanguage: newPouLang.value,
  };

  projectStore.addPou(pou);
  showNewPouDialog.value = false;
  newPouName.value = "";
  newPouType.value = "program";
  newPouLang.value = "LD";

  // 自动打开新建的 POU
  openPou(pou);
}

function showPouMenu(event: MouseEvent, pou: POU) {
  contextMenu.visible = true;
  contextMenu.x = event.clientX;
  contextMenu.y = event.clientY;
  contextMenu.type = "pou";
  contextMenu.targetName = pou.name;
  contextMenu.currentLang = getBodyLanguage(pou);
}

function renamePou() {
  const oldName = contextMenu.targetName;
  const newName = prompt("输入新名称:", oldName);
  if (!newName || newName === oldName) {
    contextMenu.visible = false;
    return;
  }
  if (projectStore.pous.some((p) => p.name === newName)) {
    alert(`POU "${newName}" 已存在`);
    contextMenu.visible = false;
    return;
  }
  projectStore.updatePou(oldName, { name: newName });
  contextMenu.visible = false;
}

function deletePou() {
  const name = contextMenu.targetName;
  if (confirm(`确定要删除 "${name}" 吗？`)) {
    projectStore.removePou(name);
    const tab = editorStore.tabs.find((t) => t.pouName === name);
    if (tab) editorStore.closeTab(tab.id);
  }
  contextMenu.visible = false;
}

function switchPouLanguage(lang: string) {
  const name = contextMenu.targetName;
  projectStore.updatePou(name, { bodyLanguage: lang });
  contextMenu.currentLang = lang;
  contextMenu.visible = false;
  // 如果该 POU 已打开，关闭后用新语言重新打开
  const existingTab = editorStore.tabs.find((t) => t.pouName === name);
  if (existingTab) {
    editorStore.closeTab(existingTab.id);
  }
  const pou = projectStore.pous.find((p) => p.name === name);
  if (pou) {
    openPou(pou);
  }
}

// ========== 数据类型操作 ==========

function openDataType(dataType: DataType) {
  editorStore.openTab({
    id: dataType.name,
    title: dataType.name,
    language: EditorLanguage.ST,
    path: `datatype/${dataType.name}`,
    modified: false,
    content: dataType.baseType,
  });
}

function createDataType() {
  const name = newDataTypeName.value.trim();
  if (!name) return;
  if (projectStore.dataTypes.some((dt) => dt.name === name)) {
    alert(`数据类型 "${name}" 已存在`);
    return;
  }
  projectStore.addDataType({
    name,
    baseType: newDataTypeBase.value,
    comment: newDataTypeComment.value || undefined,
  });
  showNewDataTypeDialog.value = false;
  newDataTypeName.value = "";
  newDataTypeBase.value = "BOOL";
  newDataTypeComment.value = "";
}

function deleteDataType() {
  const name = contextMenu.targetName;
  if (confirm(`确定要删除数据类型 "${name}" 吗？`)) {
    projectStore.removeDataType(name);
  }
  contextMenu.visible = false;
}

// ========== 可视化 / 任务配置 ==========

function openHmiEditor() {
  editorStore.openTab({
    id: "__hmi_main__",
    title: "可视化窗口",
    language: EditorLanguage.HMI,
    path: "visualization/main",
    modified: false,
    content: "",
  });
}

function openTaskConfig() {
  showTaskConfigDialog.value = true;
}

function addTask() {
  tasks.value.push({
    name: `Task${tasks.value.length + 1}`,
    interval: "10",
    priority: tasks.value.length + 1,
    type: "cycle",
  });
}

function saveTaskConfig() {
  showTaskConfigDialog.value = false;
}

// ========== 运动学操作 ==========

function createKinematics() {
  const name = newKinName.value.trim();
  if (!name) return;
  if (kinematicsItems.value.some((k) => k.name === name)) {
    alert(`运动学配置 "${name}" 已存在`);
    return;
  }
  kinematicsItems.value.push({ name, kinType: newKinType.value });
  showNewKinDialog.value = false;
  newKinName.value = "";
  newKinType.value = "cnc";
}

function openKinematics(item: KinItem) {
  // 根据类型生成不同的初始内容
  let content = "";
  if (item.kinType === "cnc") {
    content = JSON.stringify({
      kinType: "cnc",
      name: item.name,
      gcode: "G90 G21\nG00 X0 Y0 Z50\nG01 X100 Y0 Z0 F500\n",
      machineType: "gantry_3axis",
      axes: ["X", "Y", "Z"],
    });
  } else {
    content = JSON.stringify({
      kinType: "robot",
      name: item.name,
      commands: "MoveJ P1 v=100%\nMoveL P2 v=50mm/s\n",
      machineType: "robot_6axis",
      axes: ["J1", "J2", "J3", "J4", "J5", "J6"],
    });
  }

  editorStore.openTab({
    id: `kin-${item.name}`,
    title: `运动学 - ${item.name}`,
    language: EditorLanguage.Kinematics,
    path: `kinematics/${item.name}`,
    modified: false,
    content,
  });
}
</script>

<style scoped>
.project-tree-panel {
  height: 100%;
  display: flex;
  flex-direction: column;
}

.panel-top {
  flex: 1;
  display: flex;
  flex-direction: column;
  overflow: hidden;
  border-bottom: 1px solid var(--outline-variant);
}

.panel-bottom {
  height: 33%;
  display: flex;
  flex-direction: column;
  overflow: hidden;
  background: var(--surface-container-low);
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

.btn-compile {
  background: var(--primary-container);
  color: var(--on-primary-container);
  border: none;
  border-radius: var(--radius);
  padding: 2px 8px;
  font-size: 10px;
  font-weight: 700;
  letter-spacing: 0.05em;
  text-transform: uppercase;
  cursor: pointer;
  transition: filter var(--transition-fast);
}

.btn-compile:hover {
  filter: brightness(1.1);
}

.tree-scroll {
  flex: 1;
  overflow-y: auto;
  padding: var(--padding-xs) 0;
}

.tree-node {
  display: flex;
  align-items: center;
  gap: 4px;
  padding: 3px var(--padding-sm);
  padding-left: calc(var(--padding-sm) + 4px);
  font-size: 11px;
  line-height: 16px;
  color: var(--on-surface-variant);
  cursor: pointer;
  user-select: none;
  transition: background var(--transition-fast);
}

.tree-node:hover {
  background: var(--surface-variant);
}

.tree-node.active {
  background: rgba(77, 142, 255, 0.15);
  color: var(--on-surface);
}

.tree-node.active-highlight {
  background: rgba(77, 142, 255, 0.2);
  color: var(--primary);
}

.tree-node.pou-node {
  color: var(--secondary);
}

.tree-node.pou-node:hover {
  background: var(--surface-variant);
}

.tree-node.add-node {
  color: var(--on-surface-variant);
  opacity: 0.6;
}

.tree-node.add-node:hover {
  opacity: 1;
  color: var(--primary);
}

.add-label {
  font-style: italic;
}

.node-lang-badge {
  margin-left: auto;
  padding: 0 4px;
  font-size: 9px;
  font-weight: 700;
  color: var(--tertiary);
  background: var(--tertiary-container);
  border-radius: 3px;
  line-height: 14px;
}

.chevron {
  font-size: 16px;
  width: 16px;
  flex-shrink: 0;
  transition: transform var(--transition-fast);
}

.chevron.open {
  transform: rotate(90deg);
}

.node-icon {
  font-size: 16px;
  width: 16px;
  flex-shrink: 0;
}

.node-label {
  flex: 1;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.tree-children {
  padding-left: 16px;
}

/* ========== 对话框 ========== */

.dialog-overlay {
  position: fixed;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background: rgba(0, 0, 0, 0.6);
  display: flex;
  align-items: center;
  justify-content: center;
  z-index: 9999;
}

.dialog {
  background: var(--surface-container);
  border: 1px solid var(--outline-variant);
  border-radius: var(--radius-lg, 12px);
  width: 420px;
  max-height: 80vh;
  display: flex;
  flex-direction: column;
  box-shadow: 0 8px 32px rgba(0, 0, 0, 0.4);
}

.dialog-wide {
  width: 580px;
}

.dialog-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 12px 16px;
  background: var(--surface-container-high);
  border-bottom: 1px solid var(--outline-variant);
  border-radius: var(--radius-lg, 12px) var(--radius-lg, 12px) 0 0;
}

.dialog-title {
  font-size: 13px;
  font-weight: 700;
  color: var(--on-surface);
}

.dialog-close {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 24px;
  height: 24px;
  background: none;
  border: none;
  border-radius: var(--radius);
  color: var(--on-surface-variant);
  cursor: pointer;
}

.dialog-close:hover {
  background: var(--surface-variant);
  color: var(--error);
}

.dialog-close .material-symbols-outlined {
  font-size: 16px;
}

.dialog-body {
  padding: 16px;
  overflow-y: auto;
}

.dialog-footer {
  display: flex;
  justify-content: flex-end;
  gap: 8px;
  padding: 12px 16px;
  border-top: 1px solid var(--outline-variant);
}

.form-row {
  margin-bottom: 14px;
}

.form-row:last-child {
  margin-bottom: 0;
}

.form-label {
  display: block;
  font-size: 11px;
  font-weight: 600;
  color: var(--on-surface-variant);
  margin-bottom: 6px;
}

.form-input,
.form-select {
  width: 100%;
  padding: 6px 10px;
  background: var(--surface-dim);
  border: 1px solid var(--outline-variant);
  border-radius: var(--radius);
  color: var(--on-surface);
  font-size: 12px;
  font-family: "Inter", sans-serif;
  outline: none;
  transition: border-color 0.15s;
}

.form-input:focus,
.form-select:focus {
  border-color: var(--primary);
}

.radio-group {
  display: flex;
  flex-direction: column;
  gap: 6px;
}

.radio-item {
  display: flex;
  align-items: center;
  gap: 8px;
  cursor: pointer;
}

.radio-item input[type="radio"] {
  accent-color: var(--primary);
}

.radio-label {
  font-size: 12px;
  color: var(--on-surface);
}

.lang-group {
  flex-direction: row;
  gap: 8px;
}

.lang-item {
  flex: 1;
}

.lang-item input[type="radio"] {
  display: none;
}

.lang-card {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 2px;
  padding: 10px 8px;
  background: var(--surface-dim);
  border: 2px solid var(--outline-variant);
  border-radius: var(--radius);
  transition: all 0.15s;
}

.lang-item:hover .lang-card {
  border-color: var(--primary);
}

.lang-item.selected .lang-card {
  border-color: var(--primary);
  background: var(--primary-container);
}

.lang-icon {
  font-size: 18px;
  font-weight: 700;
  color: var(--primary);
  font-family: "JetBrains Mono", monospace;
}

.lang-name {
  font-size: 12px;
  font-weight: 700;
  color: var(--on-surface);
}

.lang-desc {
  font-size: 9px;
  color: var(--on-surface-variant);
}

.btn {
  padding: 6px 16px;
  border: none;
  border-radius: var(--radius);
  font-size: 12px;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.15s;
}

.btn-cancel {
  background: var(--surface-variant);
  color: var(--on-surface-variant);
}

.btn-cancel:hover {
  background: var(--surface-container-high);
}

.btn-create {
  background: var(--primary);
  color: var(--on-primary);
}

.btn-create:hover {
  filter: brightness(1.1);
}

.btn-create:disabled {
  opacity: 0.4;
  cursor: not-allowed;
}

/* ========== 任务配置表格 ========== */

.task-table {
  display: flex;
  flex-direction: column;
  gap: 4px;
}

.task-table-header {
  display: flex;
  gap: 6px;
  padding: 6px 8px;
  background: var(--surface-container-high);
  border-radius: var(--radius);
  font-size: 10px;
  font-weight: 700;
  color: var(--on-surface-variant);
  text-transform: uppercase;
  letter-spacing: 0.05em;
}

.task-table-row {
  display: flex;
  gap: 6px;
  padding: 4px 8px;
  background: var(--surface-dim);
  border-radius: var(--radius);
  align-items: center;
}

.task-input,
.task-select {
  padding: 4px 6px;
  background: var(--surface-container);
  border: 1px solid var(--outline-variant);
  border-radius: 4px;
  color: var(--on-surface);
  font-size: 11px;
  font-family: "Inter", sans-serif;
  outline: none;
}

.task-input:focus,
.task-select:focus {
  border-color: var(--primary);
}

.task-col-name {
  flex: 2;
}
.task-col-interval {
  flex: 1;
}
.task-col-priority {
  flex: 1;
}
.task-col-type {
  flex: 1.5;
}
.task-col-actions {
  width: 28px;
  flex-shrink: 0;
}

.task-del-btn {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 24px;
  height: 24px;
  background: none;
  border: none;
  border-radius: 4px;
  color: var(--on-surface-variant);
  cursor: pointer;
}

.task-del-btn:hover {
  background: var(--error-container, rgba(244, 67, 54, 0.1));
  color: var(--error);
}

.task-del-btn .material-symbols-outlined {
  font-size: 14px;
}

.task-add-btn {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 4px;
  padding: 6px;
  background: none;
  border: 1px dashed var(--outline-variant);
  border-radius: var(--radius);
  color: var(--on-surface-variant);
  font-size: 11px;
  cursor: pointer;
}

.task-add-btn:hover {
  border-color: var(--primary);
  color: var(--primary);
}

.task-add-btn .material-symbols-outlined {
  font-size: 14px;
}

/* ========== 右键菜单 ========== */

.context-menu-overlay {
  position: fixed;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  z-index: 9999;
}

.context-menu {
  position: fixed;
  background: var(--surface-container-high);
  border: 1px solid var(--outline-variant);
  border-radius: var(--radius);
  box-shadow: 0 4px 16px rgba(0, 0, 0, 0.3);
  min-width: 140px;
  padding: 4px 0;
  z-index: 10000;
}

.context-menu-items {
  display: flex;
  flex-direction: column;
}

.context-menu-item {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 6px 12px;
  font-size: 11px;
  color: var(--on-surface);
  cursor: pointer;
  transition: background 0.1s;
}

.context-menu-item:hover {
  background: var(--surface-variant);
}

.context-menu-item.danger {
  color: var(--error);
}

.context-menu-item.danger:hover {
  background: var(--error-container, rgba(244, 67, 54, 0.1));
}

.context-menu-item.menu-active {
  color: var(--primary);
  font-weight: 600;
}

.context-menu-item .material-symbols-outlined {
  font-size: 14px;
}

.context-menu-divider {
  height: 1px;
  background: var(--outline-variant);
  margin: 4px 8px;
}

.context-menu-label {
  padding: 4px 12px;
  font-size: 9px;
  font-weight: 700;
  color: var(--on-surface-variant);
  text-transform: uppercase;
  letter-spacing: 0.05em;
}

/* ========== 运动学 ========== */

.kin-node {
  color: var(--tertiary);
}

.kin-node:hover {
  background: var(--surface-variant);
}

.kin-type-group {
  display: flex;
  gap: 10px;
}

.kin-type-card {
  flex: 1;
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 4px;
  padding: 14px 10px;
  background: var(--surface-dim);
  border: 2px solid var(--outline-variant);
  border-radius: var(--radius);
  cursor: pointer;
  transition: all 0.15s;
  text-align: center;
}

.kin-type-card:hover {
  border-color: var(--primary);
}

.kin-type-card.selected {
  border-color: var(--primary);
  background: var(--primary-container);
}

.kin-type-icon {
  font-size: 24px;
  line-height: 1;
}

.kin-type-name {
  font-size: 13px;
  font-weight: 700;
  color: var(--on-surface);
}

.kin-type-desc {
  font-size: 10px;
  color: var(--on-surface-variant);
}

.kin-type-detail {
  font-size: 9px;
  color: var(--on-surface-variant);
  line-height: 1.3;
  margin-top: 4px;
}
</style>
