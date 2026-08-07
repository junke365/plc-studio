#!/usr/bin/env python3
"""
ROS 2 能力桥接服务 (Smart PLC Studio)

在安装了 ROS 2 的机器(或 QEMU 虚拟机)上运行, 把 ROS 2 能力以 HTTP/WebSocket
暴露给 PLC Studio 宿主后端。宿主后端通过环境变量 ROS2_BRIDGE_URL 连接。

HTTP 端点:
  GET  /nodes /topics /services /actions /topic-type?topic=X /status
  POST /publish {topic, type, message}
  POST /call    {service, type, request}

WebSocket 端点:
  ws://<host>:<port>/ws
  发送: {"type":"subscribe","topic":"/xxx"} / {"type":"unsubscribe","topic":"/xxx"}
  接收: {"event":"subscribed"|"message"|"error", "data":...}

架构: 独立 rclpy 线程拥有节点, 通过任务队列与 asyncio 交互(rosbridge 模式)。
"""

import argparse
import asyncio
import base64
import hashlib
import importlib
import json
import queue
import signal
import sys
import threading
import traceback

import rclpy
from rclpy.node import Node
from rclpy.qos import QoSProfile, ReliabilityPolicy
from rclpy.executors import SingleThreadedExecutor

from rosidl_runtime_py.convert import message_to_ordereddict


class Ros2BridgeNode(Node):
    def __init__(self):
        super().__init__("smart_plc_ros2_bridge")
        self.subs = {}
        self.pubs = {}
        self.svc_clients = {}
        self.svc_futures = {}
        self.on_message = None

    # ---------- 基础能力 ----------
    def list_nodes(self):
        return [
            f"{ns}{n}" if ns == "/" else f"{ns}/{n}"
            for ns, n in self.get_node_names_and_namespaces()
        ]

    def list_topics(self):
        return [
            {"name": name, "types": types}
            for name, types in sorted(self.get_topic_names_and_types())
            if name.startswith("/")
        ]

    def list_services(self):
        return [
            name for name, _ in sorted(self.get_service_names_and_types())
            if name.startswith("/")
        ]

    def list_actions(self):
        return [
            name for name, _ in sorted(self.get_action_names_and_types())
            if name.startswith("/")
        ]

    def topic_type(self, topic):
        for name, types in self.get_topic_names_and_types():
            if name == topic:
                return types
        return []

    # ---------- 发布 ----------
    def publish(self, topic, type_name, message):
        msg_cls = resolve_msg_class(type_name)
        if msg_cls is None:
            raise ValueError(f"无法解析消息类型: {type_name}")
        key = (topic, type_name)
        pub = self.pubs.get(key)
        if pub is None:
            qos = QoSProfile(depth=10, reliability=ReliabilityPolicy.BEST_EFFORT)
            pub = self.create_publisher(msg_cls, topic, qos)
            self.pubs[key] = pub
        msg = msg_cls()
        apply_dict(msg, message)
        for _ in range(3):
            pub.publish(msg)
        return True

    # ---------- 调服务(在 spin 线程异步轮询) ----------
    def start_service_call(self, service, type_name, request):
        srv_cls = resolve_service_class(type_name)
        if srv_cls is None:
            raise ValueError(f"无法解析服务类型: {type_name}")
        key = (service, type_name)
        client = self.svc_clients.get(key)
        if client is None:
            client = self.create_client(srv_cls, service)
            self.svc_clients[key] = client
        if not client.service_is_ready():
            raise TimeoutError(f"服务未就绪: {service}")
        req = srv_cls.Request()
        apply_dict(req, request)
        future = client.call_async(req)
        self.svc_futures[key] = (future, service)
        return True

    def poll_service_futures(self):
        results = []
        for key, (future, service) in list(self.svc_futures.items()):
            if future.done():
                del self.svc_futures[key]
                result = future.result()
                results.append(
                    {"event": "service_result", "data": {
                        "service": service,
                        "result": message_to_ordereddict(result) if result is not None else {"error": "调用失败"}}}
                )
        return results

    # ---------- 订阅 ----------
    def subscribe(self, topic):
        if topic in self.subs:
            return True
        type_names = self.topic_type(topic)
        if not type_names:
            return False
        msg_cls = resolve_msg_class(type_names[0])
        if msg_cls is None:
            return False
        qos = QoSProfile(depth=10, reliability=ReliabilityPolicy.BEST_EFFORT)
        sub = self.create_subscription(
            msg_cls, topic,
            lambda m, t=topic: self._on_msg(t, m), qos,
        )
        self.subs[topic] = sub
        return True

    def unsubscribe(self, topic):
        sub = self.subs.pop(topic, None)
        if sub is not None:
            self.destroy_subscription(sub)

    def _on_msg(self, topic, msg):
        if self.on_message is not None:
            self.on_message(
                {"event": "message", "data": {"topic": topic, "payload": message_to_ordereddict(msg)}}
            )


# ================= rclpy 工作线程 =================

class Ros2Worker:
    """在独立线程中运行节点与 executor, 通过队列接收任务。"""

    def __init__(self):
        self.node = None
        self.executor = None
        self.tasks = queue.Queue()
        self._stop = threading.Event()

    def run(self, on_message):
        rclpy.init()
        node = Ros2BridgeNode()
        node.on_message = on_message
        executor = SingleThreadedExecutor()
        executor.add_node(node)
        self.node = node
        self.executor = executor
        while not self._stop.is_set():
            try:
                self.tasks.get_nowait()()
            except queue.Empty:
                pass
            executor.spin_once(timeout_sec=0.05)
            for event in node.poll_service_futures():
                on_message(event)
        executor.shutdown()
        node.destroy_node()
        rclpy.shutdown()

    def submit(self, fn, *args):
        result = queue.Queue()
        def job():
            try:
                result.put(("ok", fn(*args)))
            except Exception as e:
                result.put(("err", e))
        self.tasks.put(job)
        return result

    def stop(self):
        self._stop.set()


def resolve_msg_class(type_name):
    parts = type_name.split("/")
    if len(parts) == 2:
        pkg, name = parts
    elif len(parts) == 3 and parts[1] == "msg":
        pkg, _, name = parts
    else:
        return None
    module = importlib.import_module(f"{pkg}.msg")
    return getattr(module, name)


def resolve_service_class(type_name):
    parts = type_name.split("/")
    if len(parts) == 2:
        pkg, name = parts
    elif len(parts) == 3 and parts[1] == "srv":
        pkg, _, name = parts
    else:
        return None
    module = importlib.import_module(f"{pkg}.srv")
    return getattr(module, name)


def apply_dict(msg, data):
    for key, value in (data or {}).items():
        if not hasattr(msg, key):
            continue
        try:
            target = getattr(msg, key)
            if is_msg(target):
                apply_dict(target, value)
            else:
                setattr(msg, key, value)
        except Exception:
            pass


def is_msg(v):
    return hasattr(v, "get_fields_and_field_types")


def log_exc(where):
    print(f"[ros2-bridge] {where} 异常:\n{traceback.format_exc()}", flush=True)


# ================= HTTP / WS 实现 =================

WS_MAGIC = b"258EAFA5-E914-47DA-95CA-C5AB0DC85B11"


def ws_accept(key: bytes) -> bytes:
    return base64.b64encode(hashlib.sha1(key + WS_MAGIC).digest())


def ws_frame(payload: bytes) -> bytes:
    if not isinstance(payload, bytes):
        payload = payload.encode()
    header = bytearray([0x81])
    n = len(payload)
    if n < 126:
        header.append(n)
    elif n < 65536:
        header.append(126)
        header += n.to_bytes(2, "big")
    else:
        header.append(127)
        header += n.to_bytes(8, "big")
    return bytes(header) + payload


async def read_headers(reader):
    result = {}
    while True:
        line = await reader.readline()
        if line in (b"\r\n", b"\n", b""):
            break
        text = line.decode(errors="ignore").strip()
        if ":" in text:
            k, v = text.split(":", 1)
            result[k.lower()] = v.strip()
    return result


async def read_body(reader, headers=None):
    content_length = 0
    if headers:
        try:
            content_length = int(headers.get("content-length", 0))
        except ValueError:
            content_length = 0
    if content_length:
        return await reader.read(content_length)
    buf = b""
    while True:
        chunk = await reader.read(4096)
        if not chunk:
            break
        buf += chunk
        if len(buf) > 4 * 1024 * 1024:
            break
    return buf


class BridgeApp:
    """持有 worker 与 WS 客户端注册表。"""

    def __init__(self, worker):
        self.worker = worker
        self.loop = None
        self.clients = []

    def set_loop(self, loop):
        self.loop = loop

    def on_ros_event(self, event):
        if self.loop is not None and not self.loop.is_closed():
            self.loop.call_soon_threadsafe(self._dispatch, event)

    def _dispatch(self, event):
        for client in list(self.clients):
            try:
                client["queue"].put_nowait(json.dumps(event))
            except Exception:
                pass

    async def run_job(self, fn, *args):
        result_queue = self.worker.submit(fn, *args)
        loop = asyncio.get_running_loop()
        fut = loop.create_future()
        def check():
            try:
                status, value = result_queue.get_nowait()
                if status == "ok":
                    fut.set_result(value)
                else:
                    fut.set_exception(value)
            except queue.Empty:
                pass
            except Exception as e:
                if not fut.done():
                    fut.set_exception(e)
        def poll():
            check()
            if not fut.done():
                self.loop.call_later(0.05, poll)
        self.loop.call_soon(poll)
        return await fut


async def handle_http(reader, writer, app, request_line=None, headers=None):
    try:
        if request_line is None:
            request_line = await asyncio.wait_for(reader.readline(), timeout=10)
        if not request_line:
            writer.close()
            return
        if headers is None:
            headers = await read_headers(reader)
        parts = request_line.decode().strip().split()
        if len(parts) < 2:
            writer.close()
            return
        method, target = parts[0], parts[1]
        path_only = target.split("?")[0]
        query = {}
        if "?" in target:
            for kv in target.split("?")[1].split("&"):
                if "=" in kv:
                    k, v = kv.split("=", 1)
                    query[k] = v

        status = 200
        result = None
        try:
            if method == "GET" and path_only == "/nodes":
                result = await app.run_job(app.worker.node.list_nodes)
            elif method == "GET" and path_only == "/topics":
                result = await app.run_job(app.worker.node.list_topics)
            elif method == "GET" and path_only == "/services":
                result = await app.run_job(app.worker.node.list_services)
            elif method == "GET" and path_only == "/actions":
                result = await app.run_job(app.worker.node.list_actions)
            elif method == "GET" and path_only == "/topic-type":
                result = await app.run_job(app.worker.node.topic_type, query.get("topic", ""))
            elif method == "GET" and path_only == "/status":
                result = {"strategy": "bridge", "status": "ok",
                          "node": app.worker.node.get_name()}
            elif method == "POST" and path_only == "/publish":
                body = json.loads(await read_body(reader, headers))
                await app.run_job(
                    app.worker.node.publish,
                    body.get("topic"), body.get("type"), body.get("message") or {})
                result = {"success": True}
            elif method == "POST" and path_only == "/call":
                body = json.loads(await read_body(reader, headers))
                await app.run_job(
                    app.worker.node.start_service_call,
                    body.get("service"), body.get("type"), body.get("request") or {})
                result = {"accepted": True}
            else:
                status = 404
                result = {"error": f"unknown {method} {path_only}"}
        except Exception as e:
            status = 500
            result = {"error": str(e)}

        payload = json.dumps(result).encode()
        reason = b"OK" if status == 200 else b"Error"
        header = (
            b"HTTP/1.1 %d %s\r\n"
            b"Content-Type: application/json\r\n"
            b"Content-Length: %d\r\n"
            b"Access-Control-Allow-Origin: *\r\n"
            b"Access-Control-Allow-Methods: GET,POST,OPTIONS\r\n"
            b"Access-Control-Allow-Headers: Content-Type\r\n"
            b"\r\n" % (status, reason, len(payload))
        )
        writer.write(header + payload)
        await writer.drain()
    except Exception:
        log_exc("HTTP")
        try:
            writer.close()
        except Exception:
            pass


async def handle_ws(reader, writer, app, request_line=None, headers=None):
    try:
        if request_line is None:
            request_line = await asyncio.wait_for(reader.readline(), timeout=10)
        if not request_line:
            writer.close()
            return
        if headers is None:
            headers = await read_headers(reader)
        key = headers.get("sec-websocket-key", "")
        if not key:
            writer.close()
            return
        writer.write(
            b"HTTP/1.1 101 Switching Protocols\r\n"
            b"Upgrade: websocket\r\n"
            b"Connection: Upgrade\r\n"
            b"Sec-WebSocket-Accept: " + ws_accept(key.encode()) + b"\r\n\r\n"
        )
        await writer.drain()

        queue = asyncio.Queue()
        client = {"queue": queue}
        app.clients.append(client)

        async def sender():
            try:
                while True:
                    item = await queue.get()
                    await writer.write(ws_frame(item))
                    await writer.drain()
            except Exception:
                pass

        send_task = asyncio.ensure_future(sender())
        try:
            while True:
                head = await reader.read(2)
                if len(head) < 2:
                    break
                opcode = head[0] & 0x0F
                length = head[1] & 0x7F
                if length == 126:
                    length = int.from_bytes(await reader.read(2), "big")
                elif length == 127:
                    length = int.from_bytes(await reader.read(8), "big")
                masked = head[1] & 0x80
                mask = await reader.read(4) if masked else b""
                data = await reader.read(length)
                if masked:
                    data = bytes(b ^ mask[i % 4] for i, b in enumerate(data))
                if opcode == 8:
                    break
                if opcode != 1:
                    continue
                try:
                    msg = json.loads(data.decode())
                except Exception:
                    continue
                mtype = msg.get("type")
                topic = msg.get("topic")
                if mtype == "subscribe":
                    ok = await app.run_job(app.worker.node.subscribe, topic)
                    await writer.write(ws_frame(json.dumps(
                        {"event": "subscribed" if ok else "error",
                         "data": topic if ok else f"订阅失败: {topic}"})))
                    await writer.drain()
                elif mtype == "unsubscribe":
                    await app.run_job(app.worker.node.unsubscribe, topic)
        finally:
            if client in app.clients:
                app.clients.remove(client)
            send_task.cancel()
    except Exception:
        log_exc("WS")
        try:
            writer.close()
        except Exception:
            pass


async def route_conn(reader, writer, app):
    try:
        request_line = await asyncio.wait_for(reader.readline(), timeout=10)
        if not request_line:
            writer.close()
            return
        headers = await read_headers(reader)
        if headers.get("upgrade", "").lower() == "websocket":
            await handle_ws(reader, writer, app, request_line, headers)
        else:
            await handle_http(reader, writer, app, request_line, headers)
    except Exception:
        log_exc("route")
        try:
            writer.close()
        except Exception:
            pass


async def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", type=int, default=9090)
    parser.add_argument("--host", default="0.0.0.0")
    args = parser.parse_args()

    worker = Ros2Worker()
    app = BridgeApp(worker)
    app.set_loop(asyncio.get_running_loop())
    threading.Thread(target=worker.run, args=(app.on_ros_event,), daemon=True).start()

    server = await asyncio.start_server(
        lambda r, w: route_conn(r, w, app), args.host, args.port
    )
    print(f"[ros2-bridge] HTTP+WS 桥接已启动 http://{args.host}:{args.port}", flush=True)

    stop = asyncio.Event()
    for sig in (signal.SIGINT, signal.SIGTERM):
        try:
            loop = asyncio.get_running_loop()
            loop.add_signal_handler(sig, stop.set)
        except NotImplementedError:
            pass
    async with server:
        await stop.wait()
    worker.stop()


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        sys.exit(0)
