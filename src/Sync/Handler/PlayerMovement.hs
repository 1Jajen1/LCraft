module Sync.Handler.PlayerMovement (
  movePlayer
, rotatePlayer
, updateMovingPlayer
) where

import Player
import Util.Position
import Game.State
import Sync.Monad
import Game.Event
import Util.Rotation
import Optics

movePlayer :: Player -> Position -> GameState -> Sync [Event]
movePlayer p pos _ = pure [PlayerMoved p (Just pos)]

 -- Rotate and onGround updates only matter for the associated entitiy right?
rotatePlayer :: Player -> Rotation -> GameState -> Sync [Event]
rotatePlayer p rot _ = pure []

updateMovingPlayer :: Player -> OnGround -> GameState -> Sync [Event]
updateMovingPlayer p newOnGround _
  | p ^. onGround /= newOnGround =
    -- Check if we need to apply fall-damage here
    pure [] -- TODO
  | otherwise = pure []