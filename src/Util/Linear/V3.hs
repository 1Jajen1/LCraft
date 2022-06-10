{-# LANGUAGE TypeFamilies #-}
module Util.Linear.V3 (
  V3(..)
) where

import Control.DeepSeq
import Util.Linear.Vector

-- Use a data family so that GHC unpacks the fields
data family V3 a
data instance V3 Int = V3_Int !Int !Int !Int
  deriving stock (Eq, Show)
data instance V3 Double = V3_Double !Double !Double !Double
  deriving stock (Eq, Show)

instance NFData (V3 Int) where
  rnf (V3_Int _ _ _) = ()

instance VectorSpace Int (V3 Int) where
  (V3_Int x1 y1 z1) |+| (V3_Int x2 y2 z2) = V3_Int (x1 + x2) (y1 + y2) (z1 + z2)
  l |*| (V3_Int x y z) = V3_Int (l * x) (l * y) (l * z)

instance VectorSpace Double (V3 Double) where
  (V3_Double x1 y1 z1) |+| (V3_Double x2 y2 z2) = V3_Double (x1 + x2) (y1 + y2) (z1 + z2)
  l |*| (V3_Double x y z) = V3_Double (l * x) (l * y) (l * z)