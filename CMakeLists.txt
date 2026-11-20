cmake_minimum_required(VERSION 3.16)
project(GutC VERSION 0.1.0 LANGUAGES C)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
add_library(gutc STATIC src/gutc.c)
add_library(GutC::gutc ALIAS gutc)
set_target_properties(gutc PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED YES C_EXTENSIONS NO)
target_include_directories(gutc PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
if(MSVC)
  target_compile_options(gutc PRIVATE /W4 /WX)
else()
  target_compile_options(gutc PRIVATE -Wall -Wextra -Werror -Wpedantic)
  if(UNIX AND NOT APPLE)
    target_link_libraries(gutc PUBLIC m)
  endif()
endif()
include(CTest)
if(BUILD_TESTING)
  add_executable(gutc_test tests/test_gutc.c)
  target_link_libraries(gutc_test PRIVATE GutC::gutc)
  target_compile_definitions(gutc_test PRIVATE
    GUTC_FIXTURE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/testdata/upstream-golden/identity.tsv")
  add_test(NAME gutc_foundation COMMAND gutc_test)
endif()
install(TARGETS gutc EXPORT GutCTargets ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
configure_package_config_file(cmake/GutCConfig.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/GutCConfig.cmake"
  INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/GutC")
write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/GutCConfigVersion.cmake"
  VERSION ${PROJECT_VERSION} COMPATIBILITY SameMajorVersion)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/GutCConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/GutCConfigVersion.cmake"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/GutC")
install(EXPORT GutCTargets NAMESPACE GutC:: DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/GutC")
