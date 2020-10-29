// client_app.cc

#include "client_app.hpp"
#include <charlie_messages.hpp>
#include <cstdio>
#include <charlie_network.hpp>
#include <charlie_protocol.hpp>

template <typename T, std::size_t N>
constexpr auto array_size(T(&)[N])
{
   return N;
}

ClientApp::ClientApp()
   : mouse_(window_.mouse_)
   , keyboard_(window_.keyboard_)
   , tickrate_(1.0 / 60.0)
   , input_bits_(0)
{
    globalTick = 0;
    recievedServerTick = 0;
}

bool ClientApp::on_init()
{
   network_.set_send_rate(Time(1.0 / 10.0));
   if (!network_.initialize({})) {
      return false;
   }

   connection_.set_listener(this);
   connection_.connect(network::IPAddress(127, 0, 0, 1, 54345));

   return true;
}

void ClientApp::on_exit()
{
}

bool ClientApp::on_tick (const Time& dt) {
    if (keyboard_.pressed (Keyboard::Key::Escape)) {
        return false;
    }

    accumulator_ += dt;
    while (accumulator_ >= tickrate_) {
        globalTick = recievedServerTick;
        accumulator_ -= tickrate_;

        if (positionHistory.size () > 0) {
            if (positionHistory[0].tick - tickrate_.as_milliseconds () > 200) {
                positionHistory.erase (positionHistory.begin ());
            }

            float distance = Vector2::distance (positionHistory.back ().position, entity_.position_);

            entity_.position_ = Vector2::lerp (entity_.position_, positionHistory.back ().position, tickrate_.as_seconds () * 10);
        }
        input_bits_ = 0;
        if (keyboard_.down (Keyboard::Key::W)) {
            input_bits_ |= (1 << int32 (gameplay::Action::Up));
        }
        if (keyboard_.down (Keyboard::Key::S)) {
            input_bits_ |= (1 << int32 (gameplay::Action::Down));
        }
        if (keyboard_.down (Keyboard::Key::A)) {
            input_bits_ |= (1 << int32 (gameplay::Action::Left));
        }
        if (keyboard_.down (Keyboard::Key::D)) {
            input_bits_ |= (1 << int32 (gameplay::Action::Right));
        }
        if (keyboard_.down (Keyboard::Key::Space)) {
            input_bits_ |= (1 << int32 (gameplay::Action::Shoot));
        }

        const bool player_move_up = input_bits_ & (1 << int32 (gameplay::Action::Up));
        const bool player_move_down = input_bits_ & (1 << int32 (gameplay::Action::Down));
        const bool player_move_left = input_bits_ & (1 << int32 (gameplay::Action::Left));
        const bool player_move_right = input_bits_ & (1 << int32 (gameplay::Action::Right));

        Vector2 direction;
        if (player_move_up) {
            direction.y_ -= 1.0f;
        }
        if (player_move_down) {
            direction.y_ += 1.0f;
        }
        if (player_move_left) {
            direction.x_ -= 1.0f;
        }
        if (player_move_right) {
            direction.x_ += 1.0f;
        }
        const float speed = 100.0;
        if (direction.length () > 0.0f) {
            direction.normalize ();
            player_.position_ = Vector2::lerp (player_.position_, player_.position_ + direction * speed, tickrate_.as_seconds ());
        }
        InputPrediction temp;
        temp.inputBits = input_bits_;
        temp.tick = globalTick;
        temp.calculatedPosition = player_.position_;
        inputLibrary.push_back (temp);
    }
    return true;
}

void ClientApp::on_draw()
{
   renderer_.clear({ 0.2f, 0.3f, 0.4f, 1.0f });
   renderer_.render_text({ 2, 2 }, Color::White, 1, "CLIENT");
   renderer_.render_rectangle_fill({ int32(entity_.position_.x_), int32(entity_.position_.y_), 20, 20 }, Color::Green);
   renderer_.render_rectangle_fill({ int32(player_.position_.x_), int32(player_.position_.y_), 20, 20 }, Color::Magenta);
}

void ClientApp::on_acknowledge(network::Connection *connection, 
                               const uint16 sequence)
{
}

void ClientApp::on_receive (network::Connection* connection, network::NetworkStreamReader& reader) {
    while (reader.position () < reader.length ()) {
        switch (reader.peek ()) {
        case network::NETWORK_MESSAGE_SERVER_TICK:
        {
            network::NetworkMessageServerTick message;
            if (!message.read (reader)) {
                assert (!"could not read message!");
            }

            const Time current = Time (message.server_time_);
            recievedServerTick = message.server_tick_;
        } break;

        case network::NETWORK_MESSAGE_ENTITY_STATE:
        {
            network::NetworkMessageEntityState message;
            if (!message.read (reader)) {
                assert (!"could not read message!");
            }
            PositionHistory temp;
            temp.position = message.position_;
            temp.tick = recievedServerTick;
            positionHistory.push_back (temp);
        } break;

        case network::NETWORK_MESSAGE_PLAYER_STATE:
        {
            network::NetworkMessagePlayerState message;
            if (!message.read (reader)) {
                assert (!"could not read message!");
            }

            player_.position_ = message.position_;
        } break;

        default:
        {
            assert (!"unknown message type received from server!");
        } break;
        }
    }
}

void ClientApp::on_send (network::Connection* connection, const uint16 sequence, network::NetworkStreamWriter& writer) {
    network::NetworkMessageInputCommand command (input_bits_,player_.playerID);
    if (!command.write (writer)) {
        assert (!"could not write network command!");
    }
}

network::IPAddress ClientApp::ServerDiscovery () {
   network::UDPSocket socket;
    if (socket.open ()) {
        DynamicArray<network::IPAddress> addresses;
        network::IPAddress address;
        if (network::IPAddress::local_addresses (addresses)) {
            for (auto& ad : addresses)
                address = ad;
        }
        address.host_ = (0xffffff00 & address.host_) | 0xff;
        address.port_ = 54345;

        network::NetworkStream stream;
        network::NetworkStreamWriter writer (stream);
        network::ProtocolRequestPacket packet;

        if (packet.write (writer)) {
            if (socket.send (address, stream.buffer_, stream.length_))
                printf ("Ping sent\n");
        }

        bool found = false;
        int tries = 0;
        
        while (!found) {
            stream.reset ();
            if (socket.receive (address, stream.buffer_, stream.length_)) {
                network::NetworkStreamReader reader (stream);
                if (reader.peek () == network:: ProtocolPacketType::PROTOCOL_PACKET_CHALLENGE) {
                    printf ("Server found");
                    found = true;
                    return address;
                }
            }
            else {
                tries++;
                auto error_code = network::Error::get_last ();
                if (network::Error::is_critical (error_code)) {
                    assert (false);
                    break;
                }
                if (packet.write (writer)) {
                    if (socket.send (address, stream.buffer_, stream.length_)) {
                        error_code = network::Error::get_last ();
                        if (network::Error::is_critical (error_code)) {
                            assert (false);
                            break;
                        }
                    }
                }
            }
            if (tries > 10) break;
            Time::sleep (Time (1.0 / 50.0));
        }
        printf ("Out of tries\n");

        auto error_code = network::Error::get_last ();
        printf ("ERR: %d/n", error_code);
        if (network::Error::is_critical (error_code)) {
            assert (false);
        }
        return network::IPAddress (0, 0, 0, 0, 0);
    }
    else
        return network::IPAddress ();
}

