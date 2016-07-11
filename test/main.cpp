/*
 * main.cpp -- test the zip wrapper functions
 *
 * Copyright (c) 2013-2015 David Demelier <markand@malikania.fr>
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
using namespace libzip::source;

/*
 * Sources.
 * ------------------------------------------------------------------
 */

TEST(Source, file)
{
    remove("output.zip");

    try {
        Archive archive("output.zip", ZIP_CREATE);

        archive.add(file(DIRECTORY "data.txt"), "data.txt");
    } catch (const std::exception &ex) {
        FAIL() << ex.what();
    }

    try {
        Archive archive("output.zip");

        auto stats = archive.stat("data.txt");
        auto file = archive.open("data.txt");
        auto content = file.read(stats.size);

        ASSERT_EQ("abcdef\n", content);
    } catch (const std::exception &ex) {
        FAIL() << ex.what();
    }
}

TEST(Source, buffer)
{
    remove("output.zip");

    try {
        Archive archive("output.zip", ZIP_CREATE);

        archive.add(buffer("abcdef"), "data.txt");
    } catch (const std::exception &ex) {
        FAIL() << ex.what();
    }

    try {
        Archive archive("output.zip");

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

TEST(Write, simple)
{
    remove("output.zip");

    // Open first and save some data.
    try {
        Archive archive("output.zip", ZIP_CREATE);

        archive.add(buffer("hello world!"), "DATA");
    } catch (const std::exception &ex) {
        FAIL() << ex.what();
    }

    try {
        Archive archive("output.zip");

        auto stats = archive.stat("DATA");
        auto file = archive.open("DATA");
        auto content = file.read(stats.size);

        ASSERT_EQ(static_cast<decltype(stats.size)>(12), stats.size);
        ASSERT_EQ("hello world!", content);
    } catch (const std::exception &ex) {
        FAIL() << ex.what();
    }
}

TEST(Write, notexist)
{
    remove("output.zip");

    try {
        Archive archive("output.zip", ZIP_CREATE);

        /*
         * According to the libzip, adding a file that does not exists
         * on the disk is not an error.
         */
        archive.add(file("file_not_exist"), "FILE");
    } catch (const std::exception &ex) {
        FAIL() << ex.what();
    }
}

/*
 * Reading.
 * ------------------------------------------------------------------
 */

class ReadingTest : public testing::Test {
protected:
    Archive m_archive;

public:
    ReadingTest()
        : m_archive(DIRECTORY "stats.zip")
    {
    }
};

TEST_F(ReadingTest, numEntries)
{
    ASSERT_EQ(static_cast<Int64>(4), m_archive.numEntries());
}

TEST_F(ReadingTest, stat)
{
    try {
        Stat stats = m_archive.stat("README");

        ASSERT_EQ(static_cast<decltype(stats.size)>(15), stats.size);
        ASSERT_STREQ("README", stats.name);
    } catch (const std::exception &ex) {
        FAIL() << ex.what();
    }
}

TEST_F(ReadingTest, exists)
{
    ASSERT_TRUE(m_archive.exists("README"));
}

TEST_F(ReadingTest, read)
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

TEST_F(ReadingTest, increment)
{
    {
        Archive::iterator it = m_archive.begin();

        ASSERT_STREQ("README", (*it++).name);
    }

    {
        Archive::iterator it = m_archive.begin();

        ASSERT_STREQ("INSTALL", (*++it).name);
    }

    {
        Archive::iterator it = m_archive.begin() + 1;

        ASSERT_STREQ("INSTALL", (*it).name);
    }
}

TEST_F(ReadingTest, decrement)
{
    {
        Archive::iterator it = m_archive.begin() + 1;

        ASSERT_STREQ("INSTALL", (*it--).name);
    }

    {
        Archive::iterator it = m_archive.begin() + 1;

        ASSERT_STREQ("README", (*--it).name);
    }

    {
        Archive::iterator it = m_archive.end() - 1;

        ASSERT_STREQ("doc/REFMAN", (*it).name);
    }
}

TEST_F(ReadingTest, constIncrement)
{
    {
        Archive::const_iterator it = m_archive.cbegin();

        ASSERT_STREQ("README", (*it++).name);
    }

    {
        Archive::const_iterator it = m_archive.cbegin();

        ASSERT_STREQ("INSTALL", (*++it).name);
    }

    {
        Archive::const_iterator it = m_archive.cbegin() + 1;

        ASSERT_STREQ("INSTALL", (*it).name);
    }
}

TEST_F(ReadingTest, constDecrement)
{
    {
        Archive::const_iterator it = m_archive.cbegin() + 1;

        ASSERT_STREQ("INSTALL", (*it--).name);
    }

    {
        Archive::const_iterator it = m_archive.cbegin() + 1;

        ASSERT_STREQ("README", (*--it).name);
    }

    {
        Archive::const_iterator it = m_archive.cend() - 1;

        ASSERT_STREQ("doc/REFMAN", (*it).name);
    }
}

TEST_F(ReadingTest, access)
{
    {
        Archive::iterator it = m_archive.begin();

        ASSERT_STREQ("README", it->name);
        ASSERT_STREQ("INSTALL", it[1].name);
    }

    {
        Archive::const_iterator it = m_archive.cbegin();

        ASSERT_STREQ("README", it->name);
        ASSERT_STREQ("INSTALL", it[1].name);
    }
}

TEST_F(ReadingTest, loop)
{
    std::vector<std::string> names{"README", "INSTALL", "doc/", "doc/REFMAN"};
    int i = 0;

    for (const Stat &s : m_archive)
        ASSERT_STREQ(names[i++].c_str(), s.name);
}

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
