#include <gtest/gtest.h>
#include <eargs/parser.hpp>
#include <initializer_list>
#include <vector>

namespace {
bool argv_parse(eargs::parser& parser, std::initializer_list<const char*> tokens,
                eargs::parse_mode mode = eargs::parse_mode::permissive) {
    std::vector<const char*> args{"eargs-test"};
    args.insert(args.end(), tokens.begin(), tokens.end());
    return parser.parse(args.data(), static_cast<int>(args.size()), mode);
}
}

TEST(parser, rejects_conflicting_option_definitions) {
    const std::list<eargs::option> options{
        {{"n", "name"}, "name", eargs::string, false},
        {{"other", "name"}, "other", eargs::integer, false}
    };
    try {
        eargs::parser parser(options);
        FAIL() << "Conflicting option names must be rejected during construction";
    } catch (const std::logic_error& error) {
        EXPECT_STREQ(error.what(), "Duplicate option name: name");
    }
}

TEST(parser, rejects_duplicate_aliases_in_option_list) {
    const std::list<eargs::option> options{
        {{"n", "name", "n"}, "name", eargs::string, false}
    };
    EXPECT_THROW(eargs::parser{options}, std::logic_error);
}

TEST(parser, rejects_duplicate_aliases_in_single_option_constructor) {
    const eargs::option option{{"n", "name", "n"}, "name", eargs::string, false};
    EXPECT_THROW(eargs::parser{option}, std::logic_error);
}

TEST(parser, accepts_single_and_double_dash_aliases) {
    eargs::parser parser = {{
        {{"n", "name"}, "name", eargs::string, true},
        {{"c", "count"}, "count", eargs::integer, true}
    }};
    ASSERT_TRUE(argv_parse(parser, {"-n", "Alex", "--count", "42"}, eargs::parse_mode::strict));
    EXPECT_EQ(parser.get<std::string>("name"), "Alex");
    EXPECT_EQ(parser.get<int>("c"), 42);
    ASSERT_TRUE(argv_parse(parser, {"--n", "Bob", "-count", "7"}, eargs::parse_mode::strict));
    EXPECT_EQ(parser.get<std::string>("n"), "Bob");
    EXPECT_EQ(parser.get<int>("count"), 7);
}

TEST(parser, consumes_each_option_once_regardless_of_declaration_order) {
    eargs::parser parser = {{
        {{"first"}, "first", eargs::string, true},
        {{"second"}, "second", eargs::string, true},
        {{"third"}, "third", eargs::string, true}
    }};
    ASSERT_TRUE(parser.parse("-first one -third three -second two", eargs::parse_mode::strict));
    EXPECT_EQ(parser.get<std::string>("first"), "one");
    EXPECT_EQ(parser.get<std::string>("second"), "two");
    EXPECT_EQ(parser.get<std::string>("third"), "three");
}

TEST(parser, strict_unknown_option_does_not_publish_partial_values) {
    eargs::parser parser = {{{"name"}, "name", eargs::string, false}};
    EXPECT_FALSE(parser.parse("--name Alex --unknown value", eargs::parse_mode::strict));
    EXPECT_EQ(parser.error(), "Unknown option: --unknown");
    EXPECT_FALSE(parser.contains("name"));
    EXPECT_TRUE(parser.get<std::string>("name").empty());
}

TEST(parser, permissive_mode_preserves_unknown_argument_handling) {
    eargs::parser parser = {{{"name"}, "name", eargs::string, false}};
    ASSERT_TRUE(parser.parse("--unknown value --name Alex"));
    EXPECT_EQ(parser.get<std::string>("name"), "Alex");
    EXPECT_TRUE(parser.error().empty());
}

TEST(parser, strict_rejects_repeated_aliases) {
    eargs::parser parser = {{{"n", "name"}, "name", eargs::string, false}};
    EXPECT_FALSE(parser.parse("-n Alex --name Bob", eargs::parse_mode::strict));
    EXPECT_EQ(parser.error(), "Repeated option: --name");
    EXPECT_FALSE(parser.contains("name"));
}

TEST(parser, permissive_mode_preserves_last_value_wins) {
    eargs::parser parser = {{{"n", "name"}, "name", eargs::string, false}};
    ASSERT_TRUE(parser.parse("-n Alex --name Bob"));
    EXPECT_EQ(parser.get<std::string>("n"), "Bob");
}

TEST(parser, value_options_reject_missing_values_in_both_modes) {
    for (auto mode : {eargs::parse_mode::permissive, eargs::parse_mode::strict}) {
        eargs::parser parser = {{
            {{"name"}, "name", eargs::string, false},
            {{"h", "help"}, "help", eargs::empty, false}
        }};
        EXPECT_FALSE(parser.parse("--name", mode));
        EXPECT_EQ(parser.error(), "Missing value for --name");
        EXPECT_FALSE(parser.parse("--name --help", mode));
        EXPECT_FALSE(parser.contains("help"));
    }
}

TEST(parser, empty_flags_do_not_consume_following_options) {
    eargs::parser parser = {{
        {{"h", "help"}, "help", eargs::empty, false},
        {{"name"}, "name", eargs::string, false}
    }};
    ASSERT_TRUE(parser.parse("--help -name Alex", eargs::parse_mode::strict));
    EXPECT_TRUE(parser.contains("h"));
    EXPECT_EQ(parser.get<std::string>("help"), "empty");
    EXPECT_EQ(parser.get<std::string>("name"), "Alex");
    ASSERT_TRUE(parser.parse("-h", eargs::parse_mode::strict));
    EXPECT_FALSE(parser.parse("--help=yes", eargs::parse_mode::strict));
}

TEST(parser, strict_rejects_positional_arguments) {
    eargs::parser parser = {{{"name"}, "name", eargs::string, false}};
    EXPECT_FALSE(parser.parse("value --name Alex", eargs::parse_mode::strict));
    EXPECT_EQ(parser.error(), "Unexpected positional argument");
}

TEST(parser, accepts_negative_numeric_values) {
    eargs::parser parser = {{{"count"}, "count", eargs::integer, true}};
    ASSERT_TRUE(parser.parse("--count -42", eargs::parse_mode::strict));
    EXPECT_EQ(parser.get<int>("count"), -42);
}

TEST(parser, accepts_inline_values_and_dash_prefixed_strings) {
    eargs::parser parser = {{{"name"}, "name", eargs::string, true}};
    ASSERT_TRUE(parser.parse("--name=-file", eargs::parse_mode::strict));
    EXPECT_EQ(parser.get<std::string>("name"), "-file");
    ASSERT_TRUE(parser.parse("-name=value=other", eargs::parse_mode::strict));
    EXPECT_EQ(parser.get<std::string>("name"), "value=other");
}

TEST(parser, errors_do_not_echo_inline_values) {
    eargs::parser parser = {{{"name"}, "name", eargs::string, false}};
    EXPECT_FALSE(parser.parse("--name=first --name=second", eargs::parse_mode::strict));
    EXPECT_EQ(parser.error(), "Repeated option: --name");
}

TEST(parser, argv_preserves_spaces_and_empty_required_values) {
    eargs::parser parser = {{{"name"}, "name", eargs::string, true}};
    ASSERT_TRUE(argv_parse(parser, {"--name", "Ada Lovelace"}, eargs::parse_mode::strict));
    EXPECT_EQ(parser.get<std::string>("name"), "Ada Lovelace");
    ASSERT_TRUE(argv_parse(parser, {"--name", ""}, eargs::parse_mode::strict));
    EXPECT_TRUE(parser.contains("name"));
    EXPECT_TRUE(parser.get<std::string>("name").empty());
    ASSERT_TRUE(parser.parse("--name=", eargs::parse_mode::strict));
    EXPECT_TRUE(parser.contains("name"));
}

TEST(parser, string_input_skips_repeated_whitespace) {
    eargs::parser parser = {{{"name"}, "name", eargs::string, true}};
    ASSERT_TRUE(parser.parse(" \t --name  Alex \n ", eargs::parse_mode::strict));
    EXPECT_EQ(parser.get<std::string>("name"), "Alex");
}

TEST(parser, argv_parsing_resets_values_between_calls) {
    eargs::parser parser = {{
        {{"name"}, "name", eargs::string, false},
        {{"help"}, "help", eargs::empty, false}
    }};
    ASSERT_TRUE(argv_parse(parser, {"--name", "Alex"}));
    ASSERT_TRUE(argv_parse(parser, {"--help"}));
    EXPECT_FALSE(parser.contains("name"));
    EXPECT_TRUE(parser.get<std::string>("name").empty());
    EXPECT_TRUE(parser.contains("help"));
    EXPECT_FALSE(argv_parse(parser, {}));
    EXPECT_FALSE(parser.contains("help"));
}

TEST(parser, failed_argv_parse_does_not_retain_previous_values) {
    eargs::parser parser = {{{"name"}, "name", eargs::string, true}};
    ASSERT_TRUE(argv_parse(parser, {"--name", "Alex"}, eargs::parse_mode::strict));
    EXPECT_FALSE(argv_parse(parser, {"--name"}, eargs::parse_mode::strict));
    EXPECT_FALSE(parser.contains("name"));
    ASSERT_TRUE(parser.parse("--name Bob", eargs::parse_mode::strict));
    EXPECT_TRUE(parser.error().empty());
}

TEST(parser, missing_required_option_reports_its_name) {
    eargs::parser parser = {{
        {{"name"}, "name", eargs::string, true},
        {{"help"}, "help", eargs::empty, false}
    }};
    EXPECT_FALSE(parser.parse("--help", eargs::parse_mode::strict));
    EXPECT_EQ(parser.error(), "Missing required option: name");
}

TEST(parser, mutable_argv_remains_supported) {
    eargs::parser parser = {{{"name"}, "name", eargs::string, true}};
    char program[] = "test", option[] = "--name", value[] = "Alex";
    char* args[] = {program, option, value};
    ASSERT_TRUE(parser.parse(args, 3, eargs::parse_mode::strict));
    EXPECT_EQ(parser.get<std::string>("name"), "Alex");
}

TEST(parser, null_argument_vectors_are_rejected) {
    eargs::parser parser = {{{"name"}, "name", eargs::string, false}};
    EXPECT_FALSE(parser.parse(nullptr, 2, eargs::parse_mode::strict));
    const char* args[] = {"test", nullptr};
    EXPECT_FALSE(parser.parse(args, 2, eargs::parse_mode::strict));
    EXPECT_FALSE(parser.parse(""));
}
