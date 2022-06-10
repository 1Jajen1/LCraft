module Entity.Internal (
  EntityId(..)
) where
import Data.Int
import Util.Binary
import Network.Util.FromIntegral

newtype EntityId = EntityId Int
  deriving stock Show
  deriving newtype Eq
  deriving (FromBinary, ToBinary) via FromIntegral Int Int32
