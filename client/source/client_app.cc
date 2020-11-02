// client_app.cc

#include "client_app.hpp"
#include <charlie_messages.hpp>
#include <cstdio>
#include <charlie_network.hpp>
#include <charlie_protocol.hpp>
using namespace std;

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
    gameState = gameplay::GameState::Searching;
    playerColors[0] = Color::Red;
    playerColors[1] = Color::Green;
    playerColors[2] = Color::Blue;
    playerColors[3] = Color::Yellow;

}

bool ClientApp::on_init()
{
   network_.set_send_rate(Time(1.0 / 10.0));
   if (!network_.initialize({})) {
      return false;
   }

   connection_.set_listener(this);
   playerStartPositions[0] = Vector2 (window_.width_ / 2, 0);
   playerStartPositions[1] = Vector2 (window_.width_, window_.height_ / 2);
   playerStartPositions[2] = Vector2 (window_.width_ / 2, window_.height_);
   playerStartPositions[3] = Vector2 (0, window_.height_ / 2);
   for (int i = 0; i < 4; i++) {
       if (i==localPlayer.playerID) {
           player_[i].playerColor = playerColors[i];
           player_[i].position_ = playerStartPositions[i];
           break;
       }
   }
   return true;
}

void ClientApp::on_exit () {
    
}

bool ClientApp::on_tick (const Time& dt) {
    if (keyboard_.pressed (Keyboard::Key::Escape)) {
        on_exit ();
        return false;
    }
    switch (gameState) {
    case gameplay::GameState::Searching: {
        if (!serverFound)
            serverFound = ServerDiscovery ();
        if (serverFound) {
            if (keyboard_.pressed (Keyboard::Key::Space) && (connection_.state_ == network::Connection::State::Invalid || connection_.is_disconnected ())) {
                connection_.connect (serverIP);
            }
        }
        if (connection_.is_connected ()) {
            gameState = gameplay::GameState::Gameplay;

        }
        break;
    }
    case gameplay::GameState::Gameplay: {
        accumulator_ += dt;
        while (accumulator_ >= tickrate_) {
            globalTick = recievedServerTick;
            accumulator_ -= tickrate_;

            if (positionHistory.size () > 0) {
                if (positionHistory[0].tick - tickrate_.as_milliseconds () > 200) {
                    positionHistory.erase (positionHistory.begin ());
                }
                for (int i = 0; i < positionHistory.size (); i++) {
                    for (int j = 0; j < 3; j++)
                        if (entity_[j].entityID == positionHistory[i].ID){
                            entity_[j].position_ = positionHistory[i].position;
                        }
                }
            }

            input_bits_ = 0;
            for (int i = 0; i < 4; i++) {
                if (player_[i].playerID == localPlayer.playerID) {
                    if (player_[i].position_.y_ > 0) {
                        if (keyboard_.down (Keyboard::Key::W)) {
                            input_bits_ |= (1 << int32 (gameplay::Action::Up));
                        }
                    }
                    if (player_[i].position_.y_ < window_.height_) {
                        if (keyboard_.down (Keyboard::Key::S)) {
                            input_bits_ |= (1 << int32 (gameplay::Action::Down));
                        }
                    }
                    if (player_[i].position_.x_ > 0) {
                        if (keyboard_.down (Keyboard::Key::A)) {
                            input_bits_ |= (1 << int32 (gameplay::Action::Left));
                        }
                    }
                    if (player_[i].position_.x_ < window_.width_) {
                        if (keyboard_.down (Keyboard::Key::D)) {
                            input_bits_ |= (1 << int32 (gameplay::Action::Right));
                        }
                    }
                    if (keyboard_.down (Keyboard::Key::Space)) {
                        input_bits_ |= (1 << int32 (gameplay::Action::Shoot));
                    }

                    const bool player_shoot = input_bits_ & (1 << int32 (gameplay::Action::Shoot));
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
                        player_[i].position_ = Vector2::lerp (player_[i].position_, player_[i].position_ + direction * speed, tickrate_.as_seconds ());
                    }
                    InputPrediction temp;
                    temp.inputBits = input_bits_;
                    temp.tick = globalTick;
                    temp.calculatedPosition = player_[i].position_;
                    inputLibrary.push_back (temp);
                }
            }
            break;

        }
    }
    case gameplay::GameState::Exit: {

        break;
    }
    }
    return true;
}

void ClientApp::on_draw () {
    switch (gameState) {
    case gameplay::GameState::Searching: {
        if (serverFound) {
            renderer_.render_text_va ({ 2,2 }, Color::Yellow, 1, "Server found at: %s", serverIP.as_string ());
            renderer_.render_text ({ 3,12 }, Color::Red, 1, "Do you want to connect? ");
        }
        break;
    }
    case gameplay::GameState::Gameplay: {
        renderer_.clear ({ 0.2f, 0.3f, 0.4f, 1.0f });
        renderer_.render_text ({ 2, 2 }, Color::White, 1, "CLIENT");
        for (int i = 0; i < 4; i++) {
            if(player_[i].playerID==localPlayer.playerID)
                renderer_.render_rectangle_fill ({ int32 (player_[i].position_.x_), int32 (player_[i].position_.y_), 20, 20 }, player_[i].playerColor);
        }
        break;
    }
    case charlie::gameplay::GameState::Exit: {
        break;
    }
    }
}
   

void ClientApp::on_acknowledge (network::Connection* connection, const uint16 sequence) {

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
            localPlayer.playerID = message.player_ID;
            break;
        }

        case network::NETWORK_MESSAGE_ENTITY_STATE:
        {
            network::NetworkMessageEntityState message;
            if (!message.read (reader)) {
                assert (!"could not read message!");
            }
            PositionHistory temp;
            temp.position = message.position_;
            temp.tick = recievedServerTick;
            temp.ID = message.entityID;
            positionHistory.push_back (temp);
            //for (int i = 0; i < 4; i++) {
            //    if (message.entityID == entity_[i].entityID) {
            //        entity_[i].position_ = message.position_;
            //    }
            //}
            break;
        }

        case network::NETWORK_MESSAGE_PLAYER_STATE:
        {
            network::NetworkMessagePlayerState message;
            if (!message.read (reader)) {
                assert (!"could not read message!");
            }
            localPlayer.playerID = message.playerID;
            for (int i = 0; i < 4; i++) {
                if (message.playerID == player_[i].playerID) {
                    player_[i].position_ = message.position_;
                }
            }
            break;
        }
        default:
        {
            assert (!"unknown message type received from server!");
        } break;
        }
    }
}

void ClientApp::on_send (network::Connection* connection, const uint16 sequence, network::NetworkStreamWriter& writer) {
    for (int i = 0; i < 4; i++) {
        if (player_[i].playerID == localPlayer.playerID) {
            network::NetworkMessageInputCommand command (input_bits_, player_[i].playerID);
            if (!command.write (writer)) {
                assert (!"could not write network command!");
            }
        }
    }
}

bool ClientApp::ServerDiscovery () {
    if (!socket.is_valid ())
        socket.open ();
    if (socket.is_valid ()) {
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
                if (reader.peek () == network::ProtocolPacketType::PROTOCOL_PACKET_CHALLENGE) {
                    printf ("Server found\n");
                    serverIP = address;
                    found = true;
                    return true;
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
        printf ("ERR: %d\n", error_code);
        if (network::Error::is_critical (error_code)) {
            assert (false);
        }
        return false;
    }
    else
        return false ;
}

void ConnectionHandler::on_rejected (const uint8 reason) {
    printf ("Lobby full, %d", reason);
}
