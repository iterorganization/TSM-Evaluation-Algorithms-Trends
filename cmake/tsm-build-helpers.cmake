function(tsm_apply_common_compile_and_link_settings target)
  target_compile_options(${target} PRIVATE
      -Wall -Wextra -Wpedantic -Wshadow -fPIC -Wno-variadic-macros
      $<$<CONFIG:Release>:-O3 -flto=full -pipe>
      $<$<CONFIG:Debug>:-O0 -g -fno-inline -fno-inline-small-functions -fno-default-inline --coverage -DDEBUG>
  )
  target_link_options(${target} PRIVATE $<$<CONFIG:Debug>:--coverage>)

  target_include_directories(${target} SYSTEM PRIVATE ${EXTRA_INC_DIRS})
  target_link_directories(${target} PRIVATE ${EXTRA_LIB_DIRS})
endfunction()

function(tsm_apply_common_compile_and_link_settings_epics target)
  tsm_apply_common_compile_and_link_settings(${target})

  target_include_directories(${target} SYSTEM PRIVATE
      ${EPICS_BASE}/include/os/Linux
      ${EPICS_BASE}/include/compiler/gcc
      ${EPICS_BASE}/include
  )
  target_link_directories(${target} PRIVATE ${EPICS_BASE}/lib/${EPICS_HOST_ARCH})
endfunction()
