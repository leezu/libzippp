/*
 * zip.hpp -- wrapper around libzip
 *
 * Copyright (c) 2013-2016 David Demelier <markand@malikania.fr>
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

#ifndef ZIP_HPP
#define ZIP_HPP

/**
 * \file zip.hpp
 * \brief Simple C++14 libzip wrapper.
 * \author David Demelier <markand@malikania.fr>
 */

#include <cassert>
#include <cerrno>
#include <cstring>
#include <functional>
#include <iterator>
#include <memory>
#include <string>

#include <zip.h>

/**
 * \brief The libzip namespace.
 */
namespace libzip {

/**
 * \brief zip_stat typedef.
 */
using Stat = struct zip_stat;

/**
 * \brief zip_source_cmd typedef.
 */
using SourceCommand = enum zip_source_cmd;

/**
 * \brief zip_source_cmd typedef.
 */
using Callback = zip_source_callback;

/**
 * \brief zip_flags_t typedef.
 */
using Flags = zip_flags_t;

/**
 * \brief zip_int8_t typedef.
 */
using Int8 = zip_int8_t;

/**
 * \brief zip_uint8_t typedef.
 */
using Uint8 = zip_uint8_t;

/**
 * \brief zip_int16_t typedef.
 */
using Int16 = zip_int16_t;

/**
 * \brief zip_uint16_t typedef.
 */
using Uint16 = zip_uint16_t;

/**
 * \brief zip_int32_t typedef.
 */
using Int32 = zip_int32_t;

/**
 * \brief zip_uint32_t typedef.
 */
using Uint32 = zip_uint32_t;

/**
 * \brief zip_int64_t typedef.
 */
using Int64 = zip_int64_t;

/**
 * \brief zip_uint64_t typedef.
 */
using Uint64 = zip_uint64_t;

/**
 * \class File
 * \brief File for reading
 */
class File {
private:
    std::unique_ptr<struct zip_file, int (*)(struct zip_file *)> m_handle;

    File(const File &) = delete;
    File &operator=(const File &) = delete;

public:
    /**
     * Create a File with a zip_file structure.
     *
     * \param file the file ready to be used
     */
    inline File(struct zip_file *file) noexcept
        : m_handle(file, zip_fclose)
    {
    }

    /**
     * Move constructor defaulted.
     *
     * \param other the other File
     */
    File(File &&other) noexcept = default;

    /**
     * Move operator defaulted.
     *
     * \return *this
     */
    File &operator=(File &&) noexcept = default;

    /**
     * Read some data.
     *
     * \param data the destination buffer
     * \param length the length
     * \return the number of bytes written or -1 on failure
     */
    inline int read(void *data, Uint64 length) noexcept
    {
        return zip_fread(m_handle.get(), data, length);
    }

    /**
     * Read some data to a fixed size array.
     *
     * \param data the array
     * \return the number of bytes written or -1 on failure
     */
    template <size_t Size>
    inline int read(char (&data)[Size]) noexcept
    {
        return read(data, Size);
    }

    /**
     * Optimized function for reading all characters with only one allocation.
     * Ideal for combining with Archive::stat.
     *
     * \param length the length of the file
     * \return the whole string
     * \see Archive::stat
     */
    std::string read(Uint64 length)
    {
        std::string result;

        result.resize(length);
        auto count = read(&result[0], length);

        if (count < 0)
            return "";

        result.resize(count);

        return result;
    }
};

/**
 * \brief Source creation for adding files.
 *
 * This function must be passed through Archive::add, it will be called to create the appropriate zip_source
 * inside the function.
 *
 * The user should not call the function itself as it's the responsability of libzip to destroy the
 * zip_source structure.
 *
 * \see source::buffer
 * \see source::file
 */
using Source = std::function<struct zip_source * (struct zip *)>;

/**
 * \brief Source namespace, contains various ways for adding file to an archive.
 */
namespace source {

/**
 * Add a file to the archive using a binary buffer.
 *
 * \param data the buffer
 * \return the source to add
 */
inline Source buffer(std::string data) noexcept
{
    return [data = std::move(data)] (struct zip *archive) -> struct zip_source * {
        auto size = data.size();
        auto ptr = static_cast<char *>(std::malloc(size));

        if (ptr == nullptr)
            throw std::runtime_error(std::strerror(errno));

        std::memcpy(ptr, data.data(), size);

        auto src = zip_source_buffer(archive, ptr, size, 1);

        if (src == nullptr) {
            std::free(ptr);
            throw std::runtime_error(zip_strerror(archive));
        }

        return src;
    };
}

/**
 * Add a file to the archive from the disk.
 *
 * \param path the path to the file
 * \param start the position where to start
 * \param length the number of bytes to copy
 * \return the source to add
 */
inline Source file(std::string path, Uint64 start = 0, Int64 length = -1) noexcept
{
    return [path = std::move(path), start, length] (struct zip *archive) -> struct zip_source * {
        auto src = zip_source_file(archive, path.c_str(), start, length);

        if (src == nullptr)
            throw std::runtime_error(zip_strerror(archive));

        return src;
    };
}

} // !source

/**
 * \class StatPtr
 * \brief Wrapper for Stat as pointer
 */
class StatPtr {
private:
    Stat m_stat;

public:
    /**
     * Constructor.
     *
     * \param stat the file stat
     */
    inline StatPtr(Stat stat) noexcept
        : m_stat(stat)
    {
    }

    /**
     * Get the reference.
     *
     * \return the reference
     */
    inline Stat &operator*() noexcept
    {
        return m_stat;
    }

    /**
     * Get the reference.
     *
     * \return the reference
     */
    inline const Stat &operator*() const noexcept
    {
        return m_stat;
    }

    /**
     * Access the object.
     *
     * \return the pointer
     */
    inline Stat *operator->() noexcept
    {
        return &m_stat;
    }

    /**
     * Access the object.
     *
     * \return the pointer
     */
    inline const Stat *operator->() const noexcept
    {
        return &m_stat;
    }
};

/**
 * \class Archive
 * \brief Safe wrapper on the struct zip structure
 */
class Archive {
private:
    using Handle = std::unique_ptr<struct zip, int (*)(struct zip *)>;

    Handle m_handle;

    Archive(const Archive &) = delete;
    Archive &operator=(const Archive &) = delete;

public:
    /**
     * \brief Base iterator class
     */
    class Iterator : public std::iterator<std::random_access_iterator_tag, Stat> {
    private:
        friend class Archive;

        Archive *m_archive{nullptr};
        Uint64 m_index{0};
    
        inline Iterator(Archive *archive, Uint64 index = 0) noexcept
            : m_archive(archive)
            , m_index(index)
        {
        }
    
    public:
        /**
         * Default iterator.
         */
        Iterator() noexcept = default;
    
        /**
         * Dereference the iterator.
         *
         * \return the stat information
         */
        inline Stat operator*() const
        {
            assert(m_archive);
    
            return m_archive->stat(m_index);
        }
    
        /**
         * Dereference the iterator.
         *
         * \return the stat information as point
         */
        inline StatPtr operator->() const
        {
            assert(m_archive);
    
            return StatPtr(m_archive->stat(m_index));
        }
    
        /**
         * Post increment.
         *
         * \return this
         */
        inline Iterator &operator++() noexcept
        {
            ++ m_index;
    
            return *this;
        }
    
        /**
         * Pre increment.
         *
         * \return this
         */
        inline Iterator operator++(int) noexcept
        {
            Iterator save = *this;
    
            ++ m_index;
    
            return save;
        }
    
        /**
         * Post decrement.
         *
         * \return this
         */
        inline Iterator &operator--() noexcept
        {
            -- m_index;
    
            return *this;
        }
    
        /**
         * Pre decrement.
         *
         * \return this
         */
        inline Iterator operator--(int) noexcept
        {
            Iterator save = *this;
    
            -- m_index;
    
            return save;
        }
    
        /**
         * Increment.
         *
         * \param inc the number
         * \return the new iterator
         */
        inline Iterator operator+(int inc) const noexcept
        {
            return Iterator(m_archive, m_index + inc);
        }
    
        /**
         * Decrement.
         *
         * \param dec the number
         * \return the new iterator
         */
        inline Iterator operator-(int dec) const noexcept
        {
            return Iterator(m_archive, m_index - dec);
        }
    
        /**
         * Compare equality.
         * 
         * \param other the other iterator
         * \return true if same
         */
        inline bool operator==(const Iterator &other) const noexcept
        {
            return m_index == other.m_index;
        }
    
        /**
         * Compare equality.
         * 
         * \param other the other iterator
         * \return true if different
         */
        inline bool operator!=(const Iterator &other) const noexcept
        {
            return m_index != other.m_index;
        }
    
        /**
         * Access a stat information at the specified index.
         *
         * \pre must not be default-constructed
         * \param index the new index
         * \return stat information
         * \throw std::runtime_error on errors
         */
        inline Stat operator[](int index)
        {
            assert(m_archive);
    
            return m_archive->stat(m_index + index);
        }
    };

public:
    /**
     * Iterator conversion to Stat.
     */
    using value_type = Stat;

    /**
     * Reference is a copy of Stat.
     */
    using reference = Stat;

    /**
     * Const reference is a copy of Stat.
     */
    using const_refernce = Stat;

    /**
     * Pointer is a small wrapper
     */
    using pointer = StatPtr;

    /**
     * Type of difference.
     */
    using size_type = unsigned;

    /**
     * Random access iterator.
     */
    using iterator = Iterator;

    /**
     * Const random access iterator.
     */
    using const_iterator = Iterator;

public:
    /**
     * Open an archive on the disk.
     *
     * \param path the path
     * \param flags the optional flags
     * \throw std::runtime_error on errors
     */
    Archive(const std::string &path, Flags flags = 0)
        : m_handle(nullptr, nullptr)
    {
        int error;
        struct zip *archive = zip_open(path.c_str(), flags, &error);

        if (archive == nullptr) {
            char buf[128]{0};

            zip_error_to_str(buf, sizeof (buf), error, errno);

            throw std::runtime_error(buf);
        }

        m_handle = { archive, zip_close };
    }

    /**
     * Move constructor defaulted.
     *
     * \param other the other Archive
     */
    Archive(Archive &&other) noexcept = default;

    /**
     * Move operator defaulted.
     *
     * \param other the other Archive
     * \return *this
     */
    Archive &operator=(Archive &&other) noexcept = default;

    /**
     * Get an iterator to the beginning.
     *
     * \return the iterator
     */
    inline iterator begin() noexcept
    {
        return iterator(this);
    }

    /**
     * Overloaded function.
     *
     * \return the iterator
     */
    inline const_iterator begin() const noexcept
    {
        return const_iterator(const_cast<Archive *>(this));
    }

    /**
     * Overloaded function.
     *
     * \return the iterator
     */
    inline const_iterator cbegin() const noexcept
    {
        return const_iterator(const_cast<Archive *>(this));
    }

    /**
     * Get an iterator to the end.
     *
     * \return the iterator
     */
    inline iterator end() noexcept
    {
        return iterator(this, numEntries());
    }

    /**
     * Overloaded function.
     *
     * \return the iterator
     */
    inline const_iterator end() const noexcept
    {
        return const_iterator(const_cast<Archive *>(this), numEntries());
    }

    /**
     * Overloaded function.
     *
     * \return the iterator
     */
    inline const_iterator cend() const noexcept
    {
        return const_iterator(const_cast<Archive *>(this), numEntries());
    }

    /**
     * Set a comment on a file.
     *
     * \param index the file index in the archive
     * \param text the text or empty to remove the comment
     * \param flags the optional flags
     * \throw std::runtime_error on errors
     */
    void setFileComment(Uint64 index, const std::string &text = "", Flags flags = 0)
    {
        auto size = text.size();
        auto cstr = (size == 0) ? nullptr : text.c_str();

        if (zip_file_set_comment(m_handle.get(), index, cstr, size, flags) < 0)
            throw std::runtime_error(zip_strerror(m_handle.get()));
    }

    /**
     * Get a comment from a file.
     *
     * \param index the file index in the archive
     * \param flags the optional flags
     * \return the comment
     * \throw std::runtime_error on errors
     */
    std::string fileComment(Uint64 index, Flags flags = 0) const
    {
        zip_uint32_t length = 0;
        auto text = zip_file_get_comment(m_handle.get(), index, &length, flags);

        if (text == nullptr)
            throw std::runtime_error(zip_strerror(m_handle.get()));

        return std::string(text, length);
    }

    /**
     * Set the archive comment.
     *
     * \param comment the comment
     * \throw std::runtime_error on errors
     */
    void setComment(const std::string &comment)
    {
        if (zip_set_archive_comment(m_handle.get(), comment.c_str(), comment.size()) < 0)
            throw std::runtime_error(zip_strerror(m_handle.get()));
    }

    /**
     * Get the archive comment.
     *
     * \param flags the optional flags
     * \return the comment
     * \throw std::runtime_error on errors
     */
    std::string comment(Flags flags = 0) const
    {
        int length = 0;
        auto text = zip_get_archive_comment(m_handle.get(), &length, flags);

        if (text == nullptr)
            throw std::runtime_error(zip_strerror(m_handle.get()));

        return std::string(text, static_cast<std::size_t>(length));
    }

    /**
     * Check if a file exists on the archive.
     *
     * \param name the name
     * \param flags the optional flags
     * \return if the file exists
     */
    bool exists(const std::string &name, Flags flags = 0) const noexcept
    {
        return zip_name_locate(m_handle.get(), name.c_str(), flags) >= 0;
    }

    /**
     * Locate a file on the archive.
     *
     * \param name the name
     * \param flags the optional flags
     * \return the index
     * \throw std::runtime_error on errors
     */
    Int64 find(const std::string &name, Flags flags = 0) const
    {
        auto index = zip_name_locate(m_handle.get(), name.c_str(), flags);

        if (index < 0)
            throw std::runtime_error(zip_strerror(m_handle.get()));

        return index;
    }

    /**
     * Get information about a file.
     *
     * \param name the name
     * \param flags the optional flags
     * \return the structure
     * \throw std::runtime_error on errors
     */
    Stat stat(const std::string &name, Flags flags = 0) const
    {
        Stat st;

        if (zip_stat(m_handle.get(), name.c_str(), flags, &st) < 0)
            throw std::runtime_error(zip_strerror(m_handle.get()));

        return st;
    }

    /**
     * Get information about a file. Overloaded function.
     *
     * \param index the file index in the archive
     * \param flags the optional flags
     * \return the structure
     * \throw std::runtime_error on errors
     */
    Stat stat(Uint64 index, Flags flags = 0) const
    {
        Stat st;

        if (zip_stat_index(m_handle.get(), index, flags, &st) < 0)
            throw std::runtime_error(zip_strerror(m_handle.get()));

        return st;
    }

    /**
     * Add a file to the archive.
     *
     * \param source the source
     * \param name the name entry in the archive
     * \param flags the optional flags
     * \return the new index in the archive
     * \throw std::runtime_error on errors
     * \see source::file
     * \see source::buffer
     */
    Int64 add(const Source &source, const std::string &name, Flags flags = 0)
    {
        auto src = source(m_handle.get());
        auto ret = zip_file_add(m_handle.get(), name.c_str(), src, flags);

        if (ret < 0) {
            zip_source_free(src);
            throw std::runtime_error(zip_strerror(m_handle.get()));
        }

        return ret;
    }

    /**
     * Create a directory in the archive.
     *
     * \param directory the directory name
     * \param flags the optional flags
     * \return the new index in the archive
     * \throw std::runtime_error on errors
     */
    Int64 mkdir(const std::string &directory, Flags flags = 0)
    {
        auto ret = zip_dir_add(m_handle.get(), directory.c_str(), flags);

        if (ret < 0)
            throw std::runtime_error(zip_strerror(m_handle.get()));

        return ret;
    }

    /**
     * Replace an existing file in the archive.
     *
     * \param source the source
     * \param index the file index in the archiev
     * \param flags the optional flags
     * \throw std::runtime_error on errors
     */
    void replace(const Source &source, Uint64 index, Flags flags = 0)
    {
        auto src = source(m_handle.get());

        if (zip_file_replace(m_handle.get(), index, src, flags) < 0) {
            zip_source_free(src);
            throw std::runtime_error(zip_strerror(m_handle.get()));
        }
    }

    /**
     * Open a file in the archive.
     *
     * \param name the name
     * \param flags the optional flags
     * \param password the optional password
     * \return the opened file
     * \throw std::runtime_error on errors
     */
    File open(const std::string &name, Flags flags = 0, const std::string &password = "")
    {
        struct zip_file *file;

        if (password.size() > 0)
            file = zip_fopen_encrypted(m_handle.get(), name.c_str(), flags, password.c_str());
        else
            file = zip_fopen(m_handle.get(), name.c_str(), flags);

        if (file == nullptr)
            throw std::runtime_error(zip_strerror(m_handle.get()));

        return file;
    }

    /**
     * Open a file in the archive. Overloaded function.
     *
     * \param index the file index in the archive
     * \param flags the optional flags
     * \param password the optional password
     * \return the opened file
     * \throw std::runtime_error on errors
     */
    File open(Uint64 index, Flags flags = 0, const std::string &password = "")
    {
        struct zip_file *file;

        if (password.size() > 0)
            file = zip_fopen_index_encrypted(m_handle.get(), index, flags, password.c_str());
        else
            file = zip_fopen_index(m_handle.get(), index, flags);

        if (file == nullptr)
            throw std::runtime_error(zip_strerror(m_handle.get()));

        return file;
    }

    /**
     * Rename an existing entry in the archive.
     *
     * \param index the file index in the archive
     * \param name the new name
     * \param flags the optional flags
     * \throw std::runtime_error on errors
     */
    inline void rename(Uint64 index, const std::string &name, Flags flags = 0)
    {
        if (zip_file_rename(m_handle.get(), index, name.c_str(), flags) < 0)
            throw std::runtime_error(zip_strerror(m_handle.get()));
    }

    /**
     * Set file compression.
     *
     * \param index the file index in the archive
     * \param comp the compression
     * \param flags the optional flags
     * \throw std::runtime_error on errors
     */
    inline void setFileCompression(Uint64 index, Int32 comp, Uint32 flags = 0)
    {
        if (zip_set_file_compression(m_handle.get(), index, comp, flags) < 0)
            throw std::runtime_error(zip_strerror(m_handle.get()));
    }

    /**
     * Delete a file from the archive.
     *
     * \param index the file index in the archive
     * \throw std::runtime_error on errors
     */
    inline void remove(Uint64 index)
    {
        if (zip_delete(m_handle.get(), index) < 0)
            throw std::runtime_error(zip_strerror(m_handle.get()));
    }

    /**
     * Get the number of entries in the archive.
     *
     * \param flags the optional flags
     * \return the number of entries
     */
    inline Int64 numEntries(Flags flags = 0) const noexcept
    {
        return zip_get_num_entries(m_handle.get(), flags);
    }

    /**
     * Revert changes on the file.
     *
     * \param index the index
     * \throw std::runtime_error on errors
     */
    inline void unchange(Uint64 index)
    {
        if (zip_unchange(m_handle.get(), index) < 0)
            throw std::runtime_error(zip_strerror(m_handle.get()));
    }

    /**
     * Revert all changes.
     *
     * \throw std::runtime_error on errors
     */
    inline void unchangeAll()
    {
        if (zip_unchange_all(m_handle.get()) < 0)
            throw std::runtime_error(zip_strerror(m_handle.get()));
    }

    /**
     * Revert changes to archive.
     *
     * \throw std::runtime_error on errors
     */
    void unchangeArchive()
    {
        if (zip_unchange_archive(m_handle.get()) < 0)
            throw std::runtime_error(zip_strerror(m_handle.get()));
    }

    /**
     * Set the defaut password.
     *
     * \param password the password or empty to unset it
     * \throw std::runtime_error on errors
     */
    void setDefaultPassword(const std::string &password = "")
    {
        auto cstr = (password.size() > 0) ? password.c_str() : nullptr;

        if (zip_set_default_password(m_handle.get(), cstr) < 0)
            throw std::runtime_error(zip_strerror(m_handle.get()));
    }

    /**
     * Set an archive flag.
     *
     * \param flag the flag to set
     * \param value the value
     * \throw std::runtime_error on errors
     */
    inline void setFlag(Flags flag, int value)
    {
        if (zip_set_archive_flag(m_handle.get(), flag, value) < 0)
            throw std::runtime_error(zip_strerror(m_handle.get()));
    }

    /**
     * Get an archive flag.
     *
     * \param which which flag
     * \param flags the optional flags
     * \return the value
     * \throw std::runtime_error on errors
     */
    inline int flag(Flags which, Flags flags = 0) const
    {
        auto ret = zip_get_archive_flag(m_handle.get(), which, flags);

        if (ret < 0)
            throw std::runtime_error(zip_strerror(m_handle.get()));

        return ret;
    }
};

} // !zippy

#endif // !ZIP_HPP
