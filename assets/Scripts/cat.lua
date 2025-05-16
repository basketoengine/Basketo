-- cat.lua
-- This script will play the 'walk' animation on the cat entity forever

function init(entity)
    PlayAnimation(entity, "walk")
end

function update(entity, deltaTime)
    -- Ensure the animation is always playing
    if not IsAnimationPlaying(entity, "walk") then
        PlayAnimation(entity, "walk")
    end
end
