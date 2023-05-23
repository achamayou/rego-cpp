#include "passes.h"

namespace rego
{
  PassDef lists()
  {
    return {
      (In(Input) / In(Data)) * (T(Brace) << T(List)[List]) >>
        [](Match& _) { return ObjectItemSeq << *_[List]; },

      (In(Input) / In(Data)) * (T(Brace) << End) >>
        ([](Match&) -> Node { return ObjectItemSeq; }),

      In(Group) *
          (T(Brace)
           << (T(List)
               << (T(ObjectItem)[Head] * T(ObjectItem)++[Tail] * End))) >>
        [](Match& _) { return Object << _(Head) << _[Tail]; },

      In(Group) * (T(Square) << (T(List)[List] * End)) >>
        [](Match& _) { return Array << *_[List]; },

      In(Group) * (T(Square) << T(Group)[Group]) >>
        [](Match& _) { return Array << _(Group); },

      In(Group) * (T(Brace) << (T(Group)[Head] * T(Group)++[Tail] * End)) >>
        [](Match& _) { return RuleBody << _(Head) << _[Tail]; },

      In(Group) * (T(Brace) << (T(List)[List] * End)) >>
        [](Match& _) { return Set << *_[List]; },

      In(Group) * T(EmptySet) >> ([](Match&) -> Node { return Set; }),

      // errors

      (In(Input) / In(Data)) * T(Brace)[Brace] >>
        [](Match& _) { return err(_(Brace), "Invalid input/data body"); },

      In(Group) * T(Brace)[Brace] >>
        [](Match& _) { return err(_(Brace), "Invalid object"); },

      (In(ObjectItemSeq) / In(Object)) * T(Group)[Group] >>
        [](Match& _) { return err(_(Group), "Invalid object key/value"); },

      (In(Array) / In(Set)) * T(ObjectItem)[ObjectItem] >>
        [](Match& _) { return err(_(ObjectItem), "Invalid item"); },
    };
  }
}