cmake_minimum_required(VERSION 3.10)

project(BalatroDemake VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

enable_testing()

# Add our custom cmake module path so it can find FindSDL2.cmake if needed
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# The executable target
add_executable(balatro_demake
    src/main.cpp
    src/core/Application.cpp
    src/core/AssetPath.cpp
    src/core/StateMachine.cpp
    src/states/TitleState.cpp
    src/states/GameplayState.cpp
    src/states/ShopLayout.cpp
    src/states/BlindSelectState.cpp
    src/states/ShopState.cpp
    src/game/Deck.cpp
    src/game/RunState.cpp
    src/game/Hand.cpp
    src/game/CardRenderer.cpp
    src/game/HandEvaluator.cpp
    src/game/Joker.cpp
    src/game/ShopOffer.cpp
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

if(NOT N3DS)
    add_executable(run_state_tests
        tests/RunStateTests.cpp
        src/game/Deck.cpp
        src/game/RunState.cpp
        src/game/Joker.cpp
        src/game/HandEvaluator.cpp
    )
    target_include_directories(run_state_tests PRIVATE src)
    add_test(NAME run_state_tests COMMAND run_state_tests)

    add_executable(hand_tests
        tests/HandTests.cpp
        src/game/Hand.cpp
    )
    target_include_directories(hand_tests PRIVATE src)
    add_test(NAME hand_tests COMMAND hand_tests)

    add_executable(hand_evaluator_tests
        tests/HandEvaluatorTests.cpp
        src/game/HandEvaluator.cpp
        src/game/Deck.cpp
        src/game/RunState.cpp
        src/game/Joker.cpp
    )
    target_include_directories(hand_evaluator_tests PRIVATE src)
    add_test(NAME hand_evaluator_tests COMMAND hand_evaluator_tests)

    add_executable(joker_tests
        tests/JokerTests.cpp
        src/game/Deck.cpp
        src/game/RunState.cpp
        src/game/Joker.cpp
        src/game/HandEvaluator.cpp
        src/states/ShopLayout.cpp
    )
    target_include_directories(joker_tests PRIVATE src)
    add_test(NAME joker_tests COMMAND joker_tests)

    add_executable(shop_offer_tests
        tests/ShopOfferTests.cpp
        src/game/Deck.cpp
        src/game/RunState.cpp
        src/game/Joker.cpp
        src/game/HandEvaluator.cpp
        src/game/ShopOffer.cpp
    )
    target_include_directories(shop_offer_tests PRIVATE src)
    add_test(NAME shop_offer_tests COMMAND shop_offer_tests)

    add_executable(shop_layout_tests
        tests/ShopLayoutTests.cpp
        src/states/ShopLayout.cpp
        src/game/Joker.cpp
    )
    target_include_directories(shop_layout_tests PRIVATE src)
    add_test(NAME shop_layout_tests COMMAND shop_layout_tests)

    add_executable(asset_path_tests
        tests/AssetPathTests.cpp
        src/core/AssetPath.cpp
    )
    target_include_directories(asset_path_tests PRIVATE src)
    add_test(NAME asset_path_tests COMMAND asset_path_tests)
endif()
