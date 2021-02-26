set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE dynamic)

include("base.cmake")

set(VCPKG_C_FLAGS "${BASE_C_FLAGS} /QIntel-jcc-erratum /guard:ehcont")
set(VCPKG_CXX_FLAGS "${VCPKG_C_FLAGS} ${BASE_CXX_FLAGS}")

set(VCPKG_LINKER_FLAGS "${BASE_LINKER_FLAGS} /CETCOMPAT /guard:ehcont")
