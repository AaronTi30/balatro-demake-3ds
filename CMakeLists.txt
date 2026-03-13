cmake_minimum_required(VERSION 3.10)

project(BalatroDemake VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add our custom cmake module path so it can find FindSDL2.cmake if needed
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# The executable target
add_executable(balatro_demake
    src/main.cpp
    src/core/Application.cpp
    src/core/StateMachine.cpp
    src/states/TitleState.cpp
    src/states/GameplayState.cpp
    src/game/Deck.cpp
    src/game/Hand.cpp
    src/game/CardRenderer.cpp
    src/game/HandEvaluator.cpp
    src/core/TextRenderer.cpp
)

if(N3DS)
    # 3DS has no SDL2 package, use citro2d.
    # The toolchain will already link our required libraries if we use the add_3dsx_target macro
    add_3dsx_target(balatro_demake)
    target_compile_definitions(balatro_demake PRIVATE N3DS)
else()
    # Find SDL2 for PC
    find_package(SDL2 REQUIRED)

    # Use the variables returned by FindSDL2.cmake instead of the imported target
    target_include_directories(balatro_demake PRIVATE ${SDL2_INCLUDE_DIR})
    target_link_libraries(balatro_demake PRIVATE ${SDL2_LIBRARY})

    # SDL2_ttf for text rendering
    find_library(SDL2_TTF_LIB SDL2_ttf HINTS /opt/homebrew/lib /usr/local/lib)
    if(SDL2_TTF_LIB)
        target_link_libraries(balatro_demake PRIVATE ${SDL2_TTF_LIB})
    else()
        message(WARNING "SDL2_ttf not found - text rendering will not work")
    endif()
endif()

# Also include src dir
target_include_directories(balatro_demake PRIVATE src)
