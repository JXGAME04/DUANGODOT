// jx::config - layered configuration for every JX NEXT process.
//
// Precedence (last wins):  JSON file  <  environment (JX_SECTION__KEY)  <  command line (--set a.b=c)
// Keys are dotted paths ("log.level", "net.port"); values keep their JSON type when they come from
// a file and are auto-typed ("15622" -> 15622, "true" -> true, anything else -> string) when they
// come from the environment or the command line.
#pragma once

#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

namespace jx::config {

class Config {
public:
    Config() = default;

    static Config from_json_text(std::string_view text);           // throws std::runtime_error
    static Config from_file(const std::filesystem::path& path);    // throws std::runtime_error

    void merge(const nlohmann::json& other);                       // deep merge, other wins
    void set(std::string_view dotted_path, std::string_view raw);  // auto-typed, creates parents
    void apply_overrides(const std::map<std::string, std::string>& overrides);
    void apply_env_list(const std::vector<std::string>& entries, std::string_view prefix = "JX_"); // "KEY=VALUE"
    void apply_env(std::string_view prefix = "JX_");               // the real process environment

    [[nodiscard]] bool has(std::string_view dotted_path) const;
    [[nodiscard]] std::optional<std::string> get_string(std::string_view dotted_path) const;
    [[nodiscard]] std::string get_string(std::string_view dotted_path, std::string_view def) const;
    [[nodiscard]] std::int64_t get_int(std::string_view dotted_path, std::int64_t def) const;
    [[nodiscard]] bool get_bool(std::string_view dotted_path, bool def) const;

    [[nodiscard]] const nlohmann::json& json() const noexcept { return root_; }
    [[nodiscard]] std::string dump(int indent = 2) const { return root_.dump(indent); }
    // Every setting as ("zone.port", "17001"), sorted by key, for a start-up log a person can read
    // line by line.  Keys that start with '_' are notes, not settings; a value whose key smells of
    // a secret (password, secret, token) is shown as "***".
    [[nodiscard]] std::vector<std::pair<std::string, std::string>> flatten() const;

private:
    [[nodiscard]] const nlohmann::json* find(std::string_view dotted_path) const;
    nlohmann::json root_ = nlohmann::json::object();
};

// "JX_LOG__LEVEL" -> "log.level", "JX_LOG__ROTATE_BYTES" -> "log.rotate_bytes"; "" when the key does
// not start with the prefix.  Double underscore separates sections, single underscore is kept.
std::string env_key_to_path(std::string_view key, std::string_view prefix = "JX_");

std::vector<std::string> split_path(std::string_view dotted_path);

} // namespace jx::config
