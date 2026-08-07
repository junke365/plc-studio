import * as THREE from "three";

export type ToolKind = "forceps" | "scalpel";

export class SurgeryTool extends THREE.Group {
  tip = new THREE.Object3D();
  kind: ToolKind;
  jaw = 0.75;
  jawTarget = 0.75;
  private glowMat: THREE.MeshStandardMaterial;
  private jawL: THREE.Group;
  private jawR: THREE.Group;

  constructor(kind: ToolKind = "forceps") {
    super();
    this.kind = kind;

    const metal = new THREE.MeshStandardMaterial({ color: 0x9aa4b0, roughness: 0.35, metalness: 0.85 });
    const dark = new THREE.MeshStandardMaterial({ color: 0x2a303a, roughness: 0.5, metalness: 0.4 });
    this.glowMat = new THREE.MeshStandardMaterial({ color: 0x66ffcc, emissive: 0x00ffaa, emissiveIntensity: 1.6 });

    const tipSphere = new THREE.Mesh(new THREE.SphereGeometry(0.0035, 16, 12), this.glowMat);
    this.add(tipSphere);
    this.add(this.tip);

    if (kind === "forceps") {
      const handle = new THREE.Mesh(new THREE.CylinderGeometry(0.006, 0.008, 0.09, 16), dark);
      handle.rotation.x = Math.PI / 2;
      handle.position.set(0, 0, -0.05);
      this.add(handle);

      this.jawL = new THREE.Group();
      this.jawR = new THREE.Group();
      for (const [grp, side] of [[this.jawL, -1], [this.jawR, 1]] as const) {
        const jaw = new THREE.Mesh(new THREE.BoxGeometry(0.004, 0.016, 0.022), metal);
        jaw.position.set(side * 0.004, 0, 0.012);
        jaw.rotation.z = side * 0.12;
        grp.add(jaw);
        const tipPt = new THREE.Mesh(new THREE.ConeGeometry(0.0022, 0.008, 8), metal);
        tipPt.rotation.x = -Math.PI / 2;
        tipPt.position.set(side * 0.004, 0, 0.028);
        grp.add(tipPt);
        this.add(grp);
      }
    } else {
      const handle = new THREE.Mesh(new THREE.CylinderGeometry(0.006, 0.008, 0.1, 16), dark);
      handle.rotation.x = Math.PI / 2;
      handle.position.set(0, 0, -0.055);
      this.add(handle);
      const blade = new THREE.Mesh(new THREE.BoxGeometry(0.0035, 0.018, 0.02), metal);
      blade.position.set(0, 0.004, 0.012);
      blade.rotation.z = 0.35;
      this.add(blade);
      this.jawL = this.jawR = new THREE.Group();
    }
  }

  setJaw(open01: number) {
    if (this.kind !== "forceps") return;
    this.jawTarget = open01;
    const a = (open01 - 0.5) * 0.35;
    this.jawL.rotation.z = 0.12 + a;
    this.jawR.rotation.z = -0.12 - a;
  }

  setColor(c: number) {
    this.glowMat.color.setHex(c);
    this.glowMat.emissive.setHex(c);
  }

  orient(tipPos: THREE.Vector3, camPos: THREE.Vector3) {
    this.position.copy(tipPos);
    const dir = new THREE.Vector3().subVectors(camPos, tipPos).normalize();
    this.quaternion.setFromUnitVectors(new THREE.Vector3(0, 0, -1), dir);
  }
}
