#include "passes.h"
#include "trieste/pass.h"

namespace
{
  using namespace rego;

  using Locs = std::set<Location>;
  using LocMap = std::map<Location, Location>;

  bool is_captured(Node unifybody, Node var)
  {
    Nodes defs = var->lookup();
    if (defs.size() != 1)
    {
      return false;
    }

    Node local = defs[0];

    if (local->type() != Local)
    {
      return false;
    }

    return local->parent() != unifybody.get();
  }

  void add_captures(Node body, Node node, Locs& locs)
  {
    if (node->type() == RefArgDot || node->type() == NestedBody)
    {
      return;
    }

    if (node->type() == Var)
    {
      if (is_captured(body, node))
      {
        locs.insert(node->location());
      }
    }
    else
    {
      for (Node child : *node)
      {
        add_captures(body, child, locs);
      }
    }
  }

  Locs find_invars(Node unifybody)
  {
    // input vars will be rvalues (i.e. in the Val node)
    Locs invars;
    for (auto unifyexpr : *unifybody)
    {
      if (unifyexpr->type() == UnifyExpr)
      {
        add_captures(unifybody, unifyexpr / Val, invars);
      }
    }

    return invars;
  }

  Locs find_outvars(Node unifybody)
  {
    // output vars will consist of lvalues
    Locs outvars;
    for (auto unifyexpr : *unifybody)
    {
      if (unifyexpr->type() == UnifyExpr)
      {
        Node var = unifyexpr / Var;
        if (is_captured(unifybody, unifyexpr / Var))
        {
          outvars.insert(var->location());
        }
      }
    }

    return outvars;
  }

  void replace(Node node, const LocMap& lookup)
  {
    if (node->type() == RefArgDot)
    {
      return;
    }

    if (node->type() == Var && lookup.contains(node->location()))
    {
      node->parent()->replace(node, Var ^ lookup.at(node->location()));
    }
    else
    {
      for (Node child : *node)
      {
        replace(child, lookup);
      }
    }
  }
}

namespace rego
{
  PassDef lift_to_rule()
  {
    return {
      dir::bottomup,
      {
        In(UnifyBody) *
            (T(UnifyExprEnum)([](auto& n) { return is_in(*n.first, {Module}); })
             << (T(Var)[Var] * T(Var)[Item] * T(Var)[ItemSeq] *
                 T(UnifyBody)[UnifyBody])) >>
          [](Match& _) {
            Node rulebody = _(UnifyBody);
            // in vars
            Locs invars = find_invars(rulebody);
            // out vars
            Locs outvars = find_outvars(rulebody);

            LocMap out_map;
            for (auto& loc : outvars)
            {
              // each out variable gets a new name. This makes
              // in/out variables easier to manage.
              Location out_loc = _.fresh({"out"});
              out_map[loc] = out_loc;
            }

            // replace all references to the return values
            // with their new locations.
            replace(rulebody, out_map);

            // create the out variables
            for (auto& [var, out_var] : out_map)
            {
              if (invars.contains(var))
              {
                // we don't want return values to be passed in as arguments.
                // i.e. we implicitly disable an in/out pattern, as the
                // values which are are returned from this function will
                // be merged with the varibles in the outer unification via
                // a different mechanism.
                invars.erase(var);
              }
              rulebody->push_front(Local << (Var ^ out_var) << Undefined);
            }

            // create the arguments for the rule and call
            Node ruleargs = NodeDef::create(RuleArgs);
            Node argseq = NodeDef::create(ArgSeq);
            for (auto& var : invars)
            {
              ruleargs << (ArgVar << (Var ^ var) << Undefined);
              argseq << (Expr << (RefTerm << (Var ^ var)));
            }

            Node rulename = Var ^ _.fresh({"enum"});
            Node rulevalue;
            if (out_map.empty())
            {
              // no outputs. We just return true.
              rulevalue = DataTerm << (Scalar << JSONTrue);
            }
            else
            {
              // embed the outputs in an object.
              rulevalue = NodeDef::create(Object);
              for (auto& [var, out_var] : out_map)
              {
                rulevalue
                  << (ObjectItem << (Key ^ var)
                                 << (Expr << (RefTerm << (Var ^ out_var))));
              }
              Location value = _.fresh({"value"});
              rulevalue = UnifyBody
                << (Local << (Var ^ value) << Undefined)
                << (UnifyExpr << (Var ^ value)
                              << (Expr << (Term << rulevalue)));
            }

            Node result = Seq
              << (Lift << Module
                       << (RuleFunc << rulename << ruleargs << rulebody
                                    << rulevalue << (JSONInt ^ "0")))
              << (UnifyExpr
                  << _(Item)
                  << (Expr << (Enumerate << (Expr << (RefTerm << _(ItemSeq))))))
              << (UnifyExpr
                  << _(Var)
                  << (Expr << (ExprCall << rulename->clone() << argseq)));
            for (auto var : outvars)
            {
              // we need to unify the results with the variables in the source
              // problem
              result
                << (UnifyExpr
                    << (Var ^ var)
                    << (Expr
                        << (RefTerm
                            << (SimpleRef << (Var ^ _(Var))
                                          << (RefArgDot << (Var ^ var))))));
            }
            return result;
          },

        In(UnifyBody) *
            (T(UnifyExprCompr)(
               [](auto& n) { return is_in(*n.first, {Module}); })
             << (T(Var)[Var] *
                 (T(ArrayCompr) / T(SetCompr) / T(ObjectCompr))[Compr] *
                 (T(NestedBody) << (T(Key)[Key] * T(UnifyBody)[UnifyBody])))) >>
          [](Match& _) {
            Node rulebody = _(UnifyBody);
            Locs invars = find_invars(_(UnifyBody));

            Node rulename = Var ^ _(Key);
            Location value = _.fresh({"value"});
            if (invars.empty())
            {
              // no invars. This can be expressed as a RuleComp.
              Node rulevalue = UnifyBody
                << (Local << (Var ^ value) << Undefined)
                << (UnifyExpr
                    << (Var ^ value)
                    << (Expr << (_(Compr)->type() << _(Compr) / Var)));
              return Seq << (Lift << Module
                                  << (RuleComp << rulename << rulebody
                                               << rulevalue << (JSONInt ^ "0")))
                         << (UnifyExpr
                             << _(Var)
                             << (Expr << (RefTerm << rulename->clone())));
            }
            else
            {
              // similar to Enum above, but with a single known output
              Node ruleargs = NodeDef::create(RuleArgs);
              Node argseq = NodeDef::create(ArgSeq);
              for (auto& var : invars)
              {
                ruleargs << (ArgVar << (Var ^ var) << Undefined);
                argseq << (Expr << (RefTerm << (Var ^ var)));
              }

              Node rulevalue = UnifyBody
                << (Local << (Var ^ value) << Undefined)
                << (UnifyExpr
                    << (Var ^ value)
                    << (Expr << (_(Compr)->type() << _(Compr) / Var)));

              Location partial = _.fresh({"partial"});
              return Seq << (Lift
                             << Module
                             << (RuleFunc << rulename << ruleargs << rulebody
                                          << rulevalue << (JSONInt ^ "0")))
                         << (Local << (Var ^ partial) << Undefined)
                         << (UnifyExpr
                             << (Var ^ partial)
                             << (Expr
                                 << (ExprCall << rulename->clone() << argseq)))
                         << (UnifyExpr << _(Var)
                                       << (Expr << (Merge << (Var ^ partial))));
            }
          },

        // errors

        In(ExprCall) * (T(ArgSeq)[ArgSeq] << End) >>
          [](Match& _) {
            return err(_(ArgSeq), "Syntax error: empty argument sequence");
          },

        In(RuleFunc) * (T(RuleArgs)[RuleArgs] << End) >>
          [](Match& _) {
            return err(_(RuleArgs), "Syntax error: no rule arguments");
          },

      }};
  }
}