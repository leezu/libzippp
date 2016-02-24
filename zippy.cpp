/*
 * zippy.cpp -- wrapper around libzip
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

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

#include "zippy.h"

namespace zippy {

namespace source {

Source buffer(std::string data) noexcept
{
	return [=] (struct zip *archive) -> struct zip_source * {
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

Source file(std::string path, Uint64 start, Int64 length) noexcept
{
	return [=] (struct zip *archive) -> struct zip_source * {
		auto src = zip_source_file(archive, path.c_str(), start, length);

		if (src == nullptr)
			throw std::runtime_error(zip_strerror(archive));

		return src;
	};
}

} // !source

/* --------------------------------------------------------
 * Archive
 * ------------------------------------------------------- */

Archive::Archive(const std::string &path, Flags flags)
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

void Archive::setFileComment(Uint64 index, const std::string &text, Flags flags)
{
	auto size = text.size();
	auto cstr = (size == 0) ? nullptr : text.c_str();

	if (zip_file_set_comment(m_handle.get(), index, cstr, size, flags) < 0)
		throw std::runtime_error(zip_strerror(m_handle.get()));
}

std::string Archive::fileComment(Uint64 index, Flags flags) const
{
	zip_uint32_t length = 0;
	auto text = zip_file_get_comment(m_handle.get(), index, &length, flags);

	if (text == nullptr)
		throw std::runtime_error(zip_strerror(m_handle.get()));

	return std::string(text, length);
}

void Archive::setComment(const std::string &comment)
{
	if (zip_set_archive_comment(m_handle.get(), comment.c_str(), comment.size()) < 0)
		throw std::runtime_error(zip_strerror(m_handle.get()));
}

std::string Archive::comment(Flags flags) const
{
	int length = 0;
	auto text = zip_get_archive_comment(m_handle.get(), &length, flags);

	if (text == nullptr)
		throw std::runtime_error(zip_strerror(m_handle.get()));

	return std::string(text, static_cast<std::size_t>(length));
}

bool Archive::exists(const std::string &name, Flags flags) const noexcept
{
	return zip_name_locate(m_handle.get(), name.c_str(), flags) >= 0;
}

Int64 Archive::find(const std::string &name, Flags flags) const
{
	auto index = zip_name_locate(m_handle.get(), name.c_str(), flags);

	if (index < 0)
		throw std::runtime_error(zip_strerror(m_handle.get()));

	return index;
}

Stat Archive::stat(const std::string &name, Flags flags) const
{
	Stat st;

	if (zip_stat(m_handle.get(), name.c_str(), flags, &st) < 0)
		throw std::runtime_error(zip_strerror(m_handle.get()));

	return st;
}

Stat Archive::stat(Uint64 index, Flags flags) const
{
	Stat st;

	if (zip_stat_index(m_handle.get(), index, flags, &st) < 0)
		throw std::runtime_error(zip_strerror(m_handle.get()));

	return st;
}

Int64 Archive::add(const Source &source, const std::string &name, Flags flags)
{
	auto src = source(m_handle.get());
	auto ret = zip_file_add(m_handle.get(), name.c_str(), src, flags);

	if (ret < 0) {
		zip_source_free(src);
		throw std::runtime_error(zip_strerror(m_handle.get()));
	}

	return ret;
}

Int64 Archive::mkdir(const std::string &directory, Flags flags)
{
	auto ret = zip_dir_add(m_handle.get(), directory.c_str(), flags);

	if (ret < 0)
		throw std::runtime_error(zip_strerror(m_handle.get()));

	return ret;
}

void Archive::replace(const Source &source, Uint64 index, Flags flags)
{
	auto src = source(m_handle.get());

	if (zip_file_replace(m_handle.get(), index, src, flags) < 0) {
		zip_source_free(src);
		throw std::runtime_error(zip_strerror(m_handle.get()));
	}
}

File Archive::open(const std::string &name, Flags flags, const std::string &password)
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

File Archive::open(Uint64 index, Flags flags, const std::string &password)
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

void Archive::rename(Uint64 index, const std::string &name, Flags flags)
{
	if (zip_file_rename(m_handle.get(), index, name.c_str(), flags) < 0)
		throw std::runtime_error(zip_strerror(m_handle.get()));
}

void Archive::setFileCompression(Uint64 index, Int32 comp, Uint32 flags)
{
	if (zip_set_file_compression(m_handle.get(), index, comp, flags) < 0)
		throw std::runtime_error(zip_strerror(m_handle.get()));
}

void Archive::remove(Uint64 index)
{
	if (zip_delete(m_handle.get(), index) < 0)
		throw std::runtime_error(zip_strerror(m_handle.get()));
}

Int64 Archive::numEntries(Flags flags) const noexcept
{
	return zip_get_num_entries(m_handle.get(), flags);
}

void Archive::unchange(Uint64 index)
{
	if (zip_unchange(m_handle.get(), index) < 0)
		throw std::runtime_error(zip_strerror(m_handle.get()));
}

void Archive::unchangeAll()
{
	if (zip_unchange_all(m_handle.get()) < 0)
		throw std::runtime_error(zip_strerror(m_handle.get()));
}

void Archive::unchangeArchive()
{
	if (zip_unchange_archive(m_handle.get()) < 0)
		throw std::runtime_error(zip_strerror(m_handle.get()));
}

void Archive::setDefaultPassword(const std::string &password)
{
	auto cstr = (password.size() > 0) ? password.c_str() : nullptr;

	if (zip_set_default_password(m_handle.get(), cstr) < 0)
		throw std::runtime_error(zip_strerror(m_handle.get()));
}

void Archive::setFlag(Flags flags, int value)
{
	if (zip_set_archive_flag(m_handle.get(), flags, value) < 0)
		throw std::runtime_error(zip_strerror(m_handle.get()));
}

int Archive::flag(Flags which, Flags flags) const
{
	auto ret = zip_get_archive_flag(m_handle.get(), which, flags);

	if (ret < 0)
		throw std::runtime_error(zip_strerror(m_handle.get()));

	return ret;
}

} // !zippy