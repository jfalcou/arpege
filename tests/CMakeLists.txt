# One executable per test file: TTS_MAIN provides a main() per translation unit.
function(arpg_add_test name)
  add_executable(${name} ${name}.cpp)
  target_link_libraries(${name} PRIVATE arpg_core tts arpg_warnings)
  add_test(NAME ${name} COMMAND ${name})
endfunction()

arpg_add_test(viewport)
arpg_add_test(screen_stack)
