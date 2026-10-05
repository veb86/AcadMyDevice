# Модульные тесты MyDevice на имитации ObjectARX (без AutoCAD и ObjectARX SDK).
#
#   cmake -S tests -B build
#   cmake --build build
#   ctest --test-dir build --output-on-failure
cmake_minimum_required(VERSION 3.10)
project(MyDeviceTests CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# MyDeviceOPM.cpp (COM-интерфейсы палитры свойств) собирается только MSVC;
# в тестах его заменяет mock/opm_stub.cpp.
#
# Исходники копируются в каталог сборки: директива #include "StdAfx.h" ищет файл
# сначала рядом с исходником, и копия позволяет подставить tests/mock/StdAfx.h
# вместо src/StdAfx.h, не изменяя сами исходники.
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../src)
set(COPIED_DIR ${CMAKE_CURRENT_BINARY_DIR}/src)
set(PRODUCT_FILES
    MyDevice.h
    MyDevice.cpp
    MyDeviceBlock.h
    MyDeviceBlock.cpp
    MyDeviceCommand.h
    MyDeviceCommand.cpp
    MyDeviceOPM.h
    MyDeviceProperties.h
    MyDeviceProperties.cpp
    acrxEntryPoint.cpp)
set(PRODUCT_SOURCES)
foreach(file ${PRODUCT_FILES})
    configure_file(${SRC_DIR}/${file} ${COPIED_DIR}/${file} COPYONLY)
    if(file MATCHES "\\.cpp$")
        list(APPEND PRODUCT_SOURCES ${COPIED_DIR}/${file})
    endif()
endforeach()

add_executable(MyDeviceTests
    MyDeviceTests.cpp
    mock/arx_mock.cpp
    mock/opm_stub.cpp
    ${PRODUCT_SOURCES})
target_include_directories(MyDeviceTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/mock
    ${COPIED_DIR})
target_compile_definitions(MyDeviceTests PRIVATE UNICODE _UNICODE)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(MyDeviceTests PRIVATE -Wall -Wextra -Werror)
endif()

enable_testing()
add_test(NAME MyDeviceTests COMMAND MyDeviceTests)
set_tests_properties(MyDeviceTests PROPERTIES TIMEOUT 60)
