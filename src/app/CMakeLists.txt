find_package(Threads REQUIRED)
add_library(unirally_frontend STATIC frontend.cpp save_file.cpp title_menu_audio_stream.cpp)
target_include_directories(unirally_frontend PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(unirally_frontend PUBLIC unirally_presentation unirally_audio_score Threads::Threads lab_options)
add_executable(live_presentation_runner live_presentation_runner.cpp)
target_link_libraries(live_presentation_runner PRIVATE unirally_frontend)
add_executable(dragster_fuzz_runner dragster_fuzz_runner.cpp)
target_link_libraries(dragster_fuzz_runner PRIVATE unirally_frontend)

if(LAB_BUILD_APP)
  include(FetchContent)
  file(READ "${PROJECT_SOURCE_DIR}/tools/locks/frontend.json" FRONTEND_LOCK)
  string(JSON SDL_VERSION GET "${FRONTEND_LOCK}" dependencies sdl3 version)
  string(JSON SDL_SOURCE_URL GET "${FRONTEND_LOCK}" dependencies sdl3 source_url)
  string(JSON SDL_SOURCE_SHA256 GET "${FRONTEND_LOCK}" dependencies sdl3 sha256)
  # The same archive from libsdl.org, tried in order after the GitHub release when it fails (GitHub's
  # release downloads answered 504 on and off on 8 November 2026). URL_HASH checks whichever arrives.
  # `mirror_urls` is optional: a missing key or an empty list means the GitHub URL alone.
  set(SDL_SOURCE_URLS "${SDL_SOURCE_URL}")
  string(JSON SDL_MIRROR_COUNT ERROR_VARIABLE SDL_MIRROR_ERROR
    LENGTH "${FRONTEND_LOCK}" dependencies sdl3 mirror_urls)
  if(SDL_MIRROR_ERROR)
    set(SDL_MIRROR_COUNT 0)
  endif()
  if(SDL_MIRROR_COUNT GREATER 0)
    math(EXPR SDL_MIRROR_LAST "${SDL_MIRROR_COUNT} - 1")
    foreach(SDL_MIRROR_INDEX RANGE ${SDL_MIRROR_LAST})
      string(JSON SDL_MIRROR_URL GET "${FRONTEND_LOCK}" dependencies sdl3 mirror_urls ${SDL_MIRROR_INDEX})
      list(APPEND SDL_SOURCE_URLS "${SDL_MIRROR_URL}")
    endforeach()
  endif()
  message(STATUS "Pinned local SDL ${SDL_VERSION}: ${SDL_SOURCE_SHA256}")
  set(SDL_SHARED ON CACHE BOOL "" FORCE)
  set(SDL_STATIC OFF CACHE BOOL "" FORCE)
  set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
  set(SDL_TESTS OFF CACHE BOOL "" FORCE)
  set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(SDL_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
  set(SDL_DISABLE_INSTALL_DOCS ON CACHE BOOL "" FORCE)
  FetchContent_Declare(SDL3
    URL ${SDL_SOURCE_URLS}
    URL_HASH "SHA256=${SDL_SOURCE_SHA256}"
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
  FetchContent_MakeAvailable(SDL3)
  if(APPLE)
    add_executable(unirally MACOSX_BUNDLE sdl_main.cpp sdl_audio.cpp)
    set_target_properties(unirally PROPERTIES
      MACOSX_BUNDLE_BUNDLE_NAME "Unirally"
      MACOSX_BUNDLE_GUI_IDENTIFIER "org.unirally.classic")
  else()
    add_executable(unirally WIN32 sdl_main.cpp sdl_audio.cpp)
  endif()
  target_link_libraries(unirally PRIVATE unirally_frontend SDL3::SDL3)
endif()
