// Agent.h
//
// HW2

#ifndef AGENT_H
#define AGENT_H

#include "Action.h"
#include "Percept.h"

#include "Location.h"
#include "Orientation.h"
#include "Search.h"
#include <list>

#include "WorldState.h"

class Agent
{
public:
	struct perTracker {
		int Stench = 0;
		int Breeze = 0;
		double percent = .2;
	};
	Agent ();
	~Agent ();
	void Initialize ();
	Action Process (Percept& percept);
	void GameOver (int score);

	void UpdateState (Percept& percept);
	bool FacingDeath();
	WorldState currentState;
	Action lastAction;
	Percept lastPercept;
	perTracker ptrack[15][15];
	list<Action> actionList;
	int numActions;
	SearchEngine searchEngine;
};

#endif // AGENT_H
