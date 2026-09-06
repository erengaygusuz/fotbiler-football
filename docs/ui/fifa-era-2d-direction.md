# Fotbiler 2D UI Art Direction

## Target era

Fotbiler player-facing 2D UI targets the visual and interaction language of the
Frostbite FIFA era, specifically FIFA 17 through FIFA 22. EA FC-era menu design
is not a reference target for M1A.

This is an art-direction reference, not an asset-copying plan. Fotbiler uses its
own branding, colors, icons, imagery, typography choices, layouts, and content.
No proprietary EA/FIFA artwork is included.

## Current scope

3D menu scenes, manager cinematics, transfer negotiation cinematics, tunnel
presentation, and other 3D UI presentation are intentionally deferred.

Everything that can be delivered as 2D UI should still feel like it belongs to
the FIFA 17-22 family of football interfaces.

## Core rules

1. Football-first, not app-first. Screens should feel like a football game, not
   a generic web dashboard.
2. Use asymmetric tile hierarchy. Important actions receive larger visual
   regions than secondary actions.
3. Selection must be unmistakable through a strong accent surface, motion,
   border, or contrast change.
4. Controller navigation is primary. Mouse and keyboard remain supported.
5. Keep common destinations shallow. Avoid burying core actions in deep menus.
6. Use strong full-screen composition: broad color fields, football imagery,
   competition branding, and generous negative space.
7. Use short, bold headings with restrained supporting copy.
8. Keep a consistent bottom action-hint band for Select, Back, tabs, calendar,
   and contextual actions.
9. Motion should be fast and restrained, generally around 150-250 ms.
10. Prefer contextual visual information over dense explanatory text.

## Reference blend

- FIFA 17-19: tile structure, hierarchy, readable focus states, fast menu flow.
- FIFA 20-21: Career Central presentation, news-driven surfaces, competition
  identity and contextual branding.
- FIFA 22: transition polish and controller-first usability.

## 2D component direction

The initial reusable vocabulary should include:

- top navigation tabs
- hero tile
- secondary action tile
- news tile
- fixture tile
- player card
- club/competition badge slot
- table/list row
- stat strip
- progress/form indicator
- modal/confirmation panel
- bottom controller-hint bar

## M1A visual gate

Before replacing legacy Gui2 screens, the standalone `fotbiler_ui_preview`
should establish an approved Fotbiler visual language for:

1. Main Menu
2. Career Central
3. Squad
4. Tactics
5. Transfers

Once these 2D prototypes are coherent, the same RML/RCSS documents and
components are connected to the live game navigation and data model.
