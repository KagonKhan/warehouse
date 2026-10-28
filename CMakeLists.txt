cmake_minimum_required (VERSION 3.23)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project (warehouse CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


find_package(spdlog REQUIRED)

# =======================    PROJECT    ==========================


add_executable(warehouse
    src/main.cpp

    src/app/app.cpp

    src/utils/logger.cpp
)

target_include_directories(warehouse PUBLIC src)

target_link_libraries(warehouse PUBLIC
    spdlog::spdlog
)