enable_testing()
find_package(GTest REQUIRED)

MARK_AS_ADVANCED(
    BUILD_GMOCK BUILD_GTEST BUILD_SHARED_LIBS
    gmock_build_tests gtest_build_samples gtest_build_tests
    gtest_disable_pthreads gtest_force_shared_crt gtest_hide_internal_symbols
)

MACRO(package_add_test TESTNAME)
    ADD_EXECUTABLE(${TESTNAME} ${TESTNAME}.cpp)
    ADD_TEST(NAME ${TESTNAME} COMMAND ${TESTNAME})
    TARGET_INCLUDE_DIRECTORIES(${TESTNAME} PUBLIC ..)
    TARGET_LINK_LIBRARIES(${TESTNAME} gtest gmock gtest_main PkgConfig::RtAudio Threads::Threads)
    GTEST_DISCOVER_TESTS(${TESTNAME}
        WORKING_DIRECTORY "${PROJECT_DIR}"
        PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_DIR}"
    )
    SET_TARGET_PROPERTIES(${TESTNAME} PROPERTIES FOLDER tests)
ENDMACRO()

PACKAGE_ADD_TEST(test)
