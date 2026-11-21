cmake_minimum_required(VERSION 3.25)

project(subornoime VERSION 0.1.0 LANGUAGES CXX)

option(BUILD_TESTING "Build SUBORNO test executables" OFF)
option(SUBORNO_BUILD_BENCHMARKS "Build optional Google Benchmark executables" OFF)
option(SUBORNO_ENABLE_MSVC_ANALYSIS "Enable MSVC static analysis with /analyze" ON)
option(SUBORNO_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(SUBORNO_BUILD_FUZZING "Build optional libFuzzer targets" OFF)

include("${PROJECT_SOURCE_DIR}/cmake/modules/SubornoBuildOptions.cmake")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(ICU REQUIRED COMPONENTS uc)
find_package(unofficial-sqlite3 CONFIG REQUIRED)

add_library(suborno_unicode STATIC src/unicode/bengali.cpp)
target_include_directories(suborno_unicode PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(suborno_unicode PUBLIC ICU::uc)

add_library(suborno_layout_api INTERFACE)
target_include_directories(suborno_layout_api INTERFACE "${PROJECT_SOURCE_DIR}/include")
add_library(suborno_layout_registry STATIC
    src/layout-api/layout_registry.cpp
    src/layout-api/declarative_layout.cpp
)
target_include_directories(suborno_layout_registry PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(suborno_layout_registry PUBLIC suborno_layout_api suborno_unicode)

add_library(suborno_dictionary STATIC src/dictionary/dictionary.cpp)
target_include_directories(suborno_dictionary PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(suborno_dictionary PUBLIC unofficial::sqlite3::sqlite3)

add_library(suborno_layout_sessions STATIC
    src/layouts/layout_input_session.cpp
)
target_include_directories(suborno_layout_sessions PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(suborno_layout_sessions PUBLIC
    suborno_layout_api
    suborno_unicode
    suborno_dictionary
)

add_library(suborno_phonetic STATIC
    src/layouts/phonetic/suborno_phonetic_layout.cpp
    src/layouts/phonetic/borno_phonetic_layout.cpp
    src/layouts/phonetic/avro_phonetic_layout.cpp
    src/layouts/phonetic/rule_based_phonetic_layout.cpp
    src/layouts/phonetic/phonetic_input_session.cpp
    src/layouts/phonetic/phonetic_parser.cpp
    src/layouts/phonetic/phonetic_tokenizer.cpp
)
target_include_directories(suborno_phonetic PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_include_directories(suborno_phonetic PRIVATE "${PROJECT_SOURCE_DIR}/src")
target_link_libraries(suborno_phonetic PUBLIC
    suborno_layout_api
    suborno_unicode
    suborno_layout_sessions
)

add_library(suborno_probhat STATIC
    src/layouts/probhat/probhat_layout.cpp
)
target_include_directories(suborno_probhat PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(suborno_probhat PUBLIC suborno_layout_api suborno_unicode)

add_library(suborno_national STATIC
    src/layouts/national/national_layout.cpp
)
target_include_directories(suborno_national PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(suborno_national PUBLIC suborno_layout_api suborno_unicode)

add_library(suborno_spell STATIC
    src/spell/spell_checker.cpp
)
target_include_directories(suborno_spell PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(suborno_spell PUBLIC suborno_layout_api suborno_dictionary)

add_library(suborno_candidates STATIC
    src/candidates/candidate_engine.cpp
    src/candidates/language_intelligence.cpp
)
target_include_directories(suborno_candidates PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(suborno_candidates PUBLIC
    suborno_layout_api
    suborno_phonetic
    suborno_unicode
    suborno_dictionary
)

add_library(suborno_persistence STATIC src/persistence/atomic_file.cpp)
target_include_directories(suborno_persistence PUBLIC
    "${PROJECT_SOURCE_DIR}/include"
    "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(suborno_persistence PUBLIC unofficial::sqlite3::sqlite3)

add_library(suborno_settings STATIC src/settings/settings.cpp)
target_include_directories(suborno_settings PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(suborno_settings PRIVATE suborno_persistence)

add_library(suborno_core STATIC
    src/core/project.cpp
    src/core/engine.cpp
    src/core/c_api.cpp
)
target_include_directories(suborno_core PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(suborno_core
    PUBLIC
        suborno_unicode
        suborno_layout_api
        suborno_phonetic
        suborno_probhat
        suborno_national
        suborno_candidates
        suborno_dictionary
        suborno_persistence
        suborno_layout_registry
        suborno_layout_sessions
        suborno_settings
)
target_compile_definitions(suborno_core PUBLIC SUBORNO_API_STATIC)

function(suborno_configure_target target_name)
    set_target_properties(${target_name} PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
    )
    suborno_apply_build_options(${target_name})
endfunction()

function(suborno_configure_test_target target_name)
    suborno_configure_target(${target_name})
    if(MSVC AND SUBORNO_ENABLE_MSVC_ANALYSIS)
        target_compile_options("${target_name}" PRIVATE /analyze:external-)
    endif()
endfunction()

function(suborno_mark_dependency_headers_external target_name dependency_target)
    get_target_property(dependency_include_directories
        "${dependency_target}" INTERFACE_INCLUDE_DIRECTORIES)
    if(dependency_include_directories)
        target_include_directories("${target_name}" SYSTEM PRIVATE
            ${dependency_include_directories})
    endif()
endfunction()

suborno_configure_target(suborno_unicode)
suborno_configure_target(suborno_phonetic)
suborno_configure_target(suborno_candidates)
suborno_configure_target(suborno_layout_registry)
suborno_configure_target(suborno_settings)
suborno_configure_target(suborno_persistence)
suborno_configure_target(suborno_dictionary)
suborno_configure_target(suborno_layout_sessions)
suborno_configure_target(suborno_core)

if(UNIX AND NOT APPLE)
    add_library(suborno_linux_adapter STATIC src/platform/linux/linux_adapter.cpp)
    target_include_directories(suborno_linux_adapter PUBLIC "${PROJECT_SOURCE_DIR}/include")
    target_link_libraries(suborno_linux_adapter PUBLIC suborno_core)
    suborno_configure_target(suborno_linux_adapter)
elseif(APPLE)
    add_library(suborno_macos_adapter STATIC src/platform/macos/macos_adapter.cpp)
    target_include_directories(suborno_macos_adapter PUBLIC "${PROJECT_SOURCE_DIR}/include")
    target_link_libraries(suborno_macos_adapter PUBLIC suborno_core)
    suborno_configure_target(suborno_macos_adapter)
endif()

if(WIN32)
    if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR NOT MSVC_TOOLSET_VERSION STREQUAL "143")
        message(FATAL_ERROR
            "The Windows production target requires the MSVC v143 toolset; "
            "MinGW, GCC, clang-cl, and other toolsets are unsupported.")
    endif()

    find_package(wil CONFIG REQUIRED)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    enable_language(RC)

    set(SUBORNO_ICON_FILE "${CMAKE_CURRENT_BINARY_DIR}/suborno.ico")
    execute_process(
        COMMAND "${Python3_EXECUTABLE}"
                "${PROJECT_SOURCE_DIR}/src/platform/windows/resources/generate_icon.py"
                "${SUBORNO_ICON_FILE}"
        RESULT_VARIABLE icon_result
        ERROR_VARIABLE icon_error
    )
    if(NOT "${icon_result}" STREQUAL "0")
        message(FATAL_ERROR "Could not generate the Suborno application icon: ${icon_error}")
    endif()
    configure_file(
        "${PROJECT_SOURCE_DIR}/src/platform/windows/resources/version.hpp.in"
        "${CMAKE_CURRENT_BINARY_DIR}/suborno_version.hpp"
        @ONLY
    )
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/suborno_version.iss.inc"
        "#define MyAppVersion \"${PROJECT_VERSION}\"\n")

    set(SUBORNO_FILE_DESCRIPTION "Suborno Bengali Input Method")
    set(SUBORNO_INTERNAL_NAME "Suborno")
    set(SUBORNO_ORIGINAL_FILENAME "Suborno.exe")
    set(SUBORNO_FILE_TYPE VFT_APP)
    configure_file(
        "${PROJECT_SOURCE_DIR}/src/platform/windows/resources/suborno.rc.in"
        "${CMAKE_CURRENT_BINARY_DIR}/suborno_app.rc"
        @ONLY
    )
    set(SUBORNO_FILE_TYPE VFT_DLL)
    set(SUBORNO_ORIGINAL_FILENAME "suborno_windows_tsf.dll")
    configure_file(
        "${PROJECT_SOURCE_DIR}/src/platform/windows/resources/suborno.rc.in"
        "${CMAKE_CURRENT_BINARY_DIR}/suborno_tsf.rc"
        @ONLY
    )

    add_library(suborno_windows_tsf SHARED
        src/platform/windows/app_paths.cpp
        "${CMAKE_CURRENT_BINARY_DIR}/suborno_tsf.rc"
        src/platform/windows/tsf/suborno_windows_tsf.def
        src/platform/windows/tsf/module_smoke.cpp
        src/platform/windows/tsf/guids.cpp
        src/platform/windows/tsf/text_edit_session.cpp
        src/platform/windows/tsf/text_service.cpp
        src/platform/windows/tsf/class_factory.cpp
        src/platform/windows/tsf/dll_main.cpp
        src/platform/windows/registration/com_registration.cpp
        src/platform/windows/registration/tsf_profiles.cpp
    )
    target_include_directories(suborno_windows_tsf PUBLIC "${PROJECT_SOURCE_DIR}/include")
    target_compile_definitions(suborno_windows_tsf PRIVATE SUBORNO_WINDOWS_TSF_EXPORTS)
    target_link_libraries(suborno_windows_tsf PRIVATE
        suborno_core
        WIL::WIL
        Advapi32.lib
        Ole32.lib
        Shell32.lib
        Shlwapi.lib
        User32.lib
        Uuid.lib
    )
    suborno_configure_target(suborno_windows_tsf)
    if(MSVC AND SUBORNO_ENABLE_MSVC_ANALYSIS)
        target_compile_options(suborno_windows_tsf PRIVATE /analyze:external-)
        suborno_mark_dependency_headers_external(suborno_windows_tsf WIL::WIL)
    endif()

    add_executable(suborno_app WIN32
        "${CMAKE_CURRENT_BINARY_DIR}/suborno_app.rc"
        src/platform/windows/app_paths.cpp
        src/platform/windows/app/application.cpp
        src/platform/windows/app/main.cpp
        src/platform/windows/tsf/guids.cpp
    )
    set_target_properties(suborno_app PROPERTIES OUTPUT_NAME "Suborno")
    target_include_directories(suborno_app PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
    target_link_libraries(suborno_app PRIVATE
        Advapi32.lib
        Ole32.lib
        Shell32.lib
        suborno_core
        User32.lib
        Uuid.lib
    )
    suborno_configure_target(suborno_app)
endif()

include(CTest)
if(BUILD_TESTING)
    find_package(GTest CONFIG REQUIRED)

    add_executable(suborno_unit_tests
        tests/unit/atomic_file_test.cpp
        tests/unit/project_smoke_test.cpp
        tests/unit/phonetic_parser_test.cpp
        tests/unit/phonetic_input_session_test.cpp
        tests/unit/layout_input_session_test.cpp
        tests/unit/key_event_test.cpp
        tests/unit/borno_phonetic_layout_test.cpp
        tests/unit/avro_phonetic_layout_test.cpp
        tests/unit/phonetic_candidate_test.cpp
        tests/unit/phonetic_tokenizer_test.cpp
        tests/unit/unicode_test.cpp
        tests/unit/engine_test.cpp
        tests/unit/settings_test.cpp
        tests/unit/linguistic_regression_test.cpp
        tests/unit/probhat_layout_test.cpp
        tests/unit/national_layout_test.cpp
        tests/unit/spell_checker_test.cpp
        tests/unit/personal_dictionary_test.cpp
    )
    target_include_directories(suborno_unit_tests PRIVATE "${PROJECT_SOURCE_DIR}/src")
    target_link_libraries(suborno_unit_tests PRIVATE suborno_core GTest::gtest_main suborno_spell)
    if(MSVC AND SUBORNO_ENABLE_MSVC_ANALYSIS)
        suborno_mark_dependency_headers_external(suborno_unit_tests GTest::gtest)
        suborno_mark_dependency_headers_external(suborno_unit_tests GTest::gtest_main)
    endif()
    if(UNIX AND NOT APPLE)
        target_sources(suborno_unit_tests PRIVATE tests/unit/platform_adapter_test.cpp)
        target_link_libraries(suborno_unit_tests PRIVATE suborno_linux_adapter)
    endif()
    suborno_configure_test_target(suborno_unit_tests)
    add_test(NAME suborno.unit COMMAND suborno_unit_tests)

    add_executable(suborno_integration_tests tests/integration/target_graph_smoke_test.cpp)
    target_link_libraries(suborno_integration_tests PRIVATE suborno_core GTest::gtest_main)
    suborno_configure_test_target(suborno_integration_tests)
    if(MSVC AND SUBORNO_ENABLE_MSVC_ANALYSIS)
        suborno_mark_dependency_headers_external(suborno_integration_tests GTest::gtest)
        suborno_mark_dependency_headers_external(suborno_integration_tests GTest::gtest_main)
    endif()
    add_test(NAME suborno.integration COMMAND suborno_integration_tests)

    if(WIN32)
        add_executable(suborno_windows_tests
            tests/windows/windows_build_smoke_test.cpp
            src/platform/windows/app_paths.cpp
            src/platform/windows/tsf/guids.cpp
        )
        target_link_libraries(suborno_windows_tests PRIVATE
            suborno_core
            suborno_windows_tsf
            GTest::gtest_main
            WIL::WIL
            Ole32.lib
            Shell32.lib
            Uuid.lib
        )
        suborno_configure_test_target(suborno_windows_tests)
        if(MSVC AND SUBORNO_ENABLE_MSVC_ANALYSIS)
            suborno_mark_dependency_headers_external(suborno_windows_tests GTest::gtest)
            suborno_mark_dependency_headers_external(suborno_windows_tests GTest::gtest_main)
            suborno_mark_dependency_headers_external(suborno_windows_tests WIL::WIL)
        endif()
        add_test(NAME suborno.windows.tsf_contract COMMAND suborno_windows_tests)
        set_tests_properties(suborno.windows.tsf_contract PROPERTIES LABELS "windows;non-interactive")
        add_test(NAME suborno.windows.app_smoke COMMAND suborno_app --smoke-test)
        set_tests_properties(suborno.windows.app_smoke PROPERTIES
            LABELS "windows;non-interactive"
            TIMEOUT 30
        )
    endif()
endif()

if(SUBORNO_BUILD_BENCHMARKS)
    find_package(benchmark CONFIG REQUIRED)
    add_executable(suborno_benchmarks benchmarks/infrastructure_benchmark.cpp)
    target_link_libraries(suborno_benchmarks PRIVATE suborno_core benchmark::benchmark_main)
    suborno_configure_target(suborno_benchmarks)
endif()

if(SUBORNO_BUILD_FUZZING)
    if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        message(FATAL_ERROR "The optional libFuzzer target requires a Clang compiler.")
    endif()
    add_executable(suborno_phonetic_fuzz tests/fuzz/phonetic_fuzz.cpp)
    target_link_libraries(suborno_phonetic_fuzz PRIVATE suborno_core)
    target_compile_options(suborno_phonetic_fuzz PRIVATE
        -fsanitize=fuzzer,address,undefined
        -fno-omit-frame-pointer)
    target_link_options(suborno_phonetic_fuzz PRIVATE
        -fsanitize=fuzzer,address,undefined)
endif()
