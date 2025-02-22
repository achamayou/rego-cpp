#pragma once

#include "args.h"
#include "builtins.h"
#include "value.h"
#include "variable.h"

#include <map>
#include <set>
#include <string>
#include <trieste/driver.h>
#include <vector>

namespace rego
{
  using namespace trieste;

  class UnifierDef;
  using Unifier = std::shared_ptr<UnifierDef>;
  using CallStack = std::shared_ptr<std::vector<Location>>;
  using ValuesLookup = std::map<std::string, Values>;
  using WithStack = std::shared_ptr<std::vector<ValuesLookup>>;
  using UnifierCache = std::shared_ptr<NodeMap<Unifier>>;

  class UnifierDef
  {
  public:
    UnifierDef(
      const Location& rule,
      const Node& rulebody,
      CallStack call_stack,
      WithStack with_stack,
      const BuiltIns& builtins,
      UnifierCache cache);
    Node unify();
    Nodes expressions() const;
    Nodes bindings() const;
    std::string str() const;
    std::string dependency_str() const;
    static Unifier create(
      const Location& rule,
      const Node& rulebody,
      CallStack call_stack,
      WithStack with_stack,
      const BuiltIns& builtins,
      UnifierCache cache);
    friend std::ostream& operator<<(std::ostream& os, const Unifier& unifier);

  private:
    Unifier rule_unifier(const Location& rule, const Node& rulebody);
    void init_from_body(const Node& rulebody, std::vector<Node>& statements);
    void add_variable(const Node& local);
    void add_unifyexpr(const Node& unifyexpr);
    void add_withpush(const Node& withpush);
    void add_withpop(const Node& withpop);
    void compute_dependency_scores();
    void reset();
    std::size_t compute_dependency_score(const Node& unifyexpr);
    std::size_t compute_dependency_score(
      Variable& var, std::set<Location>& visited);
    Values evaluate(const Location& var, const Node& value);
    Values evaluate_function(
      const Location& var, const std::string& func_name, const Args& args);
    Values resolve_var(const Node& var);
    Values enumerate(const Location& var, const Node& container);
    Values resolve_skip(const Node& skip);
    Values check_with(const Node& var);
    std::optional<RankedNode> resolve_rulecomp(const Node& rulecomp);
    std::optional<RankedNode> resolve_rulefunc(
      const Node& rulefunc, const Nodes& args);
    std::optional<Node> resolve_ruleset(const Nodes& ruleset);
    std::optional<Node> resolve_ruleobj(const Nodes& ruleobj);
    Node resolve_every(const Node& varseq, const Node& nestedbody);
    Values resolve_compr(const Location& var, const Node& compr);
    Values call_function(
      const Location& var, const std::string& name, const Values& args);
    bool is_local(const Node& var);
    std::size_t scan_vars(const Node& expr, std::vector<Location>& locals);
    void pass();
    void execute_statements(Nodes::iterator begin, Nodes::iterator end);
    void mark_invalid_values();
    void remove_invalid_values();
    Variable& get_variable(const Location& name);
    bool is_variable(const Location& name) const;
    Node bind_variables();
    Args create_args(const Node& args);

    bool push_rule(const Location& rule);
    void pop_rule(const Location& rule);

    void push_with(const Node& withseq);
    void pop_with();

    Location m_rule;
    std::map<Location, Variable> m_variables;
    std::vector<Node> m_statements;
    NodeMap<std::vector<Node>> m_nested_statements;
    CallStack m_call_stack;
    WithStack m_with_stack;
    BuiltIns m_builtins;
    std::size_t m_retries;
    Token m_parent_type;
    UnifierCache m_cache;
  };
}