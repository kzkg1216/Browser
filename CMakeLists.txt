cmake_minimum_required(VERSION 3.20)
project(MiniBrowser CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT WIN32)
    message(FATAL_ERROR "This project targets Windows only (WebView2).")
endif()

# --- WebView2 SDK を NuGet からダウンロードして展開する ---
set(WEBVIEW2_VERSION "1.0.2478.35")
set(WEBVIEW2_DIR "${CMAKE_BINARY_DIR}/webview2-sdk")

if(NOT EXISTS "${WEBVIEW2_DIR}/build/native/include/WebView2.h")
    message(STATUS "Downloading WebView2 SDK ${WEBVIEW2_VERSION} ...")
    file(DOWNLOAD
        "https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2/${WEBVIEW2_VERSION}"
        "${CMAKE_BINARY_DIR}/webview2.nupkg"
        SHOW_PROGRESS
        STATUS _dl_status)
    list(GET _dl_status 0 _dl_code)
    if(NOT _dl_code EQUAL 0)
        message(FATAL_ERROR "Failed to download WebView2 SDK: ${_dl_status}")
    endif()
    file(ARCHIVE_EXTRACT
        INPUT "${CMAKE_BINARY_DIR}/webview2.nupkg"
        DESTINATION "${WEBVIEW2_DIR}")
endif()

# ターゲットアーキテクチャの判定 (x64 / x86 / arm64)
if(CMAKE_SYSTEM_PROCESSOR MATCHES "ARM64|arm64")
    set(WEBVIEW2_ARCH "arm64")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(WEBVIEW2_ARCH "x64")
else()
    set(WEBVIEW2_ARCH "x86")
endif()

add_executable(MiniBrowser WIN32 src/main.cpp)

target_compile_definitions(MiniBrowser PRIVATE
    UNICODE _UNICODE
    _WIN32_WINNT=0x0A00  # Windows 10 以降
)

if(MSVC)
    target_compile_options(MiniBrowser PRIVATE /utf-8 /W4)
endif()

target_include_directories(MiniBrowser PRIVATE
    "${WEBVIEW2_DIR}/build/native/include")

target_link_libraries(MiniBrowser PRIVATE
    "${WEBVIEW2_DIR}/build/native/${WEBVIEW2_ARCH}/WebView2Loader.dll.lib"
    comctl32)

# 実行に必要な WebView2Loader.dll を exe の隣にコピー
add_custom_command(TARGET MiniBrowser POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${WEBVIEW2_DIR}/build/native/${WEBVIEW2_ARCH}/WebView2Loader.dll"
        "$<TARGET_FILE_DIR:MiniBrowser>")
