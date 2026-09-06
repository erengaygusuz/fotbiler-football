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

## M1.5A — Batch telemetry foundation

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

## M1.5B — Scenario catalogue

`core/career/career_simulation_scenarios.hpp` adds a deterministic career-focused scenario layer on top of the generic match harness.

The default catalogue currently contains 22 scenarios covering:

- controlled-team strength: weak / equal / strong;
- opposition strength: weak / equal / strong;
- paired home and away fixtures;
- all six existing strategy strings: Balanced, Attacking, Defensive, High Pressing, Counter Attack, Possession;
- low/high morale, form, and fitness perturbations;
- empty-roster and one-player-roster edge cases.

Comparative scenarios in the same family intentionally reuse the same RNG seed. This reduces noise when we compare aggregate behavior because only the scenario input changes.

### Stable opponent ratings

Named opponents currently derive strength from `std::hash<std::string>`. The C++ standard does not require those hash values to match across standard-library implementations.

The scenario catalogue therefore leaves `opponentName` empty and uses the existing numeric opponent-id path. The current formula maps numeric id `N` to rating `45 + (N mod 44)`, so ids `0..43` give explicit ratings `45..88`. M1.5 scenarios can therefore target an exact opponent strength without introducing a second simulation formula or depending on implementation-defined hashing.

### Current baseline observations

The scenario matrix intentionally records behavior before tuning it:

- home/away affects expected goals and possession in the current fast simulation;
- strategy directly changes attack/defense and possession deltas;
- morale and `matchForm` feed team attack/defense calculations;
- `fitness` is represented in the catalogue but is not currently consumed by `SimulateMatchResult`;
- an empty roster is still simulatable because the current fast simulation falls back to default team values, while scorer generation is skipped when no roster players exist.

These are observations, not final gameplay-design decisions. M1.5C will turn the intended behaviors into explicit statistical guardrails and will make gaps such as fitness coupling visible instead of silently changing them.

## Planned M1.5 slices

### M1.5A — Batch telemetry foundation

- [x] Generic `IMatchEngine` batch runner
- [x] Aggregate W/D/L, goals, shots, possession
- [x] Seeded real-engine regression tests
- [x] Interactive-vs-headless classification

### M1.5B — Scenario catalogue

- [x] Named deterministic scenarios (weak/equal/strong opposition)
- [x] Controlled-team strength variants
- [x] Home/away pairs
- [x] Strategy matrix (Balanced, Attacking, Defensive, High Pressing, Counter Attack, Possession)
- [x] Morale/form/fitness perturbations
- [x] Empty/minimal roster edge cases
- [x] Stable numeric opponent ratings for cross-toolchain scenario inputs

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
