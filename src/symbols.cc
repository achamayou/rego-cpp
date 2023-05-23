#include "passes.h"

#include <sstream>

namespace rego
{
  PassDef symbols()
  {
    return {
      In(ModuleSeq) *
          (T(Module) << ((T(Package) << T(Var)[Id]) * T(Policy)[Policy])) >>
        [](Match& _) { return Module << _(Id) << _(Policy); },

      T(Rule)
          << ((
                T(RuleHead)
                << (T(Var)[Id] *
                    (T(RuleHeadComp) << (T(AssignOperator) * T(Expr)[Expr])))) *
              T(RuleBodySeq)[RuleBodySeq]) >>
        [](Match& _) { return RuleComp << _(Id) << _(Expr) << _(RuleBodySeq); },

      In(ObjectItem) * ((T(ObjectItemHead) << T(Scalar)[Scalar])) >>
        [](Match& _) {
          std::string key = to_json(_(Scalar));
          if (key.starts_with('"'))
          {
            key = key.substr(1, key.size() - 2);
          }

          return Key ^ key;
        },

      In(Object) *
          (T(ObjectItem)
           << ((T(ObjectItemHead) << T(Var)[Var]) * T(Expr)[Expr])) >>
        [](Match& _) {
          return RefObjectItem << (Ref << _(Var) << RefArgSeq) << _(Expr);
        },

      In(Object) *
          (T(ObjectItem)
           << ((T(ObjectItemHead) << T(Ref)[Ref]) * T(Expr)[Expr])) >>
        [](Match& _) { return RefObjectItem << _(Ref) << _(Expr); },

      In(Expr) * (T(Term) << (T(Ref) / T(Var))[Value]) >>
        [](Match& _) { return RefTerm << _(Value); },

      In(Expr) *
          (T(Term) << (T(Scalar) << (T(JSONInt) / T(JSONFloat))[Value])) >>
        [](Match& _) { return NumTerm << _(Value); },

      In(RefArgBrack) * T(Var)[Var] >>
        [](Match& _) { return RefTerm << _(Var); },

      // errors

      In(ObjectItem) * T(ObjectItemHead)[ObjectItemHead] >>
        [](Match& _) { return err(_(ObjectItemHead), "Invalid object key"); },
    };
  }

}