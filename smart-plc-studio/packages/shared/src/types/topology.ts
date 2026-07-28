export interface TopologyDevice {
  id: string
  name: string
  type: string
  x: number
  y: number
  config?: Record<string, unknown>
}

export interface TopologyLink {
  id: string
  sourceId: string
  targetId: string
  protocol: string
}
