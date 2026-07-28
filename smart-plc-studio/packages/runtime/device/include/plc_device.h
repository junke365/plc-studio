#ifndef PLC_DEVICE_H
#define PLC_DEVICE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PLC_DEVICE_NAME_MAX   64
#define PLC_DEVICE_ID_MAX     32
#define PLC_MAX_DEVICES       64
#define PLC_MAX_CONNECTIONS   128
#define PLC_MAX_VARIABLES     1024

/* ==================== 设备类型 ==================== */
typedef enum {
  DEVICE_PLC,                /* PLC 控制器 */
  DEVICE_HMI,                /* HMI 人机界面 */
  DEVICE_DRIVE,              /* 伺服/步进驱动器 */
  DEVICE_SENSOR,             /* 传感器 */
  DEVICE_VISION,             /* 视觉系统 (OpenCV5) */
  DEVICE_ROBOT,              /* 机器人 (PX4/SOFA) */
  DEVICE_IO_MODULE,          /* I/O 模块 */
  DEVICE_GATEWAY,            /* 网关 */
  DEVICE_VIRTUAL,            /* 虚拟设备 (仿真) */
} PlcDeviceType;

/* ==================== 通信协议 ==================== */
typedef enum {
  COMM_PROTOCOL_NONE = 0,
  COMM_PROTOCOL_ETHERCAT,     /* EtherCAT */
  COMM_PROTOCOL_CANOPEN,      /* CANopen */
  COMM_PROTOCOL_MODBUS_TCP,   /* Modbus TCP */
  COMM_PROTOCOL_MODBUS_RTU,   /* Modbus RTU */
  COMM_PROTOCOL_OPC_UA,       /* OPC UA */
  COMM_PROTOCOL_MQTT,         /* MQTT */
  COMM_PROTOCOL_TCP_SOCKET,   /* 自定义 TCP */
  COMM_PROTOCOL_UDP_MULTICAST,/* UDP 组播 */
  COMM_PROTOCOL_SERIAL,       /* 串口直连 */
  COMM_PROTOCOL_VIRTUAL,      /* 虚拟环回 (同一进程) */
} CommProtocol;

/* ==================== 设备状态 ==================== */
typedef enum {
  DEVICE_STATUS_OFFLINE = 0,
  DEVICE_STATUS_ONLINE,
  DEVICE_STATUS_RUNNING,
  DEVICE_STATUS_ERROR,
  DEVICE_STATUS_BUSY,
} PlcDeviceStatus;

/* ==================== 设备变量 ==================== */
typedef struct {
  char name[64];
  uint32_t addr;
  uint32_t size;
  bool isInput;
  bool isOutput;
  float valueF;
  int32_t valueI;
  bool valueB;
  uint8_t raw[64];
} PlcDeviceVar;

#define PLC_COMM_BUF_SIZE    4096

/* ==================== 设备连接 ==================== */
typedef struct {
  int fromDevice;
  int toDevice;
  CommProtocol protocol;
  char endpoint[128];         /* 连接端点: IP:port, 串口名, VIRTUAL */
  uint32_t baudRate;
  uint32_t cycleMs;           /* 通信周期 */
  bool isRedundancy;          /* 冗余连接 */
  float quality;              /* 通信质量 0~1 */
  /* 虚拟通信缓冲区 */
  uint8_t txBuf[PLC_COMM_BUF_SIZE];
  uint32_t txLen;
  uint8_t rxBuf[PLC_COMM_BUF_SIZE];
  uint32_t rxLen;
  uint64_t lastTxTick;        /* 上次发送时间戳 */
  uint64_t lastRxTick;        /* 上次接收时间戳 */
  uint32_t txCount;           /* 发送包计数 */
  uint32_t rxCount;           /* 接收包计数 */
  uint32_t txBytes;           /* 发送字节数 */
  uint32_t rxBytes;           /* 接收字节数 */
  uint32_t errCount;          /* 错误计数 */
} PlcDeviceConnection;

/* ==================== PLC 设备 ==================== */
typedef struct {
  char id[PLC_DEVICE_ID_MAX];
  char name[PLC_DEVICE_NAME_MAX];
  PlcDeviceType type;
  PlcDeviceStatus status;
  uint8_t nodeId;             /* 网络节点 ID */
  char ipAddress[32];
  uint16_t port;
  /* 变量空间 */
  PlcDeviceVar vars[PLC_MAX_VARIABLES];
  uint32_t varCount;
  /* 程序/任务 */
  uint32_t taskCount;
  float cpuUsage;
  float memUsage;
  float cycleTimeMs;
  /* 位置 (编辑器画布坐标) */
  float canvasX, canvasY;
} PlcDevice;

/* ==================== 设备拓扑 ==================== */
typedef struct {
  PlcDevice devices[PLC_MAX_DEVICES];
  int deviceCount;
  PlcDeviceConnection connections[PLC_MAX_CONNECTIONS];
  int connectionCount;
} PlcDeviceTopology;

/* ==================== API ==================== */

/* 拓扑管理 */
void plc_topology_init(PlcDeviceTopology *topo);
int plc_topology_addDevice(PlcDeviceTopology *topo, const char *id, const char *name, PlcDeviceType type);
int plc_topology_removeDevice(PlcDeviceTopology *topo, int deviceIdx);
int plc_topology_connect(PlcDeviceTopology *topo, int from, int to, CommProtocol proto, const char *endpoint);
int plc_topology_disconnect(PlcDeviceTopology *topo, int connIdx);

/* 设备查找 */
int plc_topology_findById(PlcDeviceTopology *topo, const char *id);
int plc_topology_findByName(PlcDeviceTopology *topo, const char *name);

/* 设备控制 */
int plc_device_setVar(PlcDevice *dev, const char *name, float value);
float plc_device_getVar(const PlcDevice *dev, const char *name);
int plc_device_addVar(PlcDevice *dev, const char *name, uint32_t addr, uint32_t size, bool isInput);

/* 通信 */
int plc_topology_send(PlcDeviceTopology *topo, int fromIdx, int toIdx, const uint8_t *data, uint32_t len);
int plc_topology_broadcast(PlcDeviceTopology *topo, int fromIdx, const uint8_t *data, uint32_t len);

/* 序列化/反序列化 (用于编辑器保存/加载) */
int plc_topology_serialize(const PlcDeviceTopology *topo, char *json, uint32_t maxLen);
int plc_topology_deserialize(PlcDeviceTopology *topo, const char *json);

/* 编辑器布局 */
void plc_topology_setPos(PlcDeviceTopology *topo, int deviceIdx, float x, float y);

/* 通信统计查询 */
int plc_topology_getStats(PlcDeviceTopology *topo, int connIdx,
                           uint32_t *txBytes, uint32_t *rxBytes,
                           uint32_t *txPkts, uint32_t *rxPkts,
                           float *quality);

/* 拓扑心跳 (更新通信质量) */
void plc_topology_tick(PlcDeviceTopology *topo);

/* 错误码 */
#define DEVICE_OK           0
#define DEVICE_ERR_FULL     -1
#define DEVICE_ERR_NOT_FOUND -2
#define DEVICE_ERR_PARAM    -3
#define DEVICE_ERR_COMM     -4

#ifdef __cplusplus
}
#endif

#endif /* PLC_DEVICE_H */
