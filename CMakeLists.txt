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
    src/states/GameplayLayout.cpp
    src/states/GameplayHudStyle.cpp
    src/states/ShopLayout.cpp
    src/states/BlindSelectState.cpp
    src/states/ShopState.cpp
    src/game/Deck.cpp
    src/game/RunState.cpp
    src/game/Hand.cpp
    src/game/CardRenderer.cpp
    src/game/HandEvaluator.cpp
    src/game/ScoringAnimator.cpp
    src/game/Joker.cpp
    src/game/ShopOffer.cpp
    src/core/TextRenderer.cpp
    src/core/ScreenRenderer.cpp
)

if(N3DS)
    # Generate SMDH metadata for the 3dsx package
    set(SMDH_FILE ${CMAKE_CURRENT_BINARY_DIR}/balatro_demake.smdh)
    add_custom_command(
        OUTPUT ${SMDH_FILE}
        COMMAND ${DEVKITPRO}/tools/bin/smdhtool --create
                "Balatro Demake"
                "A Balatro demake for Nintendo 3DS"
                "balatro-demake"
                ${CMAKE_CURRENT_SOURCE_DIR}/assets/textures/icon.png
                ${SMDH_FILE}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/assets/textures/icon.png
        COMMENT "Generating SMDH metadata"
    )
    add_custom_target(balatro_smdh DEPENDS ${SMDH_FILE})

    add_3dsx_target(balatro_demake)
    add_dependencies(balatro_demake balatro_smdh)

    add_custom_command(TARGET balatro_demake POST_BUILD
        COMMAND ${DEVKITPRO}/tools/bin/3dsxtool $<TARGET_FILE:balatro_demake>
                ${CMAKE_CURRENT_BINARY_DIR}/balatro_demake.3dsx
                --romfs=${CMAKE_CURRENT_BINARY_DIR}/romfs
                --smdh=${SMDH_FILE}
        COMMENT "Generating .3dsx file"
    )
    target_compile_definitions(balatro_demake PRIVATE N3DS)

    set(ROMFS_DIR ${CMAKE_CURRENT_BINARY_DIR}/romfs)
    set(ROMFS_TEX_DIR ${ROMFS_DIR}/textures)
    set(TEX3DS_TOOL ${DEVKITPRO}/tools/bin/tex3ds)

    add_custom_command(
        OUTPUT ${ROMFS_TEX_DIR}/8bitdeck.t3x
        COMMAND ${CMAKE_COMMAND} -E make_directory ${ROMFS_TEX_DIR}
        COMMAND ${TEX3DS_TOOL} -f rgba8888 -z auto
                -o ${ROMFS_TEX_DIR}/8bitdeck.t3x
                ${CMAKE_CURRENT_SOURCE_DIR}/assets/textures/8BitDeck.png
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/assets/textures/8BitDeck.png
        COMMENT "Converting 8bitdeck.png to 8bitdeck.t3x"
    )

    add_custom_target(card_textures DEPENDS ${ROMFS_TEX_DIR}/8bitdeck.t3x)
    add_dependencies(balatro_demake card_textures)
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

    add_executable(gameplay_layout_tests
        tests/GameplayLayoutTests.cpp
        src/states/GameplayLayout.cpp
    )
    target_include_directories(gameplay_layout_tests PRIVATE src)
    add_test(NAME gameplay_layout_tests COMMAND gameplay_layout_tests)

    add_executable(gameplay_hud_style_tests
        tests/GameplayHudStyleTests.cpp
        src/states/GameplayHudStyle.cpp
    )
    target_include_directories(gameplay_hud_style_tests PRIVATE src)
    add_test(NAME gameplay_hud_style_tests COMMAND gameplay_hud_style_tests)

    add_executable(asset_path_tests
        tests/AssetPathTests.cpp
        src/core/AssetPath.cpp
    )
    target_include_directories(asset_path_tests PRIVATE src ${SDL2_INCLUDE_DIR})
    target_link_libraries(asset_path_tests PRIVATE ${SDL2_LIBRARY})
    add_test(NAME asset_path_tests COMMAND asset_path_tests)

    add_executable(card_renderer_sprite_sheet_tests
        tests/CardRendererSpriteSheetTests.cpp
        src/game/CardRenderer.cpp
        src/game/Hand.cpp
        src/core/AssetPath.cpp
        src/core/TextRenderer.cpp
    )
    target_include_directories(card_renderer_sprite_sheet_tests PRIVATE src ${SDL2_INCLUDE_DIR})
    target_link_libraries(card_renderer_sprite_sheet_tests PRIVATE ${SDL2_LIBRARY})
    if(SDL2_TTF_LIB)
        target_link_libraries(card_renderer_sprite_sheet_tests PRIVATE ${SDL2_TTF_LIB})
    endif()
    add_test(NAME card_renderer_sprite_sheet_tests COMMAND card_renderer_sprite_sheet_tests)

    add_executable(text_renderer_tests
        tests/TextRendererTests.cpp
        src/core/AssetPath.cpp
        src/core/TextRenderer.cpp
    )
    target_include_directories(text_renderer_tests PRIVATE src ${SDL2_INCLUDE_DIR})
    target_link_libraries(text_renderer_tests PRIVATE ${SDL2_LIBRARY})
    if(SDL2_TTF_LIB)
        target_link_libraries(text_renderer_tests PRIVATE ${SDL2_TTF_LIB})
    endif()
    add_test(NAME text_renderer_tests COMMAND text_renderer_tests)

    add_executable(scoring_animator_tests
        tests/ScoringAnimatorTests.cpp
        src/game/ScoringAnimator.cpp
        src/game/CardRenderer.cpp
        src/game/Hand.cpp
        src/core/AssetPath.cpp
        src/core/ScreenRenderer.cpp
        src/core/TextRenderer.cpp
        src/game/Joker.cpp
        src/game/HandEvaluator.cpp
        src/game/Deck.cpp
        src/game/RunState.cpp
    )
    target_include_directories(scoring_animator_tests PRIVATE src ${SDL2_INCLUDE_DIR})
    target_link_libraries(scoring_animator_tests PRIVATE ${SDL2_LIBRARY})
    if(SDL2_TTF_LIB)
        target_link_libraries(scoring_animator_tests PRIVATE ${SDL2_TTF_LIB})
    endif()
    add_test(NAME scoring_animator_tests COMMAND scoring_animator_tests)
endif()
