#include "distribution.hpp"

OperandDistribution OperandDistribution::operator+(
		const OperandDistribution& o) {
	OperandDistribution r;
	r.constantRate = constantRate + o.constantRate;
	r.memAccessDirectRate = memAccessDirectRate + o.memAccessDirectRate;
	r.memAccessIndirectRate = memAccessIndirectRate + o.memAccessIndirectRate;
	auto sum = r.constantRate + r.memAccessDirectRate + r.memAccessIndirectRate;
	r.constantRate = r.constantRate * DEFAULT_RATE / sum;
	r.memAccessDirectRate = r.memAccessDirectRate * DEFAULT_RATE / sum;
	r.memAccessIndirectRate = r.memAccessIndirectRate * DEFAULT_RATE / sum;
	return r;
}

Operand OperandDistribution::operator()( std::default_random_engine& engine )
{
  std::uniform_int_distribution<int> dist(0,3);
  int value = dist( engine );
  Operand::Type type;
  switch( value )
  {
  case 0:
	  type = Operand::Type::CONSTANT;
	  break;
  case 1:
	  type = Operand::Type::MEM_ACCESS_DIRECT;
	  break;
  case 2:
	  type = Operand::Type::MEM_ACCESS_INDIRECT;
	  break;
  default:
	  throw std::runtime_error( "unexpected random value" );
  }
  return Operand( type, 4 );
}

OperationDistribution OperationDistribution::operator+(
		const OperationDistribution& o) {
	OperationDistribution r;
	r.movRate = movRate + o.movRate;
	r.addRate = addRate + o.addRate;
	r.subRate = subRate + o.subRate;
	r.loopRate = loopRate + o.loopRate;
	auto sum = r.movRate + r.addRate + r.subRate + r.loopRate;
	r.movRate = r.movRate * DEFAULT_RATE / sum;
	r.addRate = r.addRate * DEFAULT_RATE / sum;
	r.subRate = r.subRate * DEFAULT_RATE / sum;
	r.loopRate = r.loopRate * DEFAULT_RATE / sum;
	return r;
}

TransitionDistribution::TransitionDistribution(Value numStates) {
	rates.data.resize(numStates);
	for (Value state = 0; state < numStates; state++) {
		rates.data[state] = DEFAULT_RATE;
	}
}

TransitionDistribution TransitionDistribution::operator+(
		const TransitionDistribution& o) {
	TransitionDistribution r;
	r.rates.data.resize(rates.data.size());
	Value sum = 0;
	for (Value i = 0; i < rates.Length(); i++) {
		r.rates.data[i] = rates.data[i] + o.rates.data[i];
		sum += r.rates.data[i];
	}
	for (Value i = 0; i < rates.Length(); i++) {
		r.rates.data[i] = r.rates.data[i] * DEFAULT_RATE / sum;
	}
	return r;
}

NormalDistribution NormalDistribution::operator+(const NormalDistribution& o) {
	NormalDistribution r;
	r.mean = (mean + o.mean) / 2.0;
	r.stddev = (stddev + o.stddev) / 2.0;
	return r;
}
