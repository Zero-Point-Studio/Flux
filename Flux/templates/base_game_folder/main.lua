-- Starter Game Logic

local Message = "Hello World!"

local function OnStart()
    print(Message)

    local Cube = Scene.AddCube("StarterCube")
    Cube.position = {x = 0, y = 5, z = 0}
end

local function onUpdate(deltaTime)
    -- This runs every frame, you can use this for checks in every frame :)
end