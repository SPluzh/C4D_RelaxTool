# C4D RelaxTool

A dedicated interactive mesh relaxation and Laplacian smoothing brush plugin for Maxon Cinema 4D (2025 and 2026), built in C++.

Extracted and refined from the QuadDraw relax brush, RelaxTool operates directly on the active PolygonObject with pure Laplacian smoothing—requiring no target surface or surface projection.

---

## Features

- **Pure Laplacian & Advanced Shape Preservation**:
  - Smooths vertices directly on the active PolygonObject.
  - Screen-space brush radius with smoothstep falloff (`t^2 * (3 - 2t)`).
  - **Deformation Algorithms**:
    - **Tangential Slide**: Restricts relaxation strictly to the surface tangent plane. Equalizes vertex spacing without inward collapsing or volume shrinkage.
    - **Project to Initial Surface**: Mathematically preserves 100% of the original surface silhouette and volume by projecting relaxed vertices back to the pre-stroke surface triangles.
    - **Standard Laplacian**: Classic 3D isotropic smoothing for evening out surface bumps and wrinkles.
  - **Hard Edge (Crease) Preservation**:
    - Automatically detects sharp edges via dihedral angle threshold.
    - Vertices on creases slide strictly along the crease contour without rounding off sharp chamfers and hard edges.
    - Crease corners and junctions remain locked.
- **Intelligent Relax Modes**:
  - **Auto-lock**: Automatically determines stroke intent based on initial click position. If initiated near mesh boundary edges, it relaxes boundary contours while locking interior vertices. If initiated on interior surfaces, it relaxes interior vertices while strictly locking mesh boundaries.
  - **Interior Only**: Smooths interior topology only; outer silhouette and boundaries remain fixed.
  - **Border Only**: Smooths boundary vertex flow along the silhouette (preserving corner vertices).
  - **All Vertices**: Simultaneously smooths both interior and boundary vertices within brush influence.
- **Multi-Pass Iterations**:
  - Configurable iterations (1 to 20) per drag step for rapid topology relaxation on dense meshes.
- **Selection Masking**:
  - Optional "Respect Vertex Selection" mode to restrict smoothing strictly to active Point Selections.
- **Interactive Viewport Feedback**:
  - Clean viewport circle display with customizable idle and active brush colors.
  - Native Cinema 4D brush resizing HUD with floating cursor statistics.
- **Full Undo Support**:
  - Fully undo-safe (`Ctrl+Z`).

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
- **Deformation**: Relaxation algorithm (`Tangential Slide`, `Project to Initial Surface`, `Standard Laplacian`).
- **Preserve Hard Edges (Creases)**: Protects sharp edges and ridge features from collapsing.
- **Crease Angle**: Dihedral angle threshold in degrees (1° to 179°) for detecting crease edges.
- **Respect Vertex Selection**: When enabled, relaxes only selected vertices.
- **Brush Color**: Viewport brush circle color in idle state.
- **Active Brush Color**: Viewport brush circle color while actively smoothing.

---

## Installation

1. Download the latest release from the [Releases](https://github.com/SPluzh/C4D_RelaxTool/releases) section.
2. Extract the corresponding version folder (`2025` or `2026`) into your Cinema 4D plugins directory:
   - **Windows**: `C:\Users\<User>\AppData\Roaming\Maxon\Maxon Cinema 4D <Version>\plugins\C4D_RelaxTool\`
3. Restart Cinema 4D.
4. Find **Relax Tool** in the Cinema 4D Tools / Extensions menu.

---

## Building from Source

- **Requirements**: Visual Studio 2022 (C++20), CMake 3.30+, Cinema 4D SDK 2025 / 2026.
- Run `build_2026.bat` (for Cinema 4D 2026) or `build_2025.bat` (for Cinema 4D 2025).
- Run `pack_release.bat` to generate the release distribution ZIP archive.
