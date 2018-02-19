#include "machine.hpp"

State State::operator+(const State& o) {
	State r;
	r.operationDist = operationDist + o.operationDist;
	r.targetOpDist = targetOpDist + o.targetOpDist;
	r.sourceOpDist = sourceOpDist + o.sourceOpDist;
	r.targetValDist = targetValDist + o.targetValDist;
	r.sourceValDist = sourceValDist + o.sourceValDist;
	r.transDist = transDist + o.transDist;
	r.posDist = posDist + o.posDist;
	return r;
}

Machine::Machine(Value numStates, int64_t maxOperations) :
		maxOperations(maxOperations) {
	states.resize(numStates);
	for (Value state = 0; state < numStates; state++) {
		states[state] = State(numStates);
	}
}

Machine Machine::operator+(const Machine& o) {
	Machine r(states.size(), o.maxOperations);
	for (Value s = 0; s < states.size(); s++) {
		r.states[s] = states[s] + o.states[s];
	}
	return r;
}

