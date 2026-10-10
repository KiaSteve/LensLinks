cmake_minimum_required(VERSION 3.20)

project(LensLinksNative)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Link the C/C++ runtime statically (/MTd for Debug, /MT for Release) so the
# DLL has no dependency on vcruntime140d.dll / ucrtbased.dll etc. Frame Server
# runs as LocalService and cannot resolve the Debug runtime DLLs.
# (CMake 3.15+ with policy CMP0091 NEW, which cmake_minimum_required 3.20 sets.)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_library(LensLinksNative SHARED
    ffi/lenslinks.cpp

    virtual_camera/virtual_camera.cpp
    virtual_camera/shared_frame.cpp
    mf_camera/lenslinks_media_source_activate.cpp

    mf_camera/frame_provider.cpp
    mf_camera/virtual_camera_registration.cpp
    mf_camera/lenslinks_media_source.cpp
    mf_camera/lenslinks_media_stream.cpp
    mf_camera/lenslinks_com.cpp
    mf_camera/ll_logging.cpp

    LensLinksNative.def
)

target_include_directories(LensLinksNative PRIVATE
    .
    ffi
    virtual_camera
    mf_camera
)

target_link_libraries(LensLinksNative
    ws2_32
    ole32
    mfplat
    mf
    mfuuid
    mfsensorgroup
)

add_executable(activate_test
    ../native_tests/activate_test.cpp
)

target_link_libraries(activate_test
    ole32
    mfplat
    mf
    mfuuid
)

add_executable(virtual_camera_test
    ../native_tests/virtual_camera_test.cpp
    mf_camera/virtual_camera_registration.cpp
)

target_include_directories(virtual_camera_test PRIVATE
    .
    mf_camera
)

target_link_libraries(virtual_camera_test
    ole32
    mfplat
    mf
    mfuuid
    mfsensorgroup
)
add_executable(producer_test
    ../native_tests/producer_test.cpp
)

target_link_libraries(producer_test
    LensLinksNative
    ole32
    mfplat
    mf
    mfuuid
)
