module VarIntSpec (
  spec
) where

import Test.Syd
import Test.Syd.Hedgehog ()

import Hedgehog
import Hedgehog.Gen qualified as Gen
import Hedgehog.Range qualified as Range

import Brokkr.VarInt

import Mason.Builder qualified as Mason
import FlatParse.Basic qualified as Flatparse

import Data.ByteString qualified as BS

spec :: Spec
spec = do
  describe "VarInt (32 bit)" $ do
    it "should parse a few examples correctly" $ do
      let encode x = Mason.toStrictByteString $ putVarInt x
          decode bs = case Flatparse.runParser (withVarInt id pure) bs of
                        Flatparse.OK res remBS | BS.null remBS -> Just res
                        _ -> Nothing
          zeroBs = BS.pack [0]
          oneBs  = BS.pack [1]
          twoFiftyBs = BS.pack [0x80, 0x01]
    
      decode zeroBs `shouldBe` Just (VarInt 0)
      encode (VarInt 0) `shouldBe` zeroBs

      decode oneBs `shouldBe` Just (VarInt 1)
      encode (VarInt 1) `shouldBe` oneBs

      decode twoFiftyBs `shouldBe` Just (VarInt 128)
      encode (VarInt 128) `shouldBe` twoFiftyBs

    it "should roundtrip" . property $ do
      let encode x = Mason.toStrictByteString $ putVarInt x
          decode bs = case Flatparse.runParser (withVarInt id pure) bs of
                        Flatparse.OK res remBS | BS.null remBS -> Just res
                        _ -> Nothing
      x <- forAll $ VarInt . fromIntegral <$> Gen.int Range.constantBounded

      tripping x encode decode
