#include "ObjectManager.h"


static ObjectManager* om = nullptr;


void main() {
	om = new ObjectManager();

	om->CreateScene("main");

	om->CreateEntity("main", PositionProxy16{ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0 });
}