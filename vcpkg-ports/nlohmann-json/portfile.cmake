# This is the vcpkg nlohmann-json portfile pinned to a develop commit, which ships the nlohmann.json
# module interface (src/modules/json.cppm). Remove this overlay once a release containing it is in vcpkg.
# https://github.com/microsoft/vcpkg/blob/39344dff01c5a5a0134caf2624cdd492f05d30ea/ports/nlohmann-json/portfile.cmake

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO nlohmann/json
    REF 755bb16d7a80436646380b5f871b93c6a443a3b7
    SHA512 0c59d072185adaf3b90fa8a3bafe42c95e3c33bf461c78a7223d85e74b61786b99edb134d7b6c61ed104cbfd3733283bda79f00e8c08923bab621c001838ea50
    HEAD_REF develop
)

if(NOT DEFINED nlohmann-json_IMPLICIT_CONVERSIONS)
    set(nlohmann-json_IMPLICIT_CONVERSIONS ON)
endif()

vcpkg_check_features(OUT_FEATURE_OPTIONS FEATURE_OPTIONS
FEATURES
    "diagnostics"           JSON_Diagnostics
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS ${FEATURE_OPTIONS}
        -DJSON_Install=ON
        -DJSON_MultipleHeaders=ON
        -DJSON_BuildTests=OFF
        -DJSON_ImplicitConversions=${nlohmann-json_IMPLICIT_CONVERSIONS}
)
vcpkg_cmake_install()
vcpkg_cmake_config_fixup(PACKAGE_NAME "nlohmann_json" CONFIG_PATH "share/cmake/nlohmann_json")
vcpkg_fixup_pkgconfig()

vcpkg_replace_string(
    "${CURRENT_PACKAGES_DIR}/share/nlohmann_json/nlohmann_jsonTargets.cmake"
    "{_IMPORT_PREFIX}/nlohmann_json.natvis"
    "{_IMPORT_PREFIX}/share/nlohmann_json/nlohmann_json.natvis"
    IGNORE_UNCHANGED
)
if(EXISTS "${CURRENT_PACKAGES_DIR}/nlohmann_json.natvis")
    file(RENAME
        "${CURRENT_PACKAGES_DIR}/nlohmann_json.natvis"
        "${CURRENT_PACKAGES_DIR}/share/nlohmann_json/nlohmann_json.natvis"
    )
endif()

# ======== Begin modifications ========

# The CMake install does not ship the module interface, since it is only built through
# NLOHMANN_JSON_BUILD_MODULES. Consumers compile it themselves.
file(INSTALL "${SOURCE_PATH}/src/modules/json.cppm" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}/modules")

# ========  End modifications  ========

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug")

# Handle copyright
file(INSTALL "${SOURCE_PATH}/LICENSE.MIT" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)

# Handle usage
file(COPY "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
