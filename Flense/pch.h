#pragma once
#include <windows.h>
#include <unknwn.h>
#include <restrictederrorinfo.h>
#include <hstring.h>

// Undefine GetCurrentTime macro to prevent
// conflict with Storyboard::GetCurrentTime
#undef GetCurrentTime

// Pre-included so later textual #includes of these headers (after ModulePreamble.h's
// imports, which transitively import std) are inert rather than redefinition errors.
#include <algorithm>
#include <chrono>
#include <coroutine>
#include <cstddef>
#include <cstdint>
#include <flat_map>
#include <format>
#include <functional>
#include <list>
#include <map>
#include <mutex>
#include <ranges>
#include <regex>
#include <span>
#include <stop_token>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <vector>
