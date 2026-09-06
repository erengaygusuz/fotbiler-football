# M1A — Modern UI Foundation

M1A modernizes Fotbiler Football's player-facing interface without requiring a flag-day rewrite of the legacy Gui2 screens.

## Technology decision

The new UI foundation uses **RmlUi 6.3**, pinned through CMake FetchContent.

Why RmlUi:

- C++ native and engine-agnostic;
- MIT licensed;
- official SDL2 and OpenGL 3 backend implementations;
- document/layout model suited to dense football-management screens;
- RCSS styling, variables, media queries, transitions, and animations;
- data binding support for future view-model driven screens.

Dear ImGui remains a possible future developer/debug tooling layer, not the primary player-facing UI.

## Migration strategy

```text
New Fotbiler screens -> RmlUi -> SDL2/OpenGL3
Legacy screens       -> Gui2  -> existing renderer
```

Gui2 remains operational during M1A. New screens must not add new reusable UI concepts directly to Gui2 unless required for compatibility.

## M1A phases

1. **RmlUi integration**
   - pin RmlUi 6.3;
   - compile SDL2 + OpenGL3 backend integration;
   - establish `RmlUiSystem` ownership boundary.
2. **Theme and typography**
   - Fotbiler design tokens through RCSS custom properties;
   - font loading and type scale;
   - base focus/hover/disabled states.
3. **Base components**
   - button, icon button, card, panel, badge, progress, tabs, modal, toast;
   - reusable layout primitives through RML/RCSS patterns.
4. **Modern main menu**
   - first complete migrated screen;
   - keyboard, mouse, and controller navigation.
5. **Career shell and dashboard**
   - persistent top bar and sidebar;
   - next match, league, inbox, board, finance, and squad cards.
6. **Football-heavy showcase**
   - squad or tactics screen using reusable football components.
7. **Polish**
   - transitions, animation, scaling, ultrawide behavior, accessibility/readability pass.

## Dependency boundary

`RmlUiSystem` lives under `src/presentation/ui/rmlui/`. It owns the RmlUi context and backend interfaces, not football domain state.

Expected flow:

```text
Career / Match ViewModel
          |
          v
Fotbiler presentation components
          |
          v
RmlUiSystem
          |
          v
RmlUi SDL2 + OpenGL3 backend
          |
          v
Existing SDL window / OpenGL context
```

## M1A.1 exit criteria

- RmlUi 6.3 is pinned and reproducibly fetched by CMake.
- RmlUi core plus SDL2/OpenGL3 backend code compiles in the normal game build.
- `RmlUiSystem` initializes against an existing SDL window/OpenGL context and owns update/render/input boundaries.
- No existing Gui2 screen is removed yet.
- Existing regression suite remains green.

Runtime hookup and the first visible RML document are the next step after this compile boundary is validated.
