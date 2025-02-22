#pragma once

#include <optional>
#include <string>
#include <trieste/driver.h>

namespace rego
{
  using namespace trieste;
  using PrintNode = std::ostream& (*)(std::ostream&, const Node&);

  class Resolver
  {
  public:
    struct NodePrinter
    {
      Node node;
      PrintNode printer;
    };

    static std::int64_t get_int(const Node& node);

    static Node scalar(int64_t value);
    static Node scalar(double value);
    static Node scalar(bool value);
    static Node scalar(const std::string& value);
    static Node scalar();
    static double get_double(const Node& node);
    static std::string get_string(const Node& node);
    static bool get_bool(const Node& node);
    static Node resolve_query(const Node& query);
    static NodePrinter stmt_str(const Node& stmt);
    static NodePrinter func_str(const Node& func);
    static NodePrinter arg_str(const Node& arg);
    static NodePrinter expr_str(const Node& unifyexpr);
    static NodePrinter enum_str(const Node& unifyexprenum);
    static NodePrinter with_str(const Node& unifyexprwith);
    static NodePrinter compr_str(const Node& unifyexprcompr);
    static NodePrinter ref_str(const Node& ref);
    static Node negate(const Node& value);
    static Node unary(const Node& value);
    static Node arithinfix(const Node& op, const Node& lhs, const Node& rhs);
    static Node bininfix(const Node& op, const Node& lhs, const Node& rhs);
    static Node boolinfix(const Node& op, const Node& lhs, const Node& rhs);
    static std::optional<Nodes> apply_access(
      const Node& container, const Node& index);
    static Node object(const Node& object_items);
    static Node array(const Node& array_members);
    static Node set(const Node& set_members);
    static Node set_intersection(const Node& lhs, const Node& rhs);
    static Node set_union(const Node& lhs, const Node& rhs);
    static Node set_difference(const Node& lhs, const Node& rhs);
    static Nodes resolve_varseq(const Node& varseq);
    static std::optional<Node> maybe_unwrap_number(const Node& term);
    static std::optional<Node> maybe_unwrap_string(const Node& term);
    static std::optional<Node> maybe_unwrap_bool(const Node& term);
    static std::optional<Node> maybe_unwrap_set(const Node& term);
    static bool is_falsy(const Node& node);
    static bool is_truthy(const Node& node);
    static Nodes object_lookdown(const Node& object, const Node& query);
    static Node inject_args(const Node& rulefunc, const Nodes& args);
  };

  std::ostream& operator<<(
    std::ostream& os, const Resolver::NodePrinter& printer);
}