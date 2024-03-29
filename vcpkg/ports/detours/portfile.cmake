vcpkg_check_linkage(ONLY_STATIC_LIBRARY)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO microsoft/Detours
    REF 4b8c659f549b0ab21cf649377c7a84eb708f5e68
    SHA512 313e3db37d3271db366641e81c73dbbf1c397fe6daa52ef281c6b151dbb70aff42ce5a8b31329cd49ea4c0784da9db01181cb58d93b74155ff762bb094f82583
    HEAD_REF main
)

set(EXTRA_FLAGS "/guard:cf /guard:ehcont /ZH:SHA_256 /Qspectre /QIntel-jcc-erratum")

set(VCPKG_CXX_FLAGS "${VCPKG_CXX_FLAGS} ${EXTRA_FLAGS}")
set(VCPKG_CXX_FLAGS_RELEASE "${VCPKG_CXX_FLAGS_RELEASE} ${EXTRA_FLAGS_RELEASE}")

set(VCPKG_C_FLAGS "${VCPKG_C_FLAGS} ${EXTRA_FLAGS}")
set(VCPKG_C_FLAGS_RELEASE "${VCPKG_C_FLAGS_RELEASE} ${EXTRA_FLAGS_RELEASE}")

vcpkg_build_nmake(
    SOURCE_PATH "${SOURCE_PATH}"
    PROJECT_SUBPATH "src"
    PROJECT_NAME "Makefile"
    OPTIONS "PROCESSOR_ARCHITECTURE=${VCPKG_TARGET_ARCHITECTURE}"
    OPTIONS_RELEASE "DETOURS_CONFIG=Release"
    OPTIONS_DEBUG "DETOURS_CONFIG=Debug"
)

if(NOT DEFINED VCPKG_BUILD_TYPE OR VCPKG_BUILD_TYPE STREQUAL "release")
    file(INSTALL "${CURRENT_BUILDTREES_DIR}/${TARGET_TRIPLET}-rel/lib.${VCPKG_TARGET_ARCHITECTURE}Release/" DESTINATION "${CURRENT_PACKAGES_DIR}/lib")
endif()
if(NOT DEFINED VCPKG_BUILD_TYPE OR VCPKG_BUILD_TYPE STREQUAL "debug")
    file(INSTALL "${CURRENT_BUILDTREES_DIR}/${TARGET_TRIPLET}-dbg/lib.${VCPKG_TARGET_ARCHITECTURE}Debug/" DESTINATION "${CURRENT_PACKAGES_DIR}/debug/lib")
endif()

if(NOT DEFINED VCPKG_BUILD_TYPE OR VCPKG_BUILD_TYPE STREQUAL "release")
    file(INSTALL "${CURRENT_BUILDTREES_DIR}/${TARGET_TRIPLET}-rel/include/" DESTINATION "${CURRENT_PACKAGES_DIR}/include" RENAME detours)
else()
    file(INSTALL "${CURRENT_BUILDTREES_DIR}/${TARGET_TRIPLET}-dbg/include/" DESTINATION "${CURRENT_PACKAGES_DIR}/include" RENAME detours)
endif()

file(INSTALL "${SOURCE_PATH}/LICENSE.md" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)
