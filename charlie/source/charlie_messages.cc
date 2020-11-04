// charlie_messages.cc

#include "charlie_messages.hpp"
#include "charlie_network.hpp"
#include "charlie_gameplay.hpp"

namespace charlie {
   namespace network {

      //NetworkMessageServerID::NetworkMessageServerID (const int32 &id)
      //    :playerID(id)
      //{
      //}

      NetworkMessageServerTick::NetworkMessageServerTick()
         : type_(NETWORK_MESSAGE_SERVER_TICK)
         , server_time_(0)
         , server_tick_(0)
      {
      }

      NetworkMessageServerTick::NetworkMessageServerTick (const int64  server_time,  const uint32 server_tick, const int32 ID)
          : type_ (NETWORK_MESSAGE_SERVER_TICK)
          , server_time_ (server_time)
          , server_tick_ (server_tick),
          playerID (ID)
      {
      }

      bool NetworkMessageServerTick::read(NetworkStreamReader &reader)
      {
         return serialize(reader);
      }

      bool NetworkMessageServerTick::write(NetworkStreamWriter &writer)
      {
         return serialize(writer);
      }

      NetworkMessageEntityState::NetworkMessageEntityState()
         : type_(NETWORK_MESSAGE_ENTITY_STATE)
      {
      }

      NetworkMessageEntityState::NetworkMessageEntityState(const Vector2 &position,const int32 &id)
         : type_(NETWORK_MESSAGE_ENTITY_STATE)
         , position_(position),
          entityID(id)
      {
      }

      bool NetworkMessageEntityState::read(NetworkStreamReader &reader)
      {
         return serialize(reader);
      }

      bool NetworkMessageEntityState::write(NetworkStreamWriter &writer)
      {
         return serialize(writer);
      }

      NetworkMessageInputCommand::NetworkMessageInputCommand()
         : type_(NETWORK_MESSAGE_INPUT_COMMAND)
         , bits_(0)
      {
      }

      NetworkMessageInputCommand::NetworkMessageInputCommand(uint8 bits, int32 playerID)
         : type_(NETWORK_MESSAGE_INPUT_COMMAND)
         , bits_(bits),
          id(playerID)
      {
      }

      bool NetworkMessageInputCommand::read(NetworkStreamReader &reader)
      {
         return serialize(reader);
      }

      bool NetworkMessageInputCommand::write(NetworkStreamWriter &writer)
      {
         return serialize(writer);
      }

      NetworkMessagePlayerState::NetworkMessagePlayerState (const Vector2& position, const int32& id)
          : type_ (NETWORK_MESSAGE_PLAYER_STATE)
          , position_ (position)
          , playerID (id) {
      }

     
      NetworkMessagePlayerState::NetworkMessagePlayerState()
         : type_(NETWORK_MESSAGE_PLAYER_STATE)
      {
      }

      bool NetworkMessagePlayerState::read(NetworkStreamReader &reader)
      {
         return serialize(reader);
      }

      bool NetworkMessagePlayerState::write(NetworkStreamWriter &writer)
      {
         return serialize(writer);
      }
   } // !network
} // !messages
