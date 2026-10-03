#include <eargs/parser.hpp>
#include <gtest/gtest.h>
#include <cstdint>

namespace {
enum class Color { unknown, red, blue };
enum class Other { red };
enum class Wide : uint64_t { large = UINT64_MAX };
enum class Signed : int64_t { negative = INT64_MIN };

eargs::parser colors(bool required = false) {
    return eargs::parser(eargs::option{
        {"c", "color"}, "color",
        eargs::choices<Color>{{{"red", Color::red}, {"r", Color::red}, {"blue", Color::blue}},
                              Color::unknown}, required});
}
}

TEST(enum_choices, converts_values_and_aliases_in_both_parse_modes) {
    auto parser = colors();
    for (auto mode : {eargs::parse_mode::strict, eargs::parse_mode::permissive}) {
        for (const auto* spelling : {"red", "r"}) {
            ASSERT_TRUE(parser.parse(std::string("--color=") + spelling, mode));
            EXPECT_EQ(parser.get<Color>("c"), Color::red);
            EXPECT_EQ(parser.get<std::string>("color"), spelling);
        }
        ASSERT_TRUE(parser.parse("-c blue", mode));
        EXPECT_EQ(parser.get<Color>("color"), Color::blue);
    }
}

TEST(enum_choices, invalid_values_do_not_publish_partial_or_previous_results) {
    eargs::parser parser = {{
        {{"name"}, "name", eargs::string, false},
        {{"color"}, "color", eargs::choices<Color>{{{"red", Color::red}}, Color::unknown}}
    }};
    for (auto mode : {eargs::parse_mode::strict, eargs::parse_mode::permissive}) {
        ASSERT_TRUE(parser.parse("--name previous --color red", mode));
        for (const auto* value : {"green", ""}) {
            EXPECT_FALSE(parser.parse(std::string("--name partial --color=") + value, mode));
            EXPECT_EQ(parser.error(), "invalid value for --color (choices: red)");
            EXPECT_FALSE(parser.contains("color"));
            EXPECT_TRUE(parser.get<std::string>("name").empty());
            EXPECT_EQ(parser.get<Color>("color"), Color::unknown);
        }
    }
}

TEST(enum_choices, absent_optional_value_uses_only_an_explicit_default) {
    auto parser = colors();
    EXPECT_EQ(parser.get<Color>("color"), Color::unknown);
    EXPECT_FALSE(parser.contains("color"));
    ASSERT_TRUE(parser.parse("--color red"));
    EXPECT_FALSE(parser.parse(""));
    EXPECT_EQ(parser.get<Color>("color"), Color::unknown);
    eargs::parser no_default = {{{"color"}, "color", eargs::choices<Color>{{{"red", Color::red}}}}};
    EXPECT_THROW(no_default.get<Color>("color"), std::logic_error);
    EXPECT_THROW(no_default.get<Color>("missing"), std::out_of_range);
}

TEST(enum_choices, required_value_is_not_satisfied_by_a_default) {
    eargs::parser parser = {{
        {{"color"}, "color", eargs::choices<Color>{{{"red", Color::red}}, Color::unknown}, true},
        {{"help"}, "help", eargs::empty, false}
    }};
    EXPECT_FALSE(parser.parse("--help"));
    EXPECT_EQ(parser.error(), "missing required option: color");
}

TEST(enum_choices, rejects_wrong_enum_types_and_non_enum_options) {
    auto parser = colors();
    EXPECT_THROW(parser.get<Other>("color"), std::logic_error);
    ASSERT_TRUE(parser.parse("--color red"));
    EXPECT_THROW(parser.get<Other>("color"), std::logic_error);
    eargs::parser strings = {{{"name"}, "name", eargs::string, false}};
    EXPECT_THROW(strings.get<Color>("name"), std::logic_error);
}

TEST(enum_choices, preserves_full_enum_values_without_integer_conversion) {
    eargs::parser parser = {{
        {{"wide"}, "wide", eargs::choices<Wide>{{{"large", Wide::large}}}},
        {{"signed"}, "signed", eargs::choices<Signed>{{{"negative", Signed::negative}}}}
    }};
    ASSERT_TRUE(parser.parse("--wide large --signed negative"));
    EXPECT_EQ(parser.get<Wide>("wide"), Wide::large);
    EXPECT_EQ(parser.get<Signed>("signed"), Signed::negative);
}

TEST(enum_choices, definitions_own_names_and_reject_empty_or_duplicate_tables) {
    std::string spelling = "red";
    eargs::parser parser = {{{"color"}, "color", eargs::choices<Color>{{{spelling, Color::red}}}}};
    spelling = "changed";
    ASSERT_TRUE(parser.parse("--color red"));
    EXPECT_EQ(parser.get<Color>("color"), Color::red);
    EXPECT_THROW((eargs::option{{"color"}, "color", eargs::choices<Color>{}}), std::invalid_argument);
    EXPECT_THROW((eargs::option{{"color"}, "color", eargs::choices<Color>{{
        {"red", Color::red}, {"red", Color::blue}}}}), std::invalid_argument);
    EXPECT_THROW((eargs::parser{eargs::option{{"color"}, "color", eargs::enumeration, false}}),
                 std::invalid_argument);
}

TEST(enum_choices, explicit_empty_spelling_is_distinct_from_an_absent_option) {
    eargs::parser parser = {{{"color"}, "color",
        eargs::choices<Color>{{{"", Color::blue}, {"red", Color::red}}, Color::unknown}}};
    EXPECT_EQ(parser.get<Color>("color"), Color::unknown);
    ASSERT_TRUE(parser.parse("--color="));
    EXPECT_TRUE(parser.contains("color"));
    EXPECT_EQ(parser.get<Color>("color"), Color::blue);
}

TEST(enum_choices, completion_uses_option_aliases_prefixes_and_declaration_order) {
    auto parser = colors();
    EXPECT_EQ(parser.complete("color"), (std::vector<std::string>{"red", "r", "blue"}));
    EXPECT_EQ(parser.complete("c", "r"), (std::vector<std::string>{"red", "r"}));
    EXPECT_TRUE(parser.complete("color", "R").empty());
    EXPECT_TRUE(parser.complete("missing").empty());
    ASSERT_TRUE(parser.parse("--color red"));
    EXPECT_EQ(parser.complete("color", "b"), (std::vector<std::string>{"blue"}));
}

TEST(enum_choices, help_lists_declared_values_including_aliases) {
    auto parser = colors();
    testing::internal::CaptureStdout();
    parser.print_help();
    const auto help = testing::internal::GetCapturedStdout();
    EXPECT_NE(help.find("choices: red, r, blue"), std::string::npos);
}
