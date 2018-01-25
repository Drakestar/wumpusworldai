# PyAgent.py

from random import randint
import Action
import Orientation
import Search

class WorldState:
    def __init__(self):
        self.worldSize = 2 # worlds are at least 2x2
        self.agentLocation = [1,1]
        self.agentOrientation = Orientation.RIGHT
        self.wumpusLocation = [0,0]
        self.goldLocation = [0,0]
        self.pitLocations = []
        self.agentAlive = True
        self.agentHasArrow = True
        self.agentHasGold = False
        self.agentInCave = True
        self.wumpusAlive = True
    
class Agent:
    def __init__(self):
        self.currentState = WorldState()
        self.searchEngine = Search.SearchEngine()
        self.actionList = []
        self.lastAction = Action.CLIMB
        self.lastPercept = {}
        self.numActions = 0
    
    def Initialize(self):
        self.currentState.agentLocation = [1,1]
        self.currentState.agentOrientation = Orientation.RIGHT
        self.currentState.agentHasArrow = True
        self.currentState.agentHasGold = False
        self.currentState.wumpusAlive = True
        self.actionList = []
        self.searchEngine.AddSafeLocation(1,1) # (1,1) always safe
        # At start of game, wumpus is alive
        self.searchEngine.RemoveSafeLocation(self.currentState.wumpusLocation[0], self.currentState.wumpusLocation[1])
        self.lastAction = Action.CLIMB # dummy action
        self.lastPercept = {}
        self.numActions = 0
    
    # Input percept is a dictionary [perceptName: boolean]
    def Process (self, percept):
        self.lastPercept = percept
        self.UpdateState(percept)
        if (not self.actionList):
            foundPlan = False
            if ((not foundPlan) and percept['Glitter']):
                self.actionList.append(Action.GRAB)
                foundPlan = True
            if ((not foundPlan) and self.currentState.agentHasGold and (self.currentState.agentLocation == [1,1])):
                self.actionList.append(Action.CLIMB)
                foundPlan = True
            if ((not foundPlan) and (self.currentState.goldLocation != [0,0]) and (not self.currentState.agentHasGold)):
                # If know gold location, but don't have it, then find path to it
                actionList2 = self.searchEngine.FindPath(self.currentState.agentLocation, self.currentState.agentOrientation, self.currentState.goldLocation, Orientation.UP)
                if (actionList2):
                    self.actionList += actionList2
                    foundPlan = True
            if ((not foundPlan) and self.currentState.agentHasGold):
                # If have gold, then find path to (1,1)
                actionList2 = self.searchEngine.FindPath(self.currentState.agentLocation, self.currentState.agentOrientation, [1,1], Orientation.DOWN)
                if (actionList2):
                    self.actionList += actionList2
                    foundPlan = True
            if ((not foundPlan) and percept['Stench'] and self.currentState.agentHasArrow):
                self.actionList.append(Action.SHOOT)
                foundPlan = True
            if ((not foundPlan) and percept['Bump']):
                self.actionList.append(Action.TURNLEFT)
                self.actionList.append(Action.GOFORWARD)
                foundPlan = True
            if (not foundPlan):
                # Random move
                randomAction = randint(0,2) # 0=GOFORWARD, 1=TURNLEFT, 2=TURNRIGHT
                self.actionList.append(randomAction)
                foundPlan = True
        action = self.actionList.pop(0)
        # One final check that we aren't moving to our death
        if ((action == Action.GOFORWARD) and self.FacingDeath()):
            action = Action.TURNLEFT
        self.lastAction = action
        self.numActions += 1
        return action

    def UpdateState (self, percept):
        x = self.currentState.agentLocation[0]
        y = self.currentState.agentLocation[1]
        orientation = self.currentState.agentOrientation
        # Check if wumpus killed
        if (percept['Scream']):
            self.currentState.wumpusAlive = False
            # Since only kill wumpus point-blank, we know its location is in front of agent
            if (orientation == Orientation.RIGHT):
                self.currentState.wumpusLocation = [x+1,y]
            if (orientation == Orientation.UP):
                self.currentState.wumpusLocation = [x,y+1]
            if (orientation == Orientation.LEFT):
                self.currentState.wumpusLocation = [x-1,y]
            if (orientation == Orientation.DOWN):
                self.currentState.wumpusLocation = [x,y-1]
        # Check if have gold
        if (self.lastAction == Action.GRAB):
            self.currentState.agentHasGold = True
            self.currentState.goldLocation = self.currentState.agentLocation
        # Check if used arrow
        if (self.lastAction == Action.SHOOT):
            self.currentState.agentHasArrow = False
        # Update orientation
        if (self.lastAction == Action.TURNLEFT):
            self.currentState.agentOrientation = (orientation + 1) % 4
        if (self.lastAction == Action.TURNRIGHT):
            self.currentState.agentOrientation = (orientation + 3) % 4
        # Update location
        if ((self.lastAction == Action.GOFORWARD) and (not percept['Bump'])):
            if (orientation == Orientation.RIGHT):
                x += 1
            if (orientation == Orientation.UP):
                y += 1
            if (orientation == Orientation.LEFT):
                x -= 1
            if (orientation == Orientation.DOWN):
                y -= 1
            self.currentState.agentLocation = [x,y]
        # Update world size
        if (x > self.currentState.worldSize):
            self.currentState.worldSize = x
        if (y > self.currentState.worldSize):
            self.currentState.worldSize = y
        # Update safe locations in search engine
        self.searchEngine.AddSafeLocation(x,y)
        if ((not percept['Breeze']) and ((not percept['Stench']) or (not self.currentState.wumpusAlive))):
            if (x > 1):
                self.searchEngine.AddSafeLocation(x-1,y)
            if (y > 1):
                self.searchEngine.AddSafeLocation(x,y-1)
            # Add safe location to the right and up, if doesn't exceed our current estimate of world size
            if (x < self.currentState.worldSize):
                self.searchEngine.AddSafeLocation(x+1,y)
            if (y < self.currentState.worldSize):
                self.searchEngine.AddSafeLocation(x,y+1)
    
    def FacingDeath(self):
        x = self.currentState.agentLocation[0]
        y = self.currentState.agentLocation[1]
        orientation = self.currentState.agentOrientation
        if (orientation == Orientation.RIGHT):
            x += 1
        if (orientation == Orientation.UP):
            y += 1
        if (orientation == Orientation.LEFT):
            x -= 1
        if (orientation == Orientation.DOWN):
            y -= 1
        if ([x,y] in self.currentState.pitLocations):
            return True
        if ((self.currentState.wumpusLocation == [x,y]) and self.currentState.wumpusAlive):
            return True
        return False

    def GameOver(self, score):
        if ((score < 0) and (self.numActions < 1000)):
            # Agent died by GOFORWARD into location with pit or wumpus
            x = self.currentState.agentLocation[0]
            y = self.currentState.agentLocation[1]
            orientation = self.currentState.agentOrientation
            if (orientation == Orientation.RIGHT):
                x += 1
            if (orientation == Orientation.UP):
                y += 1
            if (orientation == Orientation.LEFT):
                x -= 1
            if (orientation == Orientation.DOWN):
                y -= 1
            if (self.lastPercept['Breeze'] and (not self.lastPercept['Stench'])):
                self.currentState.pitLocations.append([x,y])
            if (self.lastPercept['Stench'] and (not self.lastPercept['Breeze'])):
                self.currentState.wumpusLocation = [x,y]

# Global agent
myAgent = 0

def PyAgent_Constructor ():
    global myAgent
    myAgent = Agent()

def PyAgent_Destructor ():
    global myAgent
    # nothing to do here

def PyAgent_Initialize ():
    global myAgent
    myAgent.Initialize()

def PyAgent_Process (stench,breeze,glitter,bump,scream):
    global myAgent
    percept = {'Stench': bool(stench), 'Breeze': bool(breeze), 'Glitter': bool(glitter), 'Bump': bool(bump), 'Scream': bool(scream)}
    return myAgent.Process(percept)

def PyAgent_GameOver (score):
    global myAgent
    myAgent.GameOver(score)
