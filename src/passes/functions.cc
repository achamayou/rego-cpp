#include "lang.h"
#include "passes.h"

namespace
{
  using namespace rego;
  using namespace wf::ops;

  const auto inline VarOrTerm = T(Var) / T(Term);
  const auto inline RefArg = T(RefArgDot) / T(RefArgBrack);

  // clang-format off
  inline const auto wfi =
      (ObjectItem <<= Key * Expr)
    | (RefObjectItem <<= RefTerm * Expr)
    | (NumTerm <<= JSONInt | JSONFloat)
    | (ArithArg <<= RefTerm | NumTerm | UnaryExpr | ArithInfix)
    | (BoolArg <<= Term | RefTerm | NumTerm | UnaryExpr | ArithInfix)
    | (RefArgDot <<= Var)
    | (RefArgBrack <<= Scalar | Var | Object | Array | Set)
    | (RefTerm <<= SimpleRef)
    ;
  // clang-format on
}
namespace rego
{

  // Converts all UnifyExpr statements to be of either <var> = <var>,
  // <var> = <term>, or <var> = <function> forms, where <function> is a named
  // function that takes either <var> or <term> arguments.
  PassDef functions()
  {
    return {
      (In(UnifyExpr) / In(ArgSeq)) * (T(Expr) << Any[Val]) >>
        [](Match& _) { return _(Val); },

      (In(UnifyExpr) / In(ArgSeq)) * (T(Term) << T(Scalar)[Scalar]) >>
        [](Match& _) { return _(Scalar); },

      (In(UnifyExpr) / In(ArgSeq)) * (T(Term) << T(Object)[Object]) >>
        [](Match& _) {
          Node seq = NodeDef::create(Seq);
          Location temp = _.fresh({"obj"});
          Node function = Function << (JSONString ^ "object")
                                   << (ArgSeq << *_[Object]);
          seq->push_back(
            Lift << UnifyBody << (Local << (Var ^ temp) << Undefined));
          seq->push_back(
            Lift << UnifyBody << (UnifyExpr << (Var ^ temp) << function));
          seq->push_back(Var ^ temp);
          return seq;
        },

      In(ArgSeq) * T(Key)[Key] >>
        [](Match& _) { return Scalar << (JSONString ^ _(Key)); },

      In(ArgSeq) * T(Set)[Set] >> [](Match& _) { return Term << _(Set); },

      (In(UnifyExpr) / In(ArgSeq)) * T(ObjectItem)[ObjectItem] >>
        [](Match& _) {
          Node seq = NodeDef::create(Seq);
          seq->push_back(wfi / _(ObjectItem) / Key);
          seq->push_back(wfi / _(ObjectItem) / Expr);
          return seq;
        },

      (In(UnifyExpr) / In(ArgSeq)) * T(RefObjectItem)[RefObjectItem] >>
        [](Match& _) {
          Node seq = NodeDef::create(Seq);
          seq->push_back(wfi / _(RefObjectItem) / RefTerm);
          seq->push_back(wfi / _(RefObjectItem) / Expr);
          return seq;
        },

      (In(UnifyExpr) / In(ArgSeq)) * (T(Enumerate) << T(Expr)[Expr]) >>
        [](Match& _) {
          return Function << (JSONString ^ "enumerate") << (ArgSeq << _(Expr));
        },

      (In(UnifyExpr) / In(ArgSeq)) * (T(Term) << T(Array)[Array]) >>
        [](Match& _) {
          Node seq = NodeDef::create(Seq);
          Location temp = _.fresh({"array"});
          Node function = Function << (JSONString ^ "array")
                                   << (ArgSeq << *_[Array]);
          seq->push_back(
            Lift << UnifyBody << (Local << (Var ^ temp) << Undefined));
          seq->push_back(
            Lift << UnifyBody << (UnifyExpr << (Var ^ temp) << function));
          seq->push_back(Var ^ temp);
          return seq;
        },

      (In(UnifyExpr) / In(ArgSeq)) * (T(Term) << T(Set)[Set]) >>
        [](Match& _) {
          Node seq = NodeDef::create(Seq);
          Location temp = _.fresh({"set"});
          Node function = Function << (JSONString ^ "set")
                                   << (ArgSeq << *_[Set]);
          seq->push_back(
            Lift << UnifyBody << (Local << (Var ^ temp) << Undefined));
          seq->push_back(
            Lift << UnifyBody << (UnifyExpr << (Var ^ temp) << function));
          seq->push_back(Var ^ temp);
          return seq;
        },

      (In(UnifyExpr) / In(ArgSeq)) *
          (T(ArrayCompr) / T(SetCompr) / T(ObjectCompr))[Compr] >>
        [](Match& _) {
          std::string name = _(Compr)->type().str();
          std::transform(
            name.begin(), name.end(), name.begin(), [](unsigned char c) {
              return static_cast<char>(std::tolower(c));
            });
          Location temp = _.fresh({name});
          return Function << (JSONString ^ name) << (ArgSeq << *_[Compr]);
          ;
        },

      (In(UnifyExpr) / In(ArgSeq)) *
          (T(Term) << (T(ArrayCompr) / T(SetCompr) / T(ObjectCompr)))[Compr] >>
        [](Match& _) {
          std::string name = _(Compr)->type().str();
          std::transform(
            name.begin(), name.end(), name.begin(), [](unsigned char c) {
              return static_cast<char>(std::tolower(c));
            });
          Location temp = _.fresh({name});
          return Function << (JSONString ^ name) << (ArgSeq << *_[Compr]);
          ;
        },

      (In(UnifyExpr) / In(ArgSeq)) * T(ToValues)[ToValues] >>
        [](Match& _) {
          return Function << (JSONString ^ "to-values")
                          << (ArgSeq << *_[ToValues]);
        },

      (In(UnifyExpr) / In(ArgSeq)) * (T(Merge) << T(Var)[Var]) >>
        [](Match& _) {
          return Function << (JSONString ^ "merge") << (ArgSeq << _(Var));
        },

      (In(UnifyExpr) / In(ArgSeq) * T(NumTerm)[NumTerm]) >>
        [](Match& _) { return Scalar << _(NumTerm)->front(); },

      In(ArgSeq) * T(Function)[Function]([](auto& n) {
        return is_in(*n.first, {UnifyBody});
      }) >>
        [](Match& _) {
          Node seq = NodeDef::create(Seq);
          Location temp = _.fresh({"func"});
          seq->push_back(
            Lift << UnifyBody << (Local << (Var ^ temp) << Undefined));
          seq->push_back(
            Lift << UnifyBody << (UnifyExpr << (Var ^ temp) << _(Function)));
          seq->push_back(Var ^ temp);

          return seq;
        },

      In(UnifyExpr) * (T(NotExpr) << T(Expr)[Expr]) >>
        [](Match& _) {
          Node seq = NodeDef::create(Seq);
          Location temp = _.fresh({"expr"});
          seq->push_back(
            Lift << UnifyBody << (Local << (Var ^ temp) << Undefined));
          seq->push_back(
            Lift << UnifyBody << (UnifyExpr << (Var ^ temp) << _(Expr)));
          seq->push_back(
            Function << (JSONString ^ "not") << (ArgSeq << (Var ^ temp)));

          return seq;
        },

      In(UnifyExpr) *
          (T(ExprEvery) << (T(VarSeq)[VarSeq] * T(NestedBody)[NestedBody])) >>
        [](Match& _) {
          return Function << (JSONString ^ "every")
                          << (ArgSeq << _(VarSeq) << _(NestedBody));
        },

      (In(UnifyExpr) / In(ArgSeq)) * (T(UnaryExpr) << T(ArithArg)[ArithArg]) >>
        [](Match& _) {
          return Function << (JSONString ^ "unary")
                          << (ArgSeq << _(ArithArg)->front());
        },

      (In(UnifyExpr) / In(ArgSeq)) *
          (T(ArithInfix) << (T(ArithArg)[Lhs] * Any[Op] * T(ArithArg)[Rhs])) >>
        [](Match& _) {
          return Function << (JSONString ^ "arithinfix")
                          << (ArgSeq << _(Op) << _(Lhs)->front()
                                     << _(Rhs)->front());
        },

      (In(UnifyExpr) / In(ArgSeq)) *
          (T(BinInfix) << (T(BinArg)[Lhs] * Any[Op] * T(BinArg)[Rhs])) >>
        [](Match& _) {
          return Function << (JSONString ^ "bininfix")
                          << (ArgSeq << _(Op) << _(Lhs)->front()
                                     << _(Rhs)->front());
        },

      (In(UnifyExpr) / In(ArgSeq)) *
          (T(BoolInfix) << (T(BoolArg)[Lhs] * Any[Op] * T(BoolArg)[Rhs])) >>
        [](Match& _) {
          return Function << (JSONString ^ "boolinfix")
                          << (ArgSeq << _(Op) << _(Lhs)->front()
                                     << _(Rhs)->front());
        },

      (In(UnifyExpr) / In(ArgSeq)) * (T(RefTerm) << T(Var)[Var]) >>
        [](Match& _) { return _(Var); },

      (In(UnifyExpr) / In(ArgSeq)) *
          (T(RefTerm)
           << (T(SimpleRef) << (T(Var)[Var] * (T(RefArgDot)[RefArgDot])))) >>
        [](Match& _) {
          Location field_name = (wfi / _(RefArgDot) / Var)->location();
          Node arg = Scalar << (JSONString ^ field_name);
          return Function << (JSONString ^ "apply_access")
                          << (ArgSeq << _(Var) << arg);
        },

      (In(UnifyExpr) / In(ArgSeq)) *
          (T(RefTerm)
           << (T(SimpleRef)
               << (T(Var)[Var] * (T(RefArgBrack)[RefArgBrack])))) >>
        [](Match& _) {
          Node seq = NodeDef::create(Seq);
          Node arg = _(RefArgBrack)->front();
          if (arg->type() == RefTerm)
          {
            return Function << (JSONString ^ "apply_access")
                            << (ArgSeq << _(Var) << arg);
          }
          else
          {
            return Function << (JSONString ^ "apply_access")
                            << (ArgSeq << _(Var) << (Term << arg));
          }
        },

      (In(UnifyExpr) / In(ArgSeq)) *
          (T(ExprCall) << (T(Var)[Var] * T(ArgSeq)[ArgSeq])) >>
        [](Match& _) {
          return Function << (JSONString ^ "call")
                          << (ArgSeq << _(Var) << *_[ArgSeq]);
        },

      (In(Array) / In(Set) / In(ObjectItem)) * (T(Expr) << T(Term)[Term]) >>
        [](Match& _) { return _(Term); },

      (In(Array) / In(Set) / In(ObjectItem)) *
          (T(Expr) << T(NumTerm)[NumTerm]) >>
        [](Match& _) { return Term << (Scalar << *_[NumTerm]); },

      (In(RuleComp) / In(RuleFunc) / In(RuleObj) / In(RuleSet) / In(DataItem)) *
          T(DataTerm)[DataTerm] >>
        [](Match& _) { return Term << *_[DataTerm]; },

      In(Term) * T(DataArray)[DataArray] >>
        [](Match& _) { return Array << *_[DataArray]; },

      In(Term) * T(DataSet)[DataSet] >>
        [](Match& _) { return Set << *_[DataSet]; },

      In(Term) * T(DataObject)[DataObject] >>
        [](Match& _) { return Object << *_[DataObject]; },

      (In(Object) / In(ObjectItemSeq)) * T(DataItem)[DataItem] >>
        [](Match& _) { return ObjectItem << *_[DataItem]; },

      (In(ObjectItem) / In(Array) / In(Set)) * T(DataTerm)[DataTerm] >>
        [](Match& _) { return Term << *_[DataTerm]; },

      // errors

      In(ObjectItem) * T(Expr)[Expr] >>
        [](Match& _) { return err(_(Expr), "Invalid expression in object"); },

      In(Expr) * Any[Expr] >>
        [](Match& _) { return err(_(Expr), "Invalid expression"); },

      (In(UnifyExpr) / In(ArgSeq)) * (T(RefTerm) << T(Ref)[Ref]) >>
        [](Match& _) { return err(_(Ref), "Invalid reference"); },

      In(Array) * T(Expr)[Expr] >>
        [](Match& _) { return err(_(Expr), "Invalid expression in array"); },

      In(Set) * T(Expr)[Expr] >>
        [](Match& _) { return err(_(Expr), "Invalid expression in set"); },

      In(ArgSeq) * T(Ref)[Ref] >>
        [](Match& _) { return err(_(Ref), "Invalid reference"); },

      In(Object) * T(RefObjectItem)[RefObjectItem] >>
        [](Match& _) { return err(_(RefObjectItem), "Invalid object item"); },

      In(ObjectItem) * T(Module)[Module] >>
        [](Match& _) {
          return err(
            _(Module), "Syntax error: module not allowed as object item value");
        },

      In(ArgSeq) * T(ExprEvery)[ExprEvery] >>
        [](Match& _) { return err(_(ExprEvery), "Invalid every statement"); },
    };
  }
}