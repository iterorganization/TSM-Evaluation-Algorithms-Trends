# cmake/epics-config.cmake

function(require_epics)
    # Read from env only if not already set
    if(NOT DEFINED EPICS_BASE AND DEFINED ENV{EPICS_BASE})
        set(EPICS_BASE "$ENV{EPICS_BASE}" CACHE PATH "Path to EPICS base installation" FORCE)
    endif()
    if(NOT DEFINED EPICS_HOST_ARCH AND DEFINED ENV{EPICS_HOST_ARCH})
        set(EPICS_HOST_ARCH "$ENV{EPICS_HOST_ARCH}" CACHE STRING "EPICS host architecture" FORCE)
    endif()

    # Fail immediately if still missing
    if(NOT DEFINED EPICS_BASE OR NOT EPICS_BASE)
        message(FATAL_ERROR "EPICS_BASE is not set. Set it via:
  1. Environment variable: export EPICS_BASE=/path
  2. ccmake: set EPICS_BASE in the options list
  3. Command line: cmake -DEPICS_BASE=/path
  )
")
    endif()
    if(NOT DEFINED EPICS_HOST_ARCH OR NOT EPICS_HOST_ARCH)
        message(FATAL_ERROR "EPICS_HOST_ARCH is not set. Set it via:
  1. Environment variable: export EPICS_HOST_ARCH=arch
  2. ccmake: set EPICS_HOST_ARCH in the options list
  3. Command line: cmake -DEPICS_HOST_ARCH=arch
")
    endif()
endfunction()
