#!/usr/bin/env bash
# =============================================================================
# PLC Studio ROS2 调试虚拟机 (QEMU aarch64 + Ubuntu 24.04 + ROS2 Jazzy)
#
# 用法:
#   ./qemu-ros2-vm.sh first-boot   # 首次启动(挂 seed.iso 初始化用户+装 ROS2)
#   ./qemu-ros2-vm.sh              # 后续启动
#
# 端口映射(host -> VM):
#   2222 -> 22 (ssh:  ssh ros@localhost -p 2222, 密码 ros123 或免密公钥)
#   3000 -> 3000 (PLC Studio 后端 /api)
#   5173 -> 5173 (Vite 开发服务器, 宿主机浏览器打开 http://localhost:5173)
#
# 在 VM 内: source /opt/ros/jazzy/setup.bash && cd ~/smart-plc-studio && npm run dev
# 镜像: noble-server-cloudimg-arm64.img 已放项目根目录(.gitignore 排除, 不进 git)
# =============================================================================
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
IMG="$ROOT/noble-server-cloudimg-arm64.img"
DISK="$ROOT/ubuntu-ros2.qcow2"
SEED_DIR="$ROOT/.qemu-seed"
SEED_ISO="$ROOT/seed.iso"
DISK_SIZE="${DISK_SIZE:-40G}"
MEM="${MEM:-4096}"
SMP="${SMP:-4}"
SSH_KEY="${SSH_KEY:-$HOME/.ssh/id_ed25519.pub}"

if [ "${1:-}" = "regen-seed" ]; then
  rm -f "$SEED_ISO"
fi

# ---------- 1. SSH 公钥(注入虚拟机, 免密登录) ----------
if [ ! -f "$SSH_KEY" ]; then
  echo "!! 未找到 SSH 公钥 $SSH_KEY, 正在生成..."
  ssh-keygen -t ed25519 -N "" -f "${SSH_KEY%.pub}" -C "plc-studio-ros2-vm" >/dev/null
  echo "!! 如需推送 git, 请把下面内容加到 GitHub:"
  echo "$(cat "$SSH_KEY")"
fi
PUBKEY="$(cat "$SSH_KEY")"

# ---------- 2. Ubuntu 镜像 ----------
if [ ! -f "$IMG" ]; then
  echo "!! 缺少镜像, 正在下载 (约590MB)..."
  curl -fL --retry 3 -C - -o "$IMG" \
    https://cloud-images.ubuntu.com/noble/current/noble-server-cloudimg-arm64.img
fi

# ---------- 3. 数据盘 ----------
[ -f "$DISK" ] || qemu-img create -f qcow2 "$DISK" "$DISK_SIZE"

# ---------- 4. cloud-init 种子盘(初始化用户 + 自动装 ROS2) ----------
if [ ! -f "$SEED_ISO" ]; then
  mkdir -p "$SEED_DIR"
  cat > "$SEED_DIR/user-data" <<EOF
#cloud-config
users:
  - name: ros
    sudo: ALL=(ALL) NOPASSWD:ALL
    shell: /bin/bash
    ssh_authorized_keys:
      - $PUBKEY
ssh_pwauth: true
chpasswd:
  expire: false
  users:
    - name: ros
      password: ros123
      type: text
runcmd:
  - apt-get update
  - curl -sSL https://raw.githubusercontent.com/ros/rosdistro/master/ros.key -o /usr/share/keyrings/ros-archive-keyring.gpg
  - echo "deb [signed-by=/usr/share/keyrings/ros-archive-keyring.gpg] http://packages.ros.org/ros2/ubuntu noble main" > /etc/apt/sources.list.d/ros2.list
  - apt-get update
  - DEBIAN_FRONTEND=noninteractive apt-get install -y ros-jazzy-ros-base python3-colcon-common-extensions git curl
  - echo "source /opt/ros/jazzy/setup.bash" >> /home/ros/.bashrc
EOF
  printf 'instance-id: plc-ros2\nlocal-hostname: plc-ros2\n' > "$SEED_DIR/meta-data"
  if hdiutil makehybrid -iso -joliet -o "$SEED_ISO" -volname cidata "$SEED_DIR" 2>/dev/null; then
    :
  elif genisoimage -o "$SEED_ISO" -volid cidata -joliet -rock "$SEED_DIR" 2>/dev/null; then
    :
  else
    echo "!! 无法生成 seed.iso (需要 hdiutil 或 genisoimage), 请安装 cdrtools 后重试"
    exit 1
  fi
fi

# ---------- 5. 启动 ----------
BOOT_OPTS=()
if [ "${1:-}" = "first-boot" ]; then
  BOOT_OPTS=(-cdrom "$SEED_ISO" -boot d)
fi

echo "==> 启动 ROS2 调试虚拟机 (首次请加 first-boot 参数)"
echo "==> ssh:  ssh ros@localhost -p 2222  (密码 ros123, 或公钥免密)"
exec qemu-system-aarch64 \
  -machine virt -cpu max -m "$MEM" -smp "$SMP" -accel hvf \
  -drive file="$DISK",if=virtio \
  "${BOOT_OPTS[@]}" \
  -netdev user,id=n0,hostfwd=tcp::2222-:22,hostfwd=tcp::3000-:3000,hostfwd=tcp::5173-:5173 \
  -device virtio-net-pci,netdev=n0 \
  -serial mon:stdio -nographic
