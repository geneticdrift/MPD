// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "MetaSourceDecoderPlugin.h"
#include "MetaSourceDomain.hxx"
#include "input/InputStream.hxx"
#include "input/LocalOpen.hxx"
#include "fs/Path.hxx"
#include "fs/NarrowPath.hxx"
#include "Log.hxx"
#include "tag/Builder.hxx"
#include "tag/Handler.hxx"
#include "util/ScopeExit.hxx"
#include "util/StringAPI.hxx"
#include "util/StringCompare.hxx"
#include "song/DetachedSong.hxx"
#include "../DecoderAPI.hxx"
#include "FfmpegIo.hxx"
#include "io/FileLineReader.hxx"
#include "util/StringSplit.hxx"
#include "util/StringStrip.hxx"
#include <fmt/format.h>

static const char META_SOURCE_COMMENT = '#';

static bool
meta_source_init(const ConfigBlock &block)
{
	FmtDebug(meta_source_domain, "init");
	return true;
}

static void
meta_source_finish() noexcept
{
	FmtDebug(meta_source_domain, "finish");
}

static bool
meta_source_scan_file(Path path_fs, TagHandler &handler) noexcept
{
	FmtDebug(meta_source_domain, "scan_file: {}", path_fs.c_str());

	FileLineReader file{path_fs};

	const char *line;
	while ((line = file.ReadLine()) != nullptr) {
		if (*line == 0 || *line == META_SOURCE_COMMENT)
			continue;

		const auto [k, v] = Split(std::string_view{line}, ':');

		auto key = Strip(k);
		auto value = Strip(v);

		if (StringIsEqualIgnoreCase(key, "target_uri")) {
			handler.OnTargetUri(Strip(value));
			continue;
		}

		if (StringIsEqualIgnoreCase(key, "genre")) {
			handler.OnTag(TAG_GENRE, value);
			continue;
		}

		if (StringIsEqualIgnoreCase(key, "title")) {
			handler.OnTag(TAG_TITLE, value);
			continue;
		}

		if (StringIsEqualIgnoreCase(key, "duration")) {
			handler.OnDuration(SongTime(std::strtol(value.data(), nullptr, 10)));
			continue;
		}

		if (StringIsEqualIgnoreCase(key, "category")) {
			handler.OnPair("category", value);
			continue;
		}

		if (StringIsEqualIgnoreCase(key, "country")) {
			handler.OnPair("country", value);

			continue;
		}

		FmtError(meta_source_domain,
			 "Unrecognized line in meta source file: {}",
			 line);

	}
	return true;
}

static std::set<std::string, std::less<>>
meta_source_suffixes() noexcept
{
	return std::set<std::string, std::less<>> { "mpdmeta" };
}

static const char *const meta_source_mime_types[] = {
	"application/mpd-meta-source",
};

static std::forward_list<DetachedSong>
meta_source_container_scan(Path path_fs) {
	FmtDebug(meta_source_domain, "container_scan: {}", path_fs.c_str());
	std::forward_list<DetachedSong> list;
	TagBuilder tag_builder;
	{
		AddTagHandler h(tag_builder);
		h.WantDuration();
		h.OnDuration(SongTime(17833));
		h.OnTag(TAG_TITLE, "RFI Monde");
		h.OnTag(TAG_ALBUM, "Blala");
		h.OnTag(TAG_TRACK, fmt::format_int{1}.c_str());
		h.OnTag(TAG_GENRE, "Blalaaaaa");
		h.OnTag(TAG_ARTIST, "Glalala Opa");
		DetachedSong detached("#1", tag_builder.Commit());
		detached.SetRealURI("http://live02.rfi.fr/rfimonde-64.mp3");
		list.emplace_front(detached);
	}

	{
		AddTagHandler h(tag_builder);
		h.WantDuration();
		h.OnDuration(SongTime(417833));
		h.OnTag(TAG_TITLE, "Hirchmitsh Radio Progressive");
		h.OnTag(TAG_ALBUM, "Gla Gla");
		h.OnTag(TAG_TRACK, fmt::format_int{1}.c_str());
		h.OnTag(TAG_GENRE, "Brrrr");
		h.OnTag(TAG_ARTIST, "Opa la");
		DetachedSong detached("#2", tag_builder.Commit());
		detached.SetRealURI("http://hirschmilch.de:7000/progressive.aac");
		list.emplace_front(detached);
	}

	return list;
}

constexpr DecoderPlugin meta_source_decoder_plugin =
	DecoderPlugin("meta_source", nullptr, meta_source_scan_file)
	.WithInit(meta_source_init, meta_source_finish)
	.WithSuffixes(meta_source_suffixes)
	.WithMimeTypes(meta_source_mime_types)/*
	.WithContainer(meta_source_container_scan)*/;
