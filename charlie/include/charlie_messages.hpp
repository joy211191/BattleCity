// charlie_messages.hpp

#ifndef CHARLIE_MESSAGES_HPP_INCLUDED
#define CHARLIE_MESSAGES_HPP_INCLUDED

#include <charlie.hpp>
#include <charlie_gameplay.hpp>

namespace charlie {
   namespace network {
      struct NetworkStreamReader;
      struct NetworkStreamWriter;

      enum NetworkMessageType {
         NETWORK_MESSAGE_SERVER_TICK,
         NETWORK_MESSAGE_ENTITY_STATE,
         NETWORK_MESSAGE_INPUT_COMMAND,
         NETWORK_MESSAGE_PLAYER_STATE,
         NETWORK_MESSAGE_COUNT,
         NETWORK_MESSAGE_SHOOT,
         NETWORK_MESSAGE_WINNER

      };

      static_assert(NETWORK_MESSAGE_COUNT <= 255, "network message type cannot exceed 255!");

      struct NetworkMessageShoot {
          NetworkMessageShoot ();
          explicit NetworkMessageShoot (const uint8  active, const uint32 server_tick, const int32 id, const Vector2 position,const Vector2 shootDirection);

          bool read (NetworkStreamReader& reader);
          bool write (NetworkStreamWriter& writer);

          template <typename Stream>
          bool serialize (Stream& stream) {
              bool result = true;
              result &= stream.serialize (type_);
              result &= stream.serialize (bulletActive);
              result &= stream.serialize (server_tick_);
              result &= stream.serialize (playerID);
              result &= stream.serialize (bulletPosition.x_);
              result &= stream.serialize (bulletPosition.y_);
              result &= stream.serialize (direction.x_);
              result &= stream.serialize (direction.y_);
              return result;
          }

          uint8 type_;
          uint8 bulletActive;
          uint32 server_tick_;
          int32 playerID;
          Vector2 bulletPosition;
          Vector2 direction;
      };

      struct NetworkMessageServerTick {
         NetworkMessageServerTick();
         explicit NetworkMessageServerTick(const int64  server_time, const uint32 server_tick, const int32 id);

         bool read(NetworkStreamReader &reader);
         bool write(NetworkStreamWriter &writer);

         template <typename Stream>
         bool serialize(Stream &stream)
         {
            bool result = true;
            result &= stream.serialize(type_);
            result &= stream.serialize(server_time_);
            result &= stream.serialize(server_tick_);
            result &= stream.serialize (playerID);
            return result;
         }

         uint8 type_;
         int64 server_time_;
         uint32 server_tick_;
         int32 playerID;
      };

      struct NetworkMessageEntityState {
         NetworkMessageEntityState();
         explicit NetworkMessageEntityState(const Vector2 &position,const int32 &entID, const uint8& alive);

         bool read(NetworkStreamReader &reader);
         bool write(NetworkStreamWriter &writer);

         template <typename Stream>
         bool serialize(Stream &stream)
         {
            bool result = true;
            result &= stream.serialize(type_);
            result &= stream.serialize(position_.x_);
            result &= stream.serialize(position_.y_);
            result &= stream.serialize (entityID);
            result &= stream.serialize (entityAlive);
            return result;
         }

         uint8 entityAlive;
         uint8 type_;
         Vector2 position_;
         int32 entityID;
      };

      struct NetworkMessageInputCommand {
         NetworkMessageInputCommand();
         explicit NetworkMessageInputCommand(uint8 bits,int32 playerID,int32 sequence);

         bool read(NetworkStreamReader &reader);
         bool write(NetworkStreamWriter &writer);

         template <typename Stream>
         bool serialize(Stream &stream)
         {
            bool result = true;
            result &= stream.serialize(type_);
            result &= stream.serialize(bits_);
            result &= stream.serialize (id);
            result &= stream.serialize (sequenceNumber);
            return result;
         }

         uint8 type_;
         uint8 bits_;
         int32 id;
         int32 sequenceNumber;
      };

      struct NetworkMessagePlayerState {
         NetworkMessagePlayerState();
         explicit NetworkMessagePlayerState(const Vector2 &position,const int32 &id,const uint8 &alive);

         bool read(NetworkStreamReader &reader);
         bool write(NetworkStreamWriter &writer);

         template <typename Stream>
         bool serialize(Stream &stream)
         {
            bool result = true;
            result &= stream.serialize(type_);
            result &= stream.serialize(position_.x_);
            result &= stream.serialize(position_.y_);
            result &= stream.serialize (playerID);
            result &= stream.serialize (playerAlive);
            return result;
         }

         uint8 playerAlive;
         uint8 type_;
         Vector2 position_;
         int32 playerID;
      };

      struct NetoworkMessageWinner {
          NetoworkMessageWinner ();
          explicit NetoworkMessageWinner (const int32 id);

          bool read (NetworkStreamReader& reader);
          bool write (NetworkStreamWriter& writer);

          template <typename Stream>
          bool serialize (Stream& stream) {
              bool result = true;
              result &= stream.serialize (type_);
              result &= stream.serialize (winnerID);
              return result;
          }
          uint8 type_;
          int32 winnerID;
      };
   } // !network
} // !charlie

#endif // !CHARLIE_MESSAGES_HPP_INCLUDED
