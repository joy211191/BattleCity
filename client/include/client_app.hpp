// client_app.hpp

#ifndef CLIENT_APP_HPP_INCLUDED
#define CLIENT_APP_HPP_INCLUDED

#include <charlie_application.hpp>
#include <charlie_network.hpp>

using namespace charlie;

struct ClientApp final : Application, network::IConnectionListener {
   ClientApp();

   // note: Application
   virtual bool on_init();
   virtual void on_exit();
   virtual bool on_tick(const Time &dt);
   virtual void on_draw();

   // note: IConnectionListener 
   virtual void on_acknowledge(network::Connection *connection, const uint16 sequence);
   virtual void on_receive(network::Connection *connection, network::NetworkStreamReader &reader);
   virtual void on_send(network::Connection *connection, const uint16 sequence, network::NetworkStreamWriter &writer);

   Mouse &mouse_;
   Keyboard &keyboard_;
   network::Connection connection_;
   const Time tickrate_;
   Time accumulator_;

   uint8 input_bits_;
   gameplay::Player player_;
   gameplay::Entity entity_;

   class PositionHistory {
   public:
	   Vector2 position;
	   uint32 tick;
   };

   charlie::DynamicArray<PositionHistory> positionHistory;

   class InputPrediction {
   public:
	   uint8 inputBits;
	   uint32 tick;
	   Vector2 calculatedPosition;
   };
   charlie::DynamicArray<InputPrediction> inputLibrary;

   uint32 globalTick;
   uint32 recievedServerTick;

   network::IPAddress ServerDiscovery ();
};

#endif // !CLIENT_APP_HPP_INCLUDED
