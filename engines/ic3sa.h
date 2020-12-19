/*********************                                                  */
/*! \file ic3sa.h
** \verbatim
** Top contributors (to current version):
**   Makai Mann
** This file is part of the pono project.
** Copyright (c) 2019 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved.  See the file LICENSE in the top-level source
** directory for licensing information.\endverbatim
**
** \brief IC3 with Syntax-Guided Abstraction based on
**
**        Model Checking of Verilog RTL Using IC3 with Syntax-Guided
*Abstraction.
**            -- Aman Goel, Karem Sakallah
**
**
**  within Pono, we are building on the bit-level IC3 instead of directly
**  on IC3Base, because a lot of the functionality is the same
**  In particular, we don't need to override inductive generalization
**
**/

#pragma once

#include "engines/ic3.h"

namespace pono {

using EquivalenceClasses = std::unordered_map<smt::Sort, smt::DisjointSet>;

class IC3SA : public IC3
{
 public:
  IC3SA(Property & p, smt::SolverEnum se);
  IC3SA(Property & p, const smt::SmtSolver & s);
  IC3SA(const PonoOptions & opt, Property & p, smt::SolverEnum se);
  IC3SA(const PonoOptions & opt, Property & p, const smt::SmtSolver & s);
  virtual ~IC3SA() {}

  typedef IC3 super;

 protected:
  std::unordered_map<smt::Sort, smt::UnorderedTermSet> term_abstraction_;
  ///< stores all the current terms in the abstraction organized by sort

  // virtual method implementations

  IC3Formula get_model_ic3formula(
      smt::TermVec * out_inputs = nullptr,
      smt::TermVec * out_nexts = nullptr) const override;

  bool ic3formula_check_valid(const IC3Formula & u) const override;

  IC3Formula generalize_predecessor(size_t i, const IC3Formula & c) override;

  void check_ts() const override;

  void initialize() override;

  RefineResult refine() override;

  // IC3SA specific methods

  /** Get equivalence classes over all current terms in term_abstraction_
   *  from the current model
   *  @requires solver_ state is sat
   *  @return EquivalenceClass partition of the current term abstraction
   */
  EquivalenceClasses get_equivalence_classes_from_model() const;
};

}  // namespace pono
