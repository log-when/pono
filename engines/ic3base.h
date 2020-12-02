/*********************                                                  */
/*! \file ic3base.h
** \verbatim
** Top contributors (to current version):
**   Makai Mann, Ahmed Irfan
** This file is part of the pono project.
** Copyright (c) 2019 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved.  See the file LICENSE in the top-level source
** directory for licensing information.\endverbatim
**
** \brief Abstract base class implementation of IC3 parameterized by
**        the unit used in frames, pre-image computation, and inductive
**        and predecessor generalization techniques.
**
**        To create a particular IC3 instantiation, you must implement the
*following:
**           - an IC3UnitHandler implementation, e.g. a ClauseHandler
**           - implement get_unit and give it semantics to produce the
**             corresponding IC3Unit with an IC3UnitHandler
**           - implement inductive_generalization
**           - implement generalize_predecessor
**           - implement check_ts which just checks if there are any
**             theories / syntax used in the transition system which
**             is not supported by this instantiation
**
**/
#pragma once

#include "engines/prover.h"
#include "smt-switch/utils.h"

namespace pono {

struct IC3Unit
{
  // nullary constructor
  IC3Unit() {}
  IC3Unit(const smt::Term & t, const smt::TermVec & c, bool n)
      : term(t), children(c), negated(n)
  {
  }

  IC3Unit(const IC3Unit & other)
      : term(other.term), children(other.children), negated(other.negated)
  {
  }

  virtual ~IC3Unit() {}

  /** Returns true iff this IC3Unit has not been initialized */
  bool is_null() const { return (term == nullptr); };

  smt::Term term;
  smt::TermVec children;
  bool negated;
};

// abstract base class for handling different IC3Units
// e.g. Clause/Cube, Predicate Clause/Cube, etc...

class IC3UnitHandler
{
 public:
  IC3UnitHandler(const smt::SmtSolver & s) : solver_(s) {}

  virtual ~IC3UnitHandler() {}

  /** Creates an IC3Unit from a vector of terms
   *  @param c the children terms
   *  @ensures resulting IC3Unit children == c
   *  @ensures resulting IC3Unit not negated
   */
  virtual IC3Unit create(const smt::TermVec & c) const = 0;

  /** Creates a negated IC3Unit from a vector of terms
   *  @param c the children terms
   *  @ensures resulting IC3Unit children == c
   *  @ensures resulting IC3Unit negated
   *  e.g. for a ClauseHandler, this method will create a cube
   *  note: assumes the children are already in the right polarity
   *  (doesn't negate them)
   */
  virtual IC3Unit create_negated(const smt::TermVec & c) const = 0;

  /** Negates an IC3Unit
   *  @param u the IC3Unit to negate
   */
  virtual IC3Unit negate(const IC3Unit & u) const = 0;

  /** Check whether a given IC3Unit is valid
   *  e.g. if this is a ClauseHandler it would
   *    check that it's a disjunction of literals
   *  (for debugging)
   *  @param u the IC3Unit to check
   *  @return true iff this is a valid IC3Unit for this
   *          kind of handler
   */
  virtual bool check_valid(const IC3Unit & u) const = 0;

 protected:
  const smt::SmtSolver & solver_;

  /** Negates a term by stripping the leading Not if it's there,
   ** or applying Not if the term is not already negated.
   */
  smt::Term smart_not(const smt::Term & t) const;
};

// TODO change back to ProofGoal once refactor is done
// don't want to clash with name in MBIC3 for now
struct IC3Goal
{
  // based on open-source ic3ia ProofObligation
  IC3Unit target;
  size_t idx;
  // TODO: see if we can make this a unique_ptr
  //       made it complicated to move from this struct to another place
  std::shared_ptr<IC3Goal> next;

  IC3Goal(IC3Unit u, size_t i, std::shared_ptr<IC3Goal> n)
      : target(u), idx(i), next(n)
  {
  }

  IC3Goal(const IC3Goal & other)
      : target(other.target), idx(other.idx), next(other.next)
  {
  }
};

class IC3Base : public Prover
{
 public:
  /** IC3Base constructors take the normal arguments for a prover
   *  + a function that can create an IC3Unit
   *  Depending on the derived class IC3 implementation, the exact
   *  type of IC3Unit will differ: e.g. Clause, Disjunction
   */
  IC3Base(Property & p,
          smt::SolverEnum se,
          std::unique_ptr<IC3UnitHandler> && h);
  IC3Base(Property & p,
          const smt::SmtSolver & s,
          std::unique_ptr<IC3UnitHandler> && h);
  IC3Base(const PonoOptions & opt,
          Property & p,
          smt::SolverEnum se,
          std::unique_ptr<IC3UnitHandler> && h);
  IC3Base(const PonoOptions & opt,
          Property & p,
          const smt::SmtSolver & s,
          std::unique_ptr<IC3UnitHandler> && h);

  typedef Prover super;

  void initialize() override;

  ProverResult check_until(int k) override;

  bool witness(std::vector<smt::UnorderedTermMap> & out) override;

 protected:
  std::unique_ptr<IC3UnitHandler> handler_;

  smt::UnsatCoreReducer reducer_;

  ///< keeps track of the current context-level of the solver
  // NOTE: if solver is passed in, it could be off
  //       currently no way to check
  //       then solver_context_ is relative to the starting context
  size_t solver_context_;

  ///< the frames data structure.
  ///< a vector of the given Unit template
  ///< which changes depending on the implementation
  std::vector<std::vector<IC3Unit>> frames_;

  ///< stack of outstanding proof goals
  std::vector<IC3Goal> proof_goals_;

  // labels for activating assertions
  smt::Term init_label_;       ///< label to activate init
  smt::Term trans_label_;      ///< label to activate trans
  smt::TermVec frame_labels_;  ///< labels to activate frames
  smt::UnorderedTermMap labels_;  //< labels for unsat cores

  // useful terms
  smt::Term solver_true_;

  // TODO Make sure all comments are updated!

  // ********************************** Main Methods
  // ******************************

  // ********************************** Virtual Methods
  // These methods should be implemented by a derived class for a particular
  // "flavor" of IC3 in accordance with the associated IC3Unit

  /** Attempt to generalize before adding to a frame
   *  The standard approach is inductive generalization
   *  @requires !get_predecessor(i, c, _)
   *  @param i the frame number to generalize it against
   *  @param c the cube to find a general predecessor for
   *  @return a vector of IC3Units. Standard IC3 implementations will have a
   *          a size one vector (e.g. a single clause)
   *  @ensures P -> !c /\ F[i-1] /\ P /\ T /\ !P' is unsat
   */
  virtual std::vector<IC3Unit> inductive_generalization(size_t i,
                                                        const IC3Unit & c) = 0;

  /** Generalize a counterexample
   *  @requires get_predecessor(i, c)
   *  @param i the frame number
   *  @param c the cube to generalize
   *  @return a new cube d
   *  @ensures d -> F[i-1] /\ forall s \in [d] exists s' \in [c]. (d,c) \in [T]
   */
  virtual IC3Unit generalize_predecessor(size_t i, const IC3Unit & c) = 0;

  /** Checks if every thing in the current transition system is supported
   *  by the current instantiation
   *  throws a PonoException with a relevant message if not.
   */
  virtual void check_ts() const = 0;

  /** Get an IC3Unit from the current model
   *  @requires last call to check_sat of solver_ was satisfiable and context
   * hasn't changed
   *  @return an IC3Unit over current state variables
   */
  virtual IC3Unit get_unit() const = 0;

  // ********************************** Common Methods
  // These methods are common to all flavors of IC3 currently implemented

  /** Check if a transition from the second to last frame can result in a bad
   * state
   *  @return true iff the last frame intersects with bad
   *  post-condition: if true is returned, bad cube added to proof goals
   */
  bool intersects_bad();

  /** Perform a IC3 step
   *  @param i
   */
  ProverResult step(int i);

  /** Perform the base IC3 step (zero case)
   */
  ProverResult step_0();

  /** Get the predecessor of a cube c in frame i
   *  aka see if c is reachable from frame i-1
   *  @requires c -> F[i]
   *  @param i the frame number
   *  @param t the term to check
   *  @param out_pred the cube to populate with a predecessor, or if there's no predecessor
   *         then it can be an unsat core reduced version of c
   *  @return true iff c is reachable from the frame i
   *  @ensures returns true  : pred -> F[i-1] /\ (pred, c) \in [T]
   *           returns false : pred unchanged, F[i-1] /\ T /\ c' is unsat
   */
  bool get_predecessor(size_t i, const IC3Unit & c, IC3Unit & out_pred);

  // Helper methods

  /** Attempt to block all proof goals
   *  to ensure termination, always choose proof goal with
   *  smallest time
   *  @return true iff all proof goals were blocked
   */
  bool block_all();

  /** Attempt to block cube c at frame i
   *  @param i the frame number
   *  @param c the cube to try blocking
   *  @return true iff the cube was blocked, otherwise a new proof goal was
   * added to the proof goals
   */
  bool block(const IC3Goal & pg);

  /** Try propagating all clauses from frame index i to the next frame.
   *  @param i the frame index to propagate
   *  @return true iff all the clauses are propagated (this means property was
   * proven)
   */
  bool propagate(size_t i);

  /** Add a new frame */
  void push_frame();

  /** Adds a constraint to frame i and (implicitly) all frames below it
   *  @param i highest frame to add constraint to
   *  @param constraint the constraint to add
   */
  void constrain_frame(size_t i, const IC3Unit & constraint);

  /** Add all the terms at Frame i
   *  Note: the frames_ data structure keeps terms only in the
   *  highest frame where they are known to hold
   *  Thus, asserting Fi actually needs to add terms
   *  from Fi and all frames after it
   *  @param i the frame number
   */
  void assert_frame_labels(size_t i) const;

  smt::Term get_frame(size_t i) const;

  void assert_trans_label() const;

  /** Check if there are more proof goals
   *  @return true iff there are more proof goals
   */
  bool has_proof_goals() const;

  /** Gets a new proof goal (and removes it from proof_goals_)
   *  @requires has_proof_goals()
   *  @return a proof goal with the lowest available frame number
   *  @alters proof_goals_
   *  @ensures returned proof goal is from lowest frame in proof goals
   */
  IC3Goal get_next_proof_goal();

  /** Create and add a proof goal for cube c for frame i
   *  @param c the cube of the proof goal
   *  @param i the frame number for the proof goal
   *  @param n pointer to the proof goal that led to this one -- null for bad
   *  (i.e. end of trace)
   */
  void add_proof_goal(const IC3Unit & c, size_t i, std::shared_ptr<IC3Goal> n);

  /** Check if there are common assignments
   *  between A and B
   *  i.e. if A /\ B is SAT
   *  @param A the first term
   *  @param B the second term
   *  @return true iff there is an intersection
   */
  bool intersects(const smt::Term & A, const smt::Term & B);

  /** Check if the term intersects with the initial states
   *  syntactic sugar for intersects(ts_.init(), t);
   *  @param t the term to check
   *  @return true iff t intersects with the initial states
   */
  bool intersects_initial(const smt::Term & t);

  void fix_if_intersects_initial(smt::TermVec & to_keep,
                                 const smt::TermVec & rem);

  /** Returns the highest frame this unit can be pushed to
   *  @param i the starting frame index
   *  @param u the unit to check how far it can be pushed
   *  @return index >= i such that this unit can be added
   *          to that frame
   */
  size_t find_highest_frame(size_t i, const IC3Unit & u);

  /** Creates a reduce and of the vector of boolean terms
   *  It also sorts the vector by the hash
   *  Note: this will fail for empty vectors
   *  @param vec the vector of boolean terms
   *  @param slv (optional) the solver to use, defaults to solver_
   *  @return the conjunction of all the terms
   */
  smt::Term make_and(smt::TermVec vec, smt::SmtSolver slv = NULL) const;

  /** Pushes a solver context and keeps track of the context level
   *  updates solver_context_
   */
  void push_solver_context();

  /** Pops a solver context and keeps track of the context level
   *  updates solver_context_
   */
  void pop_solver_context();

  /** Create a boolean label for a given term
   *  These are cached in labels_
   *  good for using unsat cores
   *
   *  @param t a boolean formula to create a label for
   *  @return the indicator variable label for this term
   */
  smt::Term label(const smt::Term & t);
};

}  // namespace pono
