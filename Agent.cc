// Agent.cc
//
// HW2

#include <iostream>
#include <list>
#include <vector>
#include <cstdlib>
#include "Agent.h"

using namespace std;

Agent::Agent ()
{
	currentState.worldSize = 2; // at least 2x2
	currentState.wumpusLocation = Location(0,0); // unknown
	currentState.goldLocation = Location(0,0); // unknown
}

Agent::~Agent ()
{

}

void Agent::Initialize ()
{
	
	currentState.agentLocation = Location(1,1);
	currentState.agentOrientation = RIGHT;
	currentState.agentHasArrow = true;
	currentState.agentHasGold = false;
	currentState.wumpusAlive = true;
	actionList.clear();
	searchEngine.AddSafeLocation(1,1); // (1,1) always safe
	ptrack[1][1] = 0;
	// At start of game, wumpus is alive
	searchEngine.RemoveSafeLocation(currentState.wumpusLocation.X, currentState.wumpusLocation.Y);
	lastAction = CLIMB; // dummy action
	numActions = 0;
}

Action Agent::Process (Percept& percept)
{
	Action action;
	list<Action> actionList2;
	bool foundPlan = false;
	int px, py;
	int p, p1=0, p2=0, p3 = 0, p4 = 0, pmax = 0;
	lastPercept = percept;
	UpdateState(percept);
	// Update knowledge base
	// PERCEPT TRACKER
	px = currentState.agentLocation.X;
	py = currentState.agentLocation.Y;
	if (percept.Stench) {
		ptrack[px][py].Stench = 2;
	} else ptrack[px][py].Stench = 1;
	if (percept.Breeze) {
		ptrack[px][py].Breeze = 2;
		// Probability changing
		// variable for number of possible pits
		// Check for the percent at location as well as within worldsize
		p = 0;
		if ((ptrack[px+1][py].percent != 0) && (px+1 < 5)) {p++; p1;}
		if ((ptrack[px-1][py].percent != 0) && (px-1 < 5)) {p++; p2;}
		if ((ptrack[px][py+1].percent != 0) && (py+1 < 5)) {p++; p3;}
		if ((ptrack[px][py-1].percent != 0) && (py-1 < 5)) {p++; p4;}
		// If there's a possible pit add percent chance of containing
		// a pit and keep track of max
		// Also because I suck at probability in c if it's greater than
		// 100% set it at 100
		if (p1) {
			 ptrack[px+1][py].percent += .3;
			 if (ptrack[px+1][py].percent > 1) ptrack[px+1][py].percent = 1;
			 pmax = ptrack[px+1][py].percent;
		}
		if (p2) {
			ptrack[px-1][py].percent += .3;
			if (ptrack[px-1][py].percent > 1) ptrack[px-1][py].percent = 1;
			if (ptrack[px-1][py].percent > pmax) pmax = ptrack[px-1][py].percent;
		}
		if (p3) {
			ptrack[px][py+1].percent += .3;
			if (ptrack[px][py+1].percent > 1) ptrack[px][py+1].percent = 1;
			if (ptrack[px][py+1].percent > pmax) pmax = ptrack[px][py+1].percent;
		}
		if (p4) {
			ptrack[px][py-1].percent += .3;
			if (ptrack[px][py-1].percent > 1) ptrack[px][py-1].percent = 1;
			if (ptrack[px][py-1].percent > pmax) pmax = ptrack[px][py-1].percent;
		}
		// If there's a possible pit and it's lower than the max
		// The max would get filtered out
		if ((p1) && (ptrack[px+1][py].percent < pmax)) ptrack[px+1][py].percent = .3;
		if ((p2) && (ptrack[px-1][py].percent < pmax)) ptrack[px-1][py].percent = .3;
		if ((p3) && (ptrack[px][py+1].percent < pmax)) ptrack[px][py+1].percent = .3;
		if ((p4) && (ptrack[px][py-1].percent < pmax)) ptrack[px][py-1].percent = .3;
		
		
		
	} else {
		ptrack[px][py].Breeze = 1;
		ptrack[px][py].percent = 0;
		ptrack[px+1][py].percent = 0;
		ptrack[px-1][py].percent = 0;
		ptrack[px][py+1].percent = 0;
		ptrack[px][py-1].percent = 0;
	}
	
	
	// Do action
	if (actionList.empty()) {
		foundPlan = false;
		if ((! foundPlan) && percept.Glitter) {
			actionList.push_back(GRAB);
			foundPlan = true;
		}
		if ((! foundPlan) && currentState.agentHasGold && (currentState.agentLocation == Location(1,1))) {
			actionList.push_back(CLIMB);
			foundPlan = true;
		}
		if ((! foundPlan) && (! (currentState.goldLocation == Location(0,0))) && (! currentState.agentHasGold)) {
			// If know gold location, but don't have it, then find path to it
			actionList2 = searchEngine.FindPath(currentState.agentLocation, currentState.agentOrientation, currentState.goldLocation, UP);
			if (actionList2.size() > 0) {
				actionList.splice(actionList.end(), actionList2);
				foundPlan = true;
			}
		}
		if ((! foundPlan) && currentState.agentHasGold) {
			// If have gold, then find path to (1,1)
			actionList2 = searchEngine.FindPath(currentState.agentLocation, currentState.agentOrientation, Location(1,1), DOWN);
			if (actionList2.size() > 0) {
				actionList.splice(actionList.end(), actionList2);
				foundPlan = true;
			}
		}
		if ((! foundPlan) && percept.Stench && currentState.agentHasArrow) {
			actionList.push_back(SHOOT);
			foundPlan = true;
		}
		if ((! foundPlan) && percept.Bump) {
			actionList.push_back(TURNLEFT);
			actionList.push_back(GOFORWARD);
			foundPlan = true;
		}
		if (! foundPlan) {
			// Random move
			action = (Action) (rand() % 3);
			actionList.push_back(action);
			foundPlan = true;
		}
	}
	action = actionList.front();
	actionList.pop_front();
	// One final check that we aren't moving to our death
    if ((action == GOFORWARD) && FacingDeath()) {
    	action = TURNLEFT;
    }
	lastAction = action;
	numActions++;
	return action;
}

void Agent::UpdateState (Percept& percept)
{
	// Check if wumpus killed
	if (percept.Scream)
	{
		currentState.wumpusAlive = false;
		// Since only kill wumpus point-blank, we know its location is in front of agent
		currentState.wumpusLocation = currentState.agentLocation;
		switch (currentState.agentOrientation)
		{
			case RIGHT: currentState.wumpusLocation.X++; break;
			case UP: currentState.wumpusLocation.Y++; break;
			case LEFT: currentState.wumpusLocation.X--; break;
			case DOWN: currentState.wumpusLocation.Y--; break;
		}
	}
	// Check if have gold
	if (lastAction == GRAB)
	{
		currentState.agentHasGold = true;
		currentState.goldLocation = currentState.agentLocation;
	}
	// Check if used arrow
	if (lastAction == SHOOT)
	{
		currentState.agentHasArrow = false;
	}
	// Update orientation
	if (lastAction == TURNLEFT)
	{
		currentState.agentOrientation = (Orientation) ((currentState.agentOrientation + 1) % 4);
	}
	if (lastAction == TURNRIGHT)
	{
		currentState.agentOrientation = (Orientation) ((currentState.agentOrientation + 3) % 4);
	}
	// Update location
	if ((lastAction == GOFORWARD) && (! percept.Bump))
	{
		switch (currentState.agentOrientation)
		{
			case RIGHT: currentState.agentLocation.X++; break;
			case UP: currentState.agentLocation.Y++; break;
			case LEFT: currentState.agentLocation.X--; break;
			case DOWN: currentState.agentLocation.Y--; break;
		}
	}
	// Update world size
	if (currentState.agentLocation.X > currentState.worldSize)
	{
		currentState.worldSize = currentState.agentLocation.X;
	}
	if (currentState.agentLocation.Y > currentState.worldSize)
	{
		currentState.worldSize = currentState.agentLocation.Y;
	}
	// Update safe locations in search engine
	int x = currentState.agentLocation.X;
	int y = currentState.agentLocation.Y;
	searchEngine.AddSafeLocation(x,y);
	if ((! percept.Breeze) && ((! percept.Stench) || (! currentState.wumpusAlive)))
	{
		if (x > 1) searchEngine.AddSafeLocation(x-1,y);
		if (y > 1) searchEngine.AddSafeLocation(x,y-1);
		// Add safe location to the right and up, if doesn't exceed our current estimate of world size
		if (x < currentState.worldSize) searchEngine.AddSafeLocation(x+1,y);
		if (y < currentState.worldSize) searchEngine.AddSafeLocation(x,y+1);
	}
}

bool Agent::FacingDeath()
{
	int x = currentState.agentLocation.X;
	int y = currentState.agentLocation.Y;
	Orientation orientation = currentState.agentOrientation;
	if (orientation == RIGHT) {
		x++;
	}
	if (orientation == UP) {
		y++;
	}
	if (orientation == LEFT) {
		x--;
	}
	if (orientation == DOWN) {
		y--;
	}
	vector<Location>::iterator itr;
	Location facingLoc = Location(x,y);
	// Probability check, if the potential location has 50%or higher don't go
	if (ptrack[facingLoc.x][facingLoc.y] >= .5) return true;
	
	for (itr = currentState.pitLocations.begin(); itr != currentState.pitLocations.end(); itr++) {
		if (*itr == facingLoc) {
			return true;
		}
	}
	if ((currentState.wumpusLocation == facingLoc) && currentState.wumpusAlive) {
		return true;
	}
    return false;
}

void Agent::GameOver (int score)
{
	if ((score < 0) and (numActions < 1000)) {
		// Agent died by GOFORWARD into location with wumpus or pit, so make that location unsafe
		int x = currentState.agentLocation.X;
		int y = currentState.agentLocation.Y;
	    Orientation orientation = currentState.agentOrientation;
	    if (orientation == RIGHT) {
	    	x++;
	    }
	    if (orientation == UP) {
	    	y++;
	    }
	    if (orientation == LEFT) {
	    	x--;
	    }
	    if (orientation == DOWN) {
	    	y--;
	    }
	    if (lastPercept.Breeze && (! lastPercept.Stench)) {
	    	currentState.pitLocations.push_back(Location(x,y));
	    	ptrack[x][y].percent = 1;
	    }
	    if (lastPercept.Stench && (! lastPercept.Breeze)) {
	    	currentState.wumpusLocation = Location(x,y);
	    }
	}
}



