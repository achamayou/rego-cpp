#include "passes.h"
#include "rules_engine.h"

namespace rego
{
  PassDef rules()
  {
    RulesEngine rules_engine = std::make_shared<RulesEngineDef>();

    PassDef rules = {
      dir::topdown | dir::once,
      {
        In(DataModuleSeq) * T(DataModule) >>
          ([](Match&) -> Node { return {}; }),
        In(Input) * T(ObjectItemSeq) >>
          ([](Match&) -> Node { return ObjectItemSeq; }),
        In(Data) * T(ObjectItemSeq) >>
          ([](Match&) -> Node { return ObjectItemSeq; }),
      }};

    rules.pre(Rego, [rules_engine](Node node) {
      Node query = node->front();
      rules_engine->resolve(query);
      return 0;
    });

    return rules;
  }
}