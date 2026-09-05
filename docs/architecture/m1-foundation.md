# M1 — Core Architecture Foundation

M1 establishes dependency boundaries for Fotbiler Football without changing the existing playable career surface.

## Dependency direction

```text
Presentation / legacy menu
        |
        v
Application / orchestration
        |
        v
Core career + match contracts
        |
        v
Persistence / external adapters
```

Core code must not depend on Gui2, menu pages, localization, SQLite, or League-Soccer presentation flow.

## Canonical boundaries

### Core career

`src/core/career/` owns career rules and state transformations, including simulation, transfers, finance, board, staff, training, sponsors, fixture-session state, and career-event state.

### Application

`src/application/career/` owns use-case orchestration that is neither presentation nor domain logic. M1 includes career save-slot lifecycle and initial career-state creation.

### Persistence

`src/persistence/career/` owns save serialization/storage and concrete SQLite adapters. Transfer rules depend on `ITransferMarketRepository`; SQLite implements that boundary rather than leaking into the transfer domain.

### Presentation

`src/presentation/career/` owns localized or display-oriented career text. Domain code returns domain values such as `FinancialHealth`; presentation maps those values to localized labels.

### Match engines

`src/core/match/match_engine.hpp` defines the common `IMatchEngine`, `MatchRequest`, and normalized `MatchResult` contract.

- `FastMatchEngine` adapts the existing career simulator.
- `Full3DMatchEngine` is the League-Soccer playable-match adapter boundary.
- Common results use controlled-club perspective (`userGoals` / `opponentGoals`) so fast simulation and raw home/away 3D scores cannot be confused.

## Legacy compatibility

`CareerDatabase` remains as a compatibility facade/orchestrator for the existing menu and game flow. It delegates save lifecycle, event/reputation state, presentation, career initialization, career modules, and match execution to the appropriate boundaries.

Legacy `src/menu/career/*.hpp` compatibility headers and small `.cpp` build-list wrappers may remain temporarily while consumers and CMake manifests are migrated. They must not become the canonical home of domain logic.

## M1 exit state

M1 is code-complete when:

- career domain modules have canonical homes outside `menu/`;
- persistence and SQLite are isolated from core career rules;
- localization/presentation is isolated from core career rules;
- career save and initialization orchestration have application boundaries;
- fast and full-3D match paths share a common match-engine contract;
- the existing build and regression suite still pass.

Additional architecture-specific tests and removal of the remaining compatibility wrappers are follow-up cleanup after the M1 regression gate, not prerequisites for defining the architecture boundary.
