import type { FastifyInstance } from 'fastify'
import { spawn } from 'child_process'
import { getStatus } from './ros2.adapter.js'

interface TopicSub {
  proc: any
  clients: Set<any>
}

const subs = new Map<string, TopicSub>()
const echoProcs = new Set<any>()

function killAll() {
  for (const { proc } of subs.values()) {
    try { proc.kill() } catch {}
  }
  subs.clear()
}

export function createRos2Gateway(fastify: FastifyInstance) {
  fastify.get('/ws/ros2', { websocket: true }, (conn: any) => {
    const send = (data: any) => {
      try {
        const ws = conn.socket || conn
        const msg = JSON.stringify(data)
        if (typeof ws.send === 'function') ws.send(msg)
        else conn.write(msg)
      } catch {}
    }

    send({ event: 'status', data: 'connected' })

    conn.on('message', async (raw: Buffer | string) => {
      let msg: any
      try {
        msg = JSON.parse(String(raw))
      } catch {
        send({ event: 'error', data: '消息必须是 JSON' })
        return
      }
      const st = await getStatus()
      if (st.strategy === 'none') {
        send({ event: 'error', data: 'ROS 2 未连接' })
        return
      }

      if (msg.type === 'subscribe') {
        const topic: string = msg.topic
        if (!topic || !topic.startsWith('/')) {
          send({ event: 'error', data: '无效 topic' })
          return
        }
        const existing = subs.get(topic)
        if (existing) {
          existing.clients.add(conn)
          send({ event: 'subscribed', data: topic })
          return
        }
        const entry: TopicSub = { proc: null, clients: new Set([conn]) }
        subs.set(topic, entry)

        if (st.strategy === 'bridge') {
          send({ event: 'subscribed', data: topic })
          return
        }

        // CLI 策略：持续 echo 并转发
        const proc = spawn('ros2', ['topic', 'echo', topic], { shell: false })
        entry.proc = proc
        echoProcs.add(proc)
        proc.on('error', (e: any) => {
          send({ event: 'error', data: `echo 启动失败: ${e?.message ?? e}` })
          subs.delete(topic)
          echoProcs.delete(proc)
        })
        let buf = ''
        proc.stdout.on('data', (chunk: Buffer) => {
          buf += chunk.toString()
          const lines = buf.split('\n')
          buf = lines.pop() ?? ''
          for (const line of lines) {
            const trimmed = line.trim()
            if (!trimmed) continue
            let data: unknown
            try {
              data = JSON.parse(trimmed)
            } catch {
              data = trimmed
            }
            for (const client of entry.clients) {
              try {
                const ws = client.socket || client
                const payload = JSON.stringify({ event: 'message', data: { topic, payload: data } })
                if (typeof ws.send === 'function') ws.send(payload)
                else client.write(payload)
              } catch {}
            }
          }
        })
        proc.stderr.on('data', (chunk: Buffer) => {
          for (const client of entry.clients) {
            try {
              const ws = client.socket || client
              const payload = JSON.stringify({ event: 'message', data: { topic, payload: chunk.toString().trim() } })
              if (typeof ws.send === 'function') ws.send(payload)
              else client.write(payload)
            } catch {}
          }
        })
        proc.on('close', () => {
          echoProcs.delete(proc)
        })
        send({ event: 'subscribed', data: topic })
      } else if (msg.type === 'unsubscribe') {
        const entry = subs.get(msg.topic)
        if (entry) {
          entry.clients.delete(conn)
          if (entry.clients.size === 0) {
            try { entry.proc?.kill() } catch {}
            subs.delete(msg.topic)
          }
        }
      }
    })

    conn.on('close', () => {
      for (const [topic, entry] of [...subs.entries()]) {
        if (entry.clients.has(conn)) {
          entry.clients.delete(conn)
          if (entry.clients.size === 0) {
            try { entry.proc?.kill() } catch {}
            subs.delete(topic)
          }
        }
      }
    })
  })
}

export function closeRos2Gateway() {
  killAll()
  for (const p of echoProcs) {
    try { p.kill() } catch {}
  }
  echoProcs.clear()
}
