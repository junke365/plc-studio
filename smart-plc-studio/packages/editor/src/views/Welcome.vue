<template>
  <div class="welcome">
    <div class="welcome-content">
      <div class="welcome-brand">
        <h1>PLC Studio</h1>
        <p>智能 PLC IDE — IEC 61131-3 集成开发环境</p>
      </div>

      <div class="welcome-actions">
        <div class="action-section">
          <h3 class="section-label">开始</h3>
          <div class="action-list">
            <div class="action-card" @click="showCreateDialog = true">
              <span class="material-symbols-outlined action-icon"
                >add_circle</span
              >
              <div class="action-text">
                <span class="action-title">新建项目</span>
                <span class="action-desc">创建新的 PLC 项目</span>
              </div>
            </div>
            <div class="action-card" @click="showOpenDialog = true">
              <span class="material-symbols-outlined action-icon"
                >folder_open</span
              >
              <div class="action-text">
                <span class="action-title">打开项目</span>
                <span class="action-desc">打开现有 PLC 项目</span>
              </div>
            </div>
            <div class="action-card" @click="openHMI">
              <span class="material-symbols-outlined action-icon hmi-icon"
                >dashboard</span
              >
              <div class="action-text">
                <span class="action-title">HMI 设计器</span>
                <span class="action-desc">创建或编辑 HMI 画面</span>
              </div>
            </div>
            <div class="action-card" @click="openKinematics">
              <span class="material-symbols-outlined action-icon kin-icon"
                >precision_manufacturing</span
              >
              <div class="action-text">
                <span class="action-title">运动学配置</span>
                <span class="action-desc">配置机器人运动学模型</span>
              </div>
            </div>
          </div>
        </div>

        <div class="action-section">
          <h3 class="section-label">最近项目</h3>
          <div class="recent-list">
            <div
              v-for="project in recentProjects"
              :key="project.path"
              class="recent-item"
              @click="openRecentProject(project)"
            >
              <span class="material-symbols-outlined recent-icon"
                >description</span
              >
              <div class="recent-text">
                <span class="recent-name">{{ project.name }}</span>
                <span class="recent-path">{{ project.path }}</span>
              </div>
            </div>
            <div v-if="recentProjects.length === 0" class="no-recent">
              暂无最近项目
            </div>
          </div>
        </div>
      </div>
    </div>

    <!-- 新建项目对话框 -->
    <div
      v-if="showCreateDialog"
      class="modal-overlay"
      @click.self="showCreateDialog = false"
    >
      <div class="modal">
        <div class="modal-header">
          <span class="modal-title">新建项目</span>
          <span
            class="material-symbols-outlined modal-close"
            @click="showCreateDialog = false"
            >close</span
          >
        </div>
        <div class="modal-body">
          <div class="form-row">
            <label class="form-label">项目名称</label>
            <input
              ref="projectNameInput"
              class="form-input"
              v-model="newProjectName"
              placeholder="例: MyPLC_Project"
              @keydown.enter="handleCreate"
              autofocus
            />
          </div>
          <div class="form-row">
            <label class="form-label">存储路径</label>
            <input
              class="form-input"
              v-model="newProjectPath"
              placeholder="例: C:/Projects/MyPLC"
            />
          </div>
          <div class="form-row">
            <label class="form-label">描述</label>
            <input
              class="form-input"
              v-model="newProjectDesc"
              placeholder="可选的项目描述"
            />
          </div>
          <div v-if="createError" class="form-error">{{ createError }}</div>
        </div>
        <div class="modal-footer">
          <button class="modal-btn cancel" @click="showCreateDialog = false">
            取消
          </button>
          <button
            class="modal-btn confirm"
            @click="handleCreate"
            :disabled="!newProjectName.trim()"
          >
            创建
          </button>
        </div>
      </div>
    </div>

    <!-- 打开项目对话框 -->
    <div
      v-if="showOpenDialog"
      class="modal-overlay"
      @click.self="showOpenDialog = false"
    >
      <div class="modal">
        <div class="modal-header">
          <span class="modal-title">打开项目</span>
          <span
            class="material-symbols-outlined modal-close"
            @click="showOpenDialog = false"
            >close</span
          >
        </div>
        <div class="modal-body">
          <div class="form-row">
            <label class="form-label">项目路径</label>
            <input
              class="form-input"
              v-model="openProjectPathInput"
              placeholder="输入 .plcproj 文件路径"
              @keydown.enter="handleOpen"
              autofocus
            />
          </div>
          <div v-if="openError" class="form-error">{{ openError }}</div>
        </div>
        <div class="modal-footer">
          <button class="modal-btn cancel" @click="showOpenDialog = false">
            取消
          </button>
          <button
            class="modal-btn confirm"
            @click="handleOpen"
            :disabled="!openProjectPathInput.trim()"
          >
            打开
          </button>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, nextTick } from "vue";
import { useProjectStore } from "../stores/project";
import { useEditorStore } from "../stores/editor";
import { PouType, EditorLanguage } from "@smart-plc/shared";
import type { PlcProject, PouVarTable } from "@smart-plc/shared";

const projectStore = useProjectStore();
const editorStore = useEditorStore();

// 最近项目列表（从 localStorage 恢复）
const recentProjects = ref<Array<{ name: string; path: string }>>(
  JSON.parse(localStorage.getItem("plc-recent-projects") || "[]"),
);

// 新建项目对话框
const showCreateDialog = ref(false);
const projectNameInput = ref<HTMLInputElement | null>(null);
const newProjectName = ref("NewProject");
const newProjectPath = ref("");
const newProjectDesc = ref("");
const createError = ref("");

// 打开项目对话框
const showOpenDialog = ref(false);
const openProjectPathInput = ref("");
const openError = ref("");

// 空的变量表
const emptyVarTable: PouVarTable = {
  inputVars: [],
  outputVars: [],
  inOutVars: [],
  localVars: [],
  tempVars: [],
  globalVars: [],
  externalVars: [],
  accessVars: [],
};

function handleCreate() {
  createError.value = "";
  const name = newProjectName.value.trim();

  if (!name) {
    createError.value = "请输入项目名称";
    return;
  }

  // 验证项目名（仅允许字母、数字、下划线）
  if (!/^[a-zA-Z_][a-zA-Z0-9_]*$/.test(name)) {
    createError.value =
      "项目名称只能包含字母、数字和下划线，且以字母或下划线开头";
    return;
  }

  const path = newProjectPath.value.trim() || `C:/Projects/${name}`;

  // 创建默认 PLC 项目
  const project: PlcProject = {
    name,
    path,
    pous: [
      {
        name: `${name}_PRG`,
        pouType: PouType.Program,
        variables: {
          ...emptyVarTable,
          localVars: [
            {
              name: "nCount",
              type: "INT",
              className: "Local" as never,
              initialValue: 0,
              comment: "计数器",
            },
            {
              name: "bStart",
              type: "BOOL",
              className: "Local" as never,
              initialValue: false,
              comment: "启动信号",
            },
            {
              name: "bStop",
              type: "BOOL",
              className: "Local" as never,
              initialValue: false,
              comment: "停止信号",
            },
          ],
        },
        body: JSON.stringify([]),
        comment: `主程序 - ${newProjectDesc.value || name}`,
      },
    ],
    dataTypes: [],
    configurations: [
      {
        name: `${name}_Config`,
        resources: [
          {
            name: "Resource1",
            tasks: [{ name: "Task1", interval: "T#10ms", priority: 1 }],
            programs: [
              {
                name: `${name}_PRG`,
                typeName: `${name}_PRG`,
                taskName: "Task1",
              },
            ],
          },
        ],
      },
    ],
  };

  // 更新项目 Store
  projectStore.openProject(path, project);

  // 自动打开主程序标签页
  editorStore.openTab({
    id: `${name}_PRG`,
    title: `${name}_PRG`,
    language: EditorLanguage.LD,
    path: `pou/${name}_PRG`,
    modified: false,
    content: project.pous[0].body,
    pouName: `${name}_PRG`,
  });

  // 保存到最近项目
  addRecentProject(name, path);

  // 关闭对话框
  showCreateDialog.value = false;

  console.log(`项目 "${name}" 已创建`);
}

function handleOpen() {
  openError.value = "";
  const path = openProjectPathInput.value.trim();

  if (!path) {
    openError.value = "请输入项目路径";
    return;
  }

  // 尝试从 localStorage 恢复项目（简化实现）
  const savedProjects = JSON.parse(
    localStorage.getItem("plc-projects") || "{}",
  );
  if (savedProjects[path]) {
    const project = savedProjects[path] as PlcProject;
    projectStore.openProject(path, project);
    showOpenDialog.value = false;
    return;
  }

  openError.value = `未找到项目: ${path}`;
}

function openRecentProject(project: { name: string; path: string }) {
  const savedProjects = JSON.parse(
    localStorage.getItem("plc-projects") || "{}",
  );
  if (savedProjects[project.path]) {
    const plcProject = savedProjects[project.path] as PlcProject;
    projectStore.openProject(project.path, plcProject);

    // 自动打开主程序
    const mainPou = plcProject.pous.find((p) => p.pouType === PouType.Program);
    if (mainPou) {
      editorStore.openTab({
        id: mainPou.name,
        title: mainPou.name,
        language: EditorLanguage.LD,
        path: `pou/${mainPou.name}`,
        modified: false,
        content: mainPou.body,
        pouName: mainPou.name,
      });
    }
  }
}

function addRecentProject(name: string, path: string) {
  const projects = recentProjects.value.filter((p) => p.path !== path);
  projects.unshift({ name, path });
  recentProjects.value = projects.slice(0, 10);
  localStorage.setItem(
    "plc-recent-projects",
    JSON.stringify(recentProjects.value),
  );
}

function openHMI() {
  editorStore.openTab({
    id: "hmi-designer",
    title: "HMI 设计器",
    language: EditorLanguage.HMI,
    path: "hmi/designer",
    modified: false,
    content: "",
  });
}

function openKinematics() {
  editorStore.openTab({
    id: "kinematics-config",
    title: "运动学配置",
    language: EditorLanguage.Kinematics,
    path: "kinematics/config",
    modified: false,
    content: "",
  });
}
</script>

<style scoped>
.welcome {
  width: 100%;
  height: 100%;
  display: flex;
  justify-content: center;
  align-items: center;
  background: var(--editor-bg);
}

.welcome-content {
  text-align: center;
  max-width: 480px;
}

.welcome-brand h1 {
  font-size: 28px;
  font-weight: 600;
  color: var(--secondary);
  margin-bottom: 8px;
}

.welcome-brand p {
  font-size: 13px;
  color: var(--on-surface-variant);
  margin-bottom: 40px;
}

.section-label {
  font-size: 10px;
  font-weight: 700;
  letter-spacing: 0.05em;
  text-transform: uppercase;
  color: var(--on-surface-variant);
  margin-bottom: var(--padding-sm);
  text-align: left;
}

.action-section {
  margin-bottom: 24px;
}

.action-list {
  display: flex;
  flex-direction: column;
  gap: var(--padding-sm);
}

.action-card {
  display: flex;
  align-items: center;
  gap: var(--padding-md);
  padding: var(--padding-md);
  background: var(--surface-container);
  border: 1px solid var(--outline-variant);
  border-radius: var(--radius);
  cursor: pointer;
  transition:
    background var(--transition-fast),
    border-color var(--transition-fast);
  text-align: left;
}

.action-card:hover {
  background: var(--surface-container-high);
  border-color: var(--outline);
}

.action-icon {
  font-size: 24px;
  color: var(--primary);
}

.action-icon.hmi-icon {
  color: var(--secondary);
}

.action-icon.kin-icon {
  color: var(--tertiary);
}

.action-text {
  display: flex;
  flex-direction: column;
  gap: 2px;
}

.action-title {
  font-size: 13px;
  color: var(--on-surface);
  font-weight: 500;
}

.action-desc {
  font-size: 11px;
  color: var(--on-surface-variant);
}

.recent-list {
  display: flex;
  flex-direction: column;
  gap: 2px;
}

.recent-item {
  display: flex;
  align-items: center;
  gap: var(--padding-sm);
  padding: var(--padding-sm) var(--padding-md);
  border-radius: var(--radius);
  cursor: pointer;
  transition: background var(--transition-fast);
  text-align: left;
}

.recent-item:hover {
  background: var(--surface-container);
}

.recent-icon {
  font-size: 18px;
  color: var(--on-surface-variant);
  flex-shrink: 0;
}

.recent-text {
  display: flex;
  flex-direction: column;
  min-width: 0;
}

.recent-name {
  font-size: 13px;
  color: var(--on-surface);
}

.recent-path {
  font-size: 11px;
  color: var(--on-surface-variant);
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.no-recent {
  font-size: 11px;
  color: var(--on-surface-variant);
  padding: var(--padding-md);
}

/* 模态对话框 */
.modal-overlay {
  position: fixed;
  inset: 0;
  background: rgba(0, 0, 0, 0.6);
  display: flex;
  align-items: center;
  justify-content: center;
  z-index: 1000;
}

.modal {
  background: var(--surface-container);
  border: 1px solid var(--outline-variant);
  border-radius: var(--radius-lg);
  width: 420px;
  max-width: 90vw;
  box-shadow: 0 8px 32px rgba(0, 0, 0, 0.4);
}

.modal-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: var(--padding-md);
  border-bottom: 1px solid var(--outline-variant);
}

.modal-title {
  font-size: 14px;
  font-weight: 600;
  color: var(--on-surface);
}

.modal-close {
  font-size: 18px;
  cursor: pointer;
  color: var(--on-surface-variant);
  transition: color var(--transition-fast);
}

.modal-close:hover {
  color: var(--error);
}

.modal-body {
  padding: var(--padding-md);
  display: flex;
  flex-direction: column;
  gap: var(--padding-sm);
}

.form-row {
  display: flex;
  flex-direction: column;
  gap: 4px;
}

.form-label {
  font-size: 11px;
  font-weight: 700;
  letter-spacing: 0.05em;
  text-transform: uppercase;
  color: var(--on-surface-variant);
}

.form-input {
  padding: 6px 10px;
  background: var(--surface-dim);
  border: 1px solid var(--outline-variant);
  border-radius: var(--radius);
  color: var(--on-surface);
  font-size: 13px;
  font-family: "Inter", sans-serif;
  outline: none;
  transition: border-color var(--transition-fast);
}

.form-input:focus {
  border-color: var(--primary);
}

.form-error {
  font-size: 11px;
  color: var(--error);
  padding: 4px 0;
}

.modal-footer {
  display: flex;
  justify-content: flex-end;
  gap: var(--padding-sm);
  padding: var(--padding-md);
  border-top: 1px solid var(--outline-variant);
}

.modal-btn {
  padding: 6px 16px;
  border: none;
  border-radius: var(--radius);
  font-size: 11px;
  font-weight: 700;
  letter-spacing: 0.05em;
  text-transform: uppercase;
  cursor: pointer;
  transition:
    filter var(--transition-fast),
    background var(--transition-fast);
}

.modal-btn.cancel {
  background: var(--surface-variant);
  color: var(--on-surface-variant);
}

.modal-btn.cancel:hover {
  background: var(--surface-container-high);
}

.modal-btn.confirm {
  background: var(--primary-container);
  color: var(--on-primary-container);
}

.modal-btn.confirm:hover {
  filter: brightness(1.1);
}

.modal-btn.confirm:disabled {
  opacity: 0.4;
  cursor: not-allowed;
  filter: none;
}
</style>
