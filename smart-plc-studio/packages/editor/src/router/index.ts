import {
  createRouter,
  createWebHashHistory,
  type RouteRecordRaw,
} from "vue-router";

const routes: RouteRecordRaw[] = [
  {
    path: "/",
    name: "IDE",
    component: () => import("../views/IDE.vue"),
  },
  {
    path: "/welcome",
    name: "Welcome",
    component: () => import("../views/Welcome.vue"),
  },
  {
    path: "/simulator",
    name: "Simulator",
    component: () => import("../views/Simulator.vue"),
  },
  {
    path: "/surgical-sim",
    name: "SurgicalSim",
    component: () => import("../views/SurgicalSim.vue"),
  },
  {
    path: "/ros2-sim",
    name: "Ros2Sim",
    component: () => import("../views/Ros2Sim.vue"),
  },
  {
    path: "/winder-sim",
    name: "WinderSim",
    component: () => import("../views/WinderSim.vue"),
  },
  {
    path: "/injector-sim",
    name: "InjectorSim",
    component: () => import("../views/InjectorSim.vue"),
  },
  {
    path: "/topology",
    name: "Topology",
    component: () => import("../views/TopologyEditor.vue"),
  },
];

const router = createRouter({
  history: createWebHashHistory(),
  routes,
});

export default router;
