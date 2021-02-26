vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO sylveon/member_thunk
    HEAD_REF master
)

file(INSTALL ${SOURCE_PATH}/include/member_thunk DESTINATION ${CURRENT_PACKAGES_DIR}/include)
file(INSTALL ${SOURCE_PATH}/LICENSE DESTINATION ${CURRENT_PACKAGES_DIR}/share/${PORT} RENAME copyright)
