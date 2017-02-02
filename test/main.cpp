/*
 * main.cpp -- test the zip wrapper functions
 *
 * Copyright (c) 2013-2017 David Demelier <markand@malikania.fr>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <gtest/gtest.h>

#include <zip.hpp>

using namespace libzip;

/*
 * Sources.
 * ------------------------------------------------------------------
 */

TEST(sources, file)
{
    remove("output.zip");

    try {
        archive archive("output.zip", ZIP_CREATE);
        archive.add(source_file(DIRECTORY "data.txt"), "data.txt");
    } catch (const std::exception &ex) {
        FAIL() << ex.what();
    }

    try {
        archive archive("output.zip");

        auto stats = archive.stat("data.txt");
        auto file = archive.open("data.txt");
        auto content = file.read(stats.size);

        ASSERT_EQ("abcdef\n", content);
    } catch (const std::exception &ex) {
        FAIL() << ex.what();
    }
}

TEST(sources, buffer)
{
    remove("output.zip");

    try {
        archive archive("output.zip", ZIP_CREATE);
        archive.add(source_buffer("abcdef"), "data.txt");
    } catch (const std::exception &ex) {
        FAIL() << ex.what();
    }

    try {
        archive archive("output.zip");

        auto stats = archive.stat("data.txt");
        auto file = archive.open("data.txt");
        auto content = file.read(stats.size);

        ASSERT_EQ("abcdef", content);
    } catch (const std::exception &ex) {
        FAIL() << ex.what();
    }
}

/*
 * Write.
 * ------------------------------------------------------------------
 */

TEST(write, simple)
{
    remove("output.zip");

    // Open first and save some data.
    try {
        archive archive("output.zip", ZIP_CREATE);
        archive.add(source_buffer("hello world!"), "DATA");
    } catch (const std::exception &ex) {
        FAIL() << ex.what();
    }

    try {
        archive archive("output.zip");

        auto stats = archive.stat("DATA");
        auto file = archive.open("DATA");
        auto content = file.read(stats.size);

        ASSERT_EQ(static_cast<decltype(stats.size)>(12), stats.size);
        ASSERT_EQ("hello world!", content);
    } catch (const std::exception &ex) {
        FAIL() << ex.what();
    }
}

TEST(write, notexist)
{
    remove("output.zip");

    try {
        archive archive("output.zip", ZIP_CREATE);

        /*
         * According to the libzip, adding a file that does not exists
         * on the disk is not an error.
         */
        archive.add(source_file("file_not_exist"), "FILE");
    } catch (const std::exception &ex) {
        FAIL() << ex.what();
    }
}

/*
 * Reading.
 * ------------------------------------------------------------------
 */

class reading_test : public testing::Test {
protected:
    archive m_archive;

public:
    reading_test()
        : m_archive(DIRECTORY "stats.zip")
    {
    }
};

TEST_F(reading_test, num_entries)
{
    ASSERT_EQ(static_cast<int64_t>(4), m_archive.num_entries());
}

TEST_F(reading_test, stat)
{
    try {
        auto stat = m_archive.stat("README");

        ASSERT_EQ(static_cast<decltype(stat.size)>(15), stat.size);
        ASSERT_STREQ("README", stat.name);
    } catch (const std::exception &ex) {
        FAIL() << ex.what();
    }
}

TEST_F(reading_test, exists)
{
    ASSERT_TRUE(m_archive.exists("README"));
}

TEST_F(reading_test, read)
{
    try {
        auto file = m_archive.open("README");
        auto stats = m_archive.stat("README");
        auto text = file.read(stats.size);

        ASSERT_EQ("This is a test\n", text);
    } catch (const std::exception &ex) {
        FAIL() << ex.what();
    }
}

TEST_F(reading_test, increment)
{
    {
        archive::iterator it = m_archive.begin();

        ASSERT_STREQ("README", (*it++).name);
    }

    {
        archive::iterator it = m_archive.begin();

        ASSERT_STREQ("INSTALL", (*++it).name);
    }

    {
        archive::iterator it = m_archive.begin() + 1;

        ASSERT_STREQ("INSTALL", (*it).name);
    }
}

TEST_F(reading_test, decrement)
{
    {
        archive::iterator it = m_archive.begin() + 1;

        ASSERT_STREQ("INSTALL", (*it--).name);
    }

    {
        archive::iterator it = m_archive.begin() + 1;

        ASSERT_STREQ("README", (*--it).name);
    }

    {
        archive::iterator it = m_archive.end() - 1;

        ASSERT_STREQ("doc/REFMAN", (*it).name);
    }
}

TEST_F(reading_test, constIncrement)
{
    {
        archive::const_iterator it = m_archive.cbegin();

        ASSERT_STREQ("README", (*it++).name);
    }

    {
        archive::const_iterator it = m_archive.cbegin();

        ASSERT_STREQ("INSTALL", (*++it).name);
    }

    {
        archive::const_iterator it = m_archive.cbegin() + 1;

        ASSERT_STREQ("INSTALL", (*it).name);
    }
}

TEST_F(reading_test, constDecrement)
{
    {
        archive::const_iterator it = m_archive.cbegin() + 1;

        ASSERT_STREQ("INSTALL", (*it--).name);
    }

    {
        archive::const_iterator it = m_archive.cbegin() + 1;

        ASSERT_STREQ("README", (*--it).name);
    }

    {
        archive::const_iterator it = m_archive.cend() - 1;

        ASSERT_STREQ("doc/REFMAN", (*it).name);
    }
}

TEST_F(reading_test, access)
{
    {
        archive::iterator it = m_archive.begin();

        ASSERT_STREQ("README", it->name);
        ASSERT_STREQ("INSTALL", it[1].name);
    }

    {
        archive::const_iterator it = m_archive.cbegin();

        ASSERT_STREQ("README", it->name);
        ASSERT_STREQ("INSTALL", it[1].name);
    }
}

TEST_F(reading_test, loop)
{
    std::vector<std::string> names{"README", "INSTALL", "doc/", "doc/REFMAN"};
    int i = 0;

    for (const auto& s : m_archive) {
        ASSERT_STREQ(names[i++].c_str(), s.name);
    }
}

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
