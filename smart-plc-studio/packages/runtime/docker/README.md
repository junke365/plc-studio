# Smart PLC Runtime - Docker/QEMU 集成测试

通过 Docker 和 QEMU 在容器中构建并测试 Linux x86 和 Linux ARM 平台的 PLC 运行时。

## 前置条件

- [Docker Desktop](https://www.docker.com/products/docker-desktop/) (Windows/Mac/Linux)
- 注册 QEMU binfmt（ARM 需要）：已内置于 `docker-test.sh` 中

## 快速开始

```bash
# 构建并测试全部平台
./scripts/docker-test.sh all

# 仅测试 x86
./scripts/docker-test.sh x86

# 仅测试 ARM（需 QEMU 支持）
./scripts/docker-test.sh arm
```

## 手动构建

```bash
# Linux x86
docker build -f docker/Dockerfile.x86 -t plc-runtime:x86 .
docker run --rm plc-runtime:x86

# Linux ARM（交叉编译 + QEMU）
docker run --rm --privileged multiarch/qemu-user-static --reset -p yes
docker build -f docker/Dockerfile.arm -t plc-runtime:arm .
docker run --rm --privileged plc-runtime:arm
```

## 架构说明

| 平台 | 构建方式 | 运行方式 | 基础镜像 |
|------|---------|---------|---------|
| Linux x86 | 本地 GCC 编译 | 原生执行 | Ubuntu 22.04 |
| Linux ARM | arm-linux-gnueabihf-gcc 交叉编译 | QEMU user-mode | Ubuntu 22.04 + qemu-user |

## 容器内测试流程

1. 构建 `plc-runtime` 可执行文件
2. 启动 PLC 运行时并运行 5 秒
3. 检查输出是否包含启动信息和错误
4. 返回测试结果
