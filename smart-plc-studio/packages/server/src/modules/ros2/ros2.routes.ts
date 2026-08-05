import type { FastifyInstance } from 'fastify'
import {
  getStatus,
  listNodes,
  listTopics,
  listServices,
  listActions,
  getTopicType,
  publishOnce,
  callService,
  bridgeRequest,
} from './ros2.adapter.js'

export async function createRos2Routes(fastify: FastifyInstance) {
  // 连接状态/策略
  fastify.get('/api/ros2/status', async () => {
    return { success: true, data: await getStatus() }
  })

  // 图查询（按当前策略分发）
  fastify.get('/api/ros2/nodes', async () => {
    const st = await getStatus()
    if (st.strategy === 'bridge') {
      return { success: true, data: await bridgeRequest(st.bridgeUrl!, 'nodes') }
    }
    if (st.strategy === 'none') {
      return { success: false, error: 'ROS 2 未连接' }
    }
    return { success: true, data: await listNodes() }
  })

  fastify.get('/api/ros2/topics', async () => {
    const st = await getStatus()
    if (st.strategy === 'bridge') {
      return { success: true, data: await bridgeRequest(st.bridgeUrl!, 'topics') }
    }
    if (st.strategy === 'none') {
      return { success: false, error: 'ROS 2 未连接' }
    }
    return { success: true, data: await listTopics() }
  })

  fastify.get('/api/ros2/services', async () => {
    const st = await getStatus()
    if (st.strategy === 'bridge') {
      return { success: true, data: await bridgeRequest(st.bridgeUrl!, 'services') }
    }
    if (st.strategy === 'none') {
      return { success: false, error: 'ROS 2 未连接' }
    }
    return { success: true, data: await listServices() }
  })

  fastify.get('/api/ros2/actions', async () => {
    const st = await getStatus()
    if (st.strategy === 'bridge') {
      return { success: true, data: await bridgeRequest(st.bridgeUrl!, 'actions') }
    }
    if (st.strategy === 'none') {
      return { success: false, error: 'ROS 2 未连接' }
    }
    return { success: true, data: await listActions() }
  })

  // 话题类型查询
  fastify.get('/api/ros2/topic-type', async (req, reply) => {
    const { topic } = req.query as { topic?: string }
    if (!topic) return reply.code(400).send({ success: false, error: '缺少 topic' })
    try {
      return { success: true, data: await getTopicType(topic) }
    } catch (err: any) {
      return reply.code(400).send({ success: false, error: err?.message ?? String(err) })
    }
  })

  // 发布话题消息
  fastify.post('/api/ros2/publish', async (req, reply) => {
    const { topic, type, message } = req.body as {
      topic?: string; type?: string; message?: Record<string, unknown>
    }
    if (!topic || !type || !message) {
      return reply.code(400).send({ success: false, error: '缺少 topic / type / message' })
    }
    const st = await getStatus()
    try {
      if (st.strategy === 'bridge') {
        await bridgeRequest(st.bridgeUrl!, 'publish', { topic, type, message })
      } else if (st.strategy === 'none') {
        return { success: false, error: 'ROS 2 未连接' }
      } else {
        await publishOnce(topic, type, message)
      }
      return { success: true }
    } catch (err: any) {
      return reply.code(400).send({ success: false, error: err?.message ?? String(err) })
    }
  })

  // 调用服务
  fastify.post('/api/ros2/call', async (req, reply) => {
    const { service, type, request } = req.body as {
      service?: string; type?: string; request?: Record<string, unknown>
    }
    if (!service || !type) {
      return reply.code(400).send({ success: false, error: '缺少 service / type' })
    }
    const st = await getStatus()
    try {
      let result: string
      if (st.strategy === 'bridge') {
        result = JSON.stringify(await bridgeRequest(st.bridgeUrl!, 'call', { service, type, request: request ?? {} }))
      } else if (st.strategy === 'none') {
        return { success: false, error: 'ROS 2 未连接' }
      } else {
        result = await callService(service, type, request ?? {})
      }
      return { success: true, data: result }
    } catch (err: any) {
      return reply.code(400).send({ success: false, error: err?.message ?? String(err) })
    }
  })
}
