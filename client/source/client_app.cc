
#include "client_app.hpp"
#include <charlie_messages.hpp>
#include <cstdio>
#include <charlie_network.hpp>
#include <charlie_protocol.hpp>
#include <string>
using namespace std;

template <typename T, std::size_t N>
constexpr auto array_size(T(&)[N])
{
   return N;
}

ClientApp::ClientApp()
   : mouse_(window_.mouse_)
   , keyboard_(window_.keyboard_)
   , tickrate_(1.0 /  60.0)
   , input_bits_(0)
{
    globalTick = 0;
    recievedServerTick = 0;
    gameState = gameplay::GameState::Searching;
    playerColors[0] = Color::Red;
    playerColors[1] = Color::Green;
    playerColors[2] = Color::Blue;
    playerColors[3] = Color::Yellow;
    player_.alive = true;
    idApplied = false;
    for (int i = 0; i < 4; i++) {
        bullets[i].active = false;
        bullets[i].bulletID = i;
    }
}

bool ClientApp::on_init () {
    networkData.packetsSent = 0;
    networkData.packetsReceived = 0;
    networkData.packetsDelivered = 0;
    networkData.packetsLost = 0;
    networkData.detailsOverlay = false;

    player_.playerID = 999;
    for (int i = 0; i < 3; i++) {
        entity_[i].entityID = 999;
    }
    network_.set_send_rate (Time (1.0 / 20.0));
    if (!network_.initialize ({})) {
        return false;
    }

    connection_.set_listener (this);
    playerStartPositions[0] = Vector2 (window_.width_ / 2, 0);
    playerStartPositions[1] = Vector2 (window_.width_ - 20, window_.height_ / 2);
    playerStartPositions[2] = Vector2 (window_.width_ / 2, window_.height_ - 20);
    playerStartPositions[3] = Vector2 (0, window_.height_ / 2);
    return true;
}

void ClientApp::on_exit () {
    connection_.disconnect ();
}

bool ClientApp::on_tick (const Time& dt) {
    if (keyboard_.pressed (Keyboard::Key::Escape)) {
        on_exit ();
        return false;
    }
    if (keyboard_.pressed (Keyboard::Key::F3)) {
        networkData.detailsOverlay = !networkData.detailsOverlay;
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
                if (interpolatorAccumulator.as_seconds () > tickrate_.as_seconds ())
                    interpolatorAccumulator = Time ();
                else
                    interpolatorAccumulator += tickrate_;
                for (auto& en : entity_) {
                    if (en.entityID < 4 && en.positionHistory.size () > 4) {
                        if (recievedServerTick - en.positionHistory[0].tick > 20000) {
                            en.positionHistory.erase (en.positionHistory.begin ());
                        }
                        for (int i = 0; i < en.positionHistory.size (); i++) {
                            if (recievedServerTick - en.positionHistory[i].tick < 20000) {
                                Vector2 direction = en.positionHistory[i].position - en.position_;
                                const float interpolator = (en.lastTick - en.positionHistory[i].tick) * tickrate_.as_seconds ();
                                const float t = interpolator/ accumulator_.as_seconds ();
                                en.position_ = Vector2::lerp (en.position_, en.positionHistory[i].position, t);
                                en.lastTick = en.positionHistory[i].tick;
                            }
                        }
                    }
                }

                for (auto& bl : bullets) {
                    if (bl.bulletID < 4 && bl.positionHistory.size () > 4) {
                        if (recievedServerTick - bl.positionHistory[0].tick > 20000) {
                            bl.positionHistory.erase (bl.positionHistory.begin ());
                        }
                        for (int i = 0; i < bl.positionHistory.size (); i++) {
                            if (recievedServerTick - bl.positionHistory[i].tick < 20000) {
                                Vector2 direction = bl.positionHistory[i].position - bl.position_;
                                const float interpolator = (bl.lastTick - bl.positionHistory[i].tick) * tickrate_.as_seconds ();
                                const float t =interpolator/ accumulator_.as_seconds ();
                                bl.position_ = Vector2::lerp (bl.position_, bl.positionHistory[i].position, t);
                                bl.lastTick = bl.positionHistory[i].tick;
                            }
                        }
                    }
                }

                input_bits_ = 0;
                if (player_.alive) {
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

                    //const bool player_shoot = input_bits_ & (1 << int32 (gameplay::Action::Shoot));
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
                    if (direction.length () > 0.0f) {
                        direction.normalize ();
                        player_.position_ = Vector2::lerp (player_.position_, player_.position_ + direction * speed, tickrate_.as_seconds ());
                    }
                    InputPrediction temp;
                    temp.inputBits = input_bits_;
                    temp.tick = globalTick;
                    temp.calculatedPosition = player_.position_;
                    inputLibrary.push_back (temp);
                    //add input prediction
                    if (inputLibrary[0].tick - tickrate_.as_ticks () > 20000) {
                        inputLibrary.erase (inputLibrary.begin ());
                    }
                }
                else {
                    gameState = gameplay::GameState::Exit;
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
    if (networkData.detailsOverlay) {
        char charArray[2048] = {};
        strcat_s (charArray, std::to_string (networkData.packetLoss).c_str ());
        renderer_.render_text_va ({ 2,12 }, Color::Red, 1, "Packet Loss: %f",networkData.packetLoss);
    }
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
                if (entity_[i].alive) {
                    renderer_.render_rectangle_fill ({ int32 (entity_[i].position_.x_), int32 (entity_[i].position_.y_), 20, 20 }, entity_[i].entityColor);
                }
            }
            for (int i = 0; i < 4; i++) {
                if (bullets[i].active) {
                    renderer_.render_rectangle_fill ({ static_cast<int32>(bullets[i].position_.x_),static_cast<int32>(bullets[i].position_.y_),5,5 }, playerColors[i]);
                }
            }
            break;
        }
        case charlie::gameplay::GameState::Exit: {
            renderer_.render_text ({ 2, 12 }, Color::White, 1, "You lost");
            break;
        }
    }
}
   
void ClientApp::on_acknowledge (network::Connection* connection, const uint16 sequence) {
    int temp = networkData.packetsDelivered;
    for (int i = 0; i < networkData.sequenceStack.size (); i++) {
        if (networkData.sequenceStack[i] == sequence) {
            for (int j = 0; j <= i; j++) {
                networkData.sequenceStack.erase (networkData.sequenceStack.begin ());
                i--;
            }
            networkData.packetsDelivered++;
            break;
        }
        else {
            continue;
        }
    }
    if (temp == networkData.packetsDelivered)
        networkData.packetsLost++;
    networkData.packetLoss = (float) networkData.packetsLost / networkData.packetsSent;
}

void ClientApp::on_receive (network::Connection* connection, network::NetworkStreamReader& reader) {
    networkData.packetsReceived++;
    ///bytes received is reader.length()
    while (reader.position () < reader.length ()) {
        switch (reader.peek ()) {
            case network::NETWORK_MESSAGE_SHOOT: {
                 network::NetworkMessageShoot message;
                if (!message.read (reader)) {
                    assert (!"could not read message!");
                }
                for (int i = 0; i < 4; i++) {
                    if (bullets[i].bulletID == message.playerID) {
                        gameplay::PositionHistory temp;
                        temp.position = message.bulletPosition;
                        temp.tick = recievedServerTick;
                        bullets[i].active = message.bulletActive;
                        bullets[i].positionHistory.push_back (temp);
                        bullets[i].position_ = message.bulletPosition;
                        break;
                    }
                }
                break;
            }

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
                        if (message.entityAlive) {
                            gameplay::PositionHistory temp;
                            temp.position = message.position_;
                            temp.tick = recievedServerTick;
                            entity_[i].positionHistory.push_back (temp);
                            entity_[i].position_ = message.position_;
                            entity_[i].entityColor = playerColors[entity_[i].entityID];
                            entity_[i].alive = message.entityAlive;
                        }
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
                    if (message.playerAlive) {
                        player_.position_ = message.position_;
                        Color tempColor;
                        player_.playerColor = playerColors[player_.playerID];
                        player_.alive = message.playerAlive;
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
    network::NetworkMessageInputCommand command (input_bits_, player_.playerID, sequence);
    networkData.sequenceNumber = sequence;
    networkData.sequenceStack.push_back (networkData.sequenceNumber);
    if (!command.write (writer)) {
        assert (!"could not write network command!");
    }
    networkData.packetsSent++;
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
        return false;
}

void ConnectionHandler::on_rejected (const uint8 reason) {
    printf ("Lobby full, %d", reason);
}
