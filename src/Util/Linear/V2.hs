{-# LANGUAGE TypeFamilies #-}
{-# LANGUAGE DeriveAnyClass #-}
module Util.Linear.V2 (
  V2(..)
) where

import Control.DeepSeq
import Util.Linear.Vector
import Data.Hashable
import GHC.Generics

-- Use a data family so that GHC unpacks the fields
data family V2 a
data instance V2 Int = V2_Int !Int !Int
  deriving stock (Eq, Show, Generic)
  deriving anyclass Hashable

data instance V2 Float = V2_Float !Float !Float
  deriving stock (Eq, Show)

instance NFData (V2 Int) where
  rnf (V2_Int _ _) = ()

instance VectorSpace Int (V2 Int) where
  (V2_Int x1 y1) |+| (V2_Int x2 y2) = V2_Int (x1 + x2) (y1 + y2)
  l |*| (V2_Int x y) = V2_Int (l * x) (l * y)

instance VectorSpace Float (V2 Float) where
  (V2_Float x1 y1) |+| (V2_Float x2 y2) = V2_Float (x1 + x2) (y1 + y2)
  l |*| (V2_Float x y) = V2_Float (l * x) (l * y)