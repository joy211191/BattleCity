
#ifndef CLIENT_APP_HPP_INCLUDED
#define CLIENT_APP_HPP_INCLUDED

#include <charlie_application.hpp>
#include <charlie_network.hpp>

using namespace charlie;


class ConnectionHandler :public network::Connection {
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

   gameplay::Bullet bullets[4];

   const float speed = 100.0;
   Vector2 playerStartPositions[4];

   struct DataStruct {
   public:
	   Time lastSent;
	   Time LastRecieved;
	   Time RTT;
	   int packetsSent;
	   int packetsDelivered;
	   int packetsLost;
	   int packetsReceived;
	   int sequenceNumber;
	   float packetLoss;
	   bool detailsOverlay;
	   charlie::DynamicArray<int> sequenceStack;
	   int32 dataSize;
	   int32 inputMispredictions;
   };

   DataStruct networkData;
   uint32 offsetTick;

   class Inputinator {
   public:
	   uint8 inputBits;
	   uint32 tick;
	   Vector2 calculatedPosition;
   };

   Vector2 direction;

   Vector2 GetInputDirection(uint8 input);

   void CheckPlayerPosition(uint32 serverTick,Vector2 serverPosition);
   void FixPlayerPositions(uint32 serverTick, Vector2 serverPosition);

   charlie::DynamicArray<Inputinator> inputLibrary;
   charlie::DynamicArray<Inputinator> sentLibrary;

   uint32 clientTick;
   uint32 recievedServerTick;

   Color playerColors[4];

   uint32 lastSentTick;

   network::UDPSocket socket;
   network::IPAddress serverIP;
   bool serverFound=false;
   int8 iterator;
   bool idApplied;
   int winnerID;
   Vector2 recievedPosition;
   Vector2 bulletServerPosition;
   Vector2 entityServerPosition;
   bool ServerDiscovery();
   bool ConnectionCheck();
   void EntityInterpolator();
   void BulletInpterpolator();
};

#endif // !CLIENT_APP_HPP_INCLUDED
