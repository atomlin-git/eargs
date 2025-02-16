#include <gtest/gtest.h>
#include <eargs/parser.hpp>

TEST(hex, required_not_exists) {
    eargs::parser parser = {
        {{"addr"}, "get hex addr", eargs::hex, true}
    };

    ASSERT_TRUE(parser.parse("-some_variable") == 0);
};

TEST(hex, required) {
    eargs::parser parser = {
        {{"addr"}, "get hex addr", eargs::hex, true}
    };

    ASSERT_TRUE(parser.parse("-addr 0xFF") != 0);
    ASSERT_EQ(parser.get<int>("addr"), 255);
};

TEST(hex, not_required) {
    eargs::parser parser = {
        {{"addr"}, "get hex addr", eargs::hex, false}
    };

    ASSERT_TRUE(parser.parse("-addr 0xFF") != 0);
    ASSERT_EQ(parser.get<int>("addr"), 255);
};