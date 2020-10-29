// server_app.cc

#include "server_app.hpp"
#include <charlie_messages.hpp>
#include <cstdio>
#include <cmath>
#include <charlie.hpp>

ServerApp::ServerApp (): tickrate_ (1.0 / 60.0), tick_ (0) {
    for (int i = 0; i < sizeof (player_input_bits_); i++) {
        player_input_bits_[i] = 0;
    }

    gameState = gameplay::GameState::Gameplay;

    playerStartPositions[0] = Vector2 (window_.width_ / 2, 0);
    playerStartPositions[1] = Vector2 (window_.width_, window_.height_ / 2);
    playerStartPositions[0] = Vector2 (window_.width_ / 2, window_.height_);
    playerStartPositions[0] = Vector2 (0, window_.height_ / 2);
}

bool ServerApp::on_init () {
    network_.set_send_rate (Time (1.0 / 10.0));
    network_.set_allow_connections (true);
    if (!network_.initialize (network::IPAddress (network::IPAddress::ANY_HOST, 54345))) {
        return false;
    }

    network_.add_service_listener (this);

    entity_.position_ = { 300.0f, 180.0f };
    for (int i = 0; i < player_.size (); i++) {
        player_[i].position_ = playerStartPositions[i];
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
        case gameplay::GameState::Lobby: {

            break;
        }
        case gameplay::GameState::Gameplay: {
            for (int i = 0; i < player_.size (); i++) {
                // note: update player
                const bool player_move_up = player_input_bits_[i] & (1 << int32 (gameplay::Action::Up));
                const bool player_move_down = player_input_bits_[i] & (1 << int32 (gameplay::Action::Down));
                const bool player_move_left = player_input_bits_[i] & (1 << int32 (gameplay::Action::Left));
                const bool player_move_right = player_input_bits_[i] & (1 << int32 (gameplay::Action::Right));

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
                    player_[i].position_ += direction * speed * tickrate_.as_seconds ();
                }
            }
            entity_.position_.x_ = 300.0f + std::cosf (Time::now ().as_seconds ()) * 150.0f;
            entity_.position_.y_ = 180.0f + std::sinf (Time::now ().as_seconds ()) * 100.0f;
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
    renderer_.clear ({ 0.4f, 0.3f, 0.2f, 1.0f });
    renderer_.render_text ({ 2, 2 }, Color::White, 2, "SERVER");
    renderer_.render_rectangle_fill ({ static_cast<int32>(send_position_.x_), static_cast<int32>(send_position_.y_),  20, 20 }, Color::Yellow);
    renderer_.render_rectangle_fill ({ static_cast<int32>(entity_.position_.x_), static_cast<int32>(entity_.position_.y_),  20, 20 }, Color::Red);
    for (int i = 0; i < player_.size (); i++) {
        renderer_.render_rectangle_fill({ static_cast<int32>(player_[i].position_.x_), static_cast<int32>(player_[i].position_.y_),  20, 20 }, Color::Magenta);
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
    if (player_.size () < 4) {
        auto id = clients_.add_client ((uint64)connection);
        // event : "player_connected"
        //Sending player the playerID back
        gameplay::Player tempPlayer;
        tempPlayer.playerID = id;
        player_.push_back (tempPlayer);
    }
}

void ServerApp::on_disconnect (network::Connection* connection) {
    connection->set_listener (nullptr);
    auto id = clients_.find_client ((uint64)connection);
    // ...
    clients_.remove_client ((uint64)connection);
}

void ServerApp::on_acknowledge (network::Connection* connection, const uint16 sequence) {

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
        for (int i = 0; player_.size (); i++) {
            if (player_[i].playerID == id) {
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
        for (int i = 0; i < player_.size(); i++) {

            if (id == player_[i].playerID) {
                send_position_ = playerStartPositions[i];
                network::NetworkMessagePlayerState message (player_[i].position_);
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