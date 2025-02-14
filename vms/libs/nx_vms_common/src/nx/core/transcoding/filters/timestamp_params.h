// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#pragma once

#include "filter_params.h"

namespace nx::core::transcoding {

struct TimestampParams
{
    FilterParams filterParams;
    qint64 displayOffset = 0;
    qint64 timeMs = 0;
    bool operator==(const TimestampParams&) const = default;
};
NX_REFLECTION_INSTRUMENT(TimestampParams, (filterParams)(displayOffset)(timeMs))
} // namespace nx::core::transcoding
