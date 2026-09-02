module;
#include <nlohmann/json.hpp>
export module nlohmann_json;

// Only wraps what Flense.Core actually uses: nlohmann::json/basic_json, .parse(), .at(),
// .get<T>(), .is_array(), .empty(). No serialization macros, custom to_json/from_json, or
// the _json UDL - those don't cross a module boundary cleanly, so if usage grows to need
// them, this wrapper needs rethinking rather than just extending the using-list below.
export namespace nlohmann
{
    using nlohmann::basic_json; // NOLINT(misc-unused-using-decls)
    using nlohmann::json;       // NOLINT(misc-unused-using-decls)
} // namespace nlohmann
