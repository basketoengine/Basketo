# Sound Effects System Documentation

## Overview

The Sound Effects System allows entities to have multiple named sound effects that can be triggered programmatically from Lua scripts or the C++ engine. This is perfect for adding dynamic audio feedback for actions like walking, jumping, attacking, collecting items, etc.

## Components

### SoundEffectsComponent

The `SoundEffectsComponent` manages multiple sound effects per entity:

```cpp
struct SoundEffectsComponent {
    std::unordered_map<std::string, std::string> soundEffects; // action name -> audio ID
    int defaultVolume = 128;
    std::vector<std::string> playQueue; // Queue of sounds to play this frame
    
    // Methods
    void addSoundEffect(const std::string& actionName, const std::string& audioId);
    void removeSoundEffect(const std::string& actionName);
    void playSound(const std::string& actionName);
    std::string getAudioId(const std::string& actionName) const;
    void clearPlayQueue();
};
```

### AudioComponent (Traditional)

The original `AudioComponent` is still available for background music and simple audio needs:

```cpp
struct AudioComponent {
    std::string audioId;
    bool isMusic = false; 
    bool playOnStart = false;
    bool loop = false;
    int volume = 128;
    bool isPlaying = false;
};
```

## Usage in Editor

### Adding Sound Effects Component

1. Select an entity in the scene
2. In the Inspector panel, choose "SoundEffects" from the component dropdown
3. Click "Add Selected Component"

### Managing Sound Effects

1. Set the **Default Volume** (0-128)
2. Add sound effects by:
   - Entering an **Action Name** (e.g., "jump", "walk", "attack")
   - Entering an **Audio ID** (the filename without extension)
   - Clicking **"Add Sound Effect"**
   - Or use **"Browse Audio..."** to select and automatically load audio files

3. Test sounds by clicking the **"Test"** button next to each sound effect
4. Remove individual sound effects with the **"Remove"** button

## Lua Script API

### Core Functions

```lua
-- Play a sound effect by action name
PlaySound(entity, "jump")
PlaySound(entity, "walk")
PlaySound(entity, "attack")

-- Add a new sound effect programmatically
AddSoundEffect(entity, "powerup", "powerup-sound")

-- Remove a sound effect
RemoveSoundEffect(entity, "walk")

-- Check if a sound effect exists
if HasSoundEffect(entity, "jump") then
    PlaySound(entity, "jump")
end

-- Check if entity has the component
if HasEntityComponent(entity, "SoundEffectsComponent") then
    -- Entity supports sound effects
end
```

### Example Player Script

```lua
local entity = ...
local wasGrounded = true

function init(entityId)
    entity = entityId
    
    -- Check if sound effects are available
    if HasEntityComponent(entity, "SoundEffectsComponent") then
        Log("Sound effects enabled for player!")
    end
end

function update(entityId, deltaTime)
    entity = entityId
    
    -- Get player state
    local velocity = GetEntityVelocity(entity)
    local contacts = GetCollisionContacts(entity)
    local grounded = false
    
    -- Check if grounded
    if contacts then
        for i, contact in pairs(contacts) do
            if contact.normalY and contact.normalY < -0.5 then
                grounded = true
                break
            end
        end
    end
    
    -- Landing sound
    if grounded and not wasGrounded then
        PlaySound(entity, "land")
    end
    
    -- Movement sounds
    if Input.isKeyDown("A") or Input.isKeyDown("D") then
        if grounded then
            PlaySound(entity, "walk")
        end
    end
    
    -- Jump sound
    if Input.isKeyDown("Space") and grounded then
        PlaySound(entity, "jump")
    end
    
    wasGrounded = grounded
end
```

## Common Sound Effect Names

Here are some suggested action names for consistency:

- **Movement**: "walk", "run", "sprint", "slide"
- **Combat**: "attack", "hit", "block", "parry"
- **Jumping**: "jump", "land", "double_jump"
- **Items**: "pickup", "drop", "use", "equip"
- **Damage**: "hurt", "death", "heal"
- **UI**: "select", "confirm", "cancel", "error"
- **Environment**: "door_open", "door_close", "switch"

## Audio File Requirements

- **Supported formats**: MP3, WAV, OGG, FLAC
- **Location**: Place audio files in `assets/Audio/` directory
- **Naming**: Use descriptive filenames (e.g., `jump.wav`, `footstep.mp3`)
- **Volume**: Sounds are played at the component's default volume
- **Looping**: Sound effects don't loop by default (use AudioComponent for looping)

## Technical Details

### Audio System Processing

The AudioSystem processes both components each frame:

1. **AudioComponent**: Handles background music and simple audio
2. **SoundEffectsComponent**: Processes the play queue and triggers sound effects

### Performance Considerations

- Sound effects are queued and played once per frame
- Multiple calls to `PlaySound()` with the same action in one frame will only play once
- The play queue is cleared after each frame
- No limit on the number of sound effects per entity

### Integration with Existing Audio

The new system works alongside the existing AudioComponent:
- Use **AudioComponent** for background music, ambient sounds, and simple audio
- Use **SoundEffectsComponent** for dynamic, action-based sound effects
- Both can exist on the same entity if needed

## Troubleshooting

### Sound Not Playing
1. Check that the audio file exists in `assets/Audio/`
2. Verify the audio ID matches the filename (without extension)
3. Ensure the entity has a SoundEffectsComponent
4. Check that the action name is spelled correctly

### Audio File Not Loading
1. Verify the file format is supported (MP3, WAV, OGG, FLAC)
2. Check the console for AssetManager error messages
3. Try using the "Browse Audio..." button to automatically copy and load files

### Script Errors
1. Check that `HasEntityComponent(entity, "SoundEffectsComponent")` returns true
2. Verify the entity ID is correct
3. Look for Lua error messages in the console
