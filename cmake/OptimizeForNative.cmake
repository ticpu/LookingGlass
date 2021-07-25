set(OPTIMIZE_FOR_NATIVE AUTO CACHE STRING "Build with -march=native")
set_property(CACHE OPTIMIZE_FOR_NATIVE PROPERTY STRINGS AUTO ON OFF)

set(OPTIMIZE_FOR_NATIVE_ARCH "none")
if(OPTIMIZE_FOR_NATIVE STREQUAL "ON" OR (OPTIMIZE_FOR_NATIVE STREQUAL "AUTO" AND OPTIMIZE_FOR_NATIVE_DEFAULT))
  CHECK_C_COMPILER_FLAG("-march=native" COMPILER_SUPPORTS_MARCH_NATIVE)
  if(COMPILER_SUPPORTS_MARCH_NATIVE)
    set(OPTIMIZE_FOR_NATIVE_ARCH "native")
    set(OPTIMIZE_FOR_NATIVE_ON ON)
  endif()
elseif(OPTIMIZE_FOR_NATIVE STREQUAL "AUTO" OR OPTIMIZE_FOR_NATIVE STREQUAL "OFF")
  CHECK_C_COMPILER_FLAG("-march=x86-64-v2" COMPILER_SUPPORTS_MARCH_X86_64_V2)
  if(COMPILER_SUPPORTS_MARCH_X86_64_V2)
    set(OPTIMIZE_FOR_NATIVE_ARCH "x86-64-v2")
  else()
    CHECK_C_COMPILER_FLAG("-march=nehalem" COMPILER_SUPPORTS_MARCH_NEHALEM)
    if(COMPILER_SUPPORTS_MARCH_NEHALEM)
      set(OPTIMIZE_FOR_NATIVE_ARCH "nehalem")
    endif()
  endif()
else()
  set(OPTIMIZE_FOR_NATIVE_ARCH "${OPTIMIZE_FOR_NATIVE}")
  set(OPTIMIZE_FOR_NATIVE_ON ON)
endif()

if(NOT OPTIMIZE_FOR_NATIVE_ARCH STREQUAL "none")
  add_compile_options("-march=${OPTIMIZE_FOR_NATIVE_ARCH}")
endif()

add_feature_info(OPTIMIZE_FOR_NATIVE OPTIMIZE_FOR_NATIVE_ON "Optimized for ${OPTIMIZE_FOR_NATIVE_ARCH}")