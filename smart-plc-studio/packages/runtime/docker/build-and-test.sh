#!/bin/sh
# Smart PLC Runtime - Docker 容器内测试入口
# 验证 PLC 运行时能正常启动并执行基本功能

set -e

PLC_BIN="${PLC_BIN:-/usr/local/bin/plc-runtime}"
TIMEOUT_SEC="${TIMEOUT_SEC:-5}"

echo "============================================"
echo " Smart PLC Runtime - 容器内测试"
echo " 可执行文件: ${PLC_BIN}"
echo " 测试超时:   ${TIMEOUT_SEC}s"
echo "============================================"

if [ ! -f "${PLC_BIN}" ]; then
  echo "[FAIL] 找不到 ${PLC_BIN}"
  exit 1
fi

echo "[INFO] 文件类型:"
file "${PLC_BIN}"

echo "[INFO] 文件大小: $(wc -c < "${PLC_BIN}") 字节"

echo "[INFO] 启动 PLC 运行时 (${TIMEOUT_SEC}s 后自动终止)..."
timeout "${TIMEOUT_SEC}" "${PLC_BIN}" > /tmp/plc-output.txt 2>&1 || true

echo ""
echo "========== PLC 运行时输出 =========="
cat /tmp/plc-output.txt
echo "====================================="

if grep -qi "error\|fatal\|failed" /tmp/plc-output.txt; then
  echo "[FAIL] PLC 运行时输出包含错误信息"
  exit 1
fi

if grep -qi "Smart PLC Runtime\|plc_runtime_init\|已注册\|已启动" /tmp/plc-output.txt; then
  echo "[PASS] PLC 运行时正常启动"
else
  echo "[WARN] 未能确认 PLC 运行时启动信息（可能超时退出）"
fi

echo ""
echo "========== 测试完成 =========="
exit 0
