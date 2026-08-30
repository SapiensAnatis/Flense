#pragma once
#include <hstring.h>
#include <restrictederrorinfo.h>
#include <unknwn.h>
#include <windows.h>

// Undefine GetCurrentTime macro to prevent
// conflict with Storyboard::GetCurrentTime
#undef GetCurrentTime

// Pre-include STL dependencies of WIL headers
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
