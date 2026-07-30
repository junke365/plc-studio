import type { FastifyInstance } from 'fastify'
import { BuildService } from './build.service.js'

const buildService = new BuildService()

export async function createBuildRoutes(fastify: FastifyInstance) {
  // 获取构建环境信息
  fastify.get('/info', async () => {
    const info = await buildService.getBuildInfo()
    return info
  })

  // 编译固件
  fastify.post<{
    Body: { platform: string }
  }>('/compile', async (request, reply) => {
    const { platform } = request.body
    if (!['stm32', 'esp32', 'linux-arm', 'linux-x86'].includes(platform)) {
      reply.status(400)
      return { success: false, error: `不支持的平台: ${platform}` }
    }
    const result = await buildService.compile(platform)
    if (result.success) {
      return { success: true, output: result.output }
    } else {
      reply.status(500)
      return { success: false, output: result.output, error: result.error }
    }
  })

  // 编译+烧录（ST-Link）
  fastify.post('/flash/stlink', async (_request, reply) => {
    const compile = await buildService.compile('stm32')
    if (!compile.success) {
      reply.status(500)
      return { success: false, step: 'compile', output: compile.output, error: compile.error }
    }
    const flash = await buildService.flashStlink()
    if (flash.success) {
      return { success: true, step: 'flash', output: flash.output }
    } else {
      reply.status(500)
      return { success: false, step: 'flash', output: flash.output, error: flash.error }
    }
  })

  // 编译+烧录（J-Link）
  fastify.post('/flash/jlink', async (_request, reply) => {
    const compile = await buildService.compile('stm32')
    if (!compile.success) {
      reply.status(500)
      return { success: false, step: 'compile', output: compile.output, error: compile.error }
    }
    const flash = await buildService.flashJlink()
    if (flash.success) {
      return { success: true, step: 'flash', output: flash.output }
    } else {
      reply.status(500)
      return { success: false, step: 'flash', output: flash.output, error: flash.error }
    }
  })

  // 编译+烧录（ESP32 UART）
  fastify.post<{
    Body: { port: string }
  }>('/flash/esp32', async (request, reply) => {
    const compile = await buildService.compile('esp32')
    if (!compile.success) {
      reply.status(500)
      return { success: false, step: 'compile', output: compile.output, error: compile.error }
    }
    const flash = await buildService.flashEsp32(request.body.port)
    if (flash.success) {
      return { success: true, step: 'flash', output: flash.output }
    } else {
      reply.status(500)
      return { success: false, step: 'flash', output: flash.output, error: flash.error }
    }
  })

  // 编译+SSH 上传（Linux）
  fastify.post<{
    Body: { host: string; port: number }
  }>('/flash/ssh', async (request, reply) => {
    const compile = await buildService.compile('linux-arm')
    if (!compile.success) {
      reply.status(500)
      return { success: false, step: 'compile', output: compile.output, error: compile.error }
    }
    const transfer = await buildService.sshTransfer(request.body.host, request.body.port)
    if (transfer.success) {
      return { success: true, step: 'transfer', output: transfer.output }
    } else {
      reply.status(500)
      return { success: false, step: 'transfer', output: transfer.output, error: transfer.error }
    }
  })
}
