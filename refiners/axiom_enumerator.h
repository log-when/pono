/*********************                                                  */
/*! \file axiom_enumerator.h
** \verbatim
** Top contributors (to current version):
**   Makai Mann
** This file is part of the pono project.
** Copyright (c) 2019 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved.  See the file LICENSE in the top-level source
** directory for licensing information.\endverbatim
**
** \brief Abstract class for enumerating axioms over a transition system
**        it does not modify the transition system, instead only returning
**        violated axioms sufficient for ruling out abstract counterexamples.
**
**/
#pragma once

#include "core/ts.h"

namespace pono {

// Non-consecutive axiom instantiations
// a struct used to represent axioms that were instantiated with
// terms from different time steps
// these axiom instantiations cannot be added directly to a transition
// system by "untiming" and using only inputs and state variables
// because it must refer to symbols from different time steps
struct NCAxiomInstantiation
{
  NCAxiomInstantiation(const smt::Term & a,
                       const std::unordered_set<TimedTerm> & insts);
  : ax(a), instantiations(insts)
  {}

  smt::Term ax;  ///< the instantiated axiom
  smt::UnorderedTermSet
      instantiations;  ///< the instantiations used in the axioms
  // Note: the instantiations are over unrolled variables, e.g. x@4 instead of x
};

class AxiomEnumerator
{
 public:
  // TODO: brainstorm the right interface
  //       would it be better for it to add axioms or not?
  AxiomEnumerator::AxiomEnumerator(const TransitionSystem & ts) : ts_(ts) {}

  virtual ~AxiomEnumerator(){};

  /** Check the axiom set over an abstract trace formula
   *  @param abs_trace_formula a formula representing the abstract trace
   *  @param bound the bound the abstract trace was unrolled to
   *  @return true iff the trace could be ruled out
   */
  virtual bool enumerate_axioms(smt::Term abs_trace_formula, size_t bound) = 0;

  /** Returns a sufficient set of violated consecutive axioms to
   *  rule out the abstract trace from the last call to
   *  enumerate_axioms
   *  @return a vector of axiom instantiations that are consecutive
   *  meaning they only involve symbols from neighboring times
   *  and can be added directly to a transition system
   *  the free variables in the axiom instantiations are all
   *  state variables or inputs
   */
  virtual smt::TermVec & get_consecutive_axioms() = 0;

  /** Returns a sufficient set of violated consecutive axioms to
   *  rule out the abstract trace from the last call to
   *  enumerate_axioms
   *  @return a vector of non-consecutive axiom instantiations
   *  These are structs that record extra information about the instantiation
   *  these axioms refer to timed symbols and cannot be added directly
   *  to a transition system.
   *  These must be handled with auxiliary variables or some other form
   *  of generalization such that the axiom becomes consecutive
   *  and can be added directly to the transition system.
   *  Examples:
   *
   *    a@4 = b@5 -> read(a@4, i@4) = read(b@5, i@4) is consecutive and can be
   *      added to the transition system as:
   *      a = b.next -> read(a, i) = read(b.next, i)
   *
   *    a@4 = b@5 -> read(a@4, i@7) = read(b@5, i@7) is non-consecutive
   *      because the mentioned times cannot be captured with just current
   *      and next.
   */
  virtual std::vector<NCAxiomInstantiation> & get_nonconsecutive_axioms() = 0;

 protected:
  const TransitionSystem & ts_;
};

}  // namespace pono