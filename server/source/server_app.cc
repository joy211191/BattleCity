
#include "server_app.hpp"
#include <charlie_messages.hpp>
#include <cstdio>
#include <cmath>
#include <charlie.hpp>
using namespace std;

ServerApp::ServerApp () : tickrate_ (1.0 / 20.0), tick_ (0) {

    for (int i = 0; i < sizeof (player_input_bits_); i++) {
        player_input_bits_[i] = 0;
    }
    winnerID = 4;
    gameState = gameplay::GameState::Searching;

    playerColors[0] = Color::Red;
    playerColors[1] = Color::Green;
    playerColors[2] = Color::Blue;
    playerColors[3] = Color::Yellow;

    for (int i = 0; i < 4; i++) {
        bullets[i].active = false;
    }
}

bool ServerApp::on_init () {

    network_.set_send_rate (Time (1.0 / 20.0));
    network_.set_allow_connections (true);
    network_.set_connection_limit (4);
    if (!network_.initialize (network::IPAddress (network::IPAddress::ANY_HOST, 54345))) {
        return false;
    }
    network_.add_service_listener (this);

    entity_.position_ = { 300.0f, 180.0f };
    for (int i = 0; i < players_.size (); i++) {
        players_[i].position_ = playerStartPositions[i];
        players_[i].playerColor = playerColors[i];
    }

    playerStartPositions[0] = Vector2 ((window_.width_ / 2)-10, 0);
    playerStartPositions[1] = Vector2 (window_.width_-20, (window_.height_ / 2)-10);
    playerStartPositions[2] = Vector2 ((window_.width_ / 2)-10, window_.height_-20);
    playerStartPositions[3] = Vector2 (0, (window_.height_ / 2)-10);
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
            if (players_.size () > 0) {
                for (int i = 0; i < players_.size ();i++) {
                    players_[i].position_ = playerStartPositions[i];
                }
            }
            if (players_.size()==4) {
                gameState = gameplay::GameState::Setup;
            }
            break;
        }
        case gameplay::GameState::Setup: {
            for (int i = 0; i < players_.size (); i++) {
                players_[i].position_ = playerStartPositions[i];
                bullets[i].position_ = playerStartPositions[i];
                bullets[i].bulletID = i;
                players_[i].alive = true;
                bullets[i].active = false;
            }
            gameState = gameplay::GameState::Gameplay;
            break;
        }
        case gameplay::GameState::Gameplay: {
            auto in = inputLibrary.begin();
            while(in!=inputLibrary.end()) {
                if (players_[(*in).playerID].alive) {
                    const float speed = 100.0;
                    const bool player_move_up = player_input_bits_[(*in).playerID] && 1;
                    const bool player_move_down = player_input_bits_[(*in).playerID] && 2;
                    const bool player_move_left = player_input_bits_[(*in).playerID] && 4;
                    const bool player_move_right = player_input_bits_[(*in).playerID] && 8;
                    const bool player_shoot = player_input_bits_[(*in).playerID] && 16;
                    inputLibrary.erase(inputLibrary.begin(), in);

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

                   if ( direction.length() > 0) {
                        charlie::gameplay::Event tempEvent;
                        tempEvent.playerID = players_[(*in).playerID].playerID;
                        tempEvent.position = players_[(*in).playerID].position_;
                        tempEvent.state = charlie::gameplay::EventStates::Shooting;
                        eventQueue.push_back(tempEvent);
                        if (!bullets[(*in).playerID].active) {
                            bullets[(*in).playerID].bulletID = (*in).playerID;
                            bullets[(*in).playerID].active = true;
                            Vector2 offsetPosition;
                            offsetPosition.x_ = 10;
                            offsetPosition.y_ = 10;
                            bullets[(*in).playerID].position_ = players_[(*in).playerID].position_ + offsetPosition;
                            bullets[(*in).playerID].direction = direction;
                            bullets[(*in).playerID].direction.normalize();
                        }
                    }
                    if (direction.length() > 0.0f) {
                        direction.normalize();
                        players_[(*in).playerID].position_ += direction * speed * tickrate_.as_seconds();
                    }
                }
                in++;
            }
            for (auto& bl : bullets) {
                Bullet (bl.bulletID, bl.direction);
            }
            int temp = 0;
            for (int i = 0; i < players_.size (); i++) {
                if (players_[i].alive){
                    temp++;
                }
            }
            if (temp == 1) {
                for (int i = 0; i < players_.size (); i++) {
                    if (players_[i].alive) {
                        winnerID = i;
                        break;
                    }
                }
                gameState = gameplay::GameState::Exit;
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

bool ServerApp::CollisionCheck (Vector2 positionA, Vector2 positionB) {
    if (positionA.x_<positionB.x_ + 20 && positionA.x_ + 20 >positionB.x_ && positionA.y_<positionB.y_ + 20 && positionA.y_ + 20>positionB.y_)
        return true;
    else
        return false;
}

void ServerApp::Bullet (int id,Vector2 direction) {
    if (!bullets[id].active)
        return;
    if (bullets[id].active) {
        bullets[id].position_ += bullets[id].direction * bulletSpeed * tickrate_.as_seconds ();
        if (bullets[id].position_.x_<0 || bullets[id].position_.x_>window_.width_ || bullets[id].position_.y_<0 || bullets[id].position_.y_>window_.height_)
            bullets[id].active = false;
        for (auto& pl : players_) {
            printf ("%d", CollisionCheck (bullets[id].position_, pl.position_));
            if (pl.playerID != id && CollisionCheck (bullets[id].position_, pl.position_)) {
                pl.alive = false;
                break;
            }
        }
    }
}

void ServerApp::on_draw () {
    switch (gameState) {
        case gameplay::GameState::Searching: {
            renderer_.render_text ({ 5,12 }, Color::White, 1, "Waiting for Connections. 4 players needed");
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
            for (int i = 0; i < players_.size (); i++) {
                if (players_[i].alive)
                    renderer_.render_rectangle_fill ({ static_cast<int32>(players_[i].position_.x_), static_cast<int32>(players_[i].position_.y_),  20, 20 }, playerColors[i]);
                if (bullets[i].active) {
                    renderer_.render_rectangle_fill ({ static_cast<int32>(bullets[i].position_.x_),static_cast<int32>(bullets[i].position_.y_),5,5 }, playerColors[i]);
                }
            }
            break;
        }
        case gameplay::GameState::Exit: {
            renderer_.render_text ({ 2,2 }, Color::White, 2, "Game over");
            renderer_.render_text_va ({ 2,12 }, playerColors[winnerID], 2, "Winner");
            break;
        }
    }
}

void ServerApp::on_timeout (network::Connection* connection) {
    connection->set_listener (nullptr);
    auto id = clients_.find_client ((uint64)connection);
    clients_.remove_client ((uint64)connection);
    printf ("Timeout");
}

void ServerApp::on_connect (network::Connection* connection) {
    connection->set_listener (this);
    if (players_.size () < 4) {
        auto id = clients_.add_client ((uint64)connection);
        gameplay::Player tempPlayer;
        tempPlayer.playerID = id;
        players_.push_back (tempPlayer);
    }
    else {
        connection->set_state (network::Connection::State::Rejected);
    }
}

void ServerApp::on_disconnect (network::Connection* connection) {
    connection->set_listener (nullptr);
    auto id = clients_.find_client ((uint64)connection);
    clients_.remove_client ((uint64)connection);
    printf ("Disconnected");
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
        printf("Recieved tick: %d\n", (int)command.tick_);
        ServerInputinator temp;
        temp.playerID = command.id;
        temp.tick = command.tick_;
        temp.inputBits = command.bits_;
        inputLibrary.push_back(temp);
    }
}

void ServerApp::on_send (network::Connection* connection, const uint16 sequence, network::NetworkStreamWriter& writer) {
    auto id = clients_.find_client ((uint64)connection);
    {
        for (int i = 0; i < players_.size (); i++) {
            network::NetworkMessageServerTick message (Time::now ().as_ticks (), tick_, id);
            if (!message.write (writer)) {
                assert (!"failed to write message!");
            }
        }
    }
    uint8 stateBits = 0;
    switch (gameState)
    {
    case gameplay::GameState::Searching: {
        stateBits = 0;
        break;
    }
    case gameplay::GameState::Setup: {
        stateBits = 1;
        break;
    }
    case gameplay::GameState::Gameplay: {
        stateBits = 2;
        break;
    }
    case gameplay::GameState::Exit: {
        stateBits = 3;
        break;
    }
    default:
        break;
    }
    for (int i = 0; i < 4; i++) {
        network::NetworkMessageGameState gameStateMessage(stateBits);
        if (!gameStateMessage.write(writer)) {
            assert(!"failed to write message!");
        }
    }

    for (int i = 0; i < 4; i++) {
        if (bullets[i].active) {
            network::NetworkMessageShoot message (bullets[i].active, tick_, i, bullets[i].position_, bullets[i].direction);
            if (!message.write (writer)) {
                assert (!"failed to write message!");
            }
        }
    }
    {
        for (uint8 i = 0; i < players_.size (); i++) {
            if (id == players_[i].playerID) {
                if (!eventQueue.empty ()) {
                    charlie::gameplay::ReliableMessage temp;
                    temp.event = eventQueue.front ();
                    temp.sequenceNumber = sequence;
                    players_[i].eventQueue.push_back (temp);
                }
                network::NetworkMessagePlayerState message (players_[i].position_, players_[i].playerID, players_[i].alive,tick_);
                if (!message.write (writer)) {
                    assert (!"failed to write message!");
                }
            }
            else {
                network::NetworkMessageEntityState message (players_[i].position_, players_[i].playerID, players_[i].alive);
                if (!message.write (writer)) {
                    assert (!"failed to write message!");
                }
            }
        }
    }
    if (gameState == gameplay::GameState::Exit) {
        network::NetoworkMessageWinner message (winnerID);
        if (!message.write (writer)) {
            assert (!"failed to write message!");
        }
    }
}