#pragma once

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <list>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace eargs {
enum types { string, integer, hex, boolean, empty };

enum class parse_mode { permissive, strict };

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

    bool contains(const std::string& name) const {
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

            printf("%s:\t %s (required: %s)\n", names.c_str(), opt.description.c_str(),
                   opt.required ? "true" : "false");
        };

        return true;
    };

    template <typename T>
        requires std::is_integral_v<T>
    T get(std::string_view name) const noexcept {
        const auto* opt = find_option(name);
        if (!opt)
            return {};

        if constexpr (std::is_same_v<T, bool>) {
            if (opt->type == eargs::empty)
                return true;

            if (opt->type != eargs::boolean && opt->type != eargs::integer)
                return false;

            T result{};
            int value{};

            const auto* begin = opt->variable.data();
            const auto* end = begin + opt->variable.size();

            const auto [ptr, ec] = std::from_chars(begin, end, value);

            if (ec != std::errc{} || ptr != end)
                return false;

            return value != 0;
        } else {
            int base = 10;

            switch (opt->type) {
            case eargs::hex:
                base = 16;
                break;

            case eargs::integer:
            case eargs::boolean:
                break;

            default:
                return {};
            }

            T result{};

            const auto* begin = opt->variable.data();
            const auto* end = begin + opt->variable.size();

            const auto [ptr, ec] = std::from_chars(begin, end, result, base);

            if (ec != std::errc{} || ptr != end)
                return {};

            return result;
        }
    }

    template <typename T>
        requires std::is_same_v<T, std::string>
    T get(std::string_view name) const {
        const auto* opt = find_option(name);
        return opt ? opt->variable : std::string{};
    }
};
}; // namespace eargs
