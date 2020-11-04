// client_app.hpp

#ifndef CLIENT_APP_HPP_INCLUDED
#define CLIENT_APP_HPP_INCLUDED

#include <charlie_application.hpp>
#include <charlie_network.hpp>

using namespace charlie;


class ConnectionHandler:public network::Connection {
public:
	void on_rejected (const uint8 reason);


private:

};


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

   gameplay::GameState gameState;

   Mouse &mouse_;
   Keyboard &keyboard_;
   network::Connection connection_;
   const Time tickrate_;
   Time accumulator_;

   uint8 input_bits_;
   gameplay::Player player_;
   gameplay::Entity entity_[3];


   const float speed = 100.0;
   Vector2 playerStartPositions[4];



   class InputPrediction {
   public:
	   uint8 inputBits;
	   uint32 tick;
	   Vector2 calculatedPosition;
   };
   charlie::DynamicArray<InputPrediction> inputLibrary;

   uint32 globalTick;
   uint32 recievedServerTick;

   Color playerColors[4];

   network::UDPSocket socket;
   network::IPAddress serverIP;
   bool ServerDiscovery ();
   bool serverFound=false;
   int8 iterator;
   bool idApplied;

   bool CollisionCheck (gameplay::Player playerA, gameplay::Player playerB);
};

#endif // !CLIENT_APP_HPP_INCLUDED
