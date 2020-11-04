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
   , tickrate_(1.0 /  20.0)
   , input_bits_(0)
{
    globalTick = 0;
    recievedServerTick = 0;
    gameState = gameplay::GameState::Searching;
    playerColors[0] = Color::Red;
    playerColors[1] = Color::Green;
    playerColors[2] = Color::Blue;
    playerColors[3] = Color::Yellow;
    iterator = 0;
    idApplied = false;
}

bool ClientApp::on_init()
{
   player_.playerID = 999;
   for (int i = 0; i < 3; i++) {
       entity_[i].entityID = 999;
   }
   network_.set_send_rate(Time(1.0 / 60.0));
   if (!network_.initialize({})) {
      return false;
   }

   connection_.set_listener(this);
   playerStartPositions[0] = Vector2 (window_.width_ / 2, 0);
   playerStartPositions[1] = Vector2 (window_.width_-20, window_.height_ / 2);
   playerStartPositions[2] = Vector2 (window_.width_ / 2, window_.height_-20);
   playerStartPositions[3] = Vector2 (0, window_.height_ / 2);
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
            for (auto &en:entity_) {
                if (en.entityID<4&& en.positionHistory.size () > 4) {
                    if (recievedServerTick - en.positionHistory[0].tick > 20000) {
                        en.positionHistory.erase (en.positionHistory.begin ());
                    }
                    else {
                        en.position_ = Vector2::lerp (en.position_, en.positionHistory[0].position, (en.lastTick-en.positionHistory[0].tick+recievedServerTick)*tickrate_.as_milliseconds());
                        en.lastTick = en.positionHistory[0].tick;
                    }
                    for (int i = 1;i< en.positionHistory.size (); i++) {
                        if (recievedServerTick - en.positionHistory[i].tick < 20000) {
                            en.position_ = Vector2::lerp (en.position_, en.positionHistory[i].position, (en.lastTick - en.positionHistory[i].tick+recievedServerTick) * tickrate_.as_milliseconds());
                            en.lastTick = en.positionHistory[i].tick;
                        }
                    }
                }
            }

            input_bits_ = 0;

            if (player_.position_.y_ > 0) {
                if (keyboard_.down (Keyboard::Key::W)) {
                    input_bits_ |= (1 << int32 (gameplay::Action::Up));
                }
            }
            if (player_.position_.y_ < window_.height_) {
                if (keyboard_.down (Keyboard::Key::S)) {
                    input_bits_ |= (1 << int32 (gameplay::Action::Down));
                }
            }
            if (player_.position_.x_ > 0) {
                if (keyboard_.down (Keyboard::Key::A)) {
                    input_bits_ |= (1 << int32 (gameplay::Action::Left));
                }
            }
            if (player_.position_.x_ < window_.width_) {
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
            if (player_shoot) {

            }
            if (direction.length () > 0.0f) {
                direction.normalize ();
                player_.position_ = Vector2::lerp (player_.position_, player_.position_ + direction * speed, tickrate_.as_seconds ());
            }
            InputPrediction temp;
            temp.inputBits = input_bits_;
            temp.tick = globalTick;
            temp.calculatedPosition = player_.position_;
            inputLibrary.push_back (temp);
            if (inputLibrary[0].tick - tickrate_.as_milliseconds() > 20000) {
                inputLibrary.erase (inputLibrary.begin ());
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

bool ClientApp::CollisionCheck (gameplay::Player playerA, gameplay::Player playerB) {
    if (playerA.position_.x_<playerB.position_.x_ + 20 && playerA.position_.x_ + 20 >playerB.position_.x_ && playerA.position_.y_<playerB.position_.y_ + 20 && playerA.position_.y_ + 20>playerB.position_.y_)
        return true;
    else
        return false;
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
        renderer_.render_rectangle_fill ({ int32 (player_.position_.x_), int32 (player_.position_.y_), 20, 20 }, player_.playerColor);
        for (int i = 0; i < 3; i++) {
                renderer_.render_rectangle_fill ({ int32 (entity_[i].position_.x_), int32 (entity_[i].position_.y_), 20, 20 }, entity_[i].entityColor);
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
            if (!idApplied) {
                player_.playerID = message.playerID;
                switch (player_.playerID) {
                    case 0: {
                        entity_[0].entityID = 1;
                        entity_[1].entityID = 2;
                        entity_[2].entityID = 3;
                        break;
                    }
                    case 1: {
                        entity_[0].entityID = 0;
                        entity_[1].entityID = 2;
                        entity_[2].entityID = 3;
                        break;
                    }
                    case 2: {
                        entity_[0].entityID = 0;
                        entity_[1].entityID = 1;
                        entity_[2].entityID = 3;
                        break;
                    }
                    case 3: {
                        entity_[0].entityID = 0;
                        entity_[1].entityID = 1;
                        entity_[2].entityID = 2;
                        break;
                    }
                }
                idApplied = true;
            }
            const Time current = Time (message.server_time_);
            recievedServerTick = message.server_tick_;
            break;
        }
        case network::NETWORK_MESSAGE_ENTITY_STATE:
        {
            network::NetworkMessageEntityState message;
            if (!message.read (reader)) {
                assert (!"could not read message!");
            }

            for (int i = 0; i < 3; i++) {
                if (entity_[i].entityID == message.entityID) {
                    gameplay::PositionHistory temp;
                    temp.position = message.position_;
                    temp.tick = recievedServerTick;
                    entity_[i].positionHistory.push_back (temp);
                    entity_[i].position_ = message.position_;
                    entity_[i].entityColor = playerColors[entity_[i].entityID];
                    break;
                }
            }
            break;
        }
        case network::NETWORK_MESSAGE_PLAYER_STATE:
        {
            network::NetworkMessagePlayerState message;
            if (!message.read (reader)) {
                assert (!"could not read message!");
            }
            if (message.playerID == player_.playerID) {
                player_.position_ = message.position_;
                Color tempColor;
                player_.playerColor = playerColors[player_.playerID];
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
        network::NetworkMessageInputCommand command (input_bits_, player_.playerID);
        if (!command.write (writer)) {
            assert (!"could not write network command!");
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
