# ZeGunner

An Unreal Engine 5.7 turret defense game. Defend your base from waves of tanks and helicopters by controlling a turret that fires rockets. The turret is positioned at (0, 0, 500) and can rotate 360 degrees following the mouse, with height adjustable via Q/E keys.

---

## Turret Pawn Setup

The **FighterPawn** (turret mode) is a first-person turret pawn fixed at position (0, 0, height). The mouse directly controls turret rotation (yaw and pitch). Left-click fires rockets that destroy both tanks and helicopters.

### Controls

| Key | Action |
|-----|--------|
| **Mouse** | Aim turret (360° yaw, clamped pitch) — crosshair always centered |
| **Left Click** | Fire rockets (hold for auto-fire) |
| **Q** | Move turret down |
| **E** | Move turret up |
| **Mouse Wheel Up** | Zoom in (up to 10x) |
| **Mouse Wheel Down** | Zoom out |
| **[** | Radar zoom in |
| **]** | Radar zoom out |
| **F** | Toggle FPS display |
| **Arrow Up/Down** | Adjust volume |
| **Arrow Left/Right** | Adjust mouse sensitivity |
| **ESC** | Pause game |
| **C** | Continue / Start wave / Resume / Restart |
| **X** | Quit game (only when paused) |

### Game Flow

1. **Instructions screen** — Game starts showing built-in instructions. Press **C** to begin.
2. **Wave gameplay** — Enemies spawn and advance toward the base at (0,0,0). Destroy all enemies to complete the wave.
3. **Wave end screen** — Shows wave duration, kill stats, and base HP. Press **C** for the next wave.
4. **Pause** — Press **ESC** to pause. Shows instructions + quit option. Press **C** to resume.
5. **Game Over** — If base HP reaches 0, the game over screen appears. Press **C** to restart.

### Base Defense

- Enemies stop at their line of fire and shoot the base at variable intervals.
- Each hit reduces **Base HP** by 1 and flashes the screen red.
- Base HP is shown in the top-left score panel. Turns red when below 25%.
- If Base HP reaches 0, you lose.
- **Rockets destroy both tanks and helicopters.**

### Wave System

- Waves are managed by `TankWaveSpawner` and `HeliWaveSpawner` actors in the level.
- Spawners wait for the turret pawn to trigger each wave (no auto-spawning).
- Each wave adds more enemies (configurable per-spawner).
- **Enemy speed increases per wave** — both min and max speed grow each wave, capped at absolute maximums.
- Kill counters show **X/Y** format (destroyed/total in current wave).

### Crosshair

- **White + crosshair** — Always centered on screen. Rockets fire toward this point.
- The turret follows mouse movement directly for maximum responsiveness.

### Height Control (Q/E Keys)

Use **Q** and **E** keys to move the turret up and down within configurable height limits.

| Parameter | Description | Default |
|-----------|-------------|---------|
| **Min Turret Height** | Minimum Z position the turret can go | 100 |
| **Max Turret Height** | Maximum Z position the turret can go | 5000 |
| **Height Change Speed** | Speed of vertical movement (units/sec) | 300 |
| **Start Altitude** | Starting Z position of the turret | 3000 |

### Radar System

A square radar display in the top-right corner shows enemies within range. The radar rotates with the turret's heading so "up" on radar is always forward.

| Parameter | Description | Default |
|-----------|-------------|---------|
| **Radar World Range** | How far the radar can detect enemies (units) | 15000 |
| **Radar Radius** | Visual size of radar on screen (pixels) | 90 |
| **Radar Zoom** | Current zoom level (1.0 = default, lower = zoomed in) | 1.0 |
| **Radar Zoom Step** | How much zoom changes per key press | 0.15 |
| **Radar Zoom Min/Max** | Zoom limits | 0.2 / 3.0 |

#### Radar Elements
- **Green triangle** — Player (turret) at center, pointing forward
- **Red diamond dots** — Tanks on the ground
- **Yellow circle dots** — Helicopters (with vertical height bar below)
- **Square border** — Outer radar boundary
- **Concentric rings** — Distance markers (66% and 33% of range)

#### Radar Controls
- **[** — Zoom in (see nearby enemies in more detail)
- **]** — Zoom out (see farther enemies)

---

## Blueprint Configuration

All parameters are editable in the **BP_Fighter** Blueprint Details panel. Select the root component to see them.

### Turret Parameters

Found under the **Turret** category in the Details panel.

| Parameter | Description | Default | Min/Max |
|-----------|-------------|---------|---------|
| **Aim Sensitivity** | Mouse sensitivity percentage (0-100, 50 = default) | 50 | 0-100 |
| **Turret Max Pitch** | Maximum angle the turret can look up (degrees) | 80 | 1-89 |
| **Turret Min Pitch** | Maximum angle the turret can look down (degrees) | 15 | 1-89 |
| **Start Altitude** | Starting height of the turret (Z coordinate) | 3000 | Any |
| **Min Turret Height** | Minimum height the turret can go | 100 | 0+ |
| **Max Turret Height** | Maximum height the turret can go | 5000 | 0+ |
| **Height Change Speed** | Speed of vertical movement with Q/E keys (units/sec) | 300 | 0+ |

#### Mouse Wheel Zoom Parameters

| Parameter | Description | Default | Min/Max |
|-----------|-------------|---------|---------|
| **Mouse Wheel Zoom Speed** | How much zoom changes per wheel tick | 0.5 | 0.1+ |
| **Max Zoom Level** | Maximum zoom multiplier (higher = more zoomed in) | 10.0 | 1.0-10.0 |
| **Min Zoom Level** | Minimum zoom multiplier (lower = zoomed out) | 1.0 | 0.1-1.0 |

#### Tuning Tips

- **More responsive aiming:** Increase `Aim Sensitivity` (e.g., 0.1-0.2 for faster rotation)
- **Slower, more precise aiming:** Decrease `Aim Sensitivity` (e.g., 0.02-0.03)
- **Wider vertical range:** Increase `Turret Max Pitch` (up to 89°) and `Turret Min Pitch`
- **Higher turret position:** Increase `Start Altitude` and `Max Turret Height`
- **Faster zoom:** Increase `Mouse Wheel Zoom Speed` (e.g., 1.0 for quicker zoom)
- **More zoom range:** Increase `Max Zoom Level` (up to 10x)

### Rocket Parameters

Found under the **Rocket** category.

| Parameter | Description | Default | Min/Max |
|-----------|-------------|---------|---------|
| **Rocket Class** | Blueprint class for the rocket projectile | None (must assign) | N/A |
| **Rocket Cooldown** | Seconds between shots | 0.15 | 0.01+ |
| **Rocket Spawn Offset** | Where rockets spawn relative to pawn origin (local space) | (300, 0, -50) | Any |
| **Crosshair Max Distance** | Maximum raycast distance for mouse aiming (units) | 50000 | 1000+ |

#### Rocket Projectile Settings (in Rocket Blueprint)

| Parameter | Description | Default | Min/Max |
|-----------|-------------|---------|---------|
| **Initial Speed** | How fast rockets travel (units/sec) | 8000 | 0+ |
| **Max Speed** | Maximum rocket speed | 8000 | 0+ |
| **Projectile Gravity Scale** | Gravity affecting rockets (0 = straight line) | 0.0 | Any |
| **Lifespan** | How long rockets exist before auto-destroying (seconds) | 5.0 | 0+ |
| **Explosion Radius** | Damage radius when rockets hit (units) | 200 | 0+ |
| **Mesh Rotation Offset** | Visual rotation of rocket mesh | (0, 90, 0) | Any |

### Radar Parameters

Found under the **Radar** category.

| Parameter | Description | Default | Min/Max |
|-----------|-------------|---------|---------|
| **Radar World Range** | How far the radar can detect enemies (units) | 15000 | 0+ |
| **Radar Radius** | Visual size of radar on screen (pixels) | 90 | 0+ |
| **Radar Zoom** | Current zoom level (1.0 = default, lower = zoomed in) | 1.0 | 0.2-3.0 |
| **Radar Zoom Step** | How much zoom changes per key press | 0.15 | 0+ |
| **Radar Zoom Min/Max** | Zoom limits | 0.2 / 3.0 | Any |

### Game Settings Parameters

| Parameter | Description | Default | Min/Max |
|-----------|-------------|---------|---------|
| **Sound Volume** | Master volume for all sounds (0.0-1.0) | 0.5 | 0.0-1.0 |
| **Volume Step** | How much volume changes per key press | 0.05 | 0+ |
| **Sensitivity Step** | How much sensitivity changes per key press (percentage points) | 5.0 | 0+ |
| **Min/Max Sensitivity** | Sensitivity percentage limits | 0.0 / 100.0 | Any |
| **FPS Update Interval** | How often FPS display refreshes (seconds) | 5.0 | 0+ |

### Base Defense Parameters

| Parameter | Description | Default | Min/Max |
|-----------|-------------|---------|---------|
| **Base HP** | Starting base health points | 100 | 1+ |
| **Base Max HP** | Maximum base health points | 100 | 1+ |
| **Damage Flash Alpha** | Current red flash intensity (0-1) | 0.0 | 0.0-1.0 |
| **Damage Flash Decay Rate** | How fast flash fades (per second) | 3.0 | 0+ |

---

## Tank Wave Spawner Configuration

Found on the **TankWaveSpawner** actor in the level. Select it in the World Outliner to see the Details panel.

### Tank Spawning

| Parameter | Description | Default | Min/Max |
|-----------|-------------|---------|---------|
| **Tank Class** | Blueprint class to spawn for tanks | None (must assign) | N/A |
| **Base Target** | Actor representing the base center | None (must assign) | N/A |
| **Initial Spawn Radius** | Distance from center where tanks spawn on wave 1 | 2000 | 100+ |
| **Max Spawn Radius** | Maximum spawn radius across all waves | 5000 | 100+ |
| **Spawn Radius Wave Increment** | How much spawn radius increases per wave | 200 | 0+ |
| **Tanks Per Wave** | Number of tanks in the first wave | 5 | 1+ |
| **Tanks Added Per Wave** | Additional tanks per subsequent wave | 2 | 0+ |
| **Wave Delay** | Time between waves (seconds) | 5.0 | 0+ |
| **Min Spawn Separation** | Minimum distance between spawned tanks | 100 | 10+ |
| **Spawn Height Offset** | Height above ground to spawn tanks | 100 | Any |
| **Mesh Rotation Offset** | Visual rotation fix for tank model (degrees) | 90 | Any |
| **Line Of Fire Distance** | Distance from base where tanks stop and fire | 500 | 0+ |
| **Rate Of Fire** | Seconds between shots at the base | 3.0 | 0.1+ |
| **Use Zigzag Movement** | Enable sailboat-style zigzag approach | false | true/false |
| **Straight Line Distance** | Distance from base where tanks stop zigzagging and go straight (0 = never) | 800 | 0+ |
| **Zigzag Min Distance** | Min distance per zigzag leg | 200 | 0+ |
| **Zigzag Max Distance** | Max distance per zigzag leg | 500 | 0+ |

### Tank Wave Speed Scaling

Each wave, the min and max speed increase by a configurable amount, capped at absolute maximums. Formula: `WaveSpeed = InitialSpeed + (Wave - 1) * IncrementPerWave`, clamped to `MaxPossible`.

| Parameter | Description | Default | Min/Max |
|-----------|-------------|---------|---------|
| **Initial Min Speed** | Minimum tank speed on wave 1 | 100 | 0+ |
| **Initial Max Speed** | Maximum tank speed on wave 1 | 300 | 0+ |
| **Max Possible Min Speed** | Absolute cap for minimum speed across all waves | 400 | 0+ |
| **Max Possible Max Speed** | Absolute cap for maximum speed across all waves | 800 | 0+ |
| **Min Speed Increment Per Wave** | How much min speed increases per wave | 15 | 0+ |
| **Max Speed Increment Per Wave** | How much max speed increases per wave | 30 | 0+ |

#### Tank Speed Example (per wave)

| Wave | Min Speed | Max Speed |
|------|-----------|-----------|
| 1 | 100 | 300 |
| 2 | 115 | 330 |
| 5 | 160 | 420 |
| 10 | 235 | 570 |
| 20 | 385 | 800 (capped) |
| 21+ | 400 (capped) | 800 (capped) |

---

## Helicopter Wave Spawner Configuration

Found on the **HeliWaveSpawner** actor in the level.

### Helicopter Spawning

| Parameter | Description | Default | Min/Max |
|-----------|-------------|---------|---------|
| **Heli Class** | Blueprint class to spawn for helicopters | None (must assign) | N/A |
| **Base Target** | Actor representing the base center | None (must assign) | N/A |
| **Initial Spawn Radius** | Distance from center where helis spawn on wave 1 | 2000 | 100+ |
| **Max Spawn Radius** | Maximum spawn radius across all waves | 5000 | 100+ |
| **Spawn Radius Wave Increment** | How much spawn radius increases per wave | 200 | 0+ |
| **Helis Per Wave** | Number of helicopters in the first wave | 3 | 1+ |
| **Helis Added Per Wave** | Additional helis per subsequent wave | 1 | 0+ |
| **Wave Delay** | Time between waves (seconds) | 5.0 | 0+ |
| **Min Spawn Separation** | Minimum distance between spawned helis | 100 | 10+ |
| **Min Spawn Height** | Minimum flying height for spawned helis | 400 | 0+ |
| **Max Spawn Height** | Maximum flying height for spawned helis | 800 | 0+ |
| **Mesh Rotation Offset** | Visual rotation fix for heli model (degrees) | 90 | Any |
| **Line Of Fire Distance** | Distance from base where helis stop and fire | 500 | 0+ |
| **Rate Of Fire** | Seconds between shots at the base | 3.0 | 0.1+ |

### Helicopter Wave Speed Scaling

Same formula as tanks: `WaveSpeed = InitialSpeed + (Wave - 1) * IncrementPerWave`, clamped to `MaxPossible`.

| Parameter | Description | Default | Min/Max |
|-----------|-------------|---------|---------|
| **Initial Min Speed** | Minimum heli speed on wave 1 | 200 | 0+ |
| **Initial Max Speed** | Maximum heli speed on wave 1 | 500 | 0+ |
| **Max Possible Min Speed** | Absolute cap for minimum speed across all waves | 600 | 0+ |
| **Max Possible Max Speed** | Absolute cap for maximum speed across all waves | 1200 | 0+ |
| **Min Speed Increment Per Wave** | How much min speed increases per wave | 20 | 0+ |
| **Max Speed Increment Per Wave** | How much max speed increases per wave | 40 | 0+ |

### Helicopter Lateral Dancing

When helicopters get within a configurable distance of the base, they start "dancing" — moving laterally (side to side) while still approaching. Each helicopter picks a random lateral speed and duration, then switches direction. This makes them much harder to hit at close range.

| Parameter | Description | Default | Min/Max |
|-----------|-------------|---------|---------|
| **Lateral Dance Distance** | Distance from base where dancing begins (units) | 1000 | 0+ |
| **Min Lateral Speed** | Minimum sideways movement speed (units/sec) | 100 | 0+ |
| **Max Lateral Speed** | Maximum sideways movement speed (units/sec) | 400 | 0+ |
| **Min Lateral Time** | Minimum time moving in one direction before switching (sec) | 0.5 | 0.1+ |
| **Max Lateral Time** | Maximum time moving in one direction before switching (sec) | 2.0 | 0.1+ |

#### Lateral Dancing Behavior

1. Helicopter approaches base normally.
2. When within `Lateral Dance Distance`, it starts moving sideways.
3. A random speed between `Min/Max Lateral Speed` and a random duration between `Min/Max Lateral Time` are picked.
4. After the timer expires, the helicopter flips direction and picks new random speed/duration.
5. This continues while the helicopter is still approaching the base.
6. Once at the stopping distance (line of fire), the helicopter stops moving and fires at the base.

---

## Game Mode Setup

To use the turret pawn in a level:

1. Create a **Game Mode** Blueprint based on `GameModeBase`
2. Set **Default Pawn Class** → `BP_Fighter`
3. Set **Player Controller Class** → `FighterPlayerController`
4. Set **HUD Class** → `FighterHUD`
5. In your level's **World Settings**, set **GameMode Override** to your new Game Mode

---

## Input Setup

### Input Actions (all Digital / bool)

Create these in Content Browser → **Input** → **Input Action**:

- `IA_FireRocket` — Left Mouse Button

The following actions are created automatically in C++ and mapped via the turret mapping context:

- **Q** — Height Down
- **E** — Height Up
- **[** — Radar Zoom In
- **]** — Radar Zoom Out
- **F** — Toggle FPS display
- **ESC** — Pause
- **C** — Continue
- **X** — Quit
- **Delete** — Debug (destroy all enemies except one of each type)

Volume and sensitivity controls are mapped via the Blueprint mapping context (`IMC_Fighter`).

---

## C++ Source Files

| File | Description |
|------|-------------|
| `FighterPawn.h/.cpp` | Turret defense pawn with mouse-aim rotation, Q/E height control, and rocket firing |
| `FighterHUD.h/.cpp` | Draws centered white crosshair, altitude, radar, score, and settings info |
| `FighterPlayerController.h/.cpp` | Configures mouse input for turret aiming (hidden OS cursor, game-only mode) |
| `RocketProjectile.h/.cpp` | Rocket projectile with straight-line flight — destroys both tanks and helicopters |
| `BombProjectile.h/.cpp` | Bomb projectile with physics and explosion (legacy, not used in turret mode) |
| `TankAI.h/.cpp` | Tank enemy AI — moves toward base, stops at line of fire, shoots base |
| `HeliAI.h/.cpp` | Helicopter enemy AI — flies toward base, stops at line of fire, shoots base |
| `TankWaveSpawner.h/.cpp` | Spawns waves of tanks with configurable count, speed, and behavior |
| `HeliWaveSpawner.h/.cpp` | Spawns waves of helicopters with configurable count, speed, and height |
| `ExplosionComponent.h/.cpp` | Reusable explosion effect component for enemies |
