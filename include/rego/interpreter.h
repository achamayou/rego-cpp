#pragma once

#include "lang.h"

#include <string>

namespace rego
{
  using namespace trieste;
  class Interpreter
  {
  public:
    Interpreter(bool disable_well_formed_checks = false);
    ~Interpreter();
    void add_module_file(const std::filesystem::path& path);
    void add_module(const std::string& name, const std::string& contents);
    void add_data_json_file(const std::filesystem::path& path);
    void add_data_json(const std::string& json);
    void add_data(const Node& node);
    void add_input_json_file(const std::filesystem::path& path);
    void add_input_json(const std::string& json);
    void add_input(const Node& node);
    std::string query(const std::string& query_expr) const;
    Node raw_query(const std::string& query_expr) const;
    Interpreter& debug_path(const std::filesystem::path& prefix);
    const std::filesystem::path& debug_path() const;
    Interpreter& debug_enabled(bool enabled);
    bool debug_enabled() const;
    Interpreter& well_formed_checks_enabled(bool enabled);
    bool well_formed_checks_enabled() const;
    Interpreter& executable(const std::filesystem::path& path);
    const std::filesystem::path& executable() const;

  private:
    void write_ast(
      std::size_t index, const std::string& pass, const Node& ast) const;
    bool has_error(const Node& ast) const;
    Parse m_parser;
    wf::Wellformed m_wf_parser;
    std::vector<PassCheck> m_passes;
    Node m_module_seq;
    Node m_data_seq;
    Node m_input;
    std::filesystem::path m_debug_path;
    bool m_debug_enabled;
    bool m_well_formed_checks_enabled;
  };
}