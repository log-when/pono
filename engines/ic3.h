/*********************                                                  */
/*! \file ic3.h
** \verbatim
** Top contributors (to current version):
**   Makai Mann, Ahmed Irfan
** This file is part of the pono project.
** Copyright (c) 2019 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved.  See the file LICENSE in the top-level source
** directory for licensing information.\endverbatim
**
** \brief Bit-level IC3 implementation using the IC3Base abstract base class
**/

#pragma once

#include "engines/ic3base.h"

namespace pono {

class IC3 : public IC3Base
{
 public:
  IC3(Property & p, smt::SolverEnum se);
  IC3(Property & p, const smt::SmtSolver & s);
  IC3(const PonoOptions & opt, Property & p, smt::SolverEnum se);
  IC3(const PonoOptions & opt, Property & p, const smt::SmtSolver & s);
  virtual ~IC3() {}

  typedef IC3Base super;

 protected:
  // pure virtual method implementations

  IC3Formula get_ic3_formula() const override;

  IC3Formula ic3_formula_disjunction(const smt::TermVec & c) const override;

  IC3Formula ic3_formula_conjunction(const smt::TermVec & c) const override;

  IC3Formula ic3_formula_negate(const IC3Formula & u) const override;

  bool ic3_formula_check_valid(const IC3Formula & u) const override;

  std::vector<IC3Formula> inductive_generalization(
      size_t i, const IC3Formula & c) override;

  IC3Formula generalize_predecessor(size_t i, const IC3Formula & c) override;

  void check_ts() const override;

};

}  // namespace pono
