# C4D RelaxTool

A dedicated interactive mesh relaxation and topology smoothing brush plugin for Maxon Cinema 4D (2025 and 2026), built in C++.

RelaxTool provides professional topology regularization directly on the active PolygonObject. It combines classic Laplacian smoothing with modern shape-preserving algorithms, including **Tangential Slide** (surface curvature and volume preservation), **Initial Surface Reprojection** (100% volume and silhouette preservation), and **Crease / Hard Edge Protection**.

---

## Features

- **Advanced Deformation & Shape Preservation**:
  - **Tangential Slide**: Restricts displacement to each vertex's local tangent plane. Equalizes edge lengths and relaxes topology without inward collapse, flattening, or volume loss.
  - **Project to Initial Surface**: Evaluates relaxation tangentially and snaps vertices back onto the pre-stroke surface triangles in real time, delivering mathematically exact silhouette and volume preservation.
  - **Standard Laplacian**: Isotropic 3D smoothing for removing bumps, surface noise, and irregularities.
  - **Hard Edge (Crease) Preservation**: Automatically identifies sharp feature edges using a dihedral angle threshold. Vertices along creases slide strictly along the ridge contour, keeping chamfers and hard edges crisp while locking corner junctions.
- **Intelligent Relax Modes**:
  - **Auto-lock**: Automatically determines stroke intent based on initial click position. If initiated near mesh boundary edges, it relaxes boundary contours while locking interior vertices. If initiated on interior surfaces, it relaxes interior vertices while strictly locking mesh boundaries.
  - **Interior Only**: Smooths interior topology only; outer silhouette and boundaries remain fixed.
  - **Border Only**: Smooths boundary vertex flow along the silhouette (preserving corner vertices).
  - **All Vertices**: Simultaneously smooths both interior and boundary vertices within brush influence.
- **Stroke-Level Performance Optimization**:
  - Half-edge topology and neighbor graphs are precomputed once at stroke initiation ($O(N \log N)$), ensuring buttery-smooth 60+ FPS brush strokes even on dense meshes.
- **Multi-Pass Iterations**:
  - Configurable iterations (1 to 20) per drag increment for rapid topology relaxation.
- **Selection Masking**:
  - Optional "Respect Vertex Selection" mode to restrict smoothing strictly to active Point Selections.
- **Interactive Viewport Feedback**:
  - Viewport brush circle display with customizable idle and active brush colors.
  - Native Cinema 4D brush resizing HUD with floating cursor statistics.
- **Full Undo Support**:
  - Fully undo-safe (`Ctrl+Z`).

---

## Deformation Algorithms & Shape Preservation

### 1. Tangential Slide (Default)
In standard Laplacian smoothing, the centroid of neighboring vertices on curved surfaces always lies inside the volume, causing inward shrinking. Tangential Slide computes vertex normals and projects the displacement vector $\mathbf{d}$ onto the tangent plane:
$$\mathbf{d}_{\text{tangent}} = \mathbf{d} - (\mathbf{d} \cdot \mathbf{n}) \mathbf{n}$$
Vertices glide smoothly along the curvature without collapsing inward or flattening organic shapes.

### 2. Project to Initial Surface
For hard-surface models, CAD geometry, or precise organic sculpts where geometric deviation must be zero. Vertices are relaxed tangentially and then immediately projected onto the closest point of the initial surface triangles captured at stroke start. Volume change is 0.000%, and the original profile is completely preserved.

### 3. Hard Edge (Crease) Preservation
When enabled, edges with dihedral angles exceeding the **Crease Angle** (default 45°) are treated as structural feature edges. Vertices on continuous creases only relax along the 1D crease curve, while crease corners and junctions are locked to prevent rounding off sharp corners.

---

## Controls

| Action | Function |
|---|---|
| **LMB Drag** | Relax mesh vertices (brush stroke) |
| **MMB Drag** (or **Ctrl + RMB Drag**) | Interactive brush adjustment: horizontal drag adjusts **Radius**, vertical drag adjusts **Strength** |
| **`[` / `]`** | Decrease / Increase brush radius |
| **Esc** | Cancel active drag operation |

---

## Tool Settings

- **Radius (px)**: Brush radius in screen space pixels (5 to 500 px).
- **Strength**: Smoothing strength factor per step (0.01 to 1.0).
- **Iterations**: Number of smoothing passes per drag increment (1 to 20).
- **Relax Mode**: Boundary constraint mode (`Auto-lock`, `Interior Only`, `Border Only`, `All Vertices`).
- **Deformation**: Relaxation algorithm:
  - `Tangential Slide`: Glides vertices along surface curvature without volume loss.
  - `Project to Initial Surface`: 100% volume and profile preservation via local triangle reprojection.
  - `Standard Laplacian`: 3D isotropic smoothing for evening out surface bumps.
- **Preserve Hard Edges (Creases)**: Protects sharp edges and ridge features from collapsing across creases.
- **Crease Angle**: Dihedral angle threshold in degrees (1° to 179°) for detecting crease edges.
- **Respect Vertex Selection**: When enabled, relaxes only selected vertices.
- **Brush Color**: Viewport brush circle color in idle state.
- **Active Brush Color**: Viewport brush circle color while actively smoothing.

---

## Installation

1. Download the latest release (`C4D_RelaxTool_v1.0.1.zip`) from the [Releases](https://github.com/SPluzh/C4D_RelaxTool/releases) section.
2. Extract the corresponding version folder (`2025` or `2026`) into your Cinema 4D plugins directory:
   - **Windows**: `C:\Users\<User>\AppData\Roaming\Maxon\Maxon Cinema 4D <Version>\plugins\C4D_RelaxTool\`
3. Restart Cinema 4D.
4. Find **Relax Tool** in the Cinema 4D Tools / Extensions menu.

---

## Building from Source

- **Requirements**: Visual Studio 2022 (C++20), CMake 3.30+, Cinema 4D SDK 2025 / 2026.
- Run `build_2026.bat` (for Cinema 4D 2026) or `build_2025.bat` (for Cinema 4D 2025).
- Run `pack_release.bat` to generate the release distribution ZIP archive.
