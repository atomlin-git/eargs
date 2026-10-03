#pragma once

#include <algorithm>
#include <any>
#include <charconv>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include <list>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace eargs {
enum types { string, integer, hex, boolean, empty, enumeration };

enum class parse_mode { permissive, strict };

template <typename Enum>
    requires std::is_enum_v<Enum>
struct choices {
    std::vector<std::pair<std::string, Enum>> values;
    std::optional<Enum> default_value;

    choices(std::initializer_list<std::pair<std::string, Enum>> values_,
            std::optional<Enum> default_value_ = {})
        : values(values_), default_value(default_value_) {};
};

struct option {
    std::list<std::string> names{};
    std::string description{};
    std::string variable{};
    eargs::types type{eargs::string};
    bool required{false};

    option(std::list<std::string> names_, std::string description_, eargs::types type_,
           bool required_)
        : names(std::move(names_)), description(std::move(description_)), type(type_),
          required(required_) {};

    template <typename Enum>
        requires std::is_enum_v<Enum>
    option(std::list<std::string> names_, std::string description_, choices<Enum> choices_,
           bool required_ = false)
        : names(std::move(names_)), description(std::move(description_)), type(eargs::enumeration),
          required(required_) {
        if (choices_.values.empty())
            throw std::invalid_argument("Enum choices must not be empty");
        std::set<std::string> spellings;
        for (auto& [name, value] : choices_.values) {
            if (!spellings.insert(name).second)
                throw std::invalid_argument("Duplicate enum choice: " + name);
            values_.push_back({std::move(name), value});
        }
        if (choices_.default_value)
            default_value_ = *choices_.default_value;
    };

  private:
    struct choice_value {
        std::string name;
        std::any value;
    };
    std::vector<choice_value> values_;
    std::any default_value_;

    std::string choice_names() const {
        std::string result;
        for (const auto& choice : values_) {
            if (!result.empty())
                result += ", ";
            result += choice.name.empty() ? "\"\"" : choice.name;
        }
        return result;
    }
    friend class parser;
};

class parser {
    std::list<eargs::option> options{};
    std::vector<bool> present{};
    std::string error_message{};

    bool fail(std::string message) {
        error_message = std::move(message);
        return false;
    };

    void reset() {
        for (auto& opt : options)
            opt.variable.clear();
        std::fill(present.begin(), present.end(), false);
        error_message.clear();
    };

    static bool negative_number(const std::string& value) {
        return value.size() > 1 && value[0] == '-' && value[1] >= '0' && value[1] <= '9';
    };

    const eargs::option* find_option(std::string_view name) const noexcept {
        for (const auto& opt : options) {
            if (std::find(opt.names.begin(), opt.names.end(), name) != opt.names.end()) {
                return &opt;
            };
        };

        return {};
    };

  public:
    parser(std::list<eargs::option> options_)
        : options(std::move(options_)), present(options.size(), false) {
        std::set<std::string> names;
        for (const auto& opt : options) {
            if (opt.type == eargs::enumeration && opt.values_.empty())
                throw std::invalid_argument("Enum option requires typed choices");
            for (const auto& name : opt.names) {
                if (!names.insert(name).second) {
                    throw std::logic_error("Duplicate option name: " + name);
                };
            };
        };
    };
    parser(eargs::option option) : parser(std::list<eargs::option>{std::move(option)}) {};

    bool parse(const std::string& str, parse_mode mode = parse_mode::permissive) {
        std::istringstream stream(str);
        std::vector<std::string> tokens{};
        for (std::string token; stream >> token;) {
            tokens.push_back(std::move(token));
        };

        std::vector<const char*> args{nullptr};
        for (const auto& token : tokens) {
            args.push_back(token.c_str());
        };

        return parse(args.data(), static_cast<int>(args.size()), mode);
    };

    bool parse(const char* const args[], int count, parse_mode mode = parse_mode::permissive) {
        reset();

        if (count <= 1) {
            return fail("No arguments supplied");
        };

        if (!args) {
            return fail("Null argument vector");
        };

        auto parsed_options = options;
        std::vector<bool> supplied(options.size(), false);

        for (auto i = 1; i < count; ++i) {
            if (!args[i]) {
                return fail("null argument");
            };

            const std::string argument = args[i];
            if (argument.empty() || argument[0] != '-') {
                if (mode == parse_mode::strict) {
                    return fail("Unexpected positional argument");
                };

                continue;
            };

            const auto prefix = argument.starts_with("--") ? 2 : 1;
            const auto equal = argument.find('=', prefix);
            const auto name =
                argument.substr(prefix, equal == std::string::npos ? equal : equal - prefix);

            const auto spelling = argument.substr(0, equal);
            auto selected = parsed_options.end();

            size_t index{};
            for (auto opt = parsed_options.begin(); opt != parsed_options.end(); ++opt, ++index) {
                if (std::find(opt->names.begin(), opt->names.end(), name) != opt->names.end()) {
                    selected = opt;
                    break;
                };
            };

            if (selected == parsed_options.end()) {
                if (mode == parse_mode::strict) {
                    return fail("unknown option: " + spelling);
                };

                continue;
            };

            if (supplied[index] && mode == parse_mode::strict) {
                return fail("repeated option: " + spelling);
            };

            if (selected->type == eargs::empty) {
                if (equal != std::string::npos) {
                    return fail("flag does not accept a value: " + spelling);
                };

                selected->variable = "empty";
            } else if (equal != std::string::npos) {
                selected->variable = argument.substr(equal + 1);
            } else {
                if (i + 1 >= count || !args[i + 1]) {
                    return fail("missing value for " + spelling);
                };

                const auto value = std::string{args[i + 1]};
                if (!value.empty() && value[0] == '-' && !negative_number(value)) {
                    return fail("missing value for " + spelling);
                };

                selected->variable = value;
                ++i;
            };

            if (selected->type == eargs::enumeration &&
                std::none_of(
                    selected->values_.begin(), selected->values_.end(),
                    [&](const auto& choice) { return choice.name == selected->variable; })) {

                return fail("invalid value for " + spelling +
                            " (choices: " + selected->choice_names() + ")");
            };

            supplied[index] = true;
        };

        size_t index{};
        for (const auto& opt : parsed_options) {
            if (opt.required && !supplied[index]) {
                return fail("missing required option: " +
                            (opt.names.empty() ? "" : opt.names.front()));
            };

            ++index;
        };

        options = std::move(parsed_options);
        present = std::move(supplied);
        return true;
    };

    const auto& error() const noexcept { return error_message; };

    bool contains(const std::string_view name) const {
        size_t index{};
        for (const auto& opt : options) {
            if (std::find(opt.names.begin(), opt.names.end(), name) != opt.names.end()) {
                return present[index];
            };

            ++index;
        };

        return false;
    };

    bool print_help() const {
        for (const auto& opt : options) {
            std::string names{};
            for (const auto& name : opt.names) {
                if (!names.empty()) {
                    names += ',';
                };

                names += (name.size() == 1 ? "-" : "--") + name;
            };

            auto description = opt.description;
            if (opt.type == eargs::enumeration)
                description += " (choices: " + opt.choice_names() + ")";

            printf("%s:\t %s (required: %s)\n", names.c_str(), description.c_str(),
                   opt.required ? "true" : "false");
        };

        return true;
    };

    std::vector<std::string> complete(const std::string_view name,
                                      std::string_view prefix = {}) const {
        std::vector<std::string> result{};
        if (const auto* opt = find_option(name)) {
            for (const auto& choice : opt->values_) {
                if (choice.name.starts_with(prefix)) {
                    result.push_back(choice.name);
                };
            };
        };

        return result;
    };

    template <typename Enum>
        requires std::is_enum_v<Enum>
    Enum get(std::string_view name) const {
        const auto* opt = find_option(name);
        if (!opt) {
            throw std::out_of_range("unknown option: " + std::string(name));
        };

        if (opt->values_.empty() || !std::any_cast<Enum>(&opt->values_.front().value)) {
            throw std::logic_error("enum type mismatch for option: " + std::string(name));
        };

        if (!contains(name)) {
            if (const auto* value = std::any_cast<Enum>(&opt->default_value_)) {
                return *value;
            };

            throw std::logic_error("option not supplied: " + std::string(name) +
                                   " (choices: " + opt->choice_names() + ")");
        };

        for (const auto& choice : opt->values_) {
            if (choice.name == opt->variable) {
                return *std::any_cast<Enum>(&choice.value);
            };
        };

        throw std::logic_error("invalid enum state for option: " + std::string(name));
    };

    template <typename T>
        requires std::is_integral_v<T>
    T get(std::string_view name) const noexcept {
        const auto* opt = find_option(name);
        if (!opt) {
            return {};
        };

        if constexpr (std::is_same_v<T, bool>) {
            if (opt->type == eargs::empty) {
                return true;
            };

            if (opt->type != eargs::boolean && opt->type != eargs::integer) {
                return false;
            };

            T result{};
            int value{};

            const auto* begin = opt->variable.data();
            const auto* end = begin + opt->variable.size();

            const auto [ptr, ec] = std::from_chars(begin, end, value);

            if (ec != std::errc{} || ptr != end) {
                return false;
            };

            return value != 0;
        } else {
            auto base = 10;

            switch (opt->type) {
            case eargs::hex: {
                base = 16;
                break;
            };

            case eargs::integer:
            case eargs::boolean: {
                break;
            };

            default: {
                return {};
            };
            };

            T result{};

            const auto* begin = opt->variable.data();
            const auto* end = begin + opt->variable.size();

            const auto [ptr, ec] = std::from_chars(begin, end, result, base);

            if (ec != std::errc{} || ptr != end) {
                return {};
            };

            return result;
        };
    };

    template <typename T>
        requires std::is_same_v<T, std::string> || std::is_same_v<T, std::string_view>
    T get(const std::string_view name, std::string_view fallback = {}) const {
        const auto* opt = find_option(name);
        return opt && contains(name) ? opt->variable : std::string(fallback);
    };
};
}; // namespace eargs
