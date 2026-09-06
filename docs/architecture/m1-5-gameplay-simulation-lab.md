# M1.5 — Gameplay Simulation Lab

## Goal

Build a repeatable, headless-friendly validation layer around the match-engine boundary before the world, competition, rules, transfer, and long-term career systems begin depending on match outcomes.

M1.5 is primarily an observability and verification milestone. It should make gameplay/simulation changes measurable before it becomes a large tuning project.

## Baseline audit

The M1 architecture already gives M1.5 a useful seam:

- `IMatchEngine` is the canonical application-facing match boundary.
- `FastMatchEngine` resolves through `CareerSim::SimulateMatchResult` without launching presentation.
- `Full3DMatchEngine` reports that interactive play is required and normalizes the playable result on completion.
- `CareerCommon::SeedRng` can make the current career RNG sequence reproducible.
- Career tests already include result bounds, relative-strength regression guards, and a 12-season/four-persona long-run audit.

The missing layer is reusable match-level batch telemetry that can be shared by focused scenarios and future tuning tools.

## First foundation

`core/match/match_simulation_lab.hpp` introduces a small engine-agnostic harness:

- run one `MatchRequest` repeatedly through any `IMatchEngine`;
- distinguish completed headless runs from interactive runs;
- aggregate W/D/L, goals, shots, and user-possession;
- expose completion rate, win rate, average goals, shots, possession, and goal difference;
- keep seeding outside the generic match layer so the core match abstraction does not depend on career RNG internals.

The initial integration tests exercise the real `FastMatchEngine` with seeded RNG and verify that:

1. a seeded batch is reproducible;
2. a much stronger squad produces a better large-sample goal difference and win rate;
3. `Full3DMatchEngine` is classified as interactive instead of being mistaken for a completed headless result;
4. invalid/non-positive batch sizes are harmless.

## Determinism note

`CareerSim::SimulateMatchResult` currently derives named-opponent strength from `std::hash<std::string>`. The seeded random sequence is reproducible inside the same runtime/toolchain, but the C++ standard does not require `std::hash` values to be identical across all standard-library implementations.

For cross-platform golden scenarios, M1.5 should later replace that identity-strength derivation with an explicit stable hash or data-driven opponent rating.

## Planned M1.5 slices

### M1.5A — Batch telemetry foundation

- [x] Generic `IMatchEngine` batch runner
- [x] Aggregate W/D/L, goals, shots, possession
- [x] Seeded real-engine regression tests
- [x] Interactive-vs-headless classification

### M1.5B — Scenario catalogue

- [ ] Named deterministic scenarios (weak/equal/strong opposition)
- [ ] Home/away pairs
- [ ] Strategy matrix (Balanced, Attacking, Defensive, High Pressing, Counter Attack, Possession)
- [ ] Morale/form/fitness perturbations
- [ ] Empty/minimal roster edge cases

### M1.5C — Statistical guardrails

- [ ] Expected goal bands by strength gap
- [ ] Home advantage band
- [ ] Win/draw/loss distribution bands
- [ ] Shot-to-goal sanity bands
- [ ] Possession sanity and strategy deltas
- [ ] Scorer-position distribution checks

### M1.5D — Headless runner

- [ ] CLI/test executable for thousands of matches without presentation
- [ ] CSV/JSON report output
- [ ] Scenario seed recorded in every report
- [ ] Optional comparison against a saved baseline

### M1.5E — Full 3D telemetry bridge

- [ ] Feed playable-match final score into the same report model
- [ ] Add shots/possession once the 3D engine exposes canonical match stats
- [ ] Compare fast-sim and 3D distributions without forcing them to be identical

## Exit criteria

M1.5 is complete when a developer can run a deterministic scenario suite headlessly, inspect stable aggregate metrics, detect statistically meaningful regressions, and compare fast-simulation behavior with the canonical 3D result boundary.

Only after that should M2 World + Date + Persistence and M3 Competition Engine rely heavily on these outcomes.
