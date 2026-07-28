<template>
  <div class="kin-editor">
    <!-- ==================== 左侧：项目浏览器 + 文件浏览 ==================== -->
    <aside class="kin-sidebar">
      <!-- 导航区 -->
      <nav class="sidebar-nav">
        <div
          class="nav-item"
          :class="{ active: sideTab === 'files' }"
          @click="sideTab = 'files'"
        >
          <span class="material-symbols-outlined nav-icon">folder_open</span>
          <span class="nav-label">Files</span>
        </div>
        <div
          class="nav-item"
          :class="{ active: sideTab === 'devices' }"
          @click="sideTab = 'devices'"
        >
          <span class="material-symbols-outlined nav-icon"
            >precision_manufacturing</span
          >
          <span class="nav-label">Devices</span>
        </div>
      </nav>

      <!-- 文件浏览 -->
      <div v-if="sideTab === 'files'" class="file-browser custom-scrollbar">
        <div class="file-section">
          <button
            class="file-section-header"
            @click="toggleFileSection('examples')"
          >
            <span
              class="material-symbols-outlined file-arrow"
              :class="{ collapsed: !fileSections.examples }"
              >expand_more</span
            >
            <span
              class="material-symbols-outlined"
              style="font-size: 14px; color: var(--secondary)"
              >inventory_2</span
            >
            <span>示例文件</span>
          </button>
          <div v-show="fileSections.examples" class="file-list">
            <div
              v-for="f in exampleFiles"
              :key="f.name"
              class="file-item"
              :class="{ active: currentFile?.name === f.name }"
              @click="openFile(f)"
            >
              <span
                class="material-symbols-outlined file-icon"
                :style="{ color: f.color }"
                >{{ f.icon }}</span
              >
              <div class="file-info">
                <div class="file-name">{{ f.name }}</div>
                <div class="file-desc">{{ f.desc }}</div>
              </div>
            </div>
          </div>
        </div>

        <div class="file-section">
          <button
            class="file-section-header"
            @click="toggleFileSection('custom')"
          >
            <span
              class="material-symbols-outlined file-arrow"
              :class="{ collapsed: !fileSections.custom }"
              >expand_more</span
            >
            <span
              class="material-symbols-outlined"
              style="font-size: 14px; color: var(--accent)"
              >folder</span
            >
            <span>自定义运动学</span>
            <button
              class="file-add-btn"
              @click.stop="createNewKin"
              title="新建运动学"
            >
              <span class="material-symbols-outlined" style="font-size: 14px"
                >add</span
              >
            </button>
          </button>
          <div v-show="fileSections.custom" class="file-list">
            <div v-if="customFiles.length === 0" class="file-empty">
              暂无自定义运动学
            </div>
            <div
              v-for="f in customFiles"
              :key="f.name"
              class="file-item"
              :class="{ active: currentFile?.name === f.name }"
              @click="openFile(f)"
            >
              <span
                class="material-symbols-outlined file-icon"
                style="color: var(--tertiary)"
                >settings_suggest</span
              >
              <div class="file-info">
                <div class="file-name">{{ f.name }}</div>
                <div class="file-desc">{{ f.joints.length }} 轴</div>
              </div>
              <button
                class="file-del-btn"
                @click.stop="deleteCustomFile(f.name)"
                title="删除"
              >
                <span class="material-symbols-outlined" style="font-size: 12px"
                  >close</span
                >
              </button>
            </div>
          </div>
        </div>

        <div class="file-section">
          <button
            class="file-section-header"
            @click="toggleFileSection('models')"
          >
            <span
              class="material-symbols-outlined file-arrow"
              :class="{ collapsed: !fileSections.models }"
              >expand_more</span
            >
            <span
              class="material-symbols-outlined"
              style="font-size: 14px; color: var(--warning)"
              >view_in_ar</span
            >
            <span>3D 模型库</span>
          </button>
          <div v-show="fileSections.models" class="file-list">
            <div
              v-for="m in modelFiles"
              :key="m.name"
              class="file-item"
              :class="{ active: selectedModelFile === m.name }"
              @click="selectedModelFile = m.name"
            >
              <span
                class="material-symbols-outlined file-icon"
                style="color: var(--warning)"
                >deployed_code</span
              >
              <div class="file-info">
                <div class="file-name">{{ m.name }}</div>
                <div class="file-desc">{{ m.format }} · {{ m.size }}</div>
              </div>
            </div>
            <div
              class="file-item import-model-item"
              @click="triggerImportModel"
            >
              <span
                class="material-symbols-outlined file-icon"
                style="color: var(--outline)"
                >upload_file</span
              >
              <div class="file-info">
                <div class="file-name">导入 STL/OBJ</div>
                <div class="file-desc">支持 .stl / .obj / .step 格式</div>
              </div>
            </div>
          </div>
        </div>
      </div>

      <!-- 设备树 -->
      <div v-if="sideTab === 'devices'" class="device-tree custom-scrollbar">
        <div class="kin-tree-section">
          <h3 class="tree-title">KINEMATICS TREE</h3>
          <ul class="tree-list">
            <li
              v-for="group in kinGroups"
              :key="group.name"
              class="tree-node"
              :class="{
                expanded: group.expanded,
                selected: selectedGroup === group.name,
              }"
              @click="selectGroup(group.name)"
            >
              <span class="material-symbols-outlined tree-arrow">
                {{ group.expanded ? "expand_more" : "chevron_right" }}
              </span>
              <span class="tree-node-name">{{ group.name }}</span>
            </li>
            <template
              v-for="group in kinGroups"
              :key="group.name + '-children'"
            >
              <li
                v-if="group.expanded && group.children"
                v-for="child in group.children"
                :key="child"
                class="tree-child"
              >
                {{ child }}
              </li>
            </template>
          </ul>
        </div>
      </div>

      <div class="sidebar-footer">
        <button class="build-btn" @click="buildKinematics">Build All</button>
      </div>
    </aside>

    <!-- ==================== 中间内容区 ==================== -->
    <main class="kin-main">
      <!-- 标签栏 -->
      <div class="kin-tabs">
        <div
          class="kin-tab"
          :class="{ active: mainTab === 'overview' }"
          @click="mainTab = 'overview'"
        >
          <span class="material-symbols-outlined" style="font-size: 14px"
            >dashboard</span
          >
          运动学概览
        </div>
        <div
          class="kin-tab"
          :class="{ active: mainTab === 'dh' }"
          @click="mainTab = 'dh'"
        >
          <span class="material-symbols-outlined" style="font-size: 14px"
            >table_chart</span
          >
          DH 参数
        </div>
        <div
          class="kin-tab"
          :class="{ active: mainTab === 'formula' }"
          @click="mainTab = 'formula'"
        >
          <span class="material-symbols-outlined" style="font-size: 14px"
            >functions</span
          >
          运动学公式
        </div>
        <div
          class="kin-tab"
          :class="{ active: mainTab === 'preview' }"
          @click="mainTab = 'preview'"
        >
          <span class="material-symbols-outlined" style="font-size: 14px"
            >view_in_ar</span
          >
          3D 预览
        </div>
      </div>

      <div class="kin-content custom-scrollbar">
        <!-- ====== Tab: 运动学概览 ====== -->
        <div v-if="mainTab === 'overview'" class="overview-content">
          <div class="overview-header">
            <h2 class="overview-title">运动学配置</h2>
            <p class="overview-subtitle">选择机器人类型或新建自定义运动学</p>
          </div>
          <div class="model-grid">
            <div
              v-for="model in kinModels"
              :key="model.name"
              class="model-card"
              :class="{ active: selectedModel === model.name }"
              @click="selectModel(model.name)"
            >
              <div class="card-header">
                <span class="card-title">{{ model.name }}</span>
                <span class="material-symbols-outlined card-badge">
                  {{ selectedModel === model.name ? "check_circle" : "info" }}
                </span>
              </div>
              <div class="card-visual">
                <img
                  v-if="model.image"
                  :src="model.image"
                  :alt="model.name"
                  class="card-visual-img"
                  loading="lazy"
                />
                <span
                  v-else
                  class="material-symbols-outlined card-visual-icon"
                  >{{ model.icon }}</span
                >
              </div>
              <div class="card-footer">
                <span>Type: {{ model.type }}</span>
                <span>Model: {{ model.model }}</span>
              </div>
            </div>
            <!-- 导入占位卡 -->
            <div class="model-card import-card" @click="createNewKin">
              <span class="material-symbols-outlined import-icon">add_box</span>
              <span class="import-label">新建运动学</span>
            </div>
          </div>
        </div>

        <!-- ====== Tab: DH 参数 ====== -->
        <div v-if="mainTab === 'dh'" class="dh-content">
          <div class="dh-header">
            <div>
              <h2 class="dh-title">DH 参数表</h2>
              <p class="dh-subtitle">
                {{ currentFile?.name || "未选择运动学" }} —
                {{ currentFile?.type || "" }}
              </p>
            </div>
            <div class="dh-actions">
              <button
                class="dh-btn"
                @click="addDhJoint"
                :disabled="!currentFile"
              >
                <span class="material-symbols-outlined" style="font-size: 14px"
                  >add</span
                >
                添加关节
              </button>
              <button
                class="dh-btn secondary"
                @click="resetDhTable"
                :disabled="!currentFile"
              >
                <span class="material-symbols-outlined" style="font-size: 14px"
                  >restart_alt</span
                >
                重置
              </button>
            </div>
          </div>

          <!-- DH 说明 -->
          <div class="dh-info-box">
            <span
              class="material-symbols-outlined"
              style="font-size: 16px; color: var(--secondary)"
              >info</span
            >
            <div>
              <strong>DH 参数 (Denavit-Hartenberg)</strong>
              <p>
                θ: 关节角(绕Z轴旋转) &nbsp;|&nbsp; d: 连杆偏移(沿Z轴平移)
                &nbsp;|&nbsp; a: 连杆长度(沿X轴平移) &nbsp;|&nbsp; α:
                连杆扭转(绕X轴旋转)
              </p>
            </div>
          </div>

          <div v-if="currentFile" class="dh-table-wrapper">
            <table class="dh-table">
              <thead>
                <tr>
                  <th>关节</th>
                  <th>类型</th>
                  <th>θ (deg)</th>
                  <th>d (mm)</th>
                  <th>a (mm)</th>
                  <th>α (deg)</th>
                  <th>范围</th>
                  <th></th>
                </tr>
              </thead>
              <tbody>
                <tr
                  v-for="(joint, idx) in currentFile.joints"
                  :key="idx"
                  class="dh-row"
                >
                  <td class="dh-cell joint-cell">
                    <span
                      class="joint-badge"
                      :style="{
                        background: jointColors[idx % jointColors.length],
                      }"
                      >J{{ idx + 1 }}</span
                    >
                  </td>
                  <td class="dh-cell">
                    <select class="dh-select" v-model="joint.type">
                      <option value="revolute">旋转 (R)</option>
                      <option value="prismatic">移动 (P)</option>
                    </select>
                  </td>
                  <td class="dh-cell">
                    <input
                      class="dh-input code-font"
                      type="number"
                      step="0.01"
                      v-model.number="joint.theta"
                      :disabled="joint.type === 'revolute'"
                      :placeholder="joint.type === 'revolute' ? 'θᵢ' : '固定'"
                    />
                  </td>
                  <td class="dh-cell">
                    <input
                      class="dh-input code-font"
                      type="number"
                      step="0.1"
                      v-model.number="joint.d"
                      :placeholder="joint.type === 'prismatic' ? 'dᵢ' : '固定'"
                    />
                  </td>
                  <td class="dh-cell">
                    <input
                      class="dh-input code-font"
                      type="number"
                      step="0.1"
                      v-model.number="joint.a"
                      placeholder="aᵢ"
                    />
                  </td>
                  <td class="dh-cell">
                    <input
                      class="dh-input code-font"
                      type="number"
                      step="0.01"
                      v-model.number="joint.alpha"
                      placeholder="αᵢ"
                    />
                  </td>
                  <td class="dh-cell range-cell">
                    <input
                      class="dh-input code-font range-input"
                      type="number"
                      v-model.number="joint.rangeMin"
                      placeholder="min"
                    />
                    <span class="range-sep">~</span>
                    <input
                      class="dh-input code-font range-input"
                      type="number"
                      v-model.number="joint.rangeMax"
                      placeholder="max"
                    />
                    <span class="range-unit">°</span>
                  </td>
                  <td class="dh-cell">
                    <button
                      class="dh-del-btn"
                      @click="removeDhJoint(idx)"
                      :disabled="currentFile.joints.length <= 1"
                    >
                      <span
                        class="material-symbols-outlined"
                        style="font-size: 14px"
                        >delete</span
                      >
                    </button>
                  </td>
                </tr>
              </tbody>
            </table>
          </div>
          <div v-else class="dh-empty">
            <span
              class="material-symbols-outlined"
              style="font-size: 48px; opacity: 0.2"
              >table_chart</span
            >
            <p>请先在左侧选择一个运动学文件</p>
          </div>

          <!-- DH 变换矩阵 -->
          <div v-if="currentFile" class="dh-matrix-section">
            <h3 class="section-title">
              <span class="material-symbols-outlined" style="font-size: 16px"
                >grid_on</span
              >
              单关节变换矩阵 Tᵢ
            </h3>
            <div class="matrix-display">
              <div class="matrix-formula">
                <span class="formula-line"
                  >Tᵢ = Rot(z, θᵢ) · Trans(z, dᵢ) · Trans(x, aᵢ) · Rot(x,
                  αᵢ)</span
                >
              </div>
              <div class="matrix-grid">
                <div class="matrix-bracket">[</div>
                <div class="matrix-body">
                  <div class="matrix-row">
                    <span>cos θᵢ</span><span>-sin θᵢ cos αᵢ</span
                    ><span>sin θᵢ sin αᵢ</span><span>aᵢ cos θᵢ</span>
                  </div>
                  <div class="matrix-row">
                    <span>sin θᵢ</span><span>cos θᵢ cos αᵢ</span
                    ><span>-cos θᵢ sin αᵢ</span><span>aᵢ sin θᵢ</span>
                  </div>
                  <div class="matrix-row">
                    <span>0</span><span>sin αᵢ</span><span>cos αᵢ</span
                    ><span>dᵢ</span>
                  </div>
                  <div class="matrix-row">
                    <span>0</span><span>0</span><span>0</span><span>1</span>
                  </div>
                </div>
                <div class="matrix-bracket">]</div>
              </div>
            </div>
          </div>
        </div>

        <!-- ====== Tab: 运动学公式 ====== -->
        <div v-if="mainTab === 'formula'" class="formula-content">
          <div class="formula-header">
            <h2 class="formula-title">运动学公式</h2>
            <p class="formula-subtitle">正运动学 (FK) 与逆运动学 (IK) 推导</p>
          </div>

          <div v-if="currentFile" class="formula-sections">
            <!-- 正运动学 -->
            <div class="formula-card">
              <div class="formula-card-header" @click="toggleFormula('fk')">
                <span
                  class="material-symbols-outlined"
                  style="font-size: 18px; color: var(--secondary)"
                  >arrow_forward</span
                >
                <h3>正运动学 (Forward Kinematics)</h3>
                <span
                  class="material-symbols-outlined toggle-arrow"
                  :class="{ collapsed: !expandedFormulas.fk }"
                  >expand_more</span
                >
              </div>
              <div v-show="expandedFormulas.fk" class="formula-card-body">
                <p class="formula-desc">
                  已知各关节角度
                  <strong>q = [θ₁, θ₂, ..., θₙ]</strong>，求末端执行器位姿
                  <strong>T₀ⁿ</strong>
                </p>
                <div class="formula-display">
                  <div class="formula-line main">
                    T₀ⁿ = T₁ · T₂ · T₃ · ... · Tₙ = ∏ᵢ₌₁ⁿ Tᵢ
                  </div>
                </div>
                <div class="formula-display">
                  <div class="formula-line sub">
                    位置: P = [T₀ⁿ(1,4), T₀ⁿ(2,4), T₀ⁿ(3,4)]ᵀ
                  </div>
                </div>
                <div class="formula-display">
                  <div class="formula-line sub">
                    姿态: R = [T₀ⁿ(1:3, 1:3)] (旋转矩阵)
                  </div>
                </div>
                <div
                  v-if="
                    currentFile.type === 'Articulated' ||
                    currentFile.type === 'SCARA'
                  "
                  class="formula-extra"
                >
                  <h4>欧拉角表示</h4>
                  <div class="formula-display">
                    <div class="formula-line">
                      ZYX 欧拉角: R = Rz(α) · Ry(β) · Rx(γ)
                    </div>
                  </div>
                  <div class="formula-display">
                    <div class="formula-line">
                      RPY 角: Roll = atan2(r32, r33), Pitch = atan2(-r31,
                      √(r32²+r33²)), Yaw = atan2(r21, r11)
                    </div>
                  </div>
                </div>
              </div>
            </div>

            <!-- 逆运动学 -->
            <div class="formula-card">
              <div class="formula-card-header" @click="toggleFormula('ik')">
                <span
                  class="material-symbols-outlined"
                  style="font-size: 18px; color: var(--accent)"
                  >arrow_back</span
                >
                <h3>逆运动学 (Inverse Kinematics)</h3>
                <span
                  class="material-symbols-outlined toggle-arrow"
                  :class="{ collapsed: !expandedFormulas.ik }"
                  >expand_more</span
                >
              </div>
              <div v-show="expandedFormulas.ik" class="formula-card-body">
                <p class="formula-desc">
                  已知末端执行器目标位姿 <strong>T₀ⁿ</strong>，求各关节角度
                  <strong>q = [θ₁, θ₂, ..., θₙ]</strong>
                </p>
                <div class="formula-display">
                  <div class="formula-line main">q = f⁻¹(T₀ⁿ)</div>
                </div>
                <div class="ik-methods">
                  <div class="ik-method">
                    <h4>解析法 (Analytical)</h4>
                    <ul>
                      <li>代数法：直接求解封闭形式方程</li>
                      <li>几何法：利用几何关系推导</li>
                      <li>Pieper 方法：适用于 6R 机器人 (满足 Pieper 条件)</li>
                      <li>通常有多个解（肘上/肘下，左手/右手）</li>
                    </ul>
                  </div>
                  <div class="ik-method">
                    <h4>数值法 (Numerical)</h4>
                    <ul>
                      <li>Newton-Raphson 迭代: qₖ₊₁ = qₖ - J⁻¹(qₖ) · e(qₖ)</li>
                      <li>雅可比矩阵 J: ∂T/∂q (速度映射)</li>
                      <li>阻尼最小二乘 (DLS): Δq = Jᵀ(JJᵀ + λ²I)⁻¹e</li>
                      <li>适用于冗余/非解析可解的机构</li>
                    </ul>
                  </div>
                </div>
              </div>
            </div>

            <!-- 特定类型公式 -->
            <div class="formula-card">
              <div
                class="formula-card-header"
                @click="toggleFormula('specific')"
              >
                <span
                  class="material-symbols-outlined"
                  style="font-size: 18px; color: var(--warning)"
                  >category</span
                >
                <h3>{{ currentFile.type }} 特定公式</h3>
                <span
                  class="material-symbols-outlined toggle-arrow"
                  :class="{ collapsed: !expandedFormulas.specific }"
                  >expand_more</span
                >
              </div>
              <div v-show="expandedFormulas.specific" class="formula-card-body">
                <!-- 6轴串联 -->
                <div
                  v-if="
                    currentFile.type === 'Articulated' ||
                    currentFile.type === '串联机器人'
                  "
                >
                  <h4>6 轴串联机器人 (Pieper 方法)</h4>
                  <div class="formula-display">
                    <div class="formula-line sub">
                      前 3 轴决定位置 (θ₁, θ₂, θ₃)：
                    </div>
                  </div>
                  <div class="formula-display">
                    <div class="formula-line">
                      θ₁ = atan2(py, px) - atan2(d₄, √(px² + py² - d₄²))
                    </div>
                  </div>
                  <div class="formula-display">
                    <div class="formula-line">
                      θ₃ = atan2(S₃, C₃), 其中 S₃ = ..., C₃ = (D² - a₂² - a₃²) /
                      (2a₂a₃)
                    </div>
                  </div>
                  <div class="formula-display">
                    <div class="formula-line sub">
                      后 3 轴决定姿态 (θ₄, θ₅, θ₆)：
                    </div>
                  </div>
                  <div class="formula-display">
                    <div class="formula-line">
                      通过 R₃⁶ = R₀³ᵀ · R₀⁶ 求解 ZYZ 欧拉角
                    </div>
                  </div>
                  <div class="formula-display">
                    <div class="formula-line">
                      θ₅ = atan2(√(r31² + r32²), r33)
                    </div>
                  </div>
                </div>
                <!-- SCARA -->
                <div v-else-if="currentFile.type === 'SCARA'">
                  <h4>SCARA 机器人</h4>
                  <div class="formula-display">
                    <div class="formula-line sub">位置 (θ₁, θ₂, d₄)：</div>
                  </div>
                  <div class="formula-display">
                    <div class="formula-line">
                      C₂ = (px² + py² - a₁² - a₂²) / (2a₁a₂)
                    </div>
                  </div>
                  <div class="formula-display">
                    <div class="formula-line">θ₂ = atan2(±√(1-C₂²), C₂)</div>
                  </div>
                  <div class="formula-display">
                    <div class="formula-line">
                      θ₁ = atan2(py, px) - atan2(a₂sin θ₂, a₁ + a₂cos θ₂)
                    </div>
                  </div>
                  <div class="formula-display">
                    <div class="formula-line">d₄ = d_base - pz</div>
                  </div>
                  <div class="formula-display">
                    <div class="formula-line">
                      θ₄ = -(θ₁ + θ₂) + φ (工具姿态角)
                    </div>
                  </div>
                </div>
                <!-- 龙门 -->
                <div
                  v-else-if="
                    currentFile.type === 'Cartesian' ||
                    currentFile.type === '龙门式'
                  "
                >
                  <h4>龙门式 (笛卡尔) 机器人</h4>
                  <div class="formula-display">
                    <div class="formula-line main">
                      直接映射: x = θ₁·pitch₁, y = θ₂·pitch₂, z = θ₃·pitch₃
                    </div>
                  </div>
                  <div class="formula-display">
                    <div class="formula-line sub">
                      变换矩阵为纯平移: T = Trans(x, y, z)
                    </div>
                  </div>
                </div>
                <!-- 并联/Delta -->
                <div
                  v-else-if="
                    currentFile.type === 'Parallel' ||
                    currentFile.type === '并联'
                  "
                >
                  <h4>Delta 并联机器人</h4>
                  <div class="formula-display">
                    <div class="formula-line sub">正运动学 (位置级)：</div>
                  </div>
                  <div class="formula-display">
                    <div class="formula-line">
                      每条支链: |P - Bᵢ - Rᵢ · qᵢ|² = L²
                    </div>
                  </div>
                  <div class="formula-display">
                    <div class="formula-line sub">逆运动学：</div>
                  </div>
                  <div class="formula-display">
                    <div class="formula-line">
                      qᵢ = (P - Bᵢ) / |P - Bᵢ| · cos(βᵢ) ± (1/|P - Bᵢ|) × ...
                    </div>
                  </div>
                  <div class="formula-display">
                    <div class="formula-line sub">
                      其中 Bᵢ 为基座铰点, L 为连杆长度, P 为末端位置
                    </div>
                  </div>
                </div>
                <!-- 码垛 -->
                <div
                  v-else-if="
                    currentFile.type === 'Palletizer' ||
                    currentFile.type === '码垛'
                  "
                >
                  <h4>4 轴码垛机器人</h4>
                  <div class="formula-display">
                    <div class="formula-line sub">
                      前 3 轴决定位置 (类似 SCARA)：
                    </div>
                  </div>
                  <div class="formula-display">
                    <div class="formula-line">
                      C₂ = (px² + py² - a₁² - a₂²) / (2a₁a₂)
                    </div>
                  </div>
                  <div class="formula-display">
                    <div class="formula-line">θ₁, θ₂ 同 SCARA 求法</div>
                  </div>
                  <div class="formula-display">
                    <div class="formula-line">
                      θ₃ = -(θ₁ + θ₂) (保持末端姿态恒定)
                    </div>
                  </div>
                  <div class="formula-display">
                    <div class="formula-line sub">第 4 轴 (升降)：z = d₄</div>
                  </div>
                </div>
                <!-- 极坐标 -->
                <div
                  v-else-if="
                    currentFile.type === 'Polar' ||
                    currentFile.type === '极坐标'
                  "
                >
                  <h4>极坐标机器人</h4>
                  <div class="formula-display">
                    <div class="formula-line main">
                      x = r·cos(θ₁)·cos(θ₂), y = r·cos(θ₁)·sin(θ₂), z =
                      r·sin(θ₁)
                    </div>
                  </div>
                  <div class="formula-display">
                    <div class="formula-line sub">
                      r = θ₃·pitch (伸缩), θ₁ = 俯仰角, θ₂ = 回转角
                    </div>
                  </div>
                </div>
                <!-- 通用 -->
                <div v-else>
                  <p class="formula-desc">
                    请在 DH 参数表中配置关节参数以生成特定公式。
                  </p>
                </div>
              </div>
            </div>

            <!-- 雅可比矩阵 -->
            <div class="formula-card">
              <div
                class="formula-card-header"
                @click="toggleFormula('jacobian')"
              >
                <span
                  class="material-symbols-outlined"
                  style="font-size: 18px; color: var(--tertiary)"
                  >grid_on</span
                >
                <h3>雅可比矩阵 (Jacobian)</h3>
                <span
                  class="material-symbols-outlined toggle-arrow"
                  :class="{ collapsed: !expandedFormulas.jacobian }"
                  >expand_more</span
                >
              </div>
              <div v-show="expandedFormulas.jacobian" class="formula-card-body">
                <div class="formula-display">
                  <div class="formula-line main">v = J(q) · q̇</div>
                </div>
                <div class="formula-display">
                  <div class="formula-line sub">
                    其中 v = [vₓ, vᵧ, v_z, ωₓ, ωᵧ, ω_z]ᵀ 为末端广义速度
                  </div>
                </div>
                <div class="formula-display">
                  <div class="formula-line sub">J(q) 为 6×n 雅可比矩阵</div>
                </div>
                <div class="formula-display">
                  <div class="formula-line">
                    旋转关节列: Jᵢ = [zᵢ₋₁ × (pₙ - pᵢ₋₁); zᵢ₋₁]
                  </div>
                </div>
                <div class="formula-display">
                  <div class="formula-line">移动关节列: Jᵢ = [zᵢ₋₁; 0]</div>
                </div>
                <div class="formula-display">
                  <div class="formula-line sub">
                    奇异性: det(J) = 0 → 机构失去某些方向的自由度
                  </div>
                </div>
              </div>
            </div>

            <!-- 齐次变换矩阵 -->
            <div class="formula-card">
              <div
                class="formula-card-header"
                @click="toggleFormula('homogeneous')"
              >
                <span
                  class="material-symbols-outlined"
                  style="font-size: 18px; color: var(--primary)"
                  >grid_on</span
                >
                <h3>齐次变换矩阵 (Homogeneous Transform)</h3>
                <span
                  class="material-symbols-outlined toggle-arrow"
                  :class="{ collapsed: !expandedFormulas.homogeneous }"
                  >expand_more</span
                >
              </div>
              <div
                v-show="expandedFormulas.homogeneous"
                class="formula-card-body"
              >
                <p class="formula-desc">
                  4×4
                  齐次变换矩阵用于描述坐标系之间的位姿关系，是运动学计算的核心工具。
                </p>
                <div class="formula-display">
                  <div class="formula-line main">
                    T = [ R p ] (旋转矩阵 R + 平移向量 p)
                  </div>
                </div>
                <div class="formula-display">
                  <div class="formula-line">[ 0 1 ]</div>
                </div>

                <h4 class="formula-sub-title">基本变换矩阵</h4>
                <div class="formula-display">
                  <div class="formula-line sub">绕 Z 轴旋转 θ：</div>
                </div>
                <div class="matrix-grid compact">
                  <div class="matrix-bracket">[</div>
                  <div class="matrix-body">
                    <div class="matrix-row">
                      <span>cos θ</span><span>-sin θ</span><span>0</span
                      ><span>0</span>
                    </div>
                    <div class="matrix-row">
                      <span>sin θ</span><span>cos θ</span><span>0</span
                      ><span>0</span>
                    </div>
                    <div class="matrix-row">
                      <span>0</span><span>0</span><span>1</span><span>0</span>
                    </div>
                    <div class="matrix-row">
                      <span>0</span><span>0</span><span>0</span><span>1</span>
                    </div>
                  </div>
                  <div class="matrix-bracket">]</div>
                </div>
                <div class="formula-display">
                  <div class="formula-line sub">沿 Z 轴平移 d：</div>
                </div>
                <div class="matrix-grid compact">
                  <div class="matrix-bracket">[</div>
                  <div class="matrix-body">
                    <div class="matrix-row">
                      <span>1</span><span>0</span><span>0</span><span>0</span>
                    </div>
                    <div class="matrix-row">
                      <span>0</span><span>1</span><span>0</span><span>0</span>
                    </div>
                    <div class="matrix-row">
                      <span>0</span><span>0</span><span>1</span><span>d</span>
                    </div>
                    <div class="matrix-row">
                      <span>0</span><span>0</span><span>0</span><span>1</span>
                    </div>
                  </div>
                  <div class="matrix-bracket">]</div>
                </div>
                <div class="formula-display">
                  <div class="formula-line sub">沿 X 轴平移 a：</div>
                </div>
                <div class="matrix-grid compact">
                  <div class="matrix-bracket">[</div>
                  <div class="matrix-body">
                    <div class="matrix-row">
                      <span>1</span><span>0</span><span>0</span><span>a</span>
                    </div>
                    <div class="matrix-row">
                      <span>0</span><span>1</span><span>0</span><span>0</span>
                    </div>
                    <div class="matrix-row">
                      <span>0</span><span>0</span><span>1</span><span>0</span>
                    </div>
                    <div class="matrix-row">
                      <span>0</span><span>0</span><span>0</span><span>1</span>
                    </div>
                  </div>
                  <div class="matrix-bracket">]</div>
                </div>
                <div class="formula-display">
                  <div class="formula-line sub">绕 X 轴旋转 α：</div>
                </div>
                <div class="matrix-grid compact">
                  <div class="matrix-bracket">[</div>
                  <div class="matrix-body">
                    <div class="matrix-row">
                      <span>1</span><span>0</span><span>0</span><span>0</span>
                    </div>
                    <div class="matrix-row">
                      <span>0</span><span>cos α</span><span>-sin α</span
                      ><span>0</span>
                    </div>
                    <div class="matrix-row">
                      <span>0</span><span>sin α</span><span>cos α</span
                      ><span>0</span>
                    </div>
                    <div class="matrix-row">
                      <span>0</span><span>0</span><span>0</span><span>1</span>
                    </div>
                  </div>
                  <div class="matrix-bracket">]</div>
                </div>

                <h4 class="formula-sub-title">逆变换与位姿提取</h4>
                <div class="formula-display">
                  <div class="formula-line">T⁻¹ = [ Rᵀ -Rᵀ·p ]</div>
                </div>
                <div class="formula-display">
                  <div class="formula-line">[ 0 1 ]</div>
                </div>
                <div class="formula-display">
                  <div class="formula-line sub">
                    位姿提取: p = [T(1,4), T(2,4), T(3,4)]ᵀ, R = T(1:3, 1:3)
                  </div>
                </div>

                <h4 class="formula-sub-title">
                  机床运动链变换 (Machine Tool Kinematic Chain)
                </h4>
                <div class="formula-display">
                  <div class="formula-line main">
                    T_W²ᵀ = T_Wᵀ · Tᵀᶜ · Tᶜˢ · Tˢᵀ
                  </div>
                </div>
                <div class="formula-display">
                  <div class="formula-line sub">
                    T_Wᵀ: 工件→工作台变换 | Tᵀᶜ: 工件坐标系→机床坐标系 | Tᶜˢ:
                    机床→主轴 | Tˢᵀ: 主轴→刀具
                  </div>
                </div>
                <div class="formula-display">
                  <div class="formula-line main">
                    T_Bᵀ = T_B₁ · T₁₂ · T₂₃ · ... · Tₙᵀ
                  </div>
                </div>
                <div class="formula-display">
                  <div class="formula-line sub">
                    T_Bᵀ: 基座→刀具端的完整运动链（每个 Tᵢ 对应一个轴的运动）
                  </div>
                </div>
              </div>
            </div>

            <!-- 机床运动学：RTCP / TCP -->
            <div class="formula-card">
              <div
                class="formula-card-header"
                @click="toggleFormula('machineTool')"
              >
                <span
                  class="material-symbols-outlined"
                  style="font-size: 18px; color: var(--error)"
                  >precision_manufacturing</span
                >
                <h3>机床运动学 (Machine Tool Kinematics)</h3>
                <span
                  class="material-symbols-outlined toggle-arrow"
                  :class="{ collapsed: !expandedFormulas.machineTool }"
                  >expand_more</span
                >
              </div>
              <div
                v-show="expandedFormulas.machineTool"
                class="formula-card-body"
              >
                <p class="formula-desc">
                  五轴机床运动学的核心：工件坐标系 (W)、机床坐标系
                  (M)、主轴坐标系 (S)、刀具坐标系 (T) 之间的齐次变换关系。
                </p>

                <h4 class="formula-sub-title">五轴机床分类</h4>
                <div class="ik-methods">
                  <div class="ik-method">
                    <h4>Table-Table (双转台)</h4>
                    <ul>
                      <li>A+C 或 B+C 转台型</li>
                      <li>工件随工作台旋转</li>
                      <li>例: Hermle C 系列, DMG monoBLOCK</li>
                      <li>运动链: W → Table(A) → Table(C) → M → Spindle → T</li>
                    </ul>
                  </div>
                  <div class="ik-method">
                    <h4>Head-Head (双摆头)</h4>
                    <ul>
                      <li>B+C 摆头型</li>
                      <li>刀具随主轴摆动</li>
                      <li>例: 龙门五轴铣床</li>
                      <li>运动链: W → M → Head(B) → Head(C) → Spindle → T</li>
                    </ul>
                  </div>
                  <div class="ik-method">
                    <h4>Head-Table (摆头+转台)</h4>
                    <ul>
                      <li>一轴在主轴端，一轴在工作台端</li>
                      <li>例: A 摆头 + C 转台</li>
                      <li>运动链: W → Table(C) → M → Head(A) → Spindle → T</li>
                    </ul>
                  </div>
                  <div class="ik-method">
                    <h4>Cartesian (龙门型)</h4>
                    <ul>
                      <li>XYZ 直线轴 + B+C 摆头</li>
                      <li>例: Gantry 五轴龙门铣</li>
                      <li>运动链: W → X → Y → Z → B → C → T</li>
                    </ul>
                  </div>
                </div>

                <h4 class="formula-sub-title">五轴正运动学 (Forward)</h4>
                <div class="formula-display">
                  <div class="formula-line main">
                    T_W²ᵀ = T_Wᵀ(A,C) · T_M(X,Y,Z) · T_S(B) · T_T
                  </div>
                </div>
                <div class="formula-display">
                  <div class="formula-line sub">
                    其中各子变换为旋转+平移的齐次矩阵：
                  </div>
                </div>
                <div class="formula-display">
                  <div class="formula-line sub">
                    T_Wᵀ(A,C) = Rot_z(C) · Rot_x(A) · Trans(offset_W)
                  </div>
                </div>
                <div class="formula-display">
                  <div class="formula-line sub">
                    T_M(X,Y,Z) = Trans(X, Y, Z)
                  </div>
                </div>
                <div class="formula-display">
                  <div class="formula-line sub">T_S(B) = Rot_y(B)</div>
                </div>

                <h4 class="formula-sub-title">五轴逆运动学 (Inverse)</h4>
                <div class="formula-display">
                  <div class="formula-line sub">
                    已知目标位姿 T_W²ᵀ = [R | p]，求各轴位置:
                  </div>
                </div>
                <div class="formula-display">
                  <div class="formula-line main">
                    T_M(X,Y,Z) = T_Wᵀ(A,C)⁻¹ · T_W²ᵀ · T_S(B)⁻¹ · T_T⁻¹
                  </div>
                </div>
                <div class="formula-display">
                  <div class="formula-line sub">
                    从提取的矩阵中获取: X, Y, Z = 平移分量, A, B, C =
                    欧拉角/轴角
                  </div>
                </div>
              </div>
            </div>

            <!-- RTCP / TCP / TCPM -->
            <div class="formula-card">
              <div class="formula-card-header" @click="toggleFormula('rtcp')">
                <span
                  class="material-symbols-outlined"
                  style="font-size: 18px; color: var(--success)"
                  >build</span
                >
                <h3>RTCP / TCP / TCPM / TRAORI</h3>
                <span
                  class="material-symbols-outlined toggle-arrow"
                  :class="{ collapsed: !expandedFormulas.rtcp }"
                  >expand_more</span
                >
              </div>
              <div v-show="expandedFormulas.rtcp" class="formula-card-body">
                <p class="formula-desc">
                  <strong>TCP (Tool Center Point)</strong>:
                  刀具中心点，是编程的参考点。<br />
                  <strong>RTCP (Rotational Tool Center Point)</strong>:
                  旋转刀具中心点补偿，五轴加工核心算法。<br />
                  当旋转轴运动时，TCP 位置会偏移，RTCP
                  自动补偿该偏移以保持刀尖位置正确。
                </p>

                <h4 class="formula-sub-title">TCP 偏移模型</h4>
                <div class="formula-display">
                  <div class="formula-line main">
                    P_TCP = P_spindle + R(A,C) · L_tool
                  </div>
                </div>
                <div class="formula-display">
                  <div class="formula-line sub">
                    P_spindle: 主轴端点位置 | R(A,C): 旋转矩阵 | L_tool:
                    刀具向量 (0,0,L)
                  </div>
                </div>
                <div class="formula-display">
                  <div class="formula-line sub">
                    当 A 或 C 轴旋转时，R(A,C) 变化，导致 P_TCP 偏移：
                  </div>
                </div>
                <div class="formula-display">
                  <div class="formula-line">ΔP = (R_new - R_old) · L_tool</div>
                </div>

                <h4 class="formula-sub-title">RTCP 补偿算法</h4>
                <div class="formula-display">
                  <div class="formula-line main">
                    核心思想: 保持 P_TCP 不变，反向补偿轴旋转导致的刀尖偏移
                  </div>
                </div>
                <div class="formula-display">
                  <div class="formula-line sub">
                    对于 Table-Table 结构 (A+C):
                  </div>
                </div>
                <div class="formula-display">
                  <div class="formula-line">
                    P_W = R_C⁻¹ · R_A⁻¹ · [P_programmed - P_offset]
                  </div>
                </div>
                <div class="formula-display">
                  <div class="formula-line sub">补偿后的轴位置:</div>
                </div>
                <div class="formula-display">
                  <div class="formula-line">
                    X_comp = X_prog - [R(A,C) - R(A₀,C₀)] · L_tool · ê_x
                  </div>
                </div>
                <div class="formula-display">
                  <div class="formula-line">
                    Y_comp = Y_prog - [R(A,C) - R(A₀,C₀)] · L_tool · ê_y
                  </div>
                </div>
                <div class="formula-display">
                  <div class="formula-line">
                    Z_comp = Z_prog - [R(A,C) - R(A₀,C₀)] · L_tool · ê_z
                  </div>
                </div>
                <div class="formula-display">
                  <div class="formula-line sub">
                    A₀, C₀: 零位角度 | L_tool: 刀具长度 | ê_x, ê_y, ê_z: 基向量
                  </div>
                </div>

                <h4 class="formula-sub-title">RTCP 旋转矩阵展开</h4>
                <div class="formula-display">
                  <div class="formula-line sub">A+C 转台型旋转矩阵:</div>
                </div>
                <div class="matrix-grid compact">
                  <div class="matrix-bracket">[</div>
                  <div class="matrix-body">
                    <div class="matrix-row">
                      <span>cos C</span><span>-sin C</span><span>0</span
                      ><span></span>
                    </div>
                    <div class="matrix-row">
                      <span>sin C</span><span>cos C</span><span>0</span
                      ><span></span>
                    </div>
                    <div class="matrix-row">
                      <span>0</span><span>0</span><span>1</span><span></span>
                    </div>
                    <div class="matrix-row">
                      <span></span><span></span><span></span><span></span>
                    </div>
                  </div>
                  <div class="matrix-bracket">]</div>
                  <div class="matrix-bracket">[</div>
                  <div class="matrix-body">
                    <div class="matrix-row">
                      <span>1</span><span>0</span><span>0</span><span></span>
                    </div>
                    <div class="matrix-row">
                      <span>0</span><span>cos A</span><span>-sin A</span
                      ><span></span>
                    </div>
                    <div class="matrix-row">
                      <span>0</span><span>sin A</span><span>cos A</span
                      ><span></span>
                    </div>
                    <div class="matrix-row">
                      <span></span><span></span><span></span><span></span>
                    </div>
                  </div>
                  <div class="matrix-bracket">]</div>
                </div>
                <div class="formula-display">
                  <div class="formula-line">R(A,C) = Rz(C) · Rx(A)</div>
                </div>
                <div class="matrix-grid compact">
                  <div class="matrix-bracket">[</div>
                  <div class="matrix-body">
                    <div class="matrix-row">
                      <span>cos C</span><span>-sin C cos A</span
                      ><span>sin C sin A</span><span></span>
                    </div>
                    <div class="matrix-row">
                      <span>sin C</span><span>cos C cos A</span
                      ><span>-cos C sin A</span><span></span>
                    </div>
                    <div class="matrix-row">
                      <span>0</span><span>sin A</span><span>cos A</span
                      ><span></span>
                    </div>
                    <div class="matrix-row">
                      <span>0</span><span>0</span><span>0</span><span>1</span>
                    </div>
                  </div>
                  <div class="matrix-bracket">]</div>
                </div>

                <h4 class="formula-sub-title">各厂商实现对照</h4>
                <div class="ik-methods">
                  <div class="ik-method">
                    <h4>Siemens — TRAORI</h4>
                    <ul>
                      <li>
                        <strong>TRAORI</strong>: Transformation Alignment
                        Orientation Interpolation
                      </li>
                      <li>G68.2: 工件坐标系变换</li>
                      <li>G43.4/G43.5: 刀具长度补偿 + RTCP</li>
                      <li>实时补偿旋转引起的 TCP 偏移</li>
                      <li>支持 XYZABC 六轴联动插补</li>
                    </ul>
                  </div>
                  <div class="ik-method">
                    <h4>Fanuc — TCPM</h4>
                    <ul>
                      <li>
                        <strong>TCPM</strong>: Tool Center Point Management
                      </li>
                      <li>G43.5 H__: 刀具长度+RTCP 补偿</li>
                      <li>自动计算旋转中心到刀尖的偏移</li>
                      <li>支持 A/B/C 旋转轴</li>
                      <li>配合 M128 (刀尖跟踪) 使用</li>
                    </ul>
                  </div>
                  <div class="ik-method">
                    <h4>Heidenhain — TCPM</h4>
                    <ul>
                      <li>
                        <strong>TCPM</strong>: Tool Center Point Management
                      </li>
                      <li>Q 程序: TQ 命令启用</li>
                      <li>PLC 功能块: TCPM_ENABLE</li>
                      <li>支持倾斜面加工 (Tilted Work Plane)</li>
                      <li>M128 启用 / M129 禁用</li>
                    </ul>
                  </div>
                  <div class="ik-method">
                    <h4>Okuma — ORiN</h4>
                    <ul>
                      <li><strong>ARP</strong>: Auto RTCP Position</li>
                      <li>G10 P__ Q__: 设置 RTCP 参数</li>
                      <li>刀具长度补偿 G43 + RTCP</li>
                      <li>支持五面体加工</li>
                      <li>五轴同步插补</li>
                    </ul>
                  </div>
                </div>

                <h4 class="formula-sub-title">TCP 刀具长度补偿</h4>
                <div class="formula-display">
                  <div class="formula-line main">
                    P_comp = P_program + T_W²ᵀ · L_tool_vector
                  </div>
                </div>
                <div class="formula-display">
                  <div class="formula-line sub">
                    P_program: NC 程序中的编程点 | L_tool_vector = [0, 0, L]ᵀ
                  </div>
                </div>
                <div class="formula-display">
                  <div class="formula-line sub">
                    G43: 启用刀具长度补偿 (仅 Z 方向)
                  </div>
                </div>
                <div class="formula-display">
                  <div class="formula-line sub">
                    G43 H__: 补偿值来自刀具表 H__ 号
                  </div>
                </div>

                <h4 class="formula-sub-title">RTCP 校准 (Calibration)</h4>
                <div class="formula-display">
                  <div class="formula-line sub">
                    校准目的: 精确测量旋转中心位置和刀具参数
                  </div>
                </div>
                <div class="formula-display">
                  <div class="formula-line main">测量步骤:</div>
                </div>
                <div class="formula-display">
                  <div class="formula-line">
                    1. 在不同角度测量标准球/校准棒中心
                  </div>
                </div>
                <div class="formula-display">
                  <div class="formula-line">
                    2. 拟合旋转中心: P_rot = f(P₁, P₂, ..., Pₙ, θ₁, θ₂, ..., θₙ)
                  </div>
                </div>
                <div class="formula-display">
                  <div class="formula-line">
                    3. 求解: 最小化 Σ|P_rot - R(θᵢ)·L - Pᵢ|² (最小二乘拟合)
                  </div>
                </div>
                <div class="formula-display">
                  <div class="formula-line">
                    4. 验证: 在多角度下 TCP 偏差 < 5μm 为合格
                  </div>
                </div>
              </div>
            </div>
          </div>
          <div v-else class="dh-empty">
            <span
              class="material-symbols-outlined"
              style="font-size: 48px; opacity: 0.2"
              >functions</span
            >
            <p>请先在左侧选择一个运动学文件</p>
          </div>
        </div>

        <!-- ====== Tab: 3D 预览 ====== -->
        <div v-if="mainTab === 'preview'" class="preview-content">
          <div class="preview-header">
            <h2 class="preview-title">3D 模型预览</h2>
            <div class="preview-actions">
              <button class="dh-btn" @click="triggerImportModel">
                <span class="material-symbols-outlined" style="font-size: 14px"
                  >upload</span
                >
                导入模型
              </button>
            </div>
          </div>
          <div class="preview-viewport">
            <div class="viewport-placeholder">
              <span
                class="material-symbols-outlined"
                style="font-size: 64px; opacity: 0.15"
                >view_in_ar</span
              >
              <p>拖拽 STL/OBJ 文件到此处或点击上方按钮导入</p>
              <p class="dim-text">支持格式: .stl, .obj, .step, .iges</p>
            </div>
          </div>
          <div v-if="currentFile?.modelFile" class="preview-info">
            <div class="preview-info-item">
              <span class="info-label">文件名</span>
              <span class="info-value code-font">{{
                currentFile.modelFile
              }}</span>
            </div>
            <div class="preview-info-item">
              <span class="info-label">命名规范</span>
              <span class="info-value code-font"
                >KIN_{类型}_{型号}_{版本}.stl</span
              >
            </div>
          </div>
        </div>
      </div>
    </main>

    <!-- ==================== 右侧属性面板 ==================== -->
    <aside class="kin-inspector">
      <div class="inspector-header">
        <h2 class="inspector-title">Properties</h2>
        <p class="inspector-subtitle">{{ currentFile?.name || "未选择" }}</p>
      </div>

      <div class="inspector-body custom-scrollbar">
        <!-- 基本信息 -->
        <div v-if="currentFile" class="config-section">
          <h3 class="config-label">基本信息</h3>
          <div class="prop-row">
            <label class="prop-lbl">名称</label>
            <input class="prop-in code-font" v-model="currentFile.name" />
          </div>
          <div class="prop-row">
            <label class="prop-lbl">类型</label>
            <select class="prop-in" v-model="currentFile.type">
              <option>串联机器人</option>
              <option>SCARA</option>
              <option>龙门式</option>
              <option>并联</option>
              <option>码垛</option>
              <option>极坐标</option>
              <option>球坐标</option>
              <option>自定义</option>
            </select>
          </div>
          <div class="prop-row">
            <label class="prop-lbl">模型文件</label>
            <input
              class="prop-in code-font"
              v-model="currentFile.modelFile"
              placeholder="KIN_6AX_R6_v1.stl"
            />
          </div>
        </div>

        <!-- 轴映射 -->
        <div class="config-section">
          <h3 class="config-label">AXIS MAPPING</h3>
          <div class="axis-grid">
            <template v-for="(axis, idx) in axisMapping" :key="idx">
              <div class="axis-name">
                Axis {{ idx + 1 }} ({{ axis.label }}):
              </div>
              <input class="axis-input code-font" v-model="axis.value" />
            </template>
          </div>
        </div>

        <!-- TCP 偏移 -->
        <div class="config-section">
          <h3 class="config-label">TCP OFFSET (mm)</h3>
          <div class="tcp-grid">
            <div class="tcp-col">
              <span class="tcp-axis-label">X</span>
              <input
                class="tcp-input code-font"
                type="number"
                v-model.number="tcpOffset.x"
              />
            </div>
            <div class="tcp-col">
              <span class="tcp-axis-label">Y</span>
              <input
                class="tcp-input code-font"
                type="number"
                v-model.number="tcpOffset.y"
              />
            </div>
            <div class="tcp-col">
              <span class="tcp-axis-label">Z</span>
              <input
                class="tcp-input code-font"
                type="number"
                v-model.number="tcpOffset.z"
              />
            </div>
          </div>
        </div>

        <!-- 动态限制 -->
        <div class="config-section">
          <h3 class="config-label">DYNAMIC LIMITS</h3>
          <div class="limits-list">
            <div class="limit-row">
              <div class="limit-header">
                <span class="limit-name">Max Velocity</span>
                <span class="limit-value code-font"
                  >{{ maxVelocity }} mm/s</span
                >
              </div>
              <input
                type="range"
                class="hmi-slider"
                min="100"
                max="5000"
                v-model.number="maxVelocity"
              />
            </div>
            <div class="limit-row">
              <div class="limit-header">
                <span class="limit-name">Max Acceleration</span>
                <span class="limit-value code-font">{{ maxAccel }} m/s²</span>
              </div>
              <input
                type="range"
                class="hmi-slider"
                min="1"
                max="50"
                step="0.5"
                v-model.number="maxAccel"
              />
            </div>
          </div>
        </div>
      </div>

      <div class="inspector-actions">
        <button class="action-btn apply-btn" @click="applyChanges">
          Apply Changes
        </button>
        <button class="action-btn reset-btn" @click="resetKinematics">
          Reset Kinematics
        </button>
      </div>
    </aside>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive, computed } from "vue";
import type { EditorTab } from "@smart-plc/shared";
import { useKinematicsStore } from "@/stores/kinematics";

const props = defineProps<{ tab: EditorTab }>();
const kinStore = useKinematicsStore();

// ==================== 接口定义 ====================
interface DhJoint {
  type: "revolute" | "prismatic";
  theta: number;
  d: number;
  a: number;
  alpha: number;
  rangeMin: number;
  rangeMax: number;
}

interface KinFile {
  name: string;
  type: string;
  joints: DhJoint[];
  modelFile?: string;
}

// ==================== 状态 ====================
const projectName = ref("Main_Controller.plc");
const sideTab = ref<"files" | "devices">("files");
const mainTab = ref<"overview" | "dh" | "formula" | "preview">("overview");
const selectedGroup = ref("Robot_6Axis_Main");
const selectedModel = ref("6 轴机器人");
const selectedModelFile = ref("");
const maxVelocity = ref(2500);
const maxAccel = ref(12.5);
const tcpOffset = reactive({ x: 120.0, y: 0.0, z: 25.4 });
const currentFile = ref<KinFile | null>(null);

const fileSections = reactive({
  examples: true,
  custom: true,
  models: true,
});

const expandedFormulas = reactive({
  fk: true,
  ik: true,
  specific: true,
  jacobian: false,
  homogeneous: true,
  machineTool: true,
  rtcp: true,
});

const jointColors = [
  "#3b82f6",
  "#22c55e",
  "#f97316",
  "#ef4444",
  "#a855f7",
  "#eab308",
];

// ==================== 示例文件 ====================
const exampleFiles = ref<KinFile[]>([
  {
    name: "6轴串联机器人.dh",
    type: "串联机器人",
    modelFile: "KIN_6AX_R6_v1.stl",
    joints: [
      {
        type: "revolute",
        theta: 0,
        d: 150,
        a: 50,
        alpha: -90,
        rangeMin: -180,
        rangeMax: 180,
      },
      {
        type: "revolute",
        theta: 0,
        d: 0,
        a: 350,
        alpha: 0,
        rangeMin: -120,
        rangeMax: 120,
      },
      {
        type: "revolute",
        theta: 0,
        d: 0,
        a: 35,
        alpha: -90,
        rangeMin: -120,
        rangeMax: 120,
      },
      {
        type: "revolute",
        theta: 0,
        d: 350,
        a: 0,
        alpha: 90,
        rangeMin: -360,
        rangeMax: 360,
      },
      {
        type: "revolute",
        theta: 0,
        d: 0,
        a: 0,
        alpha: -90,
        rangeMin: -120,
        rangeMax: 120,
      },
      {
        type: "revolute",
        theta: 0,
        d: 80,
        a: 0,
        alpha: 0,
        rangeMin: -360,
        rangeMax: 360,
      },
    ],
  },
  {
    name: "SCARA机器人.dh",
    type: "SCARA",
    modelFile: "KIN_SCARA_S400_v1.stl",
    joints: [
      {
        type: "revolute",
        theta: 0,
        d: 0,
        a: 200,
        alpha: 0,
        rangeMin: -160,
        rangeMax: 160,
      },
      {
        type: "revolute",
        theta: 0,
        d: 0,
        a: 200,
        alpha: 0,
        rangeMin: -150,
        rangeMax: 150,
      },
      {
        type: "prismatic",
        theta: 0,
        d: 0,
        a: 0,
        alpha: 0,
        rangeMin: -100,
        rangeMax: 0,
      },
      {
        type: "revolute",
        theta: 0,
        d: 0,
        a: 0,
        alpha: 0,
        rangeMin: -360,
        rangeMax: 360,
      },
    ],
  },
  {
    name: "龙门机器人.dh",
    type: "龙门式",
    modelFile: "KIN_GANTRY_235_v1.stl",
    joints: [
      {
        type: "prismatic",
        theta: 0,
        d: 0,
        a: 0,
        alpha: 0,
        rangeMin: -2000,
        rangeMax: 2000,
      },
      {
        type: "prismatic",
        theta: 0,
        d: 0,
        a: 0,
        alpha: -90,
        rangeMin: -1500,
        rangeMax: 1500,
      },
      {
        type: "prismatic",
        theta: 0,
        d: 0,
        a: 0,
        alpha: 0,
        rangeMin: -500,
        rangeMax: 500,
      },
    ],
  },
  {
    name: "Delta并联机器人.dh",
    type: "并联",
    modelFile: "KIN_DELTA_HSPD_v1.stl",
    joints: [
      {
        type: "revolute",
        theta: 0,
        d: 0,
        a: 350,
        alpha: 0,
        rangeMin: -30,
        rangeMax: 30,
      },
      {
        type: "revolute",
        theta: 0,
        d: 0,
        a: 350,
        alpha: 0,
        rangeMin: -30,
        rangeMax: 30,
      },
      {
        type: "revolute",
        theta: 0,
        d: 0,
        a: 350,
        alpha: 0,
        rangeMin: -30,
        rangeMax: 30,
      },
    ],
  },
  {
    name: "码垛机器人.dh",
    type: "码垛",
    modelFile: "KIN_PALL_P4_v1.stl",
    joints: [
      {
        type: "revolute",
        theta: 0,
        d: 0,
        a: 500,
        alpha: 0,
        rangeMin: -160,
        rangeMax: 160,
      },
      {
        type: "revolute",
        theta: 0,
        d: 0,
        a: 400,
        alpha: 0,
        rangeMin: -10,
        rangeMax: 90,
      },
      {
        type: "revolute",
        theta: 0,
        d: 0,
        a: 0,
        alpha: 0,
        rangeMin: -160,
        rangeMax: 160,
      },
      {
        type: "prismatic",
        theta: 0,
        d: 0,
        a: 0,
        alpha: -90,
        rangeMin: 0,
        rangeMax: 1500,
      },
    ],
  },
  {
    name: "极坐标机器人.dh",
    type: "极坐标",
    modelFile: "KIN_POLAR_ROB_v1.stl",
    joints: [
      {
        type: "revolute",
        theta: 0,
        d: 0,
        a: 0,
        alpha: -90,
        rangeMin: -90,
        rangeMax: 90,
      },
      {
        type: "revolute",
        theta: 0,
        d: 0,
        a: 0,
        alpha: 0,
        rangeMin: -180,
        rangeMax: 180,
      },
      {
        type: "prismatic",
        theta: 0,
        d: 0,
        a: 0,
        alpha: 0,
        rangeMin: 100,
        rangeMax: 1200,
      },
      {
        type: "revolute",
        theta: 0,
        d: 0,
        a: 0,
        alpha: 0,
        rangeMin: -360,
        rangeMax: 360,
      },
    ],
  },
]);

const customFiles = ref<KinFile[]>([]);

const modelFiles = ref([
  { name: "KIN_6AX_R6_v1.stl", format: "STL", size: "2.4 MB" },
  { name: "KIN_SCARA_S400_v1.stl", format: "STL", size: "1.8 MB" },
  { name: "KIN_GANTRY_235_v1.obj", format: "OBJ", size: "3.1 MB" },
  { name: "KIN_DELTA_HSPD_v1.stl", format: "STL", size: "1.2 MB" },
  { name: "KIN_PALL_P4_v1.step", format: "STEP", size: "5.6 MB" },
]);

// ==================== 运动学树 ====================
const kinGroups = ref([
  {
    name: "Gantry_Axis_Group",
    expanded: false,
    children: ["Joint_X", "Joint_Y", "Joint_Z"],
  },
  {
    name: "Robot_6Axis_Main",
    expanded: true,
    children: ["Joint_01 (X)", "Joint_02 (Y)", "Wrist_Tool"],
  },
]);

const axisMapping = reactive([
  { label: "Base", value: "DRIVE_X1" },
  { label: "Arm", value: "DRIVE_Y1" },
  { label: "Elbow", value: "DRIVE_Z1" },
]);

// ==================== 运动学模型卡片 ====================
const kinModels = [
  {
    name: "龙门式机器人 (2/3/5 轴)",
    type: "Cartesian",
    model: "G_235_STD",
    icon: "view_in_ar",
    image: "/images/kinematics/robot_gantry.png",
  },
  {
    name: "极坐标机器人",
    type: "Polar",
    model: "P_ROB_01",
    icon: "radar",
    image: "/images/kinematics/robot_polar.png",
  },
  {
    name: "4 轴码垛机器人",
    type: "Palletizer",
    model: "P4_600_S",
    icon: "view_in_ar",
    image: "/images/kinematics/robot_palletizer.png",
  },
  {
    name: "6 轴机器人",
    type: "Articulated",
    model: "R6_MAIN_CTRL",
    icon: "precision_manufacturing",
    image: "/images/kinematics/robot_6axis.png",
  },
  {
    name: "SCARA 机器人",
    type: "SCARA",
    model: "S_400_PRO",
    icon: "precision_manufacturing",
    image: "/images/kinematics/robot_scara.png",
  },
  {
    name: "并联机器人 (Delta)",
    type: "Parallel",
    model: "D_HIGH_SPD",
    icon: "view_in_ar",
    image: "/images/kinematics/delta_parallel.png",
  },
  {
    name: "Kin_Wrist2/3",
    type: "Wrist",
    model: "KIN_WR_X2",
    icon: "settings",
    image: "/images/kinematics/robot_wrist.png",
  },
];

// ==================== 函数 ====================
function toggleFileSection(key: keyof typeof fileSections) {
  fileSections[key] = !fileSections[key];
}

function toggleFormula(key: keyof typeof expandedFormulas) {
  expandedFormulas[key] = !expandedFormulas[key];
}

function selectGroup(name: string) {
  selectedGroup.value = name;
  const g = kinGroups.value.find((k) => k.name === name);
  if (g) g.expanded = !g.expanded;
}

function selectModel(name: string) {
  selectedModel.value = name;
}

function openFile(f: KinFile) {
  currentFile.value = f;
}

function createNewKin() {
  const name = prompt("运动学名称:", `自定义_${customFiles.value.length + 1}`);
  if (!name || !name.trim()) return;
  const type =
    prompt("类型 (串联/SCARA/龙门/并联/码垛/极坐标):", "串联机器人") ||
    "串联机器人";
  customFiles.value.push({
    name: name.trim() + ".dh",
    type,
    modelFile: "",
    joints: [
      {
        type: "revolute",
        theta: 0,
        d: 150,
        a: 50,
        alpha: -90,
        rangeMin: -180,
        rangeMax: 180,
      },
      {
        type: "revolute",
        theta: 0,
        d: 0,
        a: 300,
        alpha: 0,
        rangeMin: -120,
        rangeMax: 120,
      },
      {
        type: "revolute",
        theta: 0,
        d: 0,
        a: 30,
        alpha: -90,
        rangeMin: -120,
        rangeMax: 120,
      },
      {
        type: "revolute",
        theta: 0,
        d: 250,
        a: 0,
        alpha: 90,
        rangeMin: -360,
        rangeMax: 360,
      },
      {
        type: "revolute",
        theta: 0,
        d: 0,
        a: 0,
        alpha: -90,
        rangeMin: -120,
        rangeMax: 120,
      },
      {
        type: "revolute",
        theta: 0,
        d: 60,
        a: 0,
        alpha: 0,
        rangeMin: -360,
        rangeMax: 360,
      },
    ],
  });
  currentFile.value = customFiles.value[customFiles.value.length - 1];
  mainTab.value = "dh";
}

function deleteCustomFile(name: string) {
  const idx = customFiles.value.findIndex((f) => f.name === name);
  if (idx >= 0) {
    customFiles.value.splice(idx, 1);
    if (currentFile.value?.name === name) currentFile.value = null;
  }
}

function addDhJoint() {
  if (!currentFile.value) return;
  currentFile.value.joints.push({
    type: "revolute",
    theta: 0,
    d: 0,
    a: 0,
    alpha: 0,
    rangeMin: -180,
    rangeMax: 180,
  });
}

function removeDhJoint(idx: number) {
  if (!currentFile.value || currentFile.value.joints.length <= 1) return;
  currentFile.value.joints.splice(idx, 1);
}

function resetDhTable() {
  if (!currentFile.value) return;
  currentFile.value.joints.forEach((j) => {
    j.theta = 0;
    j.d = 0;
    j.a = 0;
    j.alpha = 0;
  });
}

function triggerImportModel() {
  const input = document.createElement("input");
  input.type = "file";
  input.accept = ".stl,.obj,.step,.iges";
  input.onchange = (e: Event) => {
    const file = (e.target as HTMLInputElement).files?.[0];
    if (file) {
      modelFiles.value.push({
        name: file.name,
        format: file.name.split(".").pop()?.toUpperCase() || "STL",
        size: (file.size / 1024 / 1024).toFixed(1) + " MB",
      });
    }
  };
  input.click();
}

function buildKinematics() {
  alert("Kinematics build successful!");
}

function applyChanges() {
  alert("Changes applied!");
}

function resetKinematics() {
  if (currentFile.value) {
    currentFile.value.joints.forEach((j) => {
      j.theta = 0;
      j.d = 0;
      j.a = 0;
      j.alpha = 0;
    });
  }
  maxVelocity.value = 2500;
  maxAccel.value = 12.5;
  tcpOffset.x = 120;
  tcpOffset.y = 0;
  tcpOffset.z = 25.4;
}
</script>

<style scoped>
.kin-editor {
  width: 100%;
  height: 100%;
  display: flex;
  overflow: hidden;
}

/* ===== 左侧边栏 ===== */
.kin-sidebar {
  width: 260px;
  flex-shrink: 0;
  background: var(--surface-container);
  border-right: 1px solid var(--outline-variant);
  display: flex;
  flex-direction: column;
}
.sidebar-header {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 12px;
  border-bottom: 1px solid var(--outline-variant);
}
.sidebar-icon-box {
  width: 32px;
  height: 32px;
  background: var(--surface-variant);
  border-radius: var(--radius);
  display: flex;
  align-items: center;
  justify-content: center;
}
.sidebar-icon-box .material-symbols-outlined {
  font-size: 20px;
  color: var(--primary);
}
.sidebar-title {
  font-size: 10px;
  font-weight: 700;
  letter-spacing: 0.05em;
  text-transform: uppercase;
  color: var(--on-surface);
}
.sidebar-subtitle {
  font-size: 11px;
  color: var(--on-surface-variant);
}

.sidebar-nav {
  display: flex;
  border-bottom: 1px solid var(--outline-variant);
}
.nav-item {
  flex: 1;
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 6px;
  padding: 8px;
  cursor: pointer;
  color: var(--on-surface-variant);
  font-size: 10px;
  font-weight: 700;
  letter-spacing: 0.05em;
  text-transform: uppercase;
  transition: all 0.15s;
  border-bottom: 2px solid transparent;
}
.nav-item:hover {
  background: var(--surface-variant);
  color: var(--on-surface);
}
.nav-item.active {
  color: var(--primary);
  border-bottom-color: var(--primary);
  background: var(--primary-container);
}
.nav-icon {
  font-size: 16px;
}

/* --- 文件浏览 --- */
.file-browser,
.device-tree {
  flex: 1;
  overflow-y: auto;
  padding: 6px;
  display: flex;
  flex-direction: column;
  gap: 4px;
}
.file-section {
  display: flex;
  flex-direction: column;
}
.file-section-header {
  display: flex;
  align-items: center;
  gap: 4px;
  padding: 5px 6px;
  background: none;
  border: none;
  color: var(--on-surface);
  font-size: 11px;
  font-weight: 600;
  cursor: pointer;
  border-radius: var(--radius);
  width: 100%;
  text-align: left;
}
.file-section-header:hover {
  background: var(--surface-variant);
}
.file-arrow {
  font-size: 14px;
  transition: transform 0.15s;
}
.file-arrow.collapsed {
  transform: rotate(-90deg);
}
.file-add-btn {
  margin-left: auto;
  width: 18px;
  height: 18px;
  display: flex;
  align-items: center;
  justify-content: center;
  background: var(--primary-container);
  color: var(--primary);
  border: none;
  border-radius: 2px;
  cursor: pointer;
}
.file-add-btn:hover {
  background: var(--primary);
  color: var(--on-primary);
}

.file-list {
  display: flex;
  flex-direction: column;
  gap: 1px;
  padding: 2px 0 2px 12px;
}
.file-empty {
  font-size: 10px;
  color: var(--outline);
  padding: 4px 8px;
}
.file-item {
  display: flex;
  align-items: center;
  gap: 6px;
  padding: 5px 6px;
  border-radius: var(--radius);
  cursor: pointer;
  transition: background 0.1s;
}
.file-item:hover {
  background: var(--surface-variant);
}
.file-item.active {
  background: var(--primary-container);
}
.file-icon {
  font-size: 16px;
  flex-shrink: 0;
}
.file-info {
  flex: 1;
  min-width: 0;
}
.file-name {
  font-size: 11px;
  font-weight: 600;
  color: var(--on-surface);
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}
.file-desc {
  font-size: 9px;
  color: var(--outline);
}
.file-del-btn {
  width: 16px;
  height: 16px;
  display: flex;
  align-items: center;
  justify-content: center;
  background: none;
  border: none;
  color: var(--on-surface-variant);
  cursor: pointer;
  border-radius: 2px;
  opacity: 0;
  transition: opacity 0.1s;
}
.file-item:hover .file-del-btn {
  opacity: 1;
}
.file-del-btn:hover {
  background: var(--error-container);
  color: var(--error);
}
.import-model-item {
  border: 1px dashed var(--outline-variant);
  margin-top: 4px;
}
.import-model-item:hover {
  border-color: var(--primary);
}

/* --- 设备树 --- */
.kin-tree-section {
  padding: 8px;
}
.tree-title {
  font-size: 10px;
  font-weight: 700;
  letter-spacing: 0.05em;
  text-transform: uppercase;
  color: var(--outline);
  margin-bottom: 8px;
}
.tree-list {
  list-style: none;
  padding: 0;
  margin: 0;
}
.tree-node {
  display: flex;
  align-items: center;
  gap: 4px;
  padding: 4px;
  border-radius: var(--radius);
  cursor: pointer;
  font-size: 12px;
  color: var(--on-surface);
}
.tree-node:hover {
  background: var(--surface-variant);
}
.tree-node.selected {
  color: var(--secondary);
  font-weight: 600;
}
.tree-arrow {
  font-size: 16px;
}
.tree-child {
  margin-left: 24px;
  padding: 2px 4px 2px 8px;
  border-left: 1px solid var(--outline-variant);
  font-size: 11px;
  color: var(--on-surface-variant);
}

.sidebar-footer {
  padding: 12px;
  border-top: 1px solid var(--outline-variant);
}
.build-btn {
  width: 100%;
  padding: 8px;
  background: var(--primary);
  color: var(--on-primary);
  border: none;
  border-radius: var(--radius);
  font-size: 10px;
  font-weight: 700;
  letter-spacing: 0.05em;
  text-transform: uppercase;
  cursor: pointer;
}
.build-btn:hover {
  opacity: 0.9;
}

/* ===== 中间主区域 ===== */
.kin-main {
  flex: 1;
  display: flex;
  flex-direction: column;
  overflow: hidden;
  min-width: 0;
}
.kin-tabs {
  display: flex;
  height: 32px;
  background: var(--surface-container-low);
  border-bottom: 1px solid var(--outline-variant);
  overflow-x: auto;
  flex-shrink: 0;
}
.kin-tab {
  display: flex;
  align-items: center;
  gap: 4px;
  padding: 0 14px;
  font-size: 12px;
  color: var(--on-surface-variant);
  cursor: pointer;
  white-space: nowrap;
  border-bottom: 2px solid transparent;
  transition: all 0.15s;
}
.kin-tab:hover {
  color: var(--on-surface);
  background: var(--surface-variant);
}
.kin-tab.active {
  color: var(--primary);
  font-weight: 600;
  border-bottom-color: var(--primary);
}
.kin-content {
  flex: 1;
  overflow-y: auto;
  padding: 16px;
}

/* --- 概览 --- */
.overview-content {
  display: flex;
  flex-direction: column;
  gap: 16px;
}
.overview-header {
  margin-bottom: 4px;
}
.overview-title {
  font-size: 18px;
  font-weight: 600;
  color: var(--on-surface);
  margin: 0;
}
.overview-subtitle {
  font-size: 12px;
  color: var(--on-surface-variant);
  margin: 4px 0 0;
}
.model-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(220px, 1fr));
  gap: 8px;
}
.model-card {
  background: var(--surface-container);
  border: 1px solid var(--outline-variant);
  padding: 10px;
  display: flex;
  flex-direction: column;
  gap: 8px;
  cursor: pointer;
  border-radius: var(--radius);
  transition: all 0.2s;
}
.model-card:hover {
  background: var(--surface-variant);
  border-color: var(--primary);
}
.model-card.active {
  outline: 2px solid var(--primary);
  z-index: 2;
}
.card-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
}
.card-title {
  font-size: 10px;
  font-weight: 700;
  letter-spacing: 0.04em;
  color: var(--secondary);
}
.card-badge {
  font-size: 16px;
  color: var(--on-surface-variant);
}
.model-card.active .card-badge {
  color: var(--secondary);
}
.card-visual {
  aspect-ratio: 16/9;
  background: var(--surface-dim);
  border: 1px solid var(--outline-variant);
  border-radius: var(--radius);
  display: flex;
  align-items: center;
  justify-content: center;
  overflow: hidden;
}
.card-visual-icon {
  font-size: 40px;
  color: var(--on-surface-variant);
  opacity: 0.4;
}
.card-visual-img {
  width: 100%;
  height: 100%;
  object-fit: contain;
  padding: 6px;
}
.card-footer {
  display: flex;
  justify-content: space-between;
  font-size: 10px;
  color: var(--on-surface-variant);
}
.import-card {
  align-items: center;
  justify-content: center;
  border: 2px dashed var(--outline-variant);
  background: var(--surface-container-low);
  min-height: 160px;
}
.import-card:hover {
  border-color: var(--primary);
  background: var(--primary-container);
}
.import-icon {
  font-size: 28px;
  color: var(--outline);
}
.import-card:hover .import-icon {
  color: var(--primary);
}
.import-label {
  font-size: 10px;
  font-weight: 700;
  letter-spacing: 0.05em;
  color: var(--outline);
}
.import-card:hover .import-label {
  color: var(--primary);
}

/* --- DH 参数 --- */
.dh-content {
  display: flex;
  flex-direction: column;
  gap: 16px;
}
.dh-header {
  display: flex;
  justify-content: space-between;
  align-items: flex-start;
}
.dh-title {
  font-size: 18px;
  font-weight: 600;
  color: var(--on-surface);
  margin: 0;
}
.dh-subtitle {
  font-size: 12px;
  color: var(--on-surface-variant);
  margin: 4px 0 0;
}
.dh-actions {
  display: flex;
  gap: 6px;
}
.dh-btn {
  display: flex;
  align-items: center;
  gap: 4px;
  padding: 5px 10px;
  font-size: 11px;
  font-weight: 600;
  border: 1px solid var(--outline-variant);
  border-radius: var(--radius);
  background: var(--surface-variant);
  color: var(--on-surface);
  cursor: pointer;
}
.dh-btn:hover {
  background: var(--primary);
  color: var(--on-primary);
  border-color: var(--primary);
}
.dh-btn:disabled {
  opacity: 0.4;
  cursor: not-allowed;
}
.dh-btn.secondary {
  background: none;
}

.dh-info-box {
  display: flex;
  gap: 8px;
  padding: 10px;
  border-radius: var(--radius);
  background: var(--secondary-container);
  font-size: 11px;
}
.dh-info-box strong {
  color: var(--on-surface);
}
.dh-info-box p {
  margin: 4px 0 0;
  color: var(--on-surface-variant);
  font-size: 10px;
  line-height: 1.6;
}

.dh-table-wrapper {
  overflow-x: auto;
}
.dh-table {
  width: 100%;
  border-collapse: collapse;
  font-size: 12px;
}
.dh-table th {
  padding: 6px 8px;
  text-align: left;
  font-size: 10px;
  font-weight: 700;
  text-transform: uppercase;
  letter-spacing: 0.05em;
  color: var(--on-surface-variant);
  background: var(--surface-variant);
  border-bottom: 2px solid var(--outline-variant);
}
.dh-cell {
  padding: 4px 6px;
  border-bottom: 1px solid var(--outline-variant);
  vertical-align: middle;
}
.dh-row:hover {
  background: var(--surface-variant);
}
.joint-cell {
  text-align: center;
}
.joint-badge {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 24px;
  height: 20px;
  border-radius: 4px;
  font-size: 10px;
  font-weight: 700;
  color: #fff;
}
.dh-input {
  width: 70px;
  padding: 3px 5px;
  background: var(--surface-container-highest);
  border: 1px solid var(--outline-variant);
  border-radius: 3px;
  color: var(--on-surface);
  font-size: 11px;
  outline: none;
  text-align: center;
}
.dh-input:focus {
  border-color: var(--primary);
}
.dh-input:disabled {
  opacity: 0.4;
}
.dh-select {
  padding: 3px 4px;
  background: var(--surface-container-highest);
  border: 1px solid var(--outline-variant);
  border-radius: 3px;
  color: var(--on-surface);
  font-size: 11px;
  outline: none;
}
.range-cell {
  display: flex;
  align-items: center;
  gap: 2px;
}
.range-input {
  width: 50px !important;
}
.range-sep {
  color: var(--outline);
  font-size: 10px;
}
.range-unit {
  color: var(--outline);
  font-size: 10px;
}
.dh-del-btn {
  width: 24px;
  height: 24px;
  display: flex;
  align-items: center;
  justify-content: center;
  background: none;
  border: none;
  color: var(--on-surface-variant);
  cursor: pointer;
  border-radius: 4px;
}
.dh-del-btn:hover:not(:disabled) {
  background: var(--error-container);
  color: var(--error);
}
.dh-del-btn:disabled {
  opacity: 0.2;
  cursor: not-allowed;
}
.dh-empty {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 8px;
  padding: 40px;
  color: var(--outline);
  font-size: 12px;
}

/* --- DH 变换矩阵 --- */
.dh-matrix-section {
  margin-top: 16px;
}
.section-title {
  display: flex;
  align-items: center;
  gap: 6px;
  font-size: 14px;
  font-weight: 600;
  color: var(--on-surface);
  margin-bottom: 12px;
}
.matrix-display {
  background: var(--surface-container-highest);
  padding: 16px;
  border-radius: var(--radius);
}
.formula-line {
  font-family: "JetBrains Mono", monospace;
  font-size: 11px;
  color: var(--on-surface);
  line-height: 1.8;
}
.formula-line.main {
  font-size: 13px;
  font-weight: 600;
  color: var(--secondary);
}
.formula-line.sub {
  color: var(--on-surface-variant);
}
.matrix-grid {
  display: flex;
  align-items: center;
  gap: 4px;
  margin-top: 12px;
}
.matrix-bracket {
  font-size: 48px;
  font-weight: 100;
  color: var(--on-surface-variant);
  line-height: 1;
}
.matrix-body {
  display: flex;
  flex-direction: column;
  gap: 2px;
}
.matrix-row {
  display: grid;
  grid-template-columns: repeat(4, 1fr);
  gap: 12px;
  text-align: center;
}
.matrix-row span {
  font-family: "JetBrains Mono", monospace;
  font-size: 11px;
  color: var(--on-surface);
  padding: 2px 4px;
}

/* --- 运动学公式 --- */
.formula-content {
  display: flex;
  flex-direction: column;
  gap: 16px;
}
.formula-header {
  margin-bottom: 4px;
}
.formula-title {
  font-size: 18px;
  font-weight: 600;
  color: var(--on-surface);
  margin: 0;
}
.formula-subtitle {
  font-size: 12px;
  color: var(--on-surface-variant);
  margin: 4px 0 0;
}
.formula-sections {
  display: flex;
  flex-direction: column;
  gap: 12px;
}
.formula-card {
  border: 1px solid var(--outline-variant);
  border-radius: var(--radius);
  overflow: hidden;
}
.formula-card-header {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 10px 12px;
  background: var(--surface-variant);
  cursor: pointer;
  transition: background 0.1s;
}
.formula-card-header:hover {
  background: var(--surface-container-highest);
}
.formula-card-header h3 {
  flex: 1;
  font-size: 13px;
  font-weight: 600;
  color: var(--on-surface);
  margin: 0;
}
.toggle-arrow {
  font-size: 18px;
  transition: transform 0.15s;
}
.toggle-arrow.collapsed {
  transform: rotate(-90deg);
}
.formula-card-body {
  padding: 12px;
  display: flex;
  flex-direction: column;
  gap: 10px;
}
.formula-desc {
  font-size: 12px;
  color: var(--on-surface-variant);
  line-height: 1.6;
}
.formula-desc strong {
  color: var(--on-surface);
}
.formula-display {
  padding: 6px 10px;
  background: var(--surface-container-highest);
  border-radius: var(--radius);
}
.formula-extra {
  margin-top: 8px;
}
.formula-extra h4 {
  font-size: 12px;
  font-weight: 600;
  color: var(--on-surface);
  margin: 8px 0 4px;
}
.formula-sub-title {
  font-size: 12px;
  font-weight: 700;
  color: var(--secondary);
  margin: 12px 0 4px;
  padding-left: 4px;
  border-left: 3px solid var(--secondary);
}
.matrix-grid.compact {
  margin-top: 4px;
}
.matrix-grid.compact .matrix-row {
  gap: 8px;
}
.matrix-grid.compact .matrix-row span {
  font-size: 10px;
  padding: 1px 3px;
}
.ik-methods {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 12px;
}
.ik-method {
  background: var(--surface-container-highest);
  padding: 10px;
  border-radius: var(--radius);
}
.ik-method h4 {
  font-size: 12px;
  font-weight: 600;
  color: var(--secondary);
  margin: 0 0 6px;
}
.ik-method ul {
  margin: 0;
  padding-left: 16px;
  font-size: 11px;
  color: var(--on-surface-variant);
  line-height: 1.8;
}

/* --- 3D 预览 --- */
.preview-content {
  display: flex;
  flex-direction: column;
  gap: 16px;
}
.preview-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
}
.preview-title {
  font-size: 18px;
  font-weight: 600;
  color: var(--on-surface);
  margin: 0;
}
.preview-actions {
  display: flex;
  gap: 6px;
}
.preview-viewport {
  aspect-ratio: 16/9;
  max-height: 400px;
  background: var(--surface-container-highest);
  border: 2px dashed var(--outline-variant);
  border-radius: var(--radius);
  display: flex;
  align-items: center;
  justify-content: center;
}
.viewport-placeholder {
  text-align: center;
  color: var(--outline);
  font-size: 12px;
}
.viewport-placeholder p {
  margin: 8px 0 0;
}
.dim-text {
  font-size: 10px;
  opacity: 0.6;
}
.preview-info {
  display: flex;
  gap: 16px;
}
.preview-info-item {
  display: flex;
  flex-direction: column;
  gap: 2px;
}
.info-label {
  font-size: 10px;
  color: var(--outline);
  text-transform: uppercase;
}
.info-value {
  font-size: 11px;
  color: var(--on-surface);
}

/* --- 诊断面板 --- */
.kin-diagnostics {
  height: 160px;
  flex-shrink: 0;
  background: var(--surface-container-low);
  border-top: 1px solid var(--outline-variant);
  display: flex;
  flex-direction: column;
}
.diag-tabs {
  display: flex;
  background: var(--surface-container-high);
  border-bottom: 1px solid var(--outline-variant);
  flex-shrink: 0;
}
.diag-tab {
  padding: 4px 12px;
  font-size: 11px;
  border: none;
  background: none;
  cursor: pointer;
  color: var(--on-surface-variant);
}
.diag-tab:hover {
  color: var(--on-surface);
}
.diag-tab.active {
  color: var(--primary);
  font-weight: 600;
  border-bottom: 2px solid var(--primary);
}
.diag-log {
  flex: 1;
  padding: 8px 12px;
  font-family: "JetBrains Mono", monospace;
  font-size: 11px;
  overflow-y: auto;
}
.log-line {
  margin: 0;
  padding: 1px 0;
  color: var(--on-surface-variant);
}
.log-info {
  color: var(--secondary);
}
.log-warn {
  color: var(--error);
}
.log-dim {
  opacity: 0.5;
}

/* ===== 右侧属性面板 ===== */
.kin-inspector {
  width: 280px;
  flex-shrink: 0;
  background: var(--surface-container);
  border-left: 1px solid var(--outline-variant);
  display: flex;
  flex-direction: column;
}
.inspector-header {
  padding: 12px;
  border-bottom: 1px solid var(--outline-variant);
}
.inspector-title {
  font-size: 14px;
  font-weight: 600;
  color: var(--on-surface);
  margin: 0;
}
.inspector-subtitle {
  font-size: 11px;
  color: var(--on-surface-variant);
  margin: 2px 0 0;
}
.inspector-body {
  flex: 1;
  overflow-y: auto;
}
.config-section {
  padding: 12px;
  border-bottom: 1px solid var(--outline-variant);
}
.config-label {
  font-size: 10px;
  font-weight: 700;
  letter-spacing: 0.05em;
  color: var(--primary);
  margin-bottom: 8px;
}

.prop-row {
  display: flex;
  align-items: center;
  gap: 6px;
  margin-bottom: 6px;
}
.prop-lbl {
  font-size: 10px;
  color: var(--on-surface-variant);
  width: 55px;
  flex-shrink: 0;
}
.prop-in {
  flex: 1;
  padding: 3px 6px;
  height: 24px;
  background: var(--surface-dim);
  border: 1px solid var(--outline-variant);
  color: var(--on-surface);
  font-size: 11px;
  outline: none;
  border-radius: var(--radius);
}
.prop-in:focus {
  border-color: var(--primary);
}
.code-font {
  font-family: "JetBrains Mono", monospace;
}

.axis-grid {
  display: grid;
  grid-template-columns: auto 1fr;
  gap: 4px;
  align-items: center;
}
.axis-name {
  font-size: 11px;
  color: var(--outline);
  text-align: right;
  padding-right: 6px;
}
.axis-input {
  background: var(--surface-dim);
  border: 1px solid var(--outline-variant);
  padding: 2px 4px;
  height: 24px;
  color: var(--secondary);
  font-size: 11px;
  outline: none;
  width: 100%;
}
.axis-input:focus {
  border-color: var(--primary);
}

.tcp-grid {
  display: grid;
  grid-template-columns: 1fr 1fr 1fr;
  gap: 6px;
}
.tcp-col {
  display: flex;
  flex-direction: column;
  gap: 4px;
}
.tcp-axis-label {
  font-size: 8px;
  font-weight: 700;
  text-align: center;
  color: var(--on-surface-variant);
}
.tcp-input {
  background: var(--surface-dim);
  border: 1px solid var(--outline-variant);
  padding: 4px;
  height: 32px;
  text-align: center;
  color: var(--on-surface);
  font-size: 13px;
  outline: none;
  width: 100%;
}
.tcp-input:focus {
  border-color: var(--primary);
}

.limits-list {
  display: flex;
  flex-direction: column;
  gap: 12px;
}
.limit-header {
  display: flex;
  justify-content: space-between;
  font-size: 11px;
  margin-bottom: 4px;
}
.limit-name {
  color: var(--on-surface-variant);
}
.limit-value {
  color: var(--on-surface);
}
.hmi-slider {
  width: 100%;
  -webkit-appearance: none;
  height: 4px;
  background: var(--surface-variant);
  border-radius: 2px;
  outline: none;
}
.hmi-slider::-webkit-slider-thumb {
  -webkit-appearance: none;
  width: 12px;
  height: 12px;
  background: var(--primary);
  border-radius: 0;
  cursor: pointer;
}

.inspector-actions {
  padding: 10px;
  display: flex;
  flex-direction: column;
  gap: 6px;
  border-top: 1px solid var(--outline-variant);
  background: var(--surface-container-high);
}
.action-btn {
  width: 100%;
  padding: 7px;
  font-size: 10px;
  font-weight: 700;
  letter-spacing: 0.05em;
  text-transform: uppercase;
  border: none;
  border-radius: var(--radius);
  cursor: pointer;
}
.apply-btn {
  background: var(--surface-variant);
  color: var(--on-surface);
  border: 1px solid var(--outline-variant);
}
.apply-btn:hover {
  background: var(--outline-variant);
}
.reset-btn {
  background: var(--error-container);
  color: var(--on-error-container);
}
.reset-btn:hover {
  opacity: 0.9;
}

/* 滚动条 */
.custom-scrollbar::-webkit-scrollbar {
  width: 4px;
  height: 4px;
}
.custom-scrollbar::-webkit-scrollbar-track {
  background: transparent;
}
.custom-scrollbar::-webkit-scrollbar-thumb {
  background: #334155;
  border-radius: 2px;
}
</style>
