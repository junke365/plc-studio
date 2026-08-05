# PLC Studio

PLC 编程与 HMI 设计一体化工具。前端为 Vue3 + Vite 的编辑器（`packages/editor`），后端为 Fastify（`packages/server`），桌面壳为 Electron（`electron/`）。

## 环境要求

- Node.js >= 20（建议 22 LTS）
- npm >= 10
- macOS / Linux / Windows 均可（C 运行时 `packages/runtime` 目前仅 Linux/Win32，不影响本工具运行与打包）

> 国内网络建议使用镜像：npm 用 `--registry=https://registry.npmmirror.com`，Electron 二进制下载用 `ELECTRON_MIRROR=https://npmmirror.com/mirrors/electron/`。

## 安装

```bash
npm install
```

## 开发模式（浏览器运行）

```bash
npm run dev
```

- 后端：http://127.0.0.1:3000 （`GET /api/health` 返回 ok）
- 前端：http://127.0.0.1:5173

开发模式下的 Electron 壳：

```bash
npm run dev:electron
```

## 构建

```bash
npm run build:electron:ts   # 编译 electron/（仅 tsc）
npm run build:shared        # 编译共享类型/常量
npm run build:plc-core      # 编译 PLC 核心
npm run build:server        # 编译服务端 TS
npm run build:server:bundle # esbuild 打成单文件 CJS bundle（dist-server/main.cjs）
npm run build:editor        # vite 构建编辑器
```

## 打包桌面应用

```bash
npm run pack:mac            # macOS dmg（x64 + arm64）
npm run pack:linux          # Linux AppImage + deb（x64 + arm64，需在 Linux 环境执行）
```

或手动指定架构：`npx electron-builder --mac --arm64 --publish never`。

产物输出到 `release/`。

### 打包注意事项

- 后端由 esbuild 打成单文件 CJS bundle（`packages/server/dist-server/main.cjs`），Electron 主进程用 `require` 加载（asar-aware），因此 ESM 无法在 asar 内 `import` 的问题不影响打包。
- native 模块（better-sqlite3、serialport）使用 NAPI 预编译产物，`electron-builder.yml` 中 `npmRebuild: false`，`.node` 文件通过 `asarUnpack` 展开。**不要用 Node 26 源码编译**（V8 要求 C++20，旧模块会失败）。
- 编辑器以 `base: "./"` + hash router 构建，可在 `file://` 下运行；非 http 环境时 serialClient 自动回退到 `http://127.0.0.1:3000`。
- macOS 打包默认关闭签名（`CSC_IDENTITY_AUTO_DISCOVERY=false`），如需正式签名请在 CI 配置证书。

## CI 发布

`.github/workflows/release.yml` 在推送 `v*` tag 或手动触发时：

- `macos-latest`：构建 x64 与 arm64 dmg
- `ubuntu-latest`：构建 x64 与 arm64 AppImage + deb
- 打 tag 时自动上传至 GitHub Release
