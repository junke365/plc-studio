import { defineConfig } from "vite";
import vue from "@vitejs/plugin-vue";
import cesium from "vite-plugin-cesium";
import { resolve } from "path";
import { rosStatic } from "./viteRosPlugin";

export default defineConfig({
  base: "./",
  plugins: [
    vue(),
    rosStatic(),
    cesium({
      cesiumBuildRootPath: "../../node_modules/cesium/Build",
      cesiumBuildPath: "../../node_modules/cesium/Build/Cesium/",
    }),
  ],
  resolve: {
    alias: {
      "@": resolve(__dirname, "src"),
      "@smart-plc/shared": resolve(__dirname, "../shared/src"),
      "@smart-plc/plc-core": resolve(__dirname, "../plc-core/src"),
    },
  },
  optimizeDeps: {
    include: ["cesium"],
  },
  server: {
    host: "0.0.0.0",
    port: 5173,
    proxy: {
      "/api": {
        target: "http://localhost:3000",
        changeOrigin: true,
      },
      "/ws": {
        target: "http://localhost:3000",
        ws: true,
      },
      "/socket.io": {
        target: "http://localhost:3000",
        ws: true,
      },
    },
  },
  build: {
    outDir: "dist",
    sourcemap: true,
  },
});
