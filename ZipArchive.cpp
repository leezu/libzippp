/*
 * ZipArchive.cpp -- wrapper around libzip
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

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

#include "ZipArchive.h"

namespace source {

/* --------------------------------------------------------
 * Buffer (zip_source_buffer)
 * -------------------------------------------------------- */

Buffer::Buffer(std::string data)
	: m_data(std::move(data))
{
}

struct zip_source *Buffer::source(struct zip *archive) const
{
	auto size = m_data.size();
	auto data = static_cast<char *>(std::malloc(size));

	if (data == nullptr)
		throw std::runtime_error(std::strerror(errno));

	std::memcpy(data, m_data.data(), size);

	auto src = zip_source_buffer(archive, data, size, 1);

	if (src == nullptr) {
		std::free(data);
		throw std::runtime_error(zip_strerror(archive));
	}

	return src;
}

/* --------------------------------------------------------
 * File (zip_source_file)
 * -------------------------------------------------------- */

File::File(std::string path, ZipUint64 start, ZipInt64 length)
	: m_path(std::move(path))
	, m_start(start)
	, m_length(length)
{
}

struct zip_source *File::source(struct zip *archive) const
{
	auto src = zip_source_file(archive, m_path.c_str(), m_start, m_length);

	if (src == nullptr)
		throw std::runtime_error(zip_strerror(archive));

	return src;
}

} // !source

/* --------------------------------------------------------
 * ZipArchive
 * ------------------------------------------------------- */

ZipArchive::ZipArchive(const std::string &path, ZipFlags flags)
	: m_handle(nullptr, nullptr)
{
	int error;
	struct zip *archive = zip_open(path.c_str(), flags, &error);

	if (archive == nullptr)
	{
		char buf[128]{};

		zip_error_to_str(buf, sizeof (buf), error, errno);

		throw std::runtime_error(buf);
	}

	m_handle = { archive, zip_close };
}

void ZipArchive::setFileComment(ZipUint64 index, const std::string &text, ZipFlags flags)
{
	auto size = text.size();
	auto cstr = (size == 0) ? nullptr : text.c_str();

	if (zip_file_set_comment(m_handle.get(), index, cstr, size, flags) < 0)
		throw std::runtime_error(zip_strerror(m_handle.get()));
}

std::string ZipArchive::getFileComment(ZipUint64 index, ZipFlags flags) const
{
	zip_uint32_t length{};
	auto text = zip_file_get_comment(m_handle.get(), index, &length, flags);

	if (text == nullptr)
		throw std::runtime_error(zip_strerror(m_handle.get()));

	return { text, length };
}

void ZipArchive::setComment(const std::string &comment)
{
	if (zip_set_archive_comment(m_handle.get(), comment.c_str(), comment.size()) < 0)
		throw std::runtime_error(zip_strerror(m_handle.get()));
}

std::string ZipArchive::getComment(ZipFlags flags) const
{
	int length{};
	auto text = zip_get_archive_comment(m_handle.get(), &length, flags);

	if (text == nullptr)
		throw std::runtime_error(zip_strerror(m_handle.get()));

	return { text, static_cast<size_t>(length) };
}

ZipInt64 ZipArchive::find(const std::string &name, ZipFlags flags)
{
	auto index = zip_name_locate(m_handle.get(), name.c_str(), flags);

	if (index < 0)
		throw std::runtime_error(zip_strerror(m_handle.get()));

	return index;
}

ZipStat ZipArchive::stat(const std::string &name, ZipFlags flags)
{
	ZipStat st;

	if (zip_stat(m_handle.get(), name.c_str(), flags, &st) < 0)
		throw std::runtime_error(zip_strerror(m_handle.get()));

	return st;
}

ZipStat ZipArchive::stat(ZipUint64 index, ZipFlags flags)
{
	ZipStat st;

	if (zip_stat_index(m_handle.get(), index, flags, &st) < 0)
		throw std::runtime_error(zip_strerror(m_handle.get()));

	return st;
}

ZipInt64 ZipArchive::add(const ZipSource &source, const std::string &name, ZipFlags flags)
{
	auto src = source.source(m_handle.get());
	auto ret = zip_file_add(m_handle.get(), name.c_str(), src, flags);

	if (ret < 0) {
		zip_source_free(src);
		throw std::runtime_error(zip_strerror(m_handle.get()));
	}

	return ret;
}

ZipInt64 ZipArchive::addDirectory(const std::string &directory, ZipFlags flags)
{
	auto ret = zip_dir_add(m_handle.get(), directory.c_str(), flags);

	if (ret < 0)
		throw std::runtime_error(zip_strerror(m_handle.get()));

	return ret;
}

void ZipArchive::replace(const ZipSource &source, ZipUint64 index, ZipFlags flags)
{
	auto src = source.source(m_handle.get());

	if (zip_file_replace(m_handle.get(), index, src, flags) < 0) {
		zip_source_free(src);
		throw std::runtime_error(zip_strerror(m_handle.get()));
	}
}

ZipFile ZipArchive::open(const std::string &name, ZipFlags flags, const std::string &password)
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

ZipFile ZipArchive::open(ZipUint64 index, ZipFlags flags, const std::string &password)
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

void ZipArchive::rename(ZipUint64 index, const std::string &name, ZipFlags flags)
{
	if (zip_file_rename(m_handle.get(), index, name.c_str(), flags) < 0)
		throw std::runtime_error(zip_strerror(m_handle.get()));
}

void ZipArchive::setFileCompression(ZipUint64 index, ZipInt32 comp, ZipUint32 flags)
{
	if (zip_set_file_compression(m_handle.get(), index, comp, flags) < 0)
		throw std::runtime_error(zip_strerror(m_handle.get()));
}

void ZipArchive::remove(ZipUint64 index)
{
	if (zip_delete(m_handle.get(), index) < 0)
		throw std::runtime_error(zip_strerror(m_handle.get()));
}

ZipInt64 ZipArchive::numEntries(ZipFlags flags) const
{
	return zip_get_num_entries(m_handle.get(), flags);
}

void ZipArchive::unchange(ZipUint64 index)
{
	if (zip_unchange(m_handle.get(), index) < 0)
		throw std::runtime_error(zip_strerror(m_handle.get()));
}

void ZipArchive::unchangeAll()
{
	if (zip_unchange_all(m_handle.get()) < 0)
		throw std::runtime_error(zip_strerror(m_handle.get()));
}

void ZipArchive::unchangeArchive()
{
	if (zip_unchange_archive(m_handle.get()) < 0)
		throw std::runtime_error(zip_strerror(m_handle.get()));
}

void ZipArchive::setDefaultPassword(const std::string &password)
{
	auto cstr = (password.size() > 0) ? password.c_str() : nullptr;

	if (zip_set_default_password(m_handle.get(), cstr) < 0)
		throw std::runtime_error(zip_strerror(m_handle.get()));
}

void ZipArchive::setFlag(ZipFlags flags, int value)
{
	if (zip_set_archive_flag(m_handle.get(), flags, value) < 0)
		throw std::runtime_error(zip_strerror(m_handle.get()));
}

int ZipArchive::getFlag(ZipFlags which, ZipFlags flags) const
{
	auto ret = zip_get_archive_flag(m_handle.get(), which, flags);

	if (ret < 0)
		throw std::runtime_error(zip_strerror(m_handle.get()));

	return ret;
}
