// Copyright 2022 PingCAP, Ltd.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <Core/Defines.h>
#include <Core/Types.h>
#include <fmt/format.h>

#include <chrono>
#include <unordered_set>
#include <vector>

namespace DB
{
using Clock = std::chrono::system_clock;
using Seconds = std::chrono::seconds;

static constexpr UInt64 MB = 1ULL * 1024 * 1024;
static constexpr UInt64 GB = MB * 1024;

enum class StorageType
{
    Log = 1,
    Data = 2,
    Meta = 3,
    KVStore = 4,
};

enum class PageStorageRunMode : UInt8
{
    ONLY_V2 = 1,
    ONLY_V3 = 2,
    MIX_MODE = 3,
    UNI_PS = 4,
};

// PageStorage V2 define
static constexpr UInt64 PAGE_SIZE_STEP = (1 << 10) * 16; // 16 KB
static constexpr UInt64 PAGE_FILE_MAX_SIZE = 1024 * 2 * MB;
static constexpr UInt64 PAGE_FILE_SMALL_SIZE = 2 * MB;
static constexpr UInt64 PAGE_FILE_ROLL_SIZE = 128 * MB;

static_assert(PAGE_SIZE_STEP >= ((1 << 10) * 16), "PAGE_SIZE_STEP should be at least 16 KB");
static_assert((PAGE_SIZE_STEP & (PAGE_SIZE_STEP - 1)) == 0, "PAGE_SIZE_STEP should be power of 2");

// PageStorage V3 define
static constexpr UInt64 BLOBFILE_LIMIT_SIZE = 256 * MB;
static constexpr UInt64 PAGE_META_ROLL_SIZE = 2 * MB;
static constexpr UInt64 MAX_PERSISTED_LOG_FILES = 4;

using NamespaceId = UInt64;
static constexpr NamespaceId MAX_NAMESPACE_ID = UINT64_MAX;
// KVStore stores it's data individually, so the actual `ns_id` value doesn't matter(just different from `MAX_NAMESPACE_ID` is enough)
static constexpr NamespaceId KVSTORE_NAMESPACE_ID = 1000000UL;
// just a random namespace id for test, the value doesn't matter
static constexpr NamespaceId TEST_NAMESPACE_ID = 1000;

using PageIdU64 = UInt64;
using PageIdU64s = std::vector<PageIdU64>;
using PageIdU64Set = std::unordered_set<PageIdU64>;
static constexpr PageIdU64 INVALID_PAGE_U64_ID = 0;

using PageIdV3Internal = UInt128;
using PageIdV3Internals = std::vector<PageIdV3Internal>;

inline PageIdV3Internal buildV3Id(NamespaceId n_id, PageIdU64 p_id)
{
    // low bits first
    return PageIdV3Internal(p_id, n_id);
}

using PageFieldOffset = UInt64;
using PageFieldOffsets = std::vector<PageFieldOffset>;
using PageFieldSizes = std::vector<UInt64>;

using PageFieldOffsetChecksums = std::vector<std::pair<PageFieldOffset, UInt64>>;

using PageFileId = UInt64;
using PageFileLevel = UInt32;
using PageFileIdAndLevel = std::pair<PageFileId, PageFileLevel>;
using PageFileIdAndLevels = std::vector<PageFileIdAndLevel>;

using PageSize = UInt64;

struct ByteBuffer
{
    using Pos = char *;

    ByteBuffer()
        : begin_pos(nullptr)
        , end_pos(nullptr)
    {}

    ByteBuffer(Pos begin_pos_, Pos end_pos_)
        : begin_pos(begin_pos_)
        , end_pos(end_pos_)
    {}

    inline Pos begin() const { return begin_pos; }
    inline Pos end() const { return end_pos; }
    inline size_t size() const { return end_pos - begin_pos; }

private:
    Pos begin_pos;
    Pos end_pos; /// 1 byte after the end of the buffer
};

/// https://stackoverflow.com/a/13938417
inline size_t alignPage(size_t n)
{
    return (n + PAGE_SIZE_STEP - 1) & ~(PAGE_SIZE_STEP - 1);
}

} // namespace DB

// https://github.com/fmtlib/fmt/blob/master/doc/api.rst#formatting-user-defined-types
template <>
struct fmt::formatter<DB::PageIdV3Internal>
{
    static constexpr auto parse(format_parse_context & ctx)
    {
        const auto * it = ctx.begin();
        const auto * end = ctx.end();
        /// Only support {}.
        if (it != end && *it != '}')
            throw format_error("invalid format");
        return it;
    }

    template <typename FormatContext>
    auto format(const DB::PageIdV3Internal & value, FormatContext & ctx) const
    {
        return format_to(ctx.out(), "{}.{}", value.high, value.low);
    }
};

template <>
struct fmt::formatter<DB::PageFileIdAndLevel>
{
    static constexpr auto parse(format_parse_context & ctx)
    {
        const auto * it = ctx.begin();
        const auto * end = ctx.end();
        /// Only support {}.
        if (it != end && *it != '}')
            throw format_error("invalid format");
        return it;
    }

    template <typename FormatContext>
    auto format(const DB::PageFileIdAndLevel & id_lvl, FormatContext & ctx) const
    {
        return format_to(ctx.out(), "{}_{}", id_lvl.first, id_lvl.second);
    }
};
