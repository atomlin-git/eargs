#include <gtest/gtest.h>
#include <eargs/parser.hpp>

TEST(integer, required_not_exists) {
    eargs::parser parser = {
        {{"phone"}, "get phonenumber", eargs::integer, true}
    };

    ASSERT_TRUE(parser.parse("-some_variable") == 0);
};

TEST(integer, required) {
    eargs::parser parser = {
        {{"phone"}, "get phonenumber", eargs::integer, true}
    };

    ASSERT_TRUE(parser.parse("-phone 1337") != 0);
    ASSERT_EQ(parser.get<int>("phone"), 1337);
};

TEST(integer, not_required) {
    eargs::parser parser = {
        {{"phone"}, "get phonenumber", eargs::integer, false}
    };

    ASSERT_TRUE(parser.parse("-phone 1337") != 0);
    ASSERT_EQ(parser.get<int>("phone"), 1337);
};