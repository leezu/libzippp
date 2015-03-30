/*
 * ZipArchive.h -- wrapper around libzip
 *
 * Copyright (c) 2013, 2014 David Demelier <markand@malikania.fr>
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

#ifndef _ZIP_ARCHIVE_H_
#define _ZIP_ARCHIVE_H_

#include <memory>
#include <string>

#include <zip.h>

using ZipStat = struct zip_stat;
using ZipSourceCommand = enum zip_source_cmd;
using ZipCallback = zip_source_callback;

using ZipFlags	= zip_flags_t;
using ZipInt8	= zip_int8_t;
using ZipUint8	= zip_uint8_t;
using ZipInt16	= zip_int16_t;
using ZipUint16	= zip_uint16_t;
using ZipInt32	= zip_int32_t;
using ZipUint32	= zip_uint32_t;
using ZipInt64	= zip_int64_t;
using ZipUint64	= zip_uint64_t;

/**
 * @class ZipSource
 * @brief Source for adding file
 */
class ZipSource {
public:
	/**
	 * Default constructor.
	 */
	ZipSource() = default;

	/**
	 * Virtual destructor.
	 */
	virtual ~ZipSource() = default;

	/**
	 * Create a zip_source structure. Must not be null, throw an exception
	 * instead.
	 *
	 * @return a zip_source ready to be used
	 * @throw std::runtime_error on errors
	 * @post must not return null
	 */
	virtual struct zip_source *source(struct zip *zip) const = 0;
};

/**
 * @class ZipFile
 * @brief File for reading
 */
class ZipFile {
private:
	std::unique_ptr<struct zip_file, int (*)(struct zip_file *)> m_handle;

	ZipFile(const ZipFile &) = delete;
	ZipFile &operator=(const ZipFile &) = delete;

public:
	/**
	 * Create a ZipFile with a zip_file structure.
	 *
	 * @param file the file ready to be used
	 */
	inline ZipFile(struct zip_file *file)
		: m_handle(file, zip_fclose)
	{
	}

	/**
	 * Move constructor defaulted.
	 *
	 * @param other the other ZipFile
	 */
	ZipFile(ZipFile &&other) noexcept = default;

	/**
	 * Move operator defaulted.
	 *
	 * @param other the other ZipFile
	 * @return *this
	 */
	ZipFile &operator=(ZipFile &&) noexcept = default;

	/**
	 * Read some data.
	 *
	 * @param data the destination buffer
	 * @param length the length
	 * @return the number of bytes written or -1 on failure
	 */
	inline int read(void *data, ZipUint64 length) noexcept
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
	 * Ideal for combining with ZipArchive::stat.
	 *
	 * @param length the length of the file
	 * @return the whole string
	 * @see ZipArchive::stat
	 */
	std::string read(unsigned length)
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

namespace source {

/**
 * @class Buffer
 * @brief Create a source from a buffer
 */
class Buffer : public ZipSource {
private:
	std::string m_data;

public:
	/**
	 * Buffer constructor. Moves the data.
	 *
	 * @param data the data
	 */
	Buffer(std::string data);

	/**
	 * @copydoc ZipSource::source
	 */
	struct zip_source *source(struct zip *archive) const override;
};

/**
 * @class File
 * @brief Create a source from a file on the disk
 */
class File : public ZipSource {
private:
	std::string m_path;
	ZipUint64 m_start;
	ZipInt64 m_length;

public:
	/**
	 * File constructor.
	 *
	 * @param path the path to the file
	 * @param start the beginning in the file
	 * @param length the maximum length
	 */
	File(std::string path, ZipUint64 start = 0, ZipInt64 length = -1);

	/**
	 * @copydoc ZipSource::source
	 */
	struct zip_source *source(struct zip *archive) const override;
};

} // !source

/**
 * @class ZipArchive
 * @brief Safe wrapper on the struct zip structure
 */
class ZipArchive {
private:
	using Handle = std::unique_ptr<struct zip, int (*)(struct zip *)>;

	Handle m_handle;

	ZipArchive(const ZipArchive &) = delete;
	ZipArchive &operator=(const ZipArchive &) = delete;

public:
	/**
	 * Open an archive on the disk.
	 *
	 * @param path the path
	 * @param flags the optional flags
	 * @throw std::runtime_error on errors
	 */
	ZipArchive(const std::string &path, ZipFlags flags = 0);

	/**
	 * Move constructor defaulted.
	 *
	 * @param other the other ZipArchive
	 */
	ZipArchive(ZipArchive &&other) noexcept = default;

	/**
	 * Move operator defaulted.
	 *
	 * @param other the other ZipArchive
	 * @return *this
	 */
	ZipArchive &operator=(ZipArchive &&other) noexcept = default;

	/**
	 * Set a comment on a file.
	 *
	 * @param index the file index in the archive
	 * @param text the text or empty to remove the comment
	 * @param flags the optional flags
	 * @throw std::runtime_error on errors
	 */
	void setFileComment(ZipUint64 index, const std::string &text = "", ZipFlags flags = 0);

	/**
	 * Get a comment from a file.
	 *
	 * @param index the file index in the archive
	 * @param flags the optional flags
	 * @return the comment
	 * @throw std::runtime_error on errors
	 */
	std::string getFileComment(ZipUint64 index, ZipFlags flags = 0) const;

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
	std::string getComment(ZipFlags flags = 0) const;

	/**
	 * Check if a file exists on the archive.
	 *
	 * @param name the name
	 * @param flags the optional flags
	 * @return if the file exists
	 */
	bool exists(const std::string &name, ZipFlags flags = 0);

	/**
	 * Locate a file on the archive.
	 *
	 * @param name the name
	 * @param flags the optional flags
	 * @return the index
	 * @throw std::runtime_error on errors
	 */
	ZipInt64 find(const std::string &name, ZipFlags flags = 0);

	/**
	 * Get information about a file.
	 *
	 * @param name the name
	 * @param flags the optional flags
	 * @return the structure
	 * @throw std::runtime_error on errors
	 */
	ZipStat stat(const std::string &name, ZipFlags flags = 0);

	/**
	 * Get information about a file. Overloaded function.
	 *
	 * @param index the file index in the archive
	 * @param flags the optional flags
	 * @return the structure
	 * @throw std::runtime_error on errors
	 */
	ZipStat stat(ZipUint64 index, ZipFlags flags = 0);

	/**
	 * Add a file to the archive.
	 *
	 * @param source the source
	 * @param name the name entry in the archive
	 * @param flags the optional flags
	 * @return the new index in the archive
	 * @throw std::runtime_error on errors
	 * @see source::File
	 * @see source::Buffer
	 */
	ZipInt64 add(const ZipSource &source, const std::string &name, ZipFlags flags = 0);

	/**
	 * Add a directory to the archive. Not a directory from the disk.
	 *
	 * @param directory the directory name
	 * @param flags the optional flags
	 * @return the new index in the archive
	 * @throw std::runtime_error on errors
	 */
	ZipInt64 addDirectory(const std::string &directory, ZipFlags flags = 0);

	/**
	 * Replace an existing file in the archive.
	 *
	 * @param source the source
	 * @param index the file index in the archiev
	 * @param flags the optional flags
	 * @throw std::runtime_error on errors
	 */
	void replace(const ZipSource &source, ZipUint64 index, ZipFlags flags = 0);

	/**
	 * Open a file in the archive.
	 *
	 * @param name the name
	 * @param flags the optional flags
	 * @param password the optional password
	 * @return the opened file
	 * @throw std::runtime_error on errors
	 */
	ZipFile open(const std::string &name, ZipFlags flags = 0, const std::string &password = "");

	/**
	 * Open a file in the archive. Overloaded function.
	 *
	 * @param index the file index in the archive
	 * @param flags the optional flags
	 * @param password the optional password
	 * @return the opened file
	 * @throw std::runtime_error on errors
	 */
	ZipFile open(ZipUint64 index, ZipFlags flags = 0, const std::string &password = "");

	/**
	 * Rename an existing entry in the archive.
	 *
	 * @param index the file index in the archive
	 * @param name the new name
	 * @param flags the optional flags
	 * @throw std::runtime_error on errors
	 */
	void rename(ZipUint64 index, const std::string &name, ZipFlags flags = 0);

	/**
	 * Set file compression.
	 *
	 * @param index the file index in the archive
	 * @param comp the compression
	 * @param flags the optional flags
	 * @throw std::runtime_error on errors
	 */
	void setFileCompression(ZipUint64 index, ZipInt32 comp, ZipUint32 flags = 0);

	/**
	 * Delete a file from the archive.
	 *
	 * @param index the file index in the archive
	 * @throw std::runtime_error on errors
	 */
	void remove(ZipUint64 index);

	/**
	 * Get the number of entries in the archive.
	 *
	 * @param flags the optional flags
	 * @return the number of entries
	 * @throw std::runtime_error on errors
	 */
	ZipInt64 numEntries(ZipFlags flags = 0) const;

	/**
	 * Revert changes on the file.
	 *
	 * @param index the index
	 * @throw std::runtime_error on errors
	 */
	void unchange(ZipUint64 index);

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
	void setFlag(ZipFlags flag, int value);

	/**
	 * Get an archive flag.
	 *
	 * @param which which flag
	 * @param flags the optional flags
	 * @return the value
	 * @throw std::runtime_error on errors
	 */
	int getFlag(ZipFlags which, ZipFlags flags = 0) const;

};

#endif // !_ZIP_ARCHIVE_H_
