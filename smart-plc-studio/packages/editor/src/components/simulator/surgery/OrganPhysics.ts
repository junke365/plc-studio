export interface OrganPhysicsDriver {
  readonly name: string;
  readonly vertexCount: number;
  readonly syncedCount: number;
  solve(dt: number): void;
  grab(x: number, y: number, z: number): void;
  setGrabTarget(x: number, y: number, z: number): void;
  release(): void;
  cut(ax: number, ay: number, az: number, bx: number, by: number, bz: number): void;
  reset(): void;
  dispose(): void;
}
