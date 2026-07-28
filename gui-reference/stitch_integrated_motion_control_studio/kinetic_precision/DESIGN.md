---
name: Kinetic Precision
colors:
  surface: '#0b1326'
  surface-dim: '#0b1326'
  surface-bright: '#31394d'
  surface-container-lowest: '#060e20'
  surface-container-low: '#131b2e'
  surface-container: '#171f33'
  surface-container-high: '#222a3d'
  surface-container-highest: '#2d3449'
  on-surface: '#dae2fd'
  on-surface-variant: '#c2c6d6'
  inverse-surface: '#dae2fd'
  inverse-on-surface: '#283044'
  outline: '#8c909f'
  outline-variant: '#424754'
  surface-tint: '#adc6ff'
  primary: '#adc6ff'
  on-primary: '#002e6a'
  primary-container: '#4d8eff'
  on-primary-container: '#00285d'
  inverse-primary: '#005ac2'
  secondary: '#4edea3'
  on-secondary: '#003824'
  secondary-container: '#00a572'
  on-secondary-container: '#00311f'
  tertiary: '#fbabff'
  on-tertiary: '#580065'
  tertiary-container: '#e14ef6'
  on-tertiary-container: '#4d0059'
  error: '#ffb4ab'
  on-error: '#690005'
  error-container: '#93000a'
  on-error-container: '#ffdad6'
  primary-fixed: '#d8e2ff'
  primary-fixed-dim: '#adc6ff'
  on-primary-fixed: '#001a42'
  on-primary-fixed-variant: '#004395'
  secondary-fixed: '#6ffbbe'
  secondary-fixed-dim: '#4edea3'
  on-secondary-fixed: '#002113'
  on-secondary-fixed-variant: '#005236'
  tertiary-fixed: '#ffd6fd'
  tertiary-fixed-dim: '#fbabff'
  on-tertiary-fixed: '#36003e'
  on-tertiary-fixed-variant: '#7c008e'
  background: '#0b1326'
  on-background: '#dae2fd'
  surface-variant: '#2d3449'
typography:
  headline-sm:
    fontFamily: Inter
    fontSize: 18px
    fontWeight: '600'
    lineHeight: 24px
  body-md:
    fontFamily: Inter
    fontSize: 13px
    fontWeight: '400'
    lineHeight: 18px
  body-sm:
    fontFamily: Inter
    fontSize: 11px
    fontWeight: '400'
    lineHeight: 16px
  code-md:
    fontFamily: JetBrains Mono
    fontSize: 13px
    fontWeight: '400'
    lineHeight: 20px
  code-sm:
    fontFamily: JetBrains Mono
    fontSize: 11px
    fontWeight: '400'
    lineHeight: 16px
  label-caps:
    fontFamily: Inter
    fontSize: 10px
    fontWeight: '700'
    lineHeight: 12px
    letterSpacing: 0.05em
rounded:
  sm: 0.125rem
  DEFAULT: 0.25rem
  md: 0.375rem
  lg: 0.5rem
  xl: 0.75rem
  full: 9999px
spacing:
  unit: 4px
  sidebar-width: 260px
  inspector-width: 300px
  toolbar-height: 40px
  gutter: 1px
  padding-xs: 4px
  padding-sm: 8px
  padding-md: 12px
---

## Brand & Style
This design system is engineered for professional PLC and motion control environments where precision, reliability, and high information density are paramount. The brand personality is "Modern Engineering"—it is utilitarian, high-performance, and technically sophisticated. It aims to evoke a sense of absolute control and stability, similar to high-end industrial machinery.

The visual style is a blend of **Minimalism** and **Modern Corporate**, optimized for long-duration desktop use. It avoids decorative flourishes in favor of structural clarity. The UI follows a strict "density-first" philosophy, ensuring that complex logic and multi-axis motion data are visible without excessive scrolling. The aesthetic is defined by sharp lines, subtle tonal shifts for hierarchy, and vibrant semantic accents that draw the eye only when action or attention is required.

## Colors
The palette is built on a deep "Midnight Navy" foundation to reduce eye strain during extended engineering sessions. 

- **Primary (Action Blue):** Used for active states, primary buttons, and focused syntax elements.
- **Secondary (Logic Green):** Reserved for "Running," "True," or "Safe" states in logic monitoring.
- **Tertiary (CNC Magenta):** Specifically used for motion axes, physical I/O mapping, and specialized hardware triggers.
- **Neutral (Slate & Navy):** These define the UI structure. The darkest shade is for the editor background, while lighter slates define sidebars, panels, and toolbars.
- **Warning (Orange):** Dedicated to non-critical alerts and forced-value indicators in the PLC logic.

## Typography
The typography system prioritizes legibility and vertical rhythm. 

- **UI Sans-Serif (Inter):** Used for all navigational elements, property labels, and menu items. It provides a clean, modern feel that remains legible at small sizes.
- **Technical Monospace (JetBrains Mono):** Used for the core editor (Structured Text, Instruction List) and the terminal/output panels. The increased x-height and distinct characters prevent errors in variable naming and logic syntax.
- **High Density:** Standard body text is set to 13px, with 11px used for dense property inspectors and tree views to maximize the visibility of deep nested hierarchies.

## Layout & Spacing
The layout follows a **Fixed-Grid / Modular Panel** approach, typical of professional IDEs. The workspace is divided into four main zones:
1. **Activity Bar:** A narrow vertical strip on the far left for top-level navigation.
2. **Sidebars:** Hierarchical tree views (Project Explorer) on the left and Property Inspectors on the right.
3. **Editor Space:** A central tabbed interface for code and graphical logic (Ladder/FBD).
4. **Bottom Panel:** For diagnostic output, terminal, and watch lists.

Spacing is based on a tight 4px scale. Components are packed closely with 1px borders (gutters) separating major panels to maximize screen real estate. Use "Condensed" spacing for lists and trees (4px vertical padding per row).

## Elevation & Depth
In this design system, depth is communicated through **Tonal Layering** and **Low-Contrast Outlines** rather than shadows. 

- **Level 0 (Background):** The darkest navy (#0F172A) is reserved for the code editor background.
- **Level 1 (Surface):** Sidebars and bottom panels use a slightly lighter slate (#1E293B) to provide contrast against the editor.
- **Level 2 (UI Elements):** Toolbars and active tab headers use the lightest slate tier to appear "closest" to the user.
- **Borders:** Panels are separated by a 1px solid border (#334155). Avoid shadows entirely to maintain a crisp, industrial feel. Only use a subtle glow for active "online" status indicators.

## Shapes
The design system utilizes **Soft (0.25rem)** roundedness for buttons and input fields to provide a modern touch without sacrificing the "engineered" precision of the layout. 

- **Structural Elements:** Panels, tabs, and window frames use 0px (sharp) corners to maintain a seamless, integrated look.
- **Interactive Elements:** Buttons, chips, and dropdowns use a 4px radius. 
- **Status Indicators:** Small circular pips (100% rounded) are used for binary status (On/Off/Fault).

## Components
- **Tabbed Editor:** Active tabs feature a top-border highlight in Action Blue. Inactive tabs have no background fill and use subtle grey text.
- **Tree Views:** Use 16px chevron icons for expansion. Hover states should highlight the entire row with a subtle slate tint. Selected variables are highlighted in a low-opacity Action Blue.
- **Property Inspectors:** A two-column grid. The left column (Property Name) is right-aligned with a subtle grey text; the right column (Value) is left-aligned with JetBrains Mono for data clarity.
- **Buttons:** Primary buttons are solid Action Blue with white text. Secondary buttons are ghost-style with a 1px Slate border.
- **Input Fields:** Flat styling with a 1px border. On focus, the border transitions to Action Blue. Invalid logic entries trigger a Warning Orange border.
- **Logic Nodes (FBD/Ladder):** Use sharp rectangular containers with 1px borders. Inputs on the left, outputs on the right. Lines (wires) should be 2px thick, turning Logic Green when the signal is "True" in online mode.