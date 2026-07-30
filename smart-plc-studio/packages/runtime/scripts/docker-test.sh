#!/bin/bash
# Smart PLC Runtime - Docker/QEMU 集成测试调度器
# 构建并在容器中运行 Linux x86 和 Linux ARM PLC 运行时
#
# 用法:
#   ./scripts/docker-test.sh              # 构建并测试全部
#   ./scripts/docker-test.sh x86          # 仅 x86
#   ./scripts/docker-test.sh arm          # 仅 ARM (需 QEMU)
#   ./scripts/docker-test.sh clean        # 清理 Docker 镜像
#   ./scripts/docker-test.sh shell x86    # 进入容器交互

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
DOCKER_DIR="${SCRIPT_DIR}/docker"
COMPOSE_FILE="${DOCKER_DIR}/docker-compose.yml"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

info()  { echo -e "${GREEN}[INFO]${NC} $*"; }
warn()  { echo -e "${YELLOW}[WARN]${NC} $*"; }
error() { echo -e "${RED}[ERROR]${NC} $*"; }

# 检查 Docker
check_docker() {
  if ! command -v docker &>/dev/null; then
    error "Docker 未安装，请先安装 Docker Desktop 或 docker-ce"
    exit 1
  fi
  if ! docker info &>/dev/null; then
    error "Docker 守护进程未运行"
    exit 1
  fi
}

# 构建并测试 x86
test_x86() {
  info "===== 构建 Linux x86 PLC 运行时 ====="
  docker compose -f "${COMPOSE_FILE}" build plc-runtime-x86
  info "===== 测试 Linux x86 PLC 运行时 ====="
  docker compose -f "${COMPOSE_FILE}" up --abort-on-container-exit plc-runtime-x86
  local rc=$?
  if [ $rc -eq 0 ]; then
    info "Linux x86 测试通过"
  else
    error "Linux x86 测试失败 (exit=$rc)"
    return $rc
  fi
}

# 构建并测试 ARM
test_arm() {
  info "===== 注册 QEMU binfmt (需特权) ====="
  docker run --rm --privileged multiarch/qemu-user-static --reset -p yes 2>/dev/null || \
    warn "qemu-user-static 注册失败，可能需要 --privileged"

  info "===== 构建 Linux ARM PLC 运行时 (交叉编译) ====="
  docker compose -f "${COMPOSE_FILE}" build plc-runtime-arm
  info "===== 测试 Linux ARM PLC 运行时 (QEMU) ====="
  docker compose -f "${COMPOSE_FILE}" up --abort-on-container-exit plc-runtime-arm
  local rc=$?
  if [ $rc -eq 0 ]; then
    info "Linux ARM 测试通过"
  else
    error "Linux ARM 测试失败 (exit=$rc)"
    return $rc
  fi
}

# 清理
clean() {
  info "清理 Docker 镜像..."
  docker rmi plc-runtime:x86 plc-runtime:arm 2>/dev/null || true
  docker image prune -f
  info "清理完成"
}

# 进入交互 shell
shell() {
  local platform="$1"
  case "$platform" in
    x86)
      docker compose -f "${COMPOSE_FILE}" run --rm plc-runtime-x86 /bin/bash
      ;;
    arm)
      docker compose -f "${COMPOSE_FILE}" run --rm plc-runtime-arm /bin/bash
      ;;
    *)
      error "未知平台: $platform (支持: x86, arm)"
      exit 1
      ;;
  esac
}

# ========== 主流程 ==========

check_docker

cd "${SCRIPT_DIR}"

case "${1:-all}" in
  all)
    test_x86
    test_arm
    info "全部测试完成"
    ;;
  x86)
    test_x86
    ;;
  arm)
    test_arm
    ;;
  clean)
    clean
    ;;
  shell)
    shell "$2"
    ;;
  *)
    echo "用法: $0 [all|x86|arm|clean|shell x86|arm]"
    echo "  all       构建并测试 x86 + ARM (默认)"
    echo "  x86       仅构建并测试 Linux x86"
    echo "  arm       仅构建并测试 Linux ARM (需 QEMU)"
    echo "  clean     清理 Docker 镜像"
    echo "  shell     进入容器交互 Shell"
    exit 1
    ;;
esac
