import type { FastifyInstance } from 'fastify'
import { getCv, processImage, type ProcessRequest } from './vision.service.js'
import { getModelsDir, listYoloModels } from './yolo.service.js'

export async function createVisionRoutes(fastify: FastifyInstance) {
  // 可用 YOLO 模型列表（models 目录下 .onnx）
  fastify.get('/api/vision/models', async () => ({
    models: listYoloModels(),
    dir: getModelsDir(),
  }))
  // OpenCV 5 版本与能力
  fastify.get('/api/vision/version', async () => {
    try {
      const cv = await getCv()
      const info = cv.getBuildInformation()
      const version = (info.match(/Version control:\s*([^\n]+)/i)?.[1] ?? 'unknown').trim()
      return {
        available: true,
        version: version || '5.x (WASM)',
        build: info.split('\n').slice(0, 3).join(' | '),
      }
    } catch (err: any) {
      return { available: false, error: err?.message ?? String(err) }
    }
  })

  // 通用视觉处理管线
  fastify.post('/api/vision/process', async (req, reply) => {
    const body = req.body as ProcessRequest
    if (!body?.image) {
      return reply.code(400).send({ success: false, error: '缺少 image（base64 PNG/JPEG）' })
    }
    try {
      const data = await processImage(body)
      return { success: true, data }
    } catch (err: any) {
      return reply.code(400).send({ success: false, error: err?.message ?? String(err) })
    }
  })

  // 便捷接口：模板匹配定位
  fastify.post('/api/vision/locate', async (req, reply) => {
    const body = req.body as ProcessRequest
    if (!body?.image || !body?.template) {
      return reply.code(400).send({ success: false, error: '缺少 image / template' })
    }
    try {
      const data = await processImage({
        image: body.image,
        ops: [{ type: 'matchTemplate', template: body.template, method: body.method, threshold: body.threshold }],
      })
      return { success: true, data }
    } catch (err: any) {
      return reply.code(400).send({ success: false, error: err?.message ?? String(err) })
    }
  })
}
