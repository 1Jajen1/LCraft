{-# LANGUAGE DataKinds #-}
module Network (
  setupTCPServer
) where
import Effectful
import Network.Effect.Network
import Network.Effect.Packet
import Effectful.State.Static.Local (runState)
import qualified Network.Packet.Server.Handshake as S
import Util.Binary (get)
import qualified Network.Simple.TCP as Network
import Data.ByteString
import Network.Effect.Network.IO (runNetwork)
import Network.Handler.Login

import Debug.Trace
import Network.Handler.Play

setupTCPServer :: IO ()
setupTCPServer = do
  _ <- Network.serve (Network.Host "192.168.178.59") "25565" $ \(sock, _sockddr) -> do
    runEff
      . runNetwork sock
      $ runProtocol
    pure ()
  pure ()

runProtocol :: Network :> es => Eff es ()
runProtocol = do
  -- The first part of the protocol uses readPacket to read individual packets, this is necessary because the protocol
  --  may change and compression and encryption can be added. This is in contrast to the play protocol which parses the
  --  entire chunk before handling the packets
  ((uid, uname), _remBs) <- runState (mempty @ByteString) $ do
    S.Handshake _ _ _ next <- traceShowId <$> readPacket (get @S.HandshakePacket)
    case next of
      S.Status -> error "Status"
      S.Login -> loginProtocol
  -- TODO Think about _remBs. It should be empty since the servers last action is sending the LoginSuccess packet and all other packets should have been reacted to...
  finalizeLogin uid uname
  playProtocol
