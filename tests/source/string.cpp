#include <gtest/gtest.h>
#include <eargs/parser.hpp>

TEST(string, required_not_exists) {
    eargs::parser parser = {
        {{"name"}, "get person name", eargs::hex, true}
    };

    ASSERT_TRUE(parser.parse("-some_variable") == 0);
};

TEST(string, fallback_applies_to_absent_and_unknown_options) {
    eargs::parser parser = {{{"n", "name"}, "name", eargs::string, false}};
    EXPECT_EQ(parser.get<std::string>("name", "fallback"), "fallback");
    EXPECT_EQ(parser.get<std::string>("missing", "fallback"), "fallback");
    EXPECT_TRUE(parser.get<std::string>("name").empty());
    ASSERT_TRUE(parser.parse("--name Alex"));
    EXPECT_EQ(parser.get<std::string>("n", "fallback"), "Alex");
}

TEST(string, explicit_empty_values_do_not_use_fallback) {
    eargs::parser parser = {{{"n", "name"}, "name", eargs::string, false}};
    ASSERT_TRUE(parser.parse("--name="));
    EXPECT_TRUE(parser.contains("name"));
    EXPECT_TRUE(parser.get<std::string>("n", "fallback").empty());
    const char* args[] = {"test", "-n", ""};
    ASSERT_TRUE(parser.parse(args, 3));
    EXPECT_TRUE(parser.get<std::string>("name", "fallback").empty());
}

TEST(string, fallback_is_used_after_values_are_reset_and_copies_string_views) {
    eargs::parser parser = {{{"name"}, "name", eargs::string, false}};
    ASSERT_TRUE(parser.parse("--name Alex"));
    EXPECT_FALSE(parser.parse("--name"));
    const std::string storage = "fallback-suffix";
    EXPECT_EQ(parser.get<std::string>("name", std::string_view(storage).substr(0, 8)), "fallback");
}

TEST(string, required) {
    eargs::parser parser = {
        {{"name"}, "get person name", eargs::hex, true}
    };

    ASSERT_TRUE(parser.parse("-name Alex") != 0);
    ASSERT_EQ(parser.get<std::string>("name"), "Alex");
};

TEST(string, not_required) {
    eargs::parser parser = {
        {{"name"}, "get person name", eargs::hex, false}
    };

    ASSERT_TRUE(parser.parse("-name Alex") != 0);
    ASSERT_EQ(parser.get<std::string>("name"), "Alex");
};
