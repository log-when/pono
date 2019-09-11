#include "kinduction.h"

using namespace smt;

namespace cosa
{

  KInduction::KInduction(const Property &p, smt::SmtSolver &solver):
    ts_(p.transition_system()),
    property_(p),
    solver_(solver),
    unroller_(ts_, solver_)
  {
    initialize();
  }

  KInduction::~KInduction()
  {
  }

  KInduction::initialize()
  {
    reached_k_ = -1;
    solver_->reset_assertions();
    simple_path_ = solver_->make_value(true);
  }

  ProverResult KInduction::check_until(int k)
  {
    for (int i = 0; i <= k; ++i) {
      if (!base_step(i)) {
	return ProverResult::FALSE;
      }
      if (inductive_step(i)) {
	return ProverResult::TRUE;
      }
    }
    return ProverResult::UNKNOWN;
  }

  bool KInduction::base_step(int i)
  {
    if (i <= reached_k_) {
      return true;
    }

    Term bad = solver_->make_term(PrimOp::Not, property_.prop());
    
    solver_->push();
    solver_->assert_formula(unroller_.at_time(ts_.init(), 0));
    solver_->assert_formula(unroller_.at_time(bad, i));
    Result r = solver_->check_sat();
    if (r.is_sat()) {
      return false;
    }
    solver_->pop();

    solver_->assert_formula(unroller_.at_time(ts_.trans(), i));
    solver_->assert_formula(unroller_.at_time(property_.prop(), i));
    
    return true;
  }

  bool KInduction::inductive_step(int i)
  {
    if (i <= reached_k_) {
      return false;
    }

    Term bad = solver_->make_term(PrimOp::Not, property_.prop());
    for (int j = 0; j < i; ++j) {
      add_simple_path_constraint(i, j);
    }

    solver_->push();
    solver_->assert_formula(simple_path_); //TODO: model-based simple-path
    solver_->assert_formula(unroller_.at_time(bad, i+1));
    Result r = solver_->check_sat();
    if (r.is_unsat()) {
      return true;
    }
    solver_->pop();

    ++reached_k_;

    return false;
  }

  void KInduction::add_simple_path_constraint(int i, int j)
  {
    Term disj = solver_->make_value(false);
    for (auto v : ts_.states()) {
      Term vi = unroller_.at_time(v, i);
      Term vj = unroller_.at_time(v, j);
      Term eq = solver_->make_term(PrimOp::Equal, vi, vj);
      Term neq = solver_->make_term(PrimOp::Not, eq);
      disj = solver_->make_term(PrimOp::Or, disj, neq);
    }
    simple_path_ = solver_->make_term(PrimOp::And, simple_path_, disj);
  }
  
} // namespace cosa
