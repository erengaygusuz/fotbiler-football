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
- aggregate W/D/L, goals, shots, user-possession, and scorer names;
- expose completion, W/D/L rates, average goals/shots/possession, goal difference, total goals, and shot conversion;
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

These are observations, not final gameplay-design decisions. Gaps such as fitness coupling remain visible instead of being silently changed inside the validation milestone.

## M1.5C — Statistical guardrails

`core/career/career_simulation_guardrails.hpp` defines explicit statistical envelopes for the current fast-simulation baseline. They are bands rather than golden values: the goal is to catch broken distributions while still allowing intentional tuning inside a plausible range.

The guardrails currently cover:

- average user goals for weak/equal/strong controlled-team strength scenarios;
- equal-strength home opponent goals and total-goal rate;
- equal-strength home win/draw/loss distribution;
- both teams' shot-to-goal conversion sanity;
- balanced home possession;
- paired home-vs-away goal-difference advantage;
- Possession and Counter Attack possession bands;
- CF/ST share of user goals, plus a zero-goalkeeper-scoring invariant for the canonical roster.

The scorer distribution is possible because `MatchSimulationTelemetry` now keeps aggregate scorer-name counts. This is intentionally generic enough for the M1.5D report writer to export the same data instead of rebuilding statistics in a separate tool.

The initial bands are deliberately wider than seeded sample variance. If a future gameplay change intentionally moves outside one of them, the expected workflow is to inspect the report first and then change the guardrail with a documented tuning reason, not simply widen tests until they pass.

## M1.5D — Headless runner

`tools/simulation/fotbiler_sim.cpp` is a standalone command-line runner for the scenario catalogue. It links only the career simulation core (`career_common.cpp` and `career_sim.cpp`); it does not depend on SDL, OpenGL, RmlUi, or the playable-match presentation stack.

The CMake option `GAMEPLAYFOOTBALL_BUILD_SIMULATION_TOOLS` controls the target and defaults to `ON`. The executable target is `fotbiler_sim`.

Typical usage:

```bash
./build/fotbiler_sim --runs 10000
./build/fotbiler_sim --scenario opposition_equal --runs 100000
./build/fotbiler_sim --scenario venue_home_equal --runs 50000 --seed 20260906
./build/fotbiler_sim --list
```

Unless overridden, output is written beside the executable under `simulation-reports/latest.csv` and `simulation-reports/latest.json`.

Custom output paths:

```bash
./build/fotbiler_sim \
  --runs 10000 \
  --csv build/reports/baseline.csv \
  --json build/reports/baseline.json
```

A previous CSV report can be supplied as a comparison baseline:

```bash
./build/fotbiler_sim \
  --runs 10000 \
  --baseline build/reports/baseline.csv
```

Baseline comparison is deliberately informational: it prints current-minus-baseline deltas for goals, goals against, goal difference, win rate, and possession. Statistical guardrail tests remain the hard regression gate; an intentional tuning change should not be blocked merely because it differs from an older report.

The CSV contains scenario inputs, seed, W/D/L, goals, shots, conversion, possession, and top scorer. JSON additionally contains the full scorer-goal map for each scenario.

CTest includes `GameplaySimulationRunner.Smoke`, which executes the real command-line tool on a small scenario and requires both CSV and JSON files to be writable.

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

- [x] Expected goal bands by strength gap
- [x] Home advantage band
- [x] Win/draw/loss distribution bands
- [x] Shot-to-goal sanity bands
- [x] Possession sanity and strategy deltas
- [x] Scorer-position distribution checks

### M1.5D — Headless runner

- [x] CLI/test executable for thousands of matches without presentation
- [x] CSV/JSON report output
- [x] Scenario seed recorded in every report
- [x] Optional comparison against a saved baseline

### M1.5E — Full 3D telemetry bridge

- [ ] Feed playable-match final score into the same report model
- [ ] Add shots/possession once the 3D engine exposes canonical match stats
- [ ] Compare fast-sim and 3D distributions without forcing them to be identical

## Exit criteria

M1.5 is complete when a developer can run a deterministic scenario suite headlessly, inspect stable aggregate metrics, detect statistically meaningful regressions, and compare fast-simulation behavior with the canonical 3D result boundary.

Only after that should M2 World + Date + Persistence and M3 Competition Engine rely heavily on these outcomes.
