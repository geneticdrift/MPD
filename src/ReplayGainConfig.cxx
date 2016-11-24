/*
 * Copyright 2003-2016 The Music Player Daemon Project
 * http://www.musicpd.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"
#include "ReplayGainConfig.hxx"
#include "config/Param.hxx"
#include "config/ConfigGlobal.hxx"
#include "util/RuntimeError.hxx"

#include <assert.h>
#include <stdlib.h>
#include <math.h>

ReplayGainMode replay_gain_mode = ReplayGainMode::OFF;

static constexpr bool DEFAULT_REPLAYGAIN_LIMIT = true;

float replay_gain_preamp = 1.0;
float replay_gain_missing_preamp = 1.0;
bool replay_gain_limit = DEFAULT_REPLAYGAIN_LIMIT;

static float
ParsePreamp(const char *s)
{
	assert(s != nullptr);

	char *endptr;
	float f = strtod(s, &endptr);
	if (endptr == s || *endptr != '\0')
		throw std::invalid_argument("Not a numeric value");

	if (f < -15 || f > 15)
		throw std::invalid_argument("Number must be between -15 and 15");

	return pow(10, f / 20.0);
}

static float
ParsePreamp(const ConfigParam &p)
{
	try {
		return ParsePreamp(p.value.c_str());
	} catch (...) {
		std::throw_with_nested(FormatRuntimeError("Failed to parse line %i",
							  p.line));
	}
}

void replay_gain_global_init(void)
{
	const auto *param = config_get_param(ConfigOption::REPLAYGAIN);

	try {
		if (param != nullptr)
			replay_gain_mode = FromString(param->value.c_str());
	} catch (...) {
		std::throw_with_nested(FormatRuntimeError("Failed to parse line %i",
							  param->line));
	}

	param = config_get_param(ConfigOption::REPLAYGAIN_PREAMP);
	if (param)
		replay_gain_preamp = ParsePreamp(*param);

	param = config_get_param(ConfigOption::REPLAYGAIN_MISSING_PREAMP);
	if (param)
		replay_gain_missing_preamp = ParsePreamp(*param);

	replay_gain_limit = config_get_bool(ConfigOption::REPLAYGAIN_LIMIT,
					    DEFAULT_REPLAYGAIN_LIMIT);
}
