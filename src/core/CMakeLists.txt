add_library(unirally_sampling STATIC track_sampling.cpp track_progress.cpp)
target_include_directories(unirally_sampling PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(unirally_sampling PUBLIC lab_options)

add_library(unirally_input_timer STATIC input_timer.cpp)
target_include_directories(unirally_input_timer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(unirally_input_timer PUBLIC lab_options)

add_library(unirally_contact STATIC flat_contact.cpp vertical_contact.cpp)
target_include_directories(unirally_contact PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(unirally_contact PUBLIC unirally_sampling lab_options)

add_library(unirally_speed_limits STATIC speed_limits.cpp)
target_include_directories(unirally_speed_limits PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(unirally_speed_limits PUBLIC lab_options)

add_library(unirally_movement STATIC
    movement.cpp content_pack.cpp rider_motion.cpp rider_pose.cpp reward_queue.cpp
    opponent_ai.cpp race_demo_controls.cpp hunter_effects.cpp trick_roll.cpp race_progress.cpp race_camera.cpp
    special_tiles.cpp stunt_event.cpp race_setup.cpp race_state_io.cpp race_update.cpp)
target_include_directories(unirally_movement PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(unirally_movement PUBLIC unirally_contact unirally_input_timer unirally_speed_limits unirally_sampling lab_options)
add_executable(movement_runner movement_runner.cpp)
target_link_libraries(movement_runner PRIVATE unirally_movement)
add_library(unirally_presentation STATIC presentation.cpp picture.cpp result_screen.cpp dragster_picture.cpp
            race_windows.cpp race_hud.cpp neon_lighting.cpp rider_object.cpp rider_look.cpp snes_screen.cpp
            text_printer.cpp front_end.cpp rider_menu.cpp screen_slide.cpp tour_menu.cpp
            track_menu.cpp now_playing.cpp race_result.cpp local_modes.cpp options_menu.cpp league_menu.cpp league_completion.cpp lap_result.cpp stunt_result.cpp
            award.cpp
            tour_ending.cpp
            tour_ending_scripts.cpp tour_ending_right_column.cpp
            hunter_ending.cpp)
target_include_directories(unirally_presentation PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(unirally_presentation PUBLIC unirally_movement lab_options)
add_executable(presentation_runner presentation_runner.cpp)
target_link_libraries(presentation_runner PRIVATE unirally_presentation)

add_executable(zoom_zoo_runner zoom_zoo_runner.cpp)
target_link_libraries(zoom_zoo_runner PRIVATE unirally_movement)

add_executable(front_end_runner front_end_runner.cpp)
target_link_libraries(front_end_runner PRIVATE unirally_presentation)
add_executable(classic_race_presentation_runner classic_race_presentation_runner.cpp)
target_link_libraries(classic_race_presentation_runner PRIVATE unirally_presentation)
add_executable(split_demo_runner split_demo_runner.cpp)
target_link_libraries(split_demo_runner PRIVATE unirally_presentation)

# Recovered data parser; cold scheduling/voice DSP integration remains in progress.
add_library(unirally_audio_score STATIC audio_score.cpp audio_score_controls.cpp audio_voice.cpp audio_voice_timing.cpp audio_timers.cpp audio_driver.cpp audio_driver_boot.cpp audio_driver_pending.cpp audio_driver_pending_boot.cpp audio_driver_pending_samples.cpp audio_driver_pending_loop.cpp audio_ipl.cpp audio_cpu_clock.cpp audio_cpu_clock_interrupt.cpp audio_cpu_clock_controllers.cpp audio_cpu_boot.cpp audio_cpu_upload.cpp audio_cpu_queue.cpp audio_cpu_scene.cpp audio_cpu_title.cpp audio_cpu_title_hold.cpp audio_cpu_menu.cpp audio_cpu_menu_records.cpp audio_cpu_menu_defaults.cpp audio_cpu_menu_league.cpp audio_cpu_interrupt.cpp)
target_include_directories(unirally_audio_score PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(unirally_audio_score PUBLIC lab_options)
