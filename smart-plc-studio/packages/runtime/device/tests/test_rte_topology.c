#include "plc_device.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

#define ASSERT(cond, msg) do { \
  if (!(cond)) { fprintf(stderr, "失败: %s\n", msg); return 1; } \
  else { printf("通过: %s\n", msg); } \
} while(0)

static int test_create_topology()
{
  PlcDeviceTopology topo;
  plc_topology_init(&topo);

  int plc1 = plc_topology_addDevice(&topo, "plc-main", "主控制器", DEVICE_PLC);
  ASSERT(plc1 == 0, "添加主控制器");

  int plc2 = plc_topology_addDevice(&topo, "plc-remote", "远程控制器", DEVICE_PLC);
  ASSERT(plc2 == 1, "添加远程控制器");

  int drive = plc_topology_addDevice(&topo, "drive-1", "伺服驱动器", DEVICE_DRIVE);
  ASSERT(drive == 2, "添加伺服驱动器");

  int vision = plc_topology_addDevice(&topo, "cam-1", "视觉相机", DEVICE_VISION);
  ASSERT(vision == 3, "添加视觉相机");

  int robot = plc_topology_addDevice(&topo, "robot-1", "手术机器人", DEVICE_ROBOT);
  ASSERT(robot == 4, "添加手术机器人");

  int io = plc_topology_addDevice(&topo, "io-rack", "IO 模块", DEVICE_IO_MODULE);
  ASSERT(io == 5, "添加 IO 模块");

  ASSERT(topo.deviceCount == 6, "总共 6 台设备");

  /* 测试查找 */
  int found = plc_topology_findById(&topo, "drive-1");
  ASSERT(found == 2, "按 ID 查找设备");
  ASSERT(plc_topology_findById(&topo, "nonexistent") < 0, "查找不存在设备返回 -1");

  found = plc_topology_findByName(&topo, "手术机器人");
  ASSERT(found == 4, "按名称查找设备");

  return 0;
}

static int test_connect_devices()
{
  PlcDeviceTopology topo;
  plc_topology_init(&topo);

  int plc = plc_topology_addDevice(&topo, "plc", "PLC", DEVICE_PLC);
  int drive = plc_topology_addDevice(&topo, "drive", "驱动器", DEVICE_DRIVE);
  int io = plc_topology_addDevice(&topo, "io", "IO", DEVICE_IO_MODULE);

  /* 建立连接 */
  int conn1 = plc_topology_connect(&topo, plc, drive, COMM_PROTOCOL_ETHERCAT, "eth0");
  ASSERT(conn1 == 0, "PLC → 驱动器 EtherCAT 连接");

  int conn2 = plc_topology_connect(&topo, plc, io, COMM_PROTOCOL_MODBUS_TCP, "192.168.1.100:502");
  ASSERT(conn2 == 1, "PLC → IO Modbus TCP 连接");

  int conn3 = plc_topology_connect(&topo, drive, io, COMM_PROTOCOL_VIRTUAL, "VIRTUAL");
  ASSERT(conn3 == 2, "驱动器 → IO 虚拟连接");

  ASSERT(topo.connectionCount == 3, "总共 3 条连接");

  /* 检查连接参数 */
  ASSERT(topo.connections[0].protocol == COMM_PROTOCOL_ETHERCAT, "连接 1 协议 EtherCAT");
  ASSERT(topo.connections[0].fromDevice == plc, "连接 1 源 PLC");
  ASSERT(topo.connections[0].toDevice == drive, "连接 1 目标驱动");

  ASSERT(topo.connections[1].protocol == COMM_PROTOCOL_MODBUS_TCP, "连接 2 协议 Modbus TCP");
  ASSERT(strcmp(topo.connections[1].endpoint, "192.168.1.100:502") == 0,
         "连接 2 端点正确");

  return 0;
}

static int test_virtual_communication()
{
  PlcDeviceTopology topo;
  plc_topology_init(&topo);

  int plc = plc_topology_addDevice(&topo, "plc", "PLC", DEVICE_PLC);
  int io = plc_topology_addDevice(&topo, "io", "IO", DEVICE_IO_MODULE);

  /* 添加变量 */
  int r1 = plc_device_addVar(&topo.devices[plc], "nOutput", 0x1000, 4, false);
  ASSERT(r1 == DEVICE_OK, "PLC 添加输出变量");
  int r2 = plc_device_addVar(&topo.devices[io], "nInput", 0x1000, 4, true);
  ASSERT(r2 == DEVICE_OK, "IO 添加输入变量");

  /* 设置虚拟连接 */
  int conn = plc_topology_connect(&topo, plc, io, COMM_PROTOCOL_VIRTUAL, "VIRTUAL");
  ASSERT(conn == 0, "虚拟连接建立");

  /* 发送数据 */
  uint8_t testData[] = {0x01, 0x02, 0x03, 0x04, 0x05};
  int sendRc = plc_topology_send(&topo, plc, io, testData, 5);
  ASSERT(sendRc == DEVICE_OK, "发送数据成功");

  /* 检查发送端缓冲区 */
  ASSERT(topo.connections[0].txLen == 5, "发送缓冲区长度 5");
  ASSERT(topo.connections[0].txBuf[0] == 0x01, "发送缓冲区数据正确");
  ASSERT(topo.connections[0].txCount == 1, "发送计数 1");
  ASSERT(topo.connections[0].txBytes == 5, "发送字节数 5");

  /* 广播 */
  int addIo = plc_topology_addDevice(&topo, "io2", "IO2", DEVICE_IO_MODULE);
  plc_topology_connect(&topo, plc, addIo, COMM_PROTOCOL_VIRTUAL, "VIRTUAL");
  int bcastRc = plc_topology_broadcast(&topo, plc, testData, 5);
  ASSERT(bcastRc == DEVICE_OK, "广播成功");

  /* 心跳更新 */
  plc_topology_tick(&topo);
  ASSERT(topo.connections[0].quality >= 0, "通信质量已更新");

  /* 通信统计 */
  uint32_t txBytes, rxBytes, txPkts, rxPkts;
  float quality;
  int statsRc = plc_topology_getStats(&topo, conn, &txBytes, &rxBytes,
                                       &txPkts, &rxPkts, &quality);
  ASSERT(statsRc == DEVICE_OK, "获取通信统计");

  return 0;
}

static int test_serialize_deserialize()
{
  PlcDeviceTopology topo;
  plc_topology_init(&topo);

  plc_topology_addDevice(&topo, "plc", "主控制器", DEVICE_PLC);
  plc_topology_addDevice(&topo, "drive", "伺服1", DEVICE_DRIVE);
  plc_topology_addDevice(&topo, "cam", "相机", DEVICE_VISION);

  plc_topology_connect(&topo, 0, 1, COMM_PROTOCOL_ETHERCAT, "eth1");
  plc_topology_connect(&topo, 0, 2, COMM_PROTOCOL_MODBUS_TCP, "192.168.1.10:502");

  plc_topology_setPos(&topo, 0, 100, 200);
  plc_topology_setPos(&topo, 1, 300, 200);
  plc_topology_setPos(&topo, 2, 500, 100);

  /* 序列化到 JSON */
  char json[4096];
  int serLen = plc_topology_serialize(&topo, json, sizeof(json));
  ASSERT(serLen > 0, "JSON 序列化成功");
  ASSERT(strstr(json, "\"plc\"") != NULL, "JSON 包含设备 ID plc");
  ASSERT(strstr(json, "\"eth1\"") != NULL, "JSON 包含端点 eth1");

  /* 反序列化到新拓扑 */
  PlcDeviceTopology topo2;
  int desLen = plc_topology_deserialize(&topo2, json);
  ASSERT(desLen == 0, "JSON 反序列化成功");
  ASSERT(topo2.deviceCount == 3, "恢复 3 台设备");
  ASSERT(topo2.connectionCount == 2, "恢复 2 条连接");

  /* 验证设备属性 */
  int idx = plc_topology_findById(&topo2, "drive");
  ASSERT(idx == 1, "反序列化后 drive 索引正确");
  ASSERT(strcmp(topo2.devices[idx].name, "伺服1") == 0, "反序列化后名称正确");
  ASSERT(topo2.devices[idx].type == DEVICE_DRIVE, "反序列化后类型正确");

  /* 验证连接 */
  ASSERT(topo2.connections[0].protocol == COMM_PROTOCOL_ETHERCAT,
         "反序列化后连接协议正确");

  return 0;
}

static int test_edge_cases()
{
  PlcDeviceTopology topo;
  plc_topology_init(&topo);

  /* 添加超过限制的设备 */
  int rc;
  char id[32], name[64];
  for (int i = 0; i < PLC_MAX_DEVICES + 5; i++) {
    snprintf(id, sizeof(id), "dev-%d", i);
    snprintf(name, sizeof(name), "设备-%d", i);
    rc = plc_topology_addDevice(&topo, id, name, DEVICE_PLC);
  }
  ASSERT(topo.deviceCount == PLC_MAX_DEVICES, "设备数不超过最大限制");
  ASSERT(rc == DEVICE_ERR_FULL, "超出限制返回 FULL");

  /* 无效参数 */
  rc = plc_topology_addDevice(NULL, "test", "test", DEVICE_PLC);
  ASSERT(rc == DEVICE_ERR_PARAM, "NULL 拓扑返回 PARAM");

  rc = plc_topology_addDevice(&topo, NULL, "test", DEVICE_PLC);
  ASSERT(rc == DEVICE_ERR_PARAM, "NULL ID 返回 PARAM");

  rc = plc_topology_connect(&topo, -1, 0, COMM_PROTOCOL_VIRTUAL, "VIRTUAL");
  ASSERT(rc == DEVICE_ERR_NOT_FOUND, "无效索引连接返回 NOT_FOUND");

  /* 删除设备 */
  plc_topology_init(&topo);
  plc_topology_addDevice(&topo, "a", "A", DEVICE_PLC);
  plc_topology_addDevice(&topo, "b", "B", DEVICE_PLC);
  plc_topology_addDevice(&topo, "c", "C", DEVICE_PLC);
  plc_topology_connect(&topo, 0, 1, COMM_PROTOCOL_VIRTUAL, "V");
  plc_topology_connect(&topo, 0, 2, COMM_PROTOCOL_VIRTUAL, "V");

  rc = plc_topology_removeDevice(&topo, 0);
  ASSERT(rc == DEVICE_OK, "删除设备 0");
  ASSERT(topo.deviceCount == 2, "删除后剩 2 台设备");
  ASSERT(topo.connectionCount == 0, "删除后关联连接也被删除");

  /* 变量操作 */
  PlcDevice dev;
  memset(&dev, 0, sizeof(dev));
  rc = plc_device_setVar(&dev, "test", 42.5f);
  ASSERT(rc == DEVICE_ERR_NOT_FOUND, "设置不存在的变量返回 NOT_FOUND");

  plc_device_addVar(&dev, "test", 0, 4, true);
  rc = plc_device_setVar(&dev, "test", 42.5f);
  ASSERT(rc == DEVICE_OK, "设置变量成功");
  float val = plc_device_getVar(&dev, "test");
  ASSERT(fabsf(val - 42.5f) < 0.001f, "获取变量值 42.5");

  val = plc_device_getVar(&dev, "nonexistent");
  ASSERT(val == 0.0f, "获取不存在的变量返回 0");

  return 0;
}

int main()
{
  int failed = 0;

  printf("===== RTE 多设备拓扑测试 =====\n\n");

  printf("--- 测试 1: 创建拓扑 ---\n");
  failed += test_create_topology();

  printf("\n--- 测试 2: 设备连接 ---\n");
  failed += test_connect_devices();

  printf("\n--- 测试 3: 虚拟通信 ---\n");
  failed += test_virtual_communication();

  printf("\n--- 测试 4: 序列化/反序列化 ---\n");
  failed += test_serialize_deserialize();

  printf("\n--- 测试 5: 边界条件 ---\n");
  failed += test_edge_cases();

  printf("\n================================\n");
  printf("结果: %s (%d 个测试失败)\n", failed ? "失败" : "全部通过", failed);

  return failed;
}
