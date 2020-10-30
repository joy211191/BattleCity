// charlie_messages.hpp

#ifndef CHARLIE_MESSAGES_HPP_INCLUDED
#define CHARLIE_MESSAGES_HPP_INCLUDED

#include <charlie.hpp>

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
      };

      static_assert(NETWORK_MESSAGE_COUNT <= 255, "network message type cannot exceed 255!");

      struct NetworkMessageServerTick {
         NetworkMessageServerTick();
         explicit NetworkMessageServerTick(const int64  server_time,
                                           const uint32 server_tick);

         bool read(NetworkStreamReader &reader);
         bool write(NetworkStreamWriter &writer);

         template <typename Stream>
         bool serialize(Stream &stream)
         {
            bool result = true;
            result &= stream.serialize(type_);
            result &= stream.serialize(server_time_);
            result &= stream.serialize(server_tick_);
            return result;
         }

         uint8 type_;
         int64 server_time_;
         uint32 server_tick_;
      };

      struct NetworkMessageEntityState {
         NetworkMessageEntityState();
         explicit NetworkMessageEntityState(const Vector2 &position);

         bool read(NetworkStreamReader &reader);
         bool write(NetworkStreamWriter &writer);

         template <typename Stream>
         bool serialize(Stream &stream)
         {
            bool result = true;
            result &= stream.serialize(type_);
            result &= stream.serialize(position_.x_);
            result &= stream.serialize(position_.y_);
            return result;
         }

         uint8 type_;
         Vector2 position_;
      };

      struct NetworkMessageInputCommand {
         NetworkMessageInputCommand();
         explicit NetworkMessageInputCommand(uint8 bits,int32 playerID);

         bool read(NetworkStreamReader &reader);
         bool write(NetworkStreamWriter &writer);

         template <typename Stream>
         bool serialize(Stream &stream)
         {
            bool result = true;
            result &= stream.serialize(type_);
            result &= stream.serialize(bits_);
            return result;
         }

         uint8 type_;
         uint8 bits_;
         int32 id;
      };

      struct NetworkMessagePlayerState {
         NetworkMessagePlayerState();
         explicit NetworkMessagePlayerState(const Vector2 &position,const int32 &id);

         bool read(NetworkStreamReader &reader);
         bool write(NetworkStreamWriter &writer);

         template <typename Stream>
         bool serialize(Stream &stream)
         {
            bool result = true;
            result &= stream.serialize(type_);
            result &= stream.serialize(position_.x_);
            result &= stream.serialize(position_.y_);
            return result;
         }

         uint8 type_;
         Vector2 position_;
         int32 playerID;
      };
   } // !network
} // !charlie

#endif // !CHARLIE_MESSAGES_HPP_INCLUDED
