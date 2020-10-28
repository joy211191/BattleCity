// server_app.cc

#include "server_app.hpp"
#include <charlie_messages.hpp>
#include <cstdio>
#include <cmath>

ServerApp::ServerApp()
   : tickrate_(1.0 / 60.0)
   , tick_(0)
   , player_input_bits_(0)
{
}

bool ServerApp::on_init()
{
   network_.set_send_rate(Time(1.0 / 10.0));
   network_.set_allow_connections(true);
   if (!network_.initialize(network::IPAddress(network::IPAddress::ANY_HOST, 54345))) {
      return false;
   }

   network_.add_service_listener(this);

   entity_.position_ = { 300.0f, 180.0f };
   player_.position_.x_ = 20.0f + random_() % 200;
   player_.position_.y_ = 200.0f + random_() % 100;

   return true;
}

void ServerApp::on_exit()
{
}

bool ServerApp::on_tick(const Time &dt)
{
   accumulator_ += dt;
   while (accumulator_ >= tickrate_) {
      accumulator_ -= tickrate_;
      tick_++;

      // note: update player
      const bool player_move_up    = player_input_bits_ & (1 << int32(gameplay::Action::Up));
      const bool player_move_down  = player_input_bits_ & (1 << int32(gameplay::Action::Down));
      const bool player_move_left  = player_input_bits_ & (1 << int32(gameplay::Action::Left));
      const bool player_move_right = player_input_bits_ & (1 << int32(gameplay::Action::Right));

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
      if (direction.length() > 0.0f) {
         direction.normalize();
         player_.position_ += direction * speed * tickrate_.as_seconds();
      }

      entity_.position_.x_ = 300.0f + std::cosf(Time::now().as_seconds()) * 150.0f;
      entity_.position_.y_ = 180.0f + std::sinf(Time::now().as_seconds()) * 100.0f;
   }

   return true;
}

void ServerApp::on_draw()
{
   renderer_.clear({ 0.4f, 0.3f, 0.2f, 1.0f });
   renderer_.render_text({ 2, 2 }, Color::White, 2, "SERVER");
   renderer_.render_rectangle_fill({ static_cast<int32>(send_position_.x_), static_cast<int32>(send_position_.y_),  20, 20 }, Color::Yellow);
   renderer_.render_rectangle_fill({ static_cast<int32>(entity_.position_.x_), static_cast<int32>(entity_.position_.y_),  20, 20 }, Color::Red);
   renderer_.render_rectangle_fill({ static_cast<int32>(player_.position_.x_), static_cast<int32>(player_.position_.y_),  20, 20 }, Color::Magenta);
}

void ServerApp::on_timeout(network::Connection *connection)
{
   connection->set_listener(nullptr);
   auto id = clients_.find_client((uint64)connection);
   // ...
   clients_.remove_client((uint64)connection);
}

void ServerApp::on_connect(network::Connection *connection)
{
   connection->set_listener(this);

   auto id = clients_.add_client((uint64)connection);
   // event : "player_connected"
   //Sending player the playerID back
}

void ServerApp::on_disconnect(network::Connection *connection)
{
   connection->set_listener(nullptr);
   auto id = clients_.find_client((uint64)connection);
   // ...
   clients_.remove_client((uint64)connection); 
}

void ServerApp::on_acknowledge(network::Connection *connection, const uint16 sequence)
{

}

void ServerApp::on_receive(network::Connection *connection, 
                           network::NetworkStreamReader &reader)
{
   auto id = clients_.find_client((uint64)connection);

   while (reader.position() < reader.length()) {
      if (reader.peek() != network::NETWORK_MESSAGE_INPUT_COMMAND) {
         break;
      }

      network::NetworkMessageInputCommand command;
      if (!command.read(reader)) {
         assert(!"could not read command!");
      }

      player_input_bits_ = command.bits_;
   }
}

void ServerApp::on_send(network::Connection *connection, 
                        const uint16 sequence, 
                        network::NetworkStreamWriter &writer)
{
   {
      network::NetworkMessageServerTick message(Time::now().as_ticks(), tick_);
      if (!message.write(writer)) {
         assert(!"failed to write message!");
      }
   }

   {
      network::NetworkMessagePlayerState message(player_.position_);
      if (!message.write(writer)) {
         assert(!"failed to write message!");
      }
   }

   auto id = clients_.find_client((uint64)connection);
   //players_[id]. ..;

   {
      send_position_ = entity_.position_;
      network::NetworkMessageEntityState message(entity_.position_);
      if (!message.write(writer)) {
         assert(!"failed to write message!");
      }
   }
}
