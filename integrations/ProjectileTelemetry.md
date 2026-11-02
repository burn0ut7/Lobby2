# Projectile contacts

`GC_TelemetryImpactEffect` observes `BaseProjectileEffect.OnEffect` on the authority.
Only the active Lobby2 session and a running recorder may write contacts. The
callback queues records; it performs no file I/O and changes no physics or damage.

The override of `Prefabs/Weapons/Core/Ammo_Bullet_Base.et` adds this effect while
preserving all installed stock fields and effects. Source: game 1.8.0.13,
`data/data007.pak`, catalogue `721cd7b05eb54d3ca3591d9097af381d`, original SHA-256
`fabcb1ca18fc185b27ff0f15042c36edb9e2db2aecbf979812dc6e4e553a7427`.
Recompare this override after game updates. Other mods replacing this base or
its effect list can remove coverage. Ammunition not inheriting this base,
explosive detonation and continuous projectile trajectories are not covered.

An `impact` records the engine contact position (`outMat[0]`), surface normal
(`outMat[2]`), optional tracked target, `evidence=engine-event` and projectile ID.
The ID uses the existing bounded entity-identity map; contacts arriving before
the shot allocate the same ID that the later shot reuses. Contacts without a
recorded shot remain valid isolated events. Each ricochet/penetration contact
may produce another event; none claims a final resting point.

Shots retain `origin=projectile.GetOrigin()` sampled in the shot callback, with
`originEvidence=projectile-entity-at-shot`. `muzzlePosition` is separately recorded
when the character weapon manager supplies the matching current weapon muzzle.
These are server observations, not a promise of the remote client visual muzzle
at the original input time. Never replace a missing impact with entity origin,
velocity extrapolation, a nearby death or a damage callback zero vector.
Damage with no usable projectile reference remains uncorrelated.

## Validation, 2026-11-02

- Enforce reload passes; three existing Arland world GUID diagnostics remain.
- Stock M855 launched through the native movement component invokes the observer
  for a ground contact without actor damage. Origin remains at launch while
  the contact has a different position.
- User-operated M16 firing: 53 shots with muzzle positions. Together with one
  explicit native-flight probe: 54 shot records, 110 linked contacts, 42 damage
  events, 3 AI kills and vehicle data. Recorder sealed 1,241 records, zero drops.
- Recording `01a19dec-5106-8e74-8400-00000064f19e`, mission
  `01a19dec-1d2e-85c1-8400-00000062696c`, imported through production website
  delivery/projection; all records verified with website database permissions.
  Original 112 source files were unchanged.
- Unknown admin requester rejected. Firing after recording stopped left its
  sealed record count unchanged. In unrelated `SCR_BaseGameMode`, a real stock
  bullet hit terrain, session lookup was absent, and telemetry outbox file count
  and byte total stayed unchanged. Arland was restored afterwards.
- Normalized prefab comparison matches installed stock except the new effect.
  A fresh addon-free live baseline, dedicated-server/remote-client latency,
  other ammo families and complete AI shot coverage remain unverified.

Optional explicit probes are in `tests/McpTestAddon`; they are not deployed.
Website model regression tests cover early-contact ordering and successive
contacts; connecting observed points is not a simulated ballistic path.

## Initial actors and ownership (2026-11-02)

The server recorder discovers characters through native AIWorld.GetAIAgents in
bounded batches (eight agents per tick, refreshed once per second after draining).
Stationary AI receive a spawn with their initial transform; unchanged positions
do not need repeated samples. This uses native game identity, not movement or
combat as an admission trigger.

Unoccupied Lobby2 slots and native playable-group bodies are excluded until
observed under player control. Once admitted, a surviving body remains tracked
after ownership loss. Ownership is sampled separately from equipment, at the
actor's 200ms cadence, and emits an empty playerIdentity when vacated.
Destroyed/removed entities still despawn normally. Existing 256-active-actor
limits and explicit overflow behavior remain; this is not unlimited AI capture.

Live Arland proof: recording 01a19e06-4852-8ff7-8400-0000006a8b6b contains 18 AI,
two successively occupied player bodies, and 378 records with zero drops.
The stationary fixture stayed at (1180,39,2730); an unused slot at
(1185,39,2730) was excluded. An authenticated slot switch emitted empty ownership
at 19847.898437ms; the old body remained, with three later state samples.
Website intake/read APIs verified all 378 records. Optional explicit discovery
probe is in tests/McpTestAddon; it is not deployed.
