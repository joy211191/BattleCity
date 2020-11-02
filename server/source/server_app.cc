// server_app.cc

#include "server_app.hpp"
#include <charlie_messages.hpp>
#include <cstdio>
#include <cmath>
#include <charlie.hpp>
using namespace std;

ServerApp::ServerApp (): tickrate_ (1.0 / 60.0), tick_ (0) {
    for (int i = 0; i < sizeof (player_input_bits_); i++) {
        player_input_bits_[i] = 0;
    }

    gameState = gameplay::GameState::Searching;

    playerStartPositions[0] = Vector2 (window_.width_ / 2, 0);
    playerStartPositions[1] = Vector2 (window_.width_, window_.height_ / 2);
    playerStartPositions[2] = Vector2 (window_.width_ / 2, window_.height_);
    playerStartPositions[3] = Vector2 (0, window_.height_ / 2);

    playerColors[0] = Color::Red;
    playerColors[1] = Color::Green;
    playerColors[2] = Color::Blue;
    playerColors[3] = Color::Yellow;
}

bool ServerApp::on_init () {
    network_.set_send_rate (Time (1.0 / 10.0));
    network_.set_allow_connections (true);
    network_.set_connection_limit (16);
    if (!network_.initialize (network::IPAddress (network::IPAddress::ANY_HOST, 54345))) {
        return false;
    }

    network_.add_service_listener (this);

    entity_.position_ = { 300.0f, 180.0f };
    for (int i = 0; i < players_.size (); i++) {
        players_[i].position_ = playerStartPositions[i];
        players_[i].playerColor = playerColors[i];
    }
    return true;
}

void ServerApp::on_exit () {
}

bool ServerApp::on_tick (const Time& dt) {
    accumulator_ += dt;
    while (accumulator_ >= tickrate_) {
        accumulator_ -= tickrate_;
        tick_++;

        switch (gameState) {
        case gameplay::GameState::Searching: {
            if (keyboard.pressed (Keyboard::Key::Space)||players_.size()==4) {
                gameState = gameplay::GameState::Gameplay;
                break;
            }
            break;
        }
        case gameplay::GameState::Gameplay: {
            for (int i = 0; i < players_.size (); i++) {
                const bool player_move_up = player_input_bits_[i] & (1 << int32 (gameplay::Action::Up));
                const bool player_move_down = player_input_bits_[i] & (1 << int32 (gameplay::Action::Down));
                const bool player_move_left = player_input_bits_[i] & (1 << int32 (gameplay::Action::Left));
                const bool player_move_right = player_input_bits_[i] & (1 << int32 (gameplay::Action::Right));
                const bool player_shoot = player_input_bits_[i] & (1 << int32 (gameplay::Action::Shoot));

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
                    charlie::gameplay::Event tempEvent;
                    tempEvent.playerID = players_[i].playerID;
                    tempEvent.position = players_[i].position_;
                    tempEvent.state = charlie::gameplay::EventStates::Shooting;
                    eventQueue.push_back (tempEvent);
                }
                const float speed = 100.0;
                if (direction.length () > 0.0f) {
                    direction.normalize ();
                    players_[i].position_ += direction * speed * tickrate_.as_seconds ();
                }
            }
            for (int i = 0; i < players_.size (); i++) {
                if (players_[i].hp == 0){
                    gameState = gameplay::GameState::Exit;
                        break;
                }
            }
            break;
        }
        case gameplay::GameState::Exit: {
           
            break;
        }
        }
    }
    return true;
}

void ServerApp::on_draw () {
    switch (gameState) {
    case gameplay::GameState::Searching: {
        renderer_.render_text ({ 5,12 }, Color::White, 1, "Waiting for Connections. Press space to start");
        if (players_.size () > 0) {
            for (int i = 0; i < players_.size (); i++) {
                renderer_.render_text_va ({ 5,22 + i * 10 }, Color::White, 1, "%d", players_[i].playerID);
            }
        }
        break;
    }
    case gameplay::GameState::Gameplay: {
        renderer_.clear ({ 0.4f, 0.3f, 0.2f, 1.0f });
        renderer_.render_text ({ 2, 2 }, Color::White, 2, "SERVER");
        renderer_.render_rectangle_fill ({ static_cast<int32>(send_position_.x_), static_cast<int32>(send_position_.y_),  20, 20 }, Color::Yellow);
        renderer_.render_rectangle_fill ({ static_cast<int32>(entity_.position_.x_), static_cast<int32>(entity_.position_.y_),  20, 20 }, Color::Red);
        for (int i = 0; i < players_.size (); i++) {
            renderer_.render_rectangle_fill ({ static_cast<int32>(players_[i].position_.x_), static_cast<int32>(players_[i].position_.y_),  20, 20 }, players_[i].playerColor);
        }
        break;
    }
    case gameplay::GameState::Exit: {

        break;
    }
    }
}

void ServerApp::on_timeout (network::Connection* connection) {
    connection->set_listener (nullptr);
    auto id = clients_.find_client ((uint64)connection);
    // ...
    clients_.remove_client ((uint64)connection);
}

void ServerApp::on_connect (network::Connection* connection) {
    connection->set_listener (this);
    if (players_.size () < 4) {
        auto id = clients_.add_client ((uint64)connection);
        gameplay::Player tempPlayer;
        tempPlayer.playerID = id;
        players_.push_back (tempPlayer);
    }
    connection->set_state (network::Connection::State::Rejected);
}

void ServerApp::on_disconnect (network::Connection* connection) {
    connection->set_listener (nullptr);
    auto id = clients_.find_client ((uint64)connection);
    // ...
    clients_.remove_client ((uint64)connection);
}

void ServerApp::on_acknowledge (network::Connection* connection, const uint16 sequence) {
    auto id = clients_.find_client ((uint64)connection);
    for (auto& pl : players_) {
        if (pl.playerID == id) {
            for (auto i = pl.eventQueue.begin (); i != pl.eventQueue.end ();) {
                if (i->sequenceNumber == sequence) {
                    i=pl.eventQueue.erase (i);
                }
                else {
                    ++i;
                }
            }
        }
    }
}

void ServerApp::on_receive (network::Connection* connection,  network::NetworkStreamReader& reader) {
    auto id = clients_.find_client ((uint64)connection);
    while (reader.position () < reader.length ()) {
        if (reader.peek () != network::NETWORK_MESSAGE_INPUT_COMMAND) {
            break;
        }

        network::NetworkMessageInputCommand command;
        if (!command.read (reader)) {
            assert (!"could not read command!");
        }
        for (int i = 0; players_.size (); i++) {
            if (players_[i].playerID == id) {
                player_input_bits_[i] = command.bits_;
                break;
            }
        }
    }
}

void ServerApp::on_send (network::Connection* connection, const uint16 sequence, network::NetworkStreamWriter& writer) {
    auto id = clients_.find_client ((uint64)connection);
    {
        network::NetworkMessageServerTick message (Time::now ().as_ticks (), tick_);
        if (!message.write (writer)) {
            assert (!"failed to write message!");
        }
    }
    {
        for (int i = 0; i < players_.size(); i++) {
            if (id == players_[i].playerID) {
                if (!eventQueue.empty ()) {
                    charlie::gameplay::ReliableMessage temp;
                    temp.event = eventQueue.front ();
                    temp.sequenceNumber = sequence;
                    players_[i].eventQueue.push_back (temp);
                }
                send_position_ = playerStartPositions[i];
                network::NetworkMessagePlayerState message (players_[i].position_,players_[i].playerID);
                if (!message.write (writer)) {
                    assert (!"failed to write message!");
                }
            }
            else {
                send_position_ = entity_.position_;
                network::NetworkMessageEntityState message (entity_.position_);
                if (!message.write (writer)) {
                    assert (!"failed to write message!");
                }
            }
        }
    }
}