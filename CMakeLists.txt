cmake_minimum_required(VERSION 3.20)
project(EchoesOfTheRift LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(raylib REQUIRED)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/imprints.cpp
    src/progression.cpp
    src/rift.cpp
    src/combat.cpp
    src/ui.cpp
    src/persistence.cpp
)
target_link_libraries(${PROJECT_NAME} PRIVATE raylib)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE winmm)
endif()

# Copy required DLLs next to executable after build
if(WIN32)
    set(MINGW_BIN "C:/msys64/mingw64/bin")
    set(DLLS_TO_COPY
        "${MINGW_BIN}/libraylib.dll"
        "${MINGW_BIN}/libgcc_s_seh-1.dll"
        "${MINGW_BIN}/libstdc++-6.dll"
        "${MINGW_BIN}/libwinpthread-1.dll"
        "${MINGW_BIN}/glfw3.dll"
    )
    foreach(DLL ${DLLS_TO_COPY})
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${DLL}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
        )
    endforeach()
endif()
