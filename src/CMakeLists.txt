project(wa_engine)

include_directories(common)

add_subdirectory(neat)
add_subdirectory(mhea)
add_subdirectory(common)

add_executable(wa_engine common/wa_engine.c)
target_link_libraries(wa_engine neatlib)
target_link_libraries(wa_engine mhealib)
target_link_libraries(wa_engine commonlib)
if(NOT MSVC)
  target_link_libraries(wa_engine m)
endif()
