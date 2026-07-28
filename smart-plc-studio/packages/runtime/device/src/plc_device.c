#include "plc_device.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static uint64_t deviceTick = 0;

void plc_topology_init(PlcDeviceTopology *topo)
{
  memset(topo, 0, sizeof(PlcDeviceTopology));
}

int plc_topology_addDevice(PlcDeviceTopology *topo, const char *id, const char *name, PlcDeviceType type)
{
  if (!topo || !id || !name) return DEVICE_ERR_PARAM;
  if (topo->deviceCount >= PLC_MAX_DEVICES) return DEVICE_ERR_FULL;
  int idx = topo->deviceCount++;
  PlcDevice *dev = &topo->devices[idx];
  memset(dev, 0, sizeof(PlcDevice));
  strncpy(dev->id, id, PLC_DEVICE_ID_MAX - 1);
  strncpy(dev->name, name, PLC_DEVICE_NAME_MAX - 1);
  dev->type = type;
  dev->status = DEVICE_STATUS_ONLINE;
  dev->nodeId = idx;
  return idx;
}

int plc_topology_removeDevice(PlcDeviceTopology *topo, int deviceIdx)
{
  if (!topo || deviceIdx < 0 || deviceIdx >= topo->deviceCount) return DEVICE_ERR_NOT_FOUND;
  for (int i = deviceIdx; i < topo->deviceCount - 1; i++)
    topo->devices[i] = topo->devices[i + 1];
  topo->deviceCount--;
  for (int i = 0; i < topo->connectionCount; i++) {
    if (topo->connections[i].fromDevice == deviceIdx ||
        topo->connections[i].toDevice == deviceIdx) {
      plc_topology_disconnect(topo, i);
      i--;
    }
  }
  return DEVICE_OK;
}

int plc_topology_connect(PlcDeviceTopology *topo, int from, int to, CommProtocol proto, const char *endpoint)
{
  if (!topo) return DEVICE_ERR_PARAM;
  if (from < 0 || from >= topo->deviceCount || to < 0 || to >= topo->deviceCount)
    return DEVICE_ERR_NOT_FOUND;
  if (topo->connectionCount >= PLC_MAX_CONNECTIONS) return DEVICE_ERR_FULL;
  int idx = topo->connectionCount++;
  PlcDeviceConnection *conn = &topo->connections[idx];
  memset(conn, 0, sizeof(PlcDeviceConnection));
  conn->fromDevice = from;
  conn->toDevice = to;
  conn->protocol = proto;
  if (endpoint)
    strncpy(conn->endpoint, endpoint, sizeof(conn->endpoint) - 1);
  conn->baudRate = 115200;
  conn->cycleMs = 10;
  conn->quality = 1.0f;
  return idx;
}

int plc_topology_disconnect(PlcDeviceTopology *topo, int connIdx)
{
  if (!topo || connIdx < 0 || connIdx >= topo->connectionCount) return DEVICE_ERR_NOT_FOUND;
  for (int i = connIdx; i < topo->connectionCount - 1; i++)
    topo->connections[i] = topo->connections[i + 1];
  topo->connectionCount--;
  return DEVICE_OK;
}

int plc_topology_findById(PlcDeviceTopology *topo, const char *id)
{
  if (!topo || !id) return -1;
  for (int i = 0; i < topo->deviceCount; i++)
    if (strcmp(topo->devices[i].id, id) == 0) return i;
  return -1;
}

int plc_topology_findByName(PlcDeviceTopology *topo, const char *name)
{
  if (!topo || !name) return -1;
  for (int i = 0; i < topo->deviceCount; i++)
    if (strcmp(topo->devices[i].name, name) == 0) return i;
  return -1;
}

int plc_device_addVar(PlcDevice *dev, const char *name, uint32_t addr, uint32_t size, bool isInput)
{
  if (!dev || !name) return DEVICE_ERR_PARAM;
  if (dev->varCount >= PLC_MAX_VARIABLES) return DEVICE_ERR_FULL;
  int idx = dev->varCount++;
  strncpy(dev->vars[idx].name, name, sizeof(dev->vars[idx].name) - 1);
  dev->vars[idx].addr = addr;
  dev->vars[idx].size = size;
  dev->vars[idx].isInput = isInput;
  dev->vars[idx].isOutput = !isInput;
  return idx;
}

int plc_device_setVar(PlcDevice *dev, const char *name, float value)
{
  if (!dev || !name) return DEVICE_ERR_PARAM;
  for (uint32_t i = 0; i < dev->varCount; i++) {
    if (strcmp(dev->vars[i].name, name) == 0) {
      dev->vars[i].valueF = value;
      dev->vars[i].valueI = (int32_t)value;
      dev->vars[i].valueB = (value != 0);
      return DEVICE_OK;
    }
  }
  return DEVICE_ERR_NOT_FOUND;
}

float plc_device_getVar(const PlcDevice *dev, const char *name)
{
  if (!dev || !name) return 0;
  for (uint32_t i = 0; i < dev->varCount; i++)
    if (strcmp(dev->vars[i].name, name) == 0)
      return dev->vars[i].valueF;
  return 0;
}

int plc_topology_send(PlcDeviceTopology *topo, int fromIdx, int toIdx, const uint8_t *data, uint32_t len)
{
  if (!topo || !data || len == 0) return DEVICE_ERR_PARAM;
  if (fromIdx < 0 || fromIdx >= topo->deviceCount) return DEVICE_ERR_NOT_FOUND;
  PlcDevice *fromDev = &topo->devices[fromIdx];

  for (int i = 0; i < topo->connectionCount; i++) {
    PlcDeviceConnection *conn = &topo->connections[i];
    if (conn->fromDevice == fromIdx) {
      int targetIdx = (conn->toDevice == toIdx || toIdx < 0) ? conn->toDevice : -1;
      if (targetIdx < 0 && toIdx >= 0) continue;

      if (len > PLC_COMM_BUF_SIZE) len = PLC_COMM_BUF_SIZE;
      memcpy(conn->txBuf, data, len);
      conn->txLen = len;
      conn->lastTxTick = deviceTick;
      conn->txCount++;
      conn->txBytes += len;
      conn->quality = 1.0f;

      if (conn->protocol == COMM_PROTOCOL_VIRTUAL && targetIdx >= 0) {
        PlcDeviceConnection *rxConn = NULL;
        for (int j = 0; j < topo->connectionCount; j++) {
          if (topo->connections[j].fromDevice == targetIdx &&
              topo->connections[j].toDevice == fromIdx &&
              topo->connections[j].protocol == COMM_PROTOCOL_VIRTUAL) {
            rxConn = &topo->connections[j];
            break;
          }
        }
        if (!rxConn) {
          for (int j = 0; j < topo->connectionCount; j++) {
            if (topo->connections[j].toDevice == targetIdx &&
                topo->connections[j].fromDevice == fromIdx) {
              rxConn = &topo->connections[j];
              break;
            }
          }
        }
        if (rxConn) {
          memcpy(rxConn->rxBuf, data, len);
          rxConn->rxLen = len;
          rxConn->lastRxTick = deviceTick;
          rxConn->rxCount++;
          rxConn->rxBytes += len;
        }
      }

      /* 同步变量到目标设备 */
      if (targetIdx >= 0 && targetIdx < topo->deviceCount) {
        PlcDevice *targetDev = &topo->devices[targetIdx];
        PlcDeviceVar *vars = fromDev->vars;
        for (uint32_t v = 0; v < fromDev->varCount; v++) {
          for (uint32_t tv = 0; tv < targetDev->varCount; tv++) {
            if (strcmp(vars[v].name, targetDev->vars[tv].name) == 0) {
              targetDev->vars[tv].valueF = vars[v].valueF;
              targetDev->vars[tv].valueI = vars[v].valueI;
              targetDev->vars[tv].valueB = vars[v].valueB;
              break;
            }
          }
        }
      }
    }
  }

  deviceTick++;
  return DEVICE_OK;
}

int plc_topology_broadcast(PlcDeviceTopology *topo, int fromIdx, const uint8_t *data, uint32_t len)
{
  if (!topo) return DEVICE_ERR_PARAM;
  for (int i = 0; i < topo->connectionCount; i++) {
    if (topo->connections[i].fromDevice == fromIdx) {
      plc_topology_send(topo, fromIdx, topo->connections[i].toDevice, data, len);
    }
  }
  return DEVICE_OK;
}

int plc_topology_serialize(const PlcDeviceTopology *topo, char *json, uint32_t maxLen)
{
  if (!topo || !json) return DEVICE_ERR_PARAM;
  uint32_t pos = 0;
  pos += snprintf(json + pos, maxLen - pos,
    "{\"version\":1,\"devices\":[");
  for (int i = 0; i < topo->deviceCount; i++) {
    if (i > 0) pos += snprintf(json + pos, maxLen - pos, ",");
    pos += snprintf(json + pos, maxLen - pos,
      "{\"id\":\"%s\",\"name\":\"%s\",\"type\":%d,\"typeName\":\"%s\","
      "\"nodeId\":%u,\"ip\":\"%s\",\"port\":%u,"
      "\"x\":%.1f,\"y\":%.1f,\"vars\":%u,\"tasks\":%u}",
      topo->devices[i].id, topo->devices[i].name,
      topo->devices[i].type,
      topo->devices[i].type == DEVICE_PLC ? "PLC" :
      topo->devices[i].type == DEVICE_HMI ? "HMI" :
      topo->devices[i].type == DEVICE_DRIVE ? "DRIVE" :
      topo->devices[i].type == DEVICE_SENSOR ? "SENSOR" :
      topo->devices[i].type == DEVICE_VISION ? "VISION" :
      topo->devices[i].type == DEVICE_ROBOT ? "ROBOT" :
      topo->devices[i].type == DEVICE_IO_MODULE ? "IO" :
      topo->devices[i].type == DEVICE_GATEWAY ? "GATEWAY" :
      topo->devices[i].type == DEVICE_VIRTUAL ? "VIRTUAL" : "UNKNOWN",
      topo->devices[i].nodeId,
      topo->devices[i].ipAddress[0] ? topo->devices[i].ipAddress : "",
      topo->devices[i].port,
      topo->devices[i].canvasX, topo->devices[i].canvasY,
      topo->devices[i].varCount, topo->devices[i].taskCount);
  }
  pos += snprintf(json + pos, maxLen - pos, "],\"connections\":[");
  for (int i = 0; i < topo->connectionCount; i++) {
    if (i > 0) pos += snprintf(json + pos, maxLen - pos, ",");
    pos += snprintf(json + pos, maxLen - pos,
      "{\"from\":%d,\"to\":%d,\"protocol\":%d,\"protoName\":\"%s\","
      "\"endpoint\":\"%s\",\"baud\":%u,\"cycleMs\":%u}",
      topo->connections[i].fromDevice, topo->connections[i].toDevice,
      topo->connections[i].protocol,
      topo->connections[i].protocol == COMM_PROTOCOL_ETHERCAT ? "EtherCAT" :
      topo->connections[i].protocol == COMM_PROTOCOL_CANOPEN ? "CANopen" :
      topo->connections[i].protocol == COMM_PROTOCOL_MODBUS_TCP ? "ModbusTCP" :
      topo->connections[i].protocol == COMM_PROTOCOL_MODBUS_RTU ? "ModbusRTU" :
      topo->connections[i].protocol == COMM_PROTOCOL_OPC_UA ? "OPCUA" :
      topo->connections[i].protocol == COMM_PROTOCOL_MQTT ? "MQTT" :
      topo->connections[i].protocol == COMM_PROTOCOL_TCP_SOCKET ? "TCP" :
      topo->connections[i].protocol == COMM_PROTOCOL_UDP_MULTICAST ? "UDP" :
      topo->connections[i].protocol == COMM_PROTOCOL_SERIAL ? "Serial" :
      topo->connections[i].protocol == COMM_PROTOCOL_VIRTUAL ? "Virtual" : "None",
      topo->connections[i].endpoint,
      topo->connections[i].baudRate, topo->connections[i].cycleMs);
  }
  pos += snprintf(json + pos, maxLen - pos, "]}");
  return pos;
}

static int skipWhitespace(const char **p)
{
  while (**p == ' ' || **p == '\t' || **p == '\n' || **p == '\r') (*p)++;
  return **p;
}

static int matchChar(const char **p, char c)
{
  skipWhitespace(p);
  if (**p == c) { (*p)++; return 1; }
  return 0;
}

static int parseString(const char **p, char *out, int maxLen)
{
  skipWhitespace(p);
  if (**p != '"') return 0;
  (*p)++;
  int len = 0;
  while (**p && **p != '"' && len < maxLen - 1) {
    if (**p == '\\') { (*p)++; if (**p) { out[len++] = **p; (*p)++; } }
    else { out[len++] = **p; (*p)++; }
  }
  out[len] = 0;
  if (**p == '"') (*p)++;
  return 1;
}

static int parseInt(const char **p, int *val)
{
  skipWhitespace(p);
  int sign = 1;
  if (**p == '-') { sign = -1; (*p)++; }
  else if (**p == '+') (*p)++;
  if (**p < '0' || **p > '9') return 0;
  *val = 0;
  while (**p >= '0' && **p <= '9') { *val = *val * 10 + (**p - '0'); (*p)++; }
  *val *= sign;
  return 1;
}

static int parseUint(const char **p, uint32_t *val)
{
  skipWhitespace(p);
  if (**p < '0' || **p > '9') return 0;
  *val = 0;
  while (**p >= '0' && **p <= '9') { *val = *val * 10 + (**p - '0'); (*p)++; }
  return 1;
}

int plc_topology_deserialize(PlcDeviceTopology *topo, const char *json)
{
  if (!topo || !json) return DEVICE_ERR_PARAM;
  plc_topology_init(topo);

  const char *p = json;
  skipWhitespace(&p);
  if (*p != '{') return DEVICE_ERR_PARAM;
  p++;

  while (*p && *p != '}') {
    char key[64];
    if (!parseString(&p, key, sizeof(key))) break;
    if (!matchChar(&p, ':')) break;

    if (strcmp(key, "devices") == 0) {
      if (!matchChar(&p, '[')) break;
      while (*p && *p != ']') {
        if (!matchChar(&p, '{')) break;
        char id[64] = "", name[64] = "", ip[32] = "";
        int type = 0, nodeId = 0, port = 0, vCount = 0, tCount = 0;
        float x = 0, y = 0;
        while (*p && *p != '}') {
          char k[64];
          if (!parseString(&p, k, sizeof(k))) break;
          if (!matchChar(&p, ':')) break;
          if (strcmp(k, "id") == 0) parseString(&p, id, sizeof(id));
          else if (strcmp(k, "name") == 0) parseString(&p, name, sizeof(name));
          else if (strcmp(k, "type") == 0) parseInt(&p, &type);
          else if (strcmp(k, "nodeId") == 0) parseInt(&p, &nodeId);
          else if (strcmp(k, "ip") == 0) parseString(&p, ip, sizeof(ip));
          else if (strcmp(k, "port") == 0) parseInt(&p, &port);
          else if (strcmp(k, "x") == 0) { int iv; parseInt(&p, &iv); x = (float)iv; }
          else if (strcmp(k, "y") == 0) { int iv; parseInt(&p, &iv); y = (float)iv; }
          else { while (*p && *p != ',' && *p != '}') p++; }
          matchChar(&p, ',');
        }
        matchChar(&p, '}');
        if (id[0] && name[0]) {
          int idx = plc_topology_addDevice(topo, id, name, (PlcDeviceType)type);
          if (idx >= 0) {
            topo->devices[idx].nodeId = nodeId;
            topo->devices[idx].port = port;
            strncpy(topo->devices[idx].ipAddress, ip, sizeof(topo->devices[idx].ipAddress) - 1);
            topo->devices[idx].canvasX = x;
            topo->devices[idx].canvasY = y;
          }
        }
        matchChar(&p, ',');
      }
      matchChar(&p, ']');
    } else if (strcmp(key, "connections") == 0) {
      if (!matchChar(&p, '[')) break;
      while (*p && *p != ']') {
        if (!matchChar(&p, '{')) break;
        int from = 0, to = 0, proto = 0;
        char endpoint[128] = "";
        while (*p && *p != '}') {
          char k[64];
          if (!parseString(&p, k, sizeof(k))) break;
          if (!matchChar(&p, ':')) break;
          if (strcmp(k, "from") == 0) parseInt(&p, &from);
          else if (strcmp(k, "to") == 0) parseInt(&p, &to);
          else if (strcmp(k, "protocol") == 0) parseInt(&p, &proto);
          else if (strcmp(k, "endpoint") == 0) parseString(&p, endpoint, sizeof(endpoint));
          else { while (*p && *p != ',' && *p != '}') p++; }
          matchChar(&p, ',');
        }
        matchChar(&p, '}');
        plc_topology_connect(topo, from, to, (CommProtocol)proto, endpoint);
        matchChar(&p, ',');
      }
      matchChar(&p, ']');
    } else {
      while (*p && *p != ',' && *p != '}') p++;
    }
    matchChar(&p, ',');
  }

  return DEVICE_OK;
}

void plc_topology_setPos(PlcDeviceTopology *topo, int deviceIdx, float x, float y)
{
  if (!topo || deviceIdx < 0 || deviceIdx >= topo->deviceCount) return;
  topo->devices[deviceIdx].canvasX = x;
  topo->devices[deviceIdx].canvasY = y;
}

int plc_topology_getStats(PlcDeviceTopology *topo, int connIdx,
                           uint32_t *txBytes, uint32_t *rxBytes,
                           uint32_t *txPkts, uint32_t *rxPkts,
                           float *quality)
{
  if (!topo || connIdx < 0 || connIdx >= topo->connectionCount) return DEVICE_ERR_NOT_FOUND;
  PlcDeviceConnection *c = &topo->connections[connIdx];
  if (txBytes) *txBytes = c->txBytes;
  if (rxBytes) *rxBytes = c->rxBytes;
  if (txPkts) *txPkts = c->txCount;
  if (rxPkts) *rxPkts = c->rxCount;
  if (quality) *quality = c->quality;
  return DEVICE_OK;
}

void plc_topology_tick(PlcDeviceTopology *topo)
{
  deviceTick++;
  for (int i = 0; i < topo->connectionCount; i++) {
    PlcDeviceConnection *c = &topo->connections[i];
    if (deviceTick - c->lastRxTick > 1000) {
      c->quality *= 0.99f;
    }
  }
}
