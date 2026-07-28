import { defineStore } from "pinia";
import { ref, computed } from "vue";

// ==================== 类型定义 ====================
export interface DhJoint {
  type: "revolute" | "prismatic";
  theta: number;
  d: number;
  a: number;
  alpha: number;
  rangeMin: number;
  rangeMax: number;
}

export interface KinConfig {
  id: string;
  name: string;
  type: string; // 串联机器人 / SCARA / 龙门式 / 并联 / 码垛 / 极坐标
  joints: DhJoint[];
  modelFile?: string;
  modelIcon?: string;
}

// ==================== Store ====================
export const useKinematicsStore = defineStore("kinematics", () => {
  // 运动学配置列表（包含预置和用户自定义）
  const kinConfigs = ref<KinConfig[]>([
    {
      id: "preset_6axis",
      name: "6轴串联机器人",
      type: "串联机器人",
      modelIcon: "precision_manufacturing",
      joints: [
        {
          type: "revolute",
          theta: 0,
          d: 150,
          a: 50,
          alpha: -90,
          rangeMin: -180,
          rangeMax: 180,
        },
        {
          type: "revolute",
          theta: 0,
          d: 0,
          a: 350,
          alpha: 0,
          rangeMin: -120,
          rangeMax: 120,
        },
        {
          type: "revolute",
          theta: 0,
          d: 0,
          a: 35,
          alpha: -90,
          rangeMin: -120,
          rangeMax: 120,
        },
        {
          type: "revolute",
          theta: 0,
          d: 350,
          a: 0,
          alpha: 90,
          rangeMin: -360,
          rangeMax: 360,
        },
        {
          type: "revolute",
          theta: 0,
          d: 0,
          a: 0,
          alpha: -90,
          rangeMin: -120,
          rangeMax: 120,
        },
        {
          type: "revolute",
          theta: 0,
          d: 80,
          a: 0,
          alpha: 0,
          rangeMin: -360,
          rangeMax: 360,
        },
      ],
    },
    {
      id: "preset_scara",
      name: "SCARA 机器人",
      type: "SCARA",
      modelIcon: "precision_manufacturing",
      joints: [
        {
          type: "revolute",
          theta: 0,
          d: 0,
          a: 200,
          alpha: 0,
          rangeMin: -160,
          rangeMax: 160,
        },
        {
          type: "revolute",
          theta: 0,
          d: 0,
          a: 200,
          alpha: 0,
          rangeMin: -150,
          rangeMax: 150,
        },
        {
          type: "prismatic",
          theta: 0,
          d: 0,
          a: 0,
          alpha: 0,
          rangeMin: -100,
          rangeMax: 0,
        },
        {
          type: "revolute",
          theta: 0,
          d: 0,
          a: 0,
          alpha: 0,
          rangeMin: -360,
          rangeMax: 360,
        },
      ],
    },
    {
      id: "preset_gantry",
      name: "龙门机器人",
      type: "龙门式",
      modelIcon: "view_in_ar",
      joints: [
        {
          type: "prismatic",
          theta: 0,
          d: 0,
          a: 0,
          alpha: 0,
          rangeMin: -2000,
          rangeMax: 2000,
        },
        {
          type: "prismatic",
          theta: 0,
          d: 0,
          a: 0,
          alpha: -90,
          rangeMin: -1500,
          rangeMax: 1500,
        },
        {
          type: "prismatic",
          theta: 0,
          d: 0,
          a: 0,
          alpha: 0,
          rangeMin: -500,
          rangeMax: 500,
        },
      ],
    },
    {
      id: "preset_delta",
      name: "Delta 并联机器人",
      type: "并联",
      modelIcon: "view_in_ar",
      joints: [
        {
          type: "revolute",
          theta: 0,
          d: 0,
          a: 350,
          alpha: 0,
          rangeMin: -30,
          rangeMax: 30,
        },
        {
          type: "revolute",
          theta: 0,
          d: 0,
          a: 350,
          alpha: 0,
          rangeMin: -30,
          rangeMax: 30,
        },
        {
          type: "revolute",
          theta: 0,
          d: 0,
          a: 350,
          alpha: 0,
          rangeMin: -30,
          rangeMax: 30,
        },
      ],
    },
    {
      id: "preset_palletizer",
      name: "码垛机器人",
      type: "码垛",
      modelIcon: "precision_manufacturing",
      joints: [
        {
          type: "revolute",
          theta: 0,
          d: 0,
          a: 500,
          alpha: 0,
          rangeMin: -160,
          rangeMax: 160,
        },
        {
          type: "revolute",
          theta: 0,
          d: 0,
          a: 400,
          alpha: 0,
          rangeMin: -10,
          rangeMax: 90,
        },
        {
          type: "revolute",
          theta: 0,
          d: 0,
          a: 0,
          alpha: 0,
          rangeMin: -160,
          rangeMax: 160,
        },
        {
          type: "prismatic",
          theta: 0,
          d: 0,
          a: 0,
          alpha: -90,
          rangeMin: 0,
          rangeMax: 1500,
        },
      ],
    },
    {
      id: "preset_polar",
      name: "极坐标机器人",
      type: "极坐标",
      modelIcon: "radar",
      joints: [
        {
          type: "revolute",
          theta: 0,
          d: 0,
          a: 0,
          alpha: -90,
          rangeMin: -90,
          rangeMax: 90,
        },
        {
          type: "revolute",
          theta: 0,
          d: 0,
          a: 0,
          alpha: 0,
          rangeMin: -180,
          rangeMax: 180,
        },
        {
          type: "prismatic",
          theta: 0,
          d: 0,
          a: 0,
          alpha: 0,
          rangeMin: 100,
          rangeMax: 1200,
        },
        {
          type: "revolute",
          theta: 0,
          d: 0,
          a: 0,
          alpha: 0,
          rangeMin: -360,
          rangeMax: 360,
        },
      ],
    },
  ]);

  // 用户自定义的运动学（非预置）
  const customKinConfigs = computed(() =>
    kinConfigs.value.filter((c) => !c.id.startsWith("preset_")),
  );

  // 全部机器人类型（含自定义）供 CNC 仿真使用
  const allRobotTypes = computed(() =>
    kinConfigs.value.map((c) => ({
      id: c.id,
      name: c.name,
      type: c.type,
      axes: c.joints.map((_j, i) => `J${i + 1}`),
      jointCount: c.joints.length,
    })),
  );

  // 全部运动学配置供全场景模型库使用
  const allKinModelItems = computed(() =>
    kinConfigs.value.map((c) => ({
      id: c.id,
      name: c.name,
      format: "GLB",
      icon: c.modelIcon || "precision_manufacturing",
      url: "",
      kinType: c.type,
    })),
  );

  // 添加运动学配置
  function addKinConfig(config: KinConfig) {
    // 检查是否已存在同 id
    const existing = kinConfigs.value.find((c) => c.id === config.id);
    if (!existing) {
      kinConfigs.value.push(config);
    }
  }

  // 删除运动学配置（仅允许删除非预置）
  function removeKinConfig(id: string) {
    if (id.startsWith("preset_")) return;
    const idx = kinConfigs.value.findIndex((c) => c.id === id);
    if (idx >= 0) kinConfigs.value.splice(idx, 1);
  }

  // 根据 id 获取配置
  function getKinConfig(id: string): KinConfig | undefined {
    return kinConfigs.value.find((c) => c.id === id);
  }

  return {
    kinConfigs,
    customKinConfigs,
    allRobotTypes,
    allKinModelItems,
    addKinConfig,
    removeKinConfig,
    getKinConfig,
  };
});
