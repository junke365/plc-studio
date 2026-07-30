/**
 * 编译 ST → C 代码，写入 runtime/generated/
 * 用法: node scripts/gen-firmware.mjs [st-file]
 */

import { readFileSync, writeFileSync, mkdirSync } from 'fs'
import { join, dirname } from 'path'
import { fileURLToPath } from 'url'

// 加载 plc-core 编译器
import { compileStCode } from '../../plc-core/dist/src/index.js'

const __dirname = dirname(fileURLToPath(import.meta.url))
const GENERATED_DIR = join(__dirname, '..', 'generated')

function main() {
  const stFile = process.argv[2] || join(GENERATED_DIR, 'sample.st')
  console.log(`[gen] 读取 ST 程序: ${stFile}`)
  const source = readFileSync(stFile, 'utf-8')

  console.log('[gen] 编译中...')
  const result = compileStCode(source)

  if (result.errors.length > 0) {
    for (const err of result.errors) {
      console.log(`  ${err.severity.toUpperCase()}: ${err.message} (L${err.line}:${err.column})`)
    }
    if (result.errors.some(e => e.severity === 'error')) {
      console.error('[gen] 编译失败，存在错误')
      process.exit(1)
    }
  }

  if (result.units.length === 0) {
    console.error('[gen] 未解析到任何 POU')
    process.exit(1)
  }

  console.log(`[gen] 解析到 ${result.units.length} 个 POU:`)
  for (const u of result.units) {
    console.log(`  - ${u.type} ${u.name}`)
  }

  // 写入 generated/
  mkdirSync(GENERATED_DIR, { recursive: true })

  const files = {
    'plc_generated.h': result.generatedH,
    'generated_init.c': result.generatedInit,
    'generated_main.c': result.generatedMain,
  }

  for (const [name, content] of Object.entries(files)) {
    const path = join(GENERATED_DIR, name)
    writeFileSync(path, content, 'utf-8')
    console.log(`[gen] 写入 ${path} (${content.length} bytes)`)
  }

  console.log('[gen] ST → C 编译完成')
}

main()
