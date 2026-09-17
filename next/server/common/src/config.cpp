#include "jx/config.hpp"

#include <cctype>
#include <charconv>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <system_error>
#include <utility>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
extern char** environ;
#endif

namespace jx::config {
namespace {

std::string lower(std::string_view v)
{
    std::string out(v);
    for (char& c : out) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return out;
}

std::vector<std::string> process_environment()
{
    std::vector<std::string> out;
#ifdef _WIN32
    LPCH block = GetEnvironmentStringsA();
    if (block == nullptr) return out;
    for (const char* p = block; *p != '\0'; p += std::strlen(p) + 1) {
        if (*p != '=') out.emplace_back(p);    // skip the hidden "=C:=C:\..." drive entries
    }
    FreeEnvironmentStringsA(block);
#else
    for (char** e = environ; e != nullptr && *e != nullptr; ++e) out.emplace_back(*e);
#endif
    return out;
}

} // namespace

std::vector<std::string> split_path(std::string_view dotted_path)
{
    std::vector<std::string> keys;
    std::size_t pos = 0;
    while (pos <= dotted_path.size()) {
        const auto dot = dotted_path.find('.', pos);
        const auto key = dotted_path.substr(pos, dot == std::string_view::npos ? std::string_view::npos : dot - pos);
        if (!key.empty()) keys.emplace_back(key);
        if (dot == std::string_view::npos) break;
        pos = dot + 1;
    }
    return keys;
}

std::string env_key_to_path(std::string_view key, std::string_view prefix)
{
    if (key.size() <= prefix.size() || key.substr(0, prefix.size()) != prefix) return {};
    const std::string_view rest = key.substr(prefix.size());
    std::string out;
    out.reserve(rest.size());
    for (std::size_t i = 0; i < rest.size(); ++i) {
        if (rest[i] == '_' && i + 1 < rest.size() && rest[i + 1] == '_') {
            out.push_back('.');
            ++i;
        } else {
            out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(rest[i]))));
        }
    }
    return out;
}

Config Config::from_json_text(std::string_view text)
{
    Config cfg;
    try {
        cfg.root_ = nlohmann::json::parse(text, nullptr, true, /*ignore_comments=*/true);
    } catch (const nlohmann::json::exception& e) {
        throw std::runtime_error(std::string("config: invalid JSON: ") + e.what());
    }
    if (!cfg.root_.is_object()) throw std::runtime_error("config: root must be a JSON object");
    return cfg;
}

Config Config::from_file(const std::filesystem::path& path)
{
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("config: cannot open " + path.string());
    const std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    try {
        return from_json_text(text);
    } catch (const std::runtime_error& e) {
        throw std::runtime_error(path.string() + ": " + e.what());
    }
}

void Config::merge(const nlohmann::json& other)
{
    root_.merge_patch(other);
}

void Config::set(std::string_view dotted_path, std::string_view raw)
{
    const auto keys = split_path(dotted_path);
    if (keys.empty()) return;

    nlohmann::json value = nlohmann::json::parse(raw, nullptr, /*allow_exceptions=*/false);
    if (value.is_discarded()) value = std::string(raw);

    nlohmann::json* cur = &root_;
    for (std::size_t i = 0; i + 1 < keys.size(); ++i) {
        if (!cur->is_object()) *cur = nlohmann::json::object();
        cur = &(*cur)[keys[i]];
    }
    if (!cur->is_object()) *cur = nlohmann::json::object();
    (*cur)[keys.back()] = std::move(value);
}

void Config::apply_overrides(const std::map<std::string, std::string>& overrides)
{
    for (const auto& [key, value] : overrides) set(key, value);
}

void Config::apply_env_list(const std::vector<std::string>& entries, std::string_view prefix)
{
    for (const std::string& entry : entries) {
        const auto eq = entry.find('=');
        if (eq == std::string::npos || eq == 0) continue;
        const std::string path = env_key_to_path(std::string_view(entry).substr(0, eq), prefix);
        if (!path.empty()) set(path, std::string_view(entry).substr(eq + 1));
    }
}

void Config::apply_env(std::string_view prefix)
{
    apply_env_list(process_environment(), prefix);
}

const nlohmann::json* Config::find(std::string_view dotted_path) const
{
    const nlohmann::json* cur = &root_;
    for (const std::string& key : split_path(dotted_path)) {
        if (!cur->is_object()) return nullptr;
        const auto it = cur->find(key);
        if (it == cur->end()) return nullptr;
        cur = &*it;
    }
    return cur;
}

bool Config::has(std::string_view dotted_path) const
{
    return find(dotted_path) != nullptr;
}

std::optional<std::string> Config::get_string(std::string_view dotted_path) const
{
    const nlohmann::json* v = find(dotted_path);
    if (v == nullptr || v->is_null()) return std::nullopt;
    if (v->is_string()) return v->get<std::string>();
    if (v->is_number() || v->is_boolean()) return v->dump();
    return std::nullopt;
}

std::string Config::get_string(std::string_view dotted_path, std::string_view def) const
{
    return get_string(dotted_path).value_or(std::string(def));
}

std::int64_t Config::get_int(std::string_view dotted_path, std::int64_t def) const
{
    const nlohmann::json* v = find(dotted_path);
    if (v == nullptr) return def;
    if (v->is_number_integer()) return v->get<std::int64_t>();
    if (v->is_number_float()) return static_cast<std::int64_t>(v->get<double>());
    if (v->is_boolean()) return v->get<bool>() ? 1 : 0;
    if (v->is_string()) {
        const std::string& s = v->get_ref<const std::string&>();
        std::int64_t parsed = 0;
        const auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), parsed);
        if (ec == std::errc{} && ptr == s.data() + s.size()) return parsed;
    }
    return def;
}

bool Config::get_bool(std::string_view dotted_path, bool def) const
{
    const nlohmann::json* v = find(dotted_path);
    if (v == nullptr) return def;
    if (v->is_boolean()) return v->get<bool>();
    if (v->is_number()) return v->get<double>() != 0.0;
    if (v->is_string()) {
        const std::string s = lower(v->get_ref<const std::string&>());
        if (s == "1" || s == "true" || s == "yes" || s == "on") return true;
        if (s == "0" || s == "false" || s == "no" || s == "off") return false;
    }
    return def;
}

} // namespace jx::config
