#include <gtest/gtest.h>
#include <eargs/parser.hpp>

TEST(string, required_not_exists) {
    eargs::parser parser = {
        {{"name"}, "get person name", eargs::hex, true}
    };

    ASSERT_TRUE(parser.parse("-some_variable") == 0);
};

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