/*
 * zippy.h -- wrapper around libzip
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

#ifndef _ZIPPY_H_
#define _ZIPPY_H_

#include <iterator>
#include <memory>
#include <string>

#include <zip.h>

/**
 * @brief Zippy namespace.
 */
namespace zippy {

using Stat = struct zip_stat;
using SourceCommand = enum zip_source_cmd;
using Callback = zip_source_callback;

using Flags	= zip_flags_t;
using Int8	= zip_int8_t;
using Uint8	= zip_uint8_t;
using Int16	= zip_int16_t;
using Uint16	= zip_uint16_t;
using Int32	= zip_int32_t;
using Uint32	= zip_uint32_t;
using Int64	= zip_int64_t;
using Uint64	= zip_uint64_t;

/**
 * @class File
 * @brief File for reading
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
	 * @param file the file ready to be used
	 */
	inline File(struct zip_file *file) noexcept
		: m_handle(file, zip_fclose)
	{
	}

	/**
	 * Move constructor defaulted.
	 *
	 * @param other the other File
	 */
	File(File &&other) noexcept = default;

	/**
	 * Move operator defaulted.
	 *
	 * @param other the other File
	 * @return *this
	 */
	File &operator=(File &&) noexcept = default;

	/**
	 * Read some data.
	 *
	 * @param data the destination buffer
	 * @param length the length
	 * @return the number of bytes written or -1 on failure
	 */
	inline int read(void *data, Uint64 length) noexcept
	{
		return zip_fread(m_handle.get(), data, length);
	}

	/**
	 * Read some data to a fixed size array.
	 *
	 * @param data the array
	 * @return the number of bytes written or -1 on failure
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
	 * @param length the length of the file
	 * @return the whole string
	 * @see Archive::stat
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
 * @brief Source creation for adding files.
 *
 * This function must be passed through Archive::add, it will be called to create the appropriate zip_source
 * inside the function.
 *
 * The user should not call the function itself as it's the responsability of libzip to destroy the
 * zip_source structure.
 *
 * @see source::buffer
 * @see source::file
 */
using Source = std::function<struct zip_source * (struct zip *)>;

/**
 * @brief Source namespace, contains various ways for adding file to an archive.
 */
namespace source {

/**
 * Add a file to the archive using a binary buffer.
 *
 * @param data the buffer
 */
Source buffer(std::string data) noexcept;

/**
 * Add a file to the archive from the disk.
 *
 * @param path the path to the file
 * @param start the position where to start
 * @param length the number of bytes to copy
 */
Source file(std::string path, Uint64 start = 0, Int64 length = -1) noexcept;

} // !source

/**
 * @class StatPtr
 * @brief Wrapper for Stat as pointer
 */
class StatPtr {
private:
	Stat m_stat;

public:
	/**
	 * Constructor.
	 *
	 * @param stat the file stat
	 */
	inline StatPtr(Stat stat) noexcept
		: m_stat(stat)
	{
	}

	/**
	 * Get the reference.
	 *
	 * @return the reference
	 */
	inline Stat &operator*() noexcept
	{
		return m_stat;
	}

	/**
	 * Get the reference.
	 *
	 * @return the reference
	 */
	inline const Stat &operator*() const noexcept
	{
		return m_stat;
	}

	/**
	 * Access the object.
	 *
	 * @return the pointer
	 */
	inline Stat *operator->() noexcept
	{
		return &m_stat;
	}

	/**
	 * Access the object.
	 *
	 * @return the pointer
	 */
	inline const Stat *operator->() const noexcept
	{
		return &m_stat;
	}
};

/**
 * @class Archive
 * @brief Safe wrapper on the struct zip structure
 */
class Archive {
private:
	using Handle = std::unique_ptr<struct zip, int (*)(struct zip *)>;

	Handle m_handle;

	Archive(const Archive &) = delete;
	Archive &operator=(const Archive &) = delete;

public:
	using value_type = Stat;
	using reference = Stat;
	using const_refernce = Stat;
	using pointer = StatPtr;
	using size_type = unsigned;

	/**
	 * @class iterator
	 * @brief Iterator
	 */
	class iterator : public std::iterator<std::random_access_iterator_tag, Archive> {
	private:
		Archive &m_archive;
		Uint64 m_index;

	public:
		explicit inline iterator(Archive &archive, Uint64 index = 0) noexcept
			: m_archive(archive)
			, m_index(index)
		{
		}

		inline Stat operator*() const
		{
			return m_archive.stat(m_index);
		}

		inline StatPtr operator->() const
		{
			return StatPtr(m_archive.stat(m_index));
		}

		inline iterator &operator++() noexcept
		{
			++ m_index;

			return *this;
		}

		inline iterator operator++(int) noexcept
		{
			iterator save = *this;

			++ m_index;

			return save;
		}

		inline iterator &operator--() noexcept
		{
			-- m_index;

			return *this;
		}

		inline iterator operator--(int) noexcept
		{
			iterator save = *this;

			-- m_index;

			return save;
		}

		inline iterator operator+(int inc) const noexcept
		{
			return iterator(m_archive, m_index + inc);
		}

		inline iterator operator-(int dec) const noexcept
		{
			return iterator(m_archive, m_index - dec);
		}

		inline bool operator==(const iterator &other) const noexcept
		{
			return m_index == other.m_index;
		}

		inline bool operator!=(const iterator &other) const noexcept
		{
			return m_index != other.m_index;
		}

		inline Stat operator[](int index) const
		{
			return m_archive.stat(index);
		}
	};

	/**
	 * @class const_iterator
	 * @brief Const iterator
	 */
	class const_iterator : public std::iterator<std::random_access_iterator_tag, Archive> {
	private:
		const Archive &m_archive;
		Uint64 m_index;

	public:
		explicit inline const_iterator(const Archive &archive, Uint64 index = 0) noexcept
			: m_archive(archive)
			, m_index(index)
		{
		}

		inline Stat operator*() const
		{
			return m_archive.stat(m_index);
		}

		inline StatPtr operator->() const
		{
			return StatPtr(m_archive.stat(m_index));
		}

		inline const_iterator &operator++() noexcept
		{
			++ m_index;

			return *this;
		}

		inline const_iterator operator++(int) noexcept
		{
			const_iterator save = *this;

			++ m_index;

			return save;
		}

		inline const_iterator &operator--() noexcept
		{
			-- m_index;

			return *this;
		}

		inline const_iterator operator--(int) noexcept
		{
			const_iterator save = *this;

			-- m_index;

			return save;
		}

		inline const_iterator operator+(int inc) const noexcept
		{
			return const_iterator(m_archive, m_index + inc);
		}

		inline const_iterator operator-(int dec) const noexcept
		{
			return const_iterator(m_archive, m_index - dec);
		}

		inline bool operator==(const const_iterator &other) const noexcept
		{
			return m_index == other.m_index;
		}

		inline bool operator!=(const const_iterator &other) const noexcept
		{
			return m_index != other.m_index;
		}

		inline Stat operator[](int index) const
		{
			return m_archive.stat(index);
		}
	};

public:
	/**
	 * Open an archive on the disk.
	 *
	 * @param path the path
	 * @param flags the optional flags
	 * @throw std::runtime_error on errors
	 */
	Archive(const std::string &path, Flags flags = 0);

	/**
	 * Move constructor defaulted.
	 *
	 * @param other the other Archive
	 */
	Archive(Archive &&other) noexcept = default;

	/**
	 * Move operator defaulted.
	 *
	 * @param other the other Archive
	 * @return *this
	 */
	Archive &operator=(Archive &&other) noexcept = default;

	/**
	 * Get an iterator to the beginning.
	 *
	 * @return the iterator
	 */
	inline iterator begin() noexcept
	{
		return iterator(*this);
	}

	/**
	 * Overloaded function.
	 *
	 * @return the iterator
	 */
	inline const_iterator begin() const noexcept
	{
		return const_iterator(*this);
	}

	/**
	 * Overloaded function.
	 *
	 * @return the iterator
	 */
	inline const_iterator cbegin() const noexcept
	{
		return const_iterator(*this);
	}

	/**
	 * Get an iterator to the end.
	 *
	 * @return the iterator
	 */
	inline iterator end() noexcept
	{
		return iterator(*this, numEntries());
	}

	/**
	 * Overloaded function.
	 *
	 * @return the iterator
	 */
	inline const_iterator end() const noexcept
	{
		return const_iterator(*this, numEntries());
	}

	/**
	 * Overloaded function.
	 *
	 * @return the iterator
	 */
	inline const_iterator cend() const noexcept
	{
		return const_iterator(*this, numEntries());
	}

	/**
	 * Set a comment on a file.
	 *
	 * @param index the file index in the archive
	 * @param text the text or empty to remove the comment
	 * @param flags the optional flags
	 * @throw std::runtime_error on errors
	 */
	void setFileComment(Uint64 index, const std::string &text = "", Flags flags = 0);

	/**
	 * Get a comment from a file.
	 *
	 * @param index the file index in the archive
	 * @param flags the optional flags
	 * @return the comment
	 * @throw std::runtime_error on errors
	 */
	std::string fileComment(Uint64 index, Flags flags = 0) const;

	/**
	 * Set the archive comment.
	 *
	 * @param comment the comment
	 * @throw std::runtime_error on errors
	 */
	void setComment(const std::string &comment);

	/**
	 * Get the archive comment.
	 *
	 * @param flags the optional flags
	 * @return the comment
	 * @throw std::runtime_error on errors
	 */
	std::string comment(Flags flags = 0) const;

	/**
	 * Check if a file exists on the archive.
	 *
	 * @param name the name
	 * @param flags the optional flags
	 * @return if the file exists
	 */
	bool exists(const std::string &name, Flags flags = 0) const noexcept;

	/**
	 * Locate a file on the archive.
	 *
	 * @param name the name
	 * @param flags the optional flags
	 * @return the index
	 * @throw std::runtime_error on errors
	 */
	Int64 find(const std::string &name, Flags flags = 0) const;

	/**
	 * Get information about a file.
	 *
	 * @param name the name
	 * @param flags the optional flags
	 * @return the structure
	 * @throw std::runtime_error on errors
	 */
	Stat stat(const std::string &name, Flags flags = 0) const;

	/**
	 * Get information about a file. Overloaded function.
	 *
	 * @param index the file index in the archive
	 * @param flags the optional flags
	 * @return the structure
	 * @throw std::runtime_error on errors
	 */
	Stat stat(Uint64 index, Flags flags = 0) const;

	/**
	 * Add a file to the archive.
	 *
	 * @param source the source
	 * @param name the name entry in the archive
	 * @param flags the optional flags
	 * @return the new index in the archive
	 * @throw std::runtime_error on errors
	 * @see source::file
	 * @see source::buffer
	 */
	Int64 add(const Source &source, const std::string &name, Flags flags = 0);

	/**
	 * Create a directory in the archive.
	 *
	 * @param directory the directory name
	 * @param flags the optional flags
	 * @return the new index in the archive
	 * @throw std::runtime_error on errors
	 */
	Int64 mkdir(const std::string &directory, Flags flags = 0);

	/**
	 * Replace an existing file in the archive.
	 *
	 * @param source the source
	 * @param index the file index in the archiev
	 * @param flags the optional flags
	 * @throw std::runtime_error on errors
	 */
	void replace(const Source &source, Uint64 index, Flags flags = 0);

	/**
	 * Open a file in the archive.
	 *
	 * @param name the name
	 * @param flags the optional flags
	 * @param password the optional password
	 * @return the opened file
	 * @throw std::runtime_error on errors
	 */
	File open(const std::string &name, Flags flags = 0, const std::string &password = "");

	/**
	 * Open a file in the archive. Overloaded function.
	 *
	 * @param index the file index in the archive
	 * @param flags the optional flags
	 * @param password the optional password
	 * @return the opened file
	 * @throw std::runtime_error on errors
	 */
	File open(Uint64 index, Flags flags = 0, const std::string &password = "");

	/**
	 * Rename an existing entry in the archive.
	 *
	 * @param index the file index in the archive
	 * @param name the new name
	 * @param flags the optional flags
	 * @throw std::runtime_error on errors
	 */
	void rename(Uint64 index, const std::string &name, Flags flags = 0);

	/**
	 * Set file compression.
	 *
	 * @param index the file index in the archive
	 * @param comp the compression
	 * @param flags the optional flags
	 * @throw std::runtime_error on errors
	 */
	void setFileCompression(Uint64 index, Int32 comp, Uint32 flags = 0);

	/**
	 * Delete a file from the archive.
	 *
	 * @param index the file index in the archive
	 * @throw std::runtime_error on errors
	 */
	void remove(Uint64 index);

	/**
	 * Get the number of entries in the archive.
	 *
	 * @param flags the optional flags
	 * @return the number of entries
	 */
	Int64 numEntries(Flags flags = 0) const noexcept;

	/**
	 * Revert changes on the file.
	 *
	 * @param index the index
	 * @throw std::runtime_error on errors
	 */
	void unchange(Uint64 index);

	/**
	 * Revert all changes.
	 *
	 * @throw std::runtime_error on errors
	 */
	void unchangeAll();

	/**
	 * Revert changes to archive.
	 *
	 * @throw std::runtime_error on errors
	 */
	void unchangeArchive();

	/**
	 * Set the defaut password.
	 *
	 * @param password the password or empty to unset it
	 * @throw std::runtime_error on errors
	 */
	void setDefaultPassword(const std::string &password = "");

	/**
	 * Set an archive flag.
	 *
	 * @param flag the flag to set
	 * @param value the value
	 * @throw std::runtime_error on errors
	 */
	void setFlag(Flags flag, int value);

	/**
	 * Get an archive flag.
	 *
	 * @param which which flag
	 * @param flags the optional flags
	 * @return the value
	 * @throw std::runtime_error on errors
	 */
	int flag(Flags which, Flags flags = 0) const;
};

} // !zippy

#endif // !_ZIPPY_H_
