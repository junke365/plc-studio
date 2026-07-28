import { defineStore } from "pinia";
import { ref, computed } from "vue";
import type {
  PlcProject,
  POU,
  DataType,
  Configuration,
} from "@smart-plc/shared";

interface RecentProject {
  name: string;
  path: string;
  lastOpened: number;
}

export const useProjectStore = defineStore("project", () => {
  // 项目状态
  const projectPath = ref<string>("");
  const projectName = ref<string>("");
  const projectModified = ref<boolean>(false);

  // PLC 项目数据
  const pous = ref<POU[]>([]);
  const dataTypes = ref<DataType[]>([]);
  const configurations = ref<Configuration[]>([]);

  // 当前编辑的 POU
  const currentPou = ref<POU | null>(null);

  // 最近项目列表
  const recentProjects = ref<RecentProject[]>([]);
  const maxRecentProjects = 10;

  // 计算属性
  const hasProject = computed(() => projectPath.value !== "");

  // 加载最近项目列表
  function loadRecentProjects() {
    try {
      const stored = localStorage.getItem("plc-recent-projects");
      if (stored) {
        recentProjects.value = JSON.parse(stored);
      }
    } catch (e) {
      console.warn("加载最近项目列表失败:", e);
    }
  }

  // 保存最近项目列表
  function saveRecentProjects() {
    try {
      localStorage.setItem(
        "plc-recent-projects",
        JSON.stringify(recentProjects.value),
      );
    } catch (e) {
      console.warn("保存最近项目列表失败:", e);
    }
  }

  // 添加最近项目
  function addRecentProject(name: string, path: string) {
    // 移除已存在的同路径项目
    recentProjects.value = recentProjects.value.filter((p) => p.path !== path);
    // 添加到列表头部
    recentProjects.value.unshift({
      name,
      path,
      lastOpened: Date.now(),
    });
    // 限制列表大小
    if (recentProjects.value.length > maxRecentProjects) {
      recentProjects.value = recentProjects.value.slice(0, maxRecentProjects);
    }
    saveRecentProjects();
  }

  // 移除最近项目
  function removeRecentProject(path: string) {
    recentProjects.value = recentProjects.value.filter((p) => p.path !== path);
    saveRecentProjects();
  }

  // 清空最近项目
  function clearRecentProjects() {
    recentProjects.value = [];
    saveRecentProjects();
  }

  // 新建项目
  function newProject(name: string, path?: string) {
    projectName.value = name;
    projectPath.value = path || "";
    pous.value = [];
    dataTypes.value = [];
    configurations.value = [];
    currentPou.value = null;
    projectModified.value = false;
    if (path) {
      addRecentProject(name, path);
    }
  }

  // 打开项目
  function openProject(path: string, project: PlcProject) {
    projectPath.value = path;
    projectName.value = project.name;
    pous.value = project.pous;
    dataTypes.value = project.dataTypes;
    configurations.value = project.configurations;
    projectModified.value = false;
    // 添加到最近项目
    addRecentProject(project.name, path);
    // 自动保存到 localStorage
    saveToStorage(path, project);
  }

  // 关闭项目
  function closeProject() {
    projectPath.value = "";
    projectName.value = "";
    pous.value = [];
    dataTypes.value = [];
    configurations.value = [];
    currentPou.value = null;
    projectModified.value = false;
  }

  // 添加 POU
  function addPou(pou: POU) {
    pous.value.push(pou);
    projectModified.value = true;
  }

  // 删除 POU
  function removePou(name: string) {
    const index = pous.value.findIndex((p) => p.name === name);
    if (index >= 0) {
      pous.value.splice(index, 1);
      projectModified.value = true;
    }
  }

  // 更新 POU
  function updatePou(name: string, updates: Partial<POU>) {
    const pou = pous.value.find((p) => p.name === name);
    if (pou) {
      Object.assign(pou, updates);
      projectModified.value = true;
    }
  }

  // 添加数据类型
  function addDataType(dataType: DataType) {
    dataTypes.value.push(dataType);
    projectModified.value = true;
  }

  // 删除数据类型
  function removeDataType(name: string) {
    const index = dataTypes.value.findIndex((dt) => dt.name === name);
    if (index >= 0) {
      dataTypes.value.splice(index, 1);
      projectModified.value = true;
    }
  }

  // 设置当前 POU
  function setCurrentPou(pou: POU | null) {
    currentPou.value = pou;
  }

  // 获取 POU 列表
  function getPouByType(type: string): POU[] {
    return pous.value.filter((p) => p.pouType === type);
  }

  // 保存项目到 localStorage
  function saveToStorage(path: string, project: PlcProject) {
    try {
      const saved = JSON.parse(localStorage.getItem("plc-projects") || "{}");
      saved[path] = project;
      localStorage.setItem("plc-projects", JSON.stringify(saved));
    } catch (e) {
      console.warn("保存项目到 localStorage 失败:", e);
    }
  }

  // 保存当前项目
  function saveCurrentProject() {
    if (projectPath.value && projectName.value) {
      const project: PlcProject = {
        name: projectName.value,
        path: projectPath.value,
        pous: pous.value,
        dataTypes: dataTypes.value,
        configurations: configurations.value,
      };
      saveToStorage(projectPath.value, project);
      projectModified.value = false;
    }
  }

  // 初始化
  loadRecentProjects();

  return {
    projectPath,
    projectName,
    projectModified,
    pous,
    dataTypes,
    configurations,
    currentPou,
    recentProjects,
    hasProject,
    newProject,
    openProject,
    closeProject,
    addPou,
    removePou,
    updatePou,
    addDataType,
    removeDataType,
    setCurrentPou,
    getPouByType,
    saveCurrentProject,
    addRecentProject,
    removeRecentProject,
    clearRecentProjects,
  };
});
