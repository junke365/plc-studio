# API 参考

## Server API

### 项目管理

#### GET /api/projects

获取项目列表。

**响应：**
```json
{
  "success": true,
  "data": [
    {
      "id": "abc123",
      "name": "My PLC Project",
      "path": "/projects/my-plc-project",
      "createdAt": "2024-01-15T10:30:00Z",
      "updatedAt": "2024-01-15T14:20:00Z"
    }
  ]
}
```

#### POST /api/projects

创建新项目。

**请求：**
```json
{
  "name": "My PLC Project",
  "template": "plc-basic",
  "path": "/projects/my-plc-project"
}
```

**响应：**
```json
{
  "success": true,
  "data": {
    "id": "abc123",
    "name": "My PLC Project",
    "path": "/projects/my-plc-project"
  }
}
```

#### GET /api/projects/:id

获取项目详情。

#### PUT /api/projects/:id

更新项目配置。

#### DELETE /api/projects/:id

删除项目。

### 文件操作

#### GET /api/projects/:id/files

获取项目文件树。

#### GET /api/projects/:id/files/:path

读取文件内容。

#### PUT /api/projects/:id/files/:path

保存文件内容。

**请求：**
```json
{
  "content": "PROGRAM Main\nVAR\n  x : INT;\nEND_VAR\nEND_PROGRAM"
}
```

### PLC 编译

#### POST /api/projects/:id/compile

编译 PLC 程序。

**响应：**
```json
{
  "success": true,
  "data": {
    "output": "/projects/my-plc-project/export/",
    "errors": [],
    "warnings": []
  }
}
```

### 运行时控制

#### POST /api/runtime/connect

连接运行时。

**请求：**
```json
{
  "host": "192.168.1.100",
  "port": 502,
  "protocol": "modbus"
}
```

#### POST /api/runtime/start

启动运行时。

#### POST /api/runtime/stop

停止运行时。

#### GET /api/runtime/status

获取运行时状态。

**响应：**
```json
{
  "success": true,
  "data": {
    "state": "running",
    "scanTime": 10,
    "cpuUsage": 45.2,
    "memoryUsage": 67.8
  }
}
```

### 变量操作

#### GET /api/runtime/variables

获取变量列表。

#### PUT /api/runtime/variables/:name

修改变量值。

**请求：**
```json
{
  "value": 100,
  "force": true
}
```

### 调试

#### POST /api/debug/breakpoint

设置断点。

**请求：**
```json
{
  "file": "main.st",
  "line": 42
}
```

#### POST /api/debug/step

单步执行。

#### GET /api/debug/stack

获取调用栈。

#### GET /api/debug/variables

获取调试变量值。

## WebSocket API

### 连接

```javascript
const ws = new WebSocket('ws://localhost:3000/debug');
```

### 事件

#### variable_update

变量值更新。

```json
{
  "type": "variable_update",
  "data": {
    "name": "counter",
    "value": 42,
    "timestamp": 1705315200000
  }
}
```

#### scan_complete

扫描周期完成。

```json
{
  "type": "scan_complete",
  "data": {
    "scanTime": 10.5,
    "taskCount": 3
  }
}
```

#### error

运行时错误。

```json
{
  "type": "error",
  "data": {
    "code": "RUNTIME_ERROR",
    "message": "Stack overflow in task 1",
    "file": "main.prg",
    "line": 128
  }
}
```

## Client SDK

### JavaScript

```javascript
import { PLCClient } from '@smart-plc/client';

const client = new PLCClient({
  host: 'localhost',
  port: 3000
});

// 连接
await client.connect();

// 读取变量
const value = await client.readVariable('counter');

// 写入变量
await client.writeVariable('counter', 100);

// 监听变量
client.on('variable_update', (name, value) => {
  console.log(`${name}: ${value}`);
});
```

### TypeScript

```typescript
import { PLCClient, Variable } from '@smart-plc/client';

const client = new PLCClient({
  host: 'localhost',
  port: 3000
});

interface PLCVariables {
  counter: number;
  enable: boolean;
  speed: number;
}

await client.connect<PLCVariables>();

const counter = await client.readVariable('counter'); // number
```

## Runtime C API

### 初始化

```c
#include "plc_runtime.h"

// 初始化运行时
PLC_Result plc_init(const PLC_Config *config);

// 启动扫描
PLC_Result plc_start(void);

// 停止扫描
PLC_Result plc_stop(void);

// 运行扫描周期
PLC_Result plc_scan(void);
```

### 变量操作

```c
// 读取变量
PLC_Result plc_var_read(const char *name, void *value, uint32_t size);

// 写入变量
PLC_Result plc_var_write(const char *name, const void *value, uint32_t size);

// 强制变量
PLC_Result plc_var_force(const char *name, const void *value);

// 取消强制
PLC_Result plc_var_unforce(const char *name);
```

### I/O 操作

```c
// 读取数字输入
int plc_di_read(uint32_t channel);

// 写入数字输出
PLC_Result plc_do_write(uint32_t channel, int value);

// 读取模拟输入
int plc_ai_read(uint32_t channel);

// 写入模拟输出
PLC_Result plc_ao_write(uint32_t channel, int value);
```

### 通信

```c
// Modbus TCP 客户端
PLC_Result plc_modbus_tcp_connect(const char *host, uint16_t port);
PLC_Result plc_modbus_tcp_read_registers(uint16_t addr, uint16_t count, uint16_t *regs);
PLC_Result plc_modbus_tcp_write_registers(uint16_t addr, uint16_t count, const uint16_t *regs);

// EtherCAT
PLC_Result plc_ethercat_init(const char *iface);
PLC_Result plc_ethercat_process(void);
```

### HMI

```c
// 创建控件
PLC_HMI_Widget* plc_hmi_create_widget(PLC_HMI_Type type);

// 设置属性
PLC_Result plc_hmi_set_property(PLC_HMI_Widget *widget, const char *prop, const void *value);

// 绑定变量
PLC_Result plc_hmi_bind_variable(PLC_HMI_Widget *widget, const char *varname);

// 渲染
PLC_Result plc_hmi_render(void);
```

## 错误码

| 错误码 | 说明 |
|--------|------|
| PLC_OK | 成功 |
| PLC_ERR_INVALID_PARAM | 无效参数 |
| PLC_ERR_MEMORY | 内存错误 |
| PLC_ERR_TIMEOUT | 超时 |
| PLC_ERR_NOT_FOUND | 未找到 |
| PLC_ERR_IO | I/O 错误 |
| PLC_ERR_PROTOCOL | 协议错误 |
| PLC_ERR_RUNTIME | 运行时错误 |
