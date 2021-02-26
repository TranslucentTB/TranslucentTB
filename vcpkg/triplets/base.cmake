set(BASE_C_FLAGS "/GS /guard:cf /fp:precise /Qspectre /sdl /ZH:SHA_256 /FC")
set(BASE_CXX_FLAGS "/Zc:preprocessor /permissive- /Zc:wchar_t /Zc:forScope /Zc:inline /GR- /std:c++latest /Zc:__cplusplus /Zc:externConstexpr")

set(VCPKG_C_FLAGS_RELEASE "/GL /Oy")
set(VCPKG_CXX_FLAGS_RELEASE "${VCPKG_C_FLAGS_RELEASE}")

set(BASE_LINKER_FLAGS "/guard:cf")
set(VCPKG_LINKER_FLAGS_RELEASE "/OPT:REF /OPT:ICF /LTCG /Brepro")
