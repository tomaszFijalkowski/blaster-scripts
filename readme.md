## About the project

**Blaster** is a multiplayer third-person shooter featuring familiar competitive game modes, a wide range of unique and satisfying weapons, powerful pickups scattered across the detailed map, and fast-paced action.

This repository provides a snapshot of all scripts and commit messages. It's intended to showcase the game's core systems and code structure - no assets or engine content are included. To learn more, visit [fijalkowski.it/blaster](https://fijalkowski.it/blaster).

## Commit messages

- Prevent respawning during cooldown
- Fix dangling pointer after dropping a flag
- Improve sound attenuation
- Add fallback spawn to prevent spawn camping
- Fix crashes when firing as clients during elimination
- Extract setting character widgets to separate polling methods
- Fix team score sometimes not being visible on clients
- Fix team assignment for rejoining players
- Refactor player state and remove persistence between game sessions
- Fix reload progress bar sometimes not disappearing after elimination
- Fix team colors sometimes not being set (again)
- Enhance network performance for character movement
- Persist player state between game sessions
- Improve multiplayer sessions plugin stability
- Lock manual reloading while waiting for empty reload
- Fix team colors sometimes not being set
- Add balance changes
- Add various improvements to animations
- Add escape menu to lobby
- Add separate material slots to wall windows
- Add script for creating steam app id file in packaged build
- Fix initial spawns being wrong in team game modes (again)
- Add small delay before reloading empty weapon
- Fix spawning more than one weapon
- Rebalance hitscan and bullet weapons
- Rebalance explosive projectiles
- Add player impact sound to explosions
- Fix match countdown animation sound
- Adjust announcement countdown text formatting
- Fix initial spawns being wrong in team game modes
- Fix hit reacts interrupting other animations
- Adjust warmup times for all game modes
- Fix shipping configuration errors
- Remake startup and lobby maps
- Add maps for all game modes based on base map
- Add modulators to ammo pickup sounds
- Add ambiance sound and interior audio volumes with reverb
- Add wind systems to base map
- Improve and refactor player spawning logic
- Add skybox scenery to base map
- Mirror blue side of the base map to create red side
- Add physical material mask to wall windows
- Fix reload progress bar sometimes not showing on server
- Fix and refactor recalculating movement speed
- Add console variables to reduce lumen flickering
- Add foliage to base map
- Add details to base map
- Fix shotgun impact effects
- Update ammo values for all weapons
- Add preview meshes to spawn points
- Add base map
- Limit walk speed with flag and improve animation
- Make projectile explosions more reliable on clients
- Fix tracer particles not disappearing at close range
- Improve post-processing and footsteps mastering
- Add rank badges in scoreboard for deathmatch mode
- Improve headshot sound
- Add weapon drop sounds
- Prevent collision with initiated projectiles
- Add dynamic scoreboard for all game modes
- Add assist tracking and update elimination notifications
- Fix inflicting damage with self-hits
- Add post-processing setting functionality
- Add custom game instance class for setting master volume
- Set sound class for all sound assets
- Add settings widget with adjustable mouse sensitivity
- Adjust sensitivity scale when aiming
- Add flag reset timer and widget
- Improve bodyshot and headshot sounds with 2D mixing
- Improve shotgun impact sounds
- Show HUD overlay when using sniper scope
- Fix bullet tracers cutting off
- Add damage falloff to weapons
- Add aim accuracy factor to weapon scatter
- Add accumulating damage numbers
- Add continuous overlap detection to weapons and pickup selection logic
- Add continuous overlap detection to pickups
- Add dynamic pickup widget positioning
- Fix grenade projectile ghost explosions
- Add throwing grenade pickup
- Fix client-side prediction for weapon handling
- Fix flickering of finished HUD buff duration bars
- Apply weapon-specific colors to HUD ammo
- Add ping amount to HUD
- Fix flag not resetting on clients
- Fix team score HUD issues on clients
- Add more custom depth stencil colors
- Add additional checks for handling the flag
- Add flag capture VFX and notifications
- Improve VFX for pickups
- Add overhead widget visibility logic
- Destroy dropped weapons after some time
- Add weapon spawn points
- Prevent shooting through walls
- Fix weapon switching replication issues
- Add dynamic buff duration bars
- Fix various HUD issues
- Add number of players selection
- Fix weapons not attaching properly on clients
- Remove plugin files from repository
- Add plugin files to .gitignore
- Fix finding sessions
- Add out of bounds zones
- Randomize hit reactions
- Disable input while in escape menu
- Add waiting for players announcement
- Add reload progress bar
- Add section 15 - game mode selection
- Add section 15 - capture the flag
- Add section 15 - blue & red flags
- Add section 14 - team winning
- Add section 14 - team score
- Add section 14 - friendly fire prevention
- Add section 14 - blue & red teams
- Add section 13 - headshots
- Add section 13 - elimination notifications
- Add section 13 - mvp
- Add section 13 - leaving & quitting the game
- Add section 13 - escape menu
- Add section 12 - fire delay server validation
- Add section 12 - advanced weapon switching
- Add section 12 - limit server-side rewind to 100 ms
- Add section 12 - projectile hit confirmation using server-side rewind
- Add section 12 - local projectile firing
- Add section 12 - post edit change property for projectile initial speed
- Add section 12 - score request for shotgun weapons
- Add section 12 - shotgun hit confirmation using server-side rewind
- Add section 12 - score request for hitscan weapons
- Add section 12 - hit confirmation using server-side rewind
- Add section 12 - frame history for hitboxes
- Add section 12 - client-side prediction for ammo & aiming
- Add section 12 - local firing & scatter replication
- Add section 11 - damage pickup
- Add section 11 - pickup spawning & secondary weapon
- Add section 11 - jump & shield pickups
- Add section 11 - health & speed pickups
- Add section 11 - ammo pickups
- Add section 10 - throwing grenades
- Add section 10 - sniper rifle & grenade launcher
- Add section 10 - shotgun
- Add section 10 - smg
- Add section 10 - pistol
- Add section 10 - rocket launcher
- Fix issues related to crosshair
- Add section 9
- Add section 8
- Add section 7
- Add section 6
- Add section 5
- Add section 4
- Add sections 1-3
