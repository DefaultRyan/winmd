#include "pch.h"
#include <winmd_writer.h>

using namespace winmd::writer;

TEST_CASE("string_heap")
{
    {
        string_heap strings;
        REQUIRE(strings.save_size() == 1);
        {
            auto elem = strings.insert("");
            REQUIRE(elem.offset == 0);
            REQUIRE(strings.save_size() == 1);
        }
        {
            auto elem = strings.insert("foo");
            REQUIRE(elem.offset == 1);
            REQUIRE(strings.save_size() == 5);
        }
    }
}