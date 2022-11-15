# GPLv2 Licence

# not in macos input-SDK

# find_path(Charset_INCLUDE_DIR tiff.h
#   "${INPUT_SDK_PATH}/include"
#  NO_DEFAULT_PATH
# )
  
find_library(Charset_LIBRARY 
  NAMES charset 
  # PATHS "${INPUT_SDK_PATH}/lib"
  # NO_DEFAULT_PATH
)

find_package_handle_standard_args(
  Charset
  REQUIRED_VARS Charset_LIBRARY # Charset_INCLUDE_DIR
)

if(Charset_FOUND AND NOT TARGET Charset::Charset)
  add_library(Charset::Charset STATIC IMPORTED)
  set_target_properties(Charset::Charset PROPERTIES
    IMPORTED_LOCATION "${Charset_LIBRARY}"
#    INTERFACE_INCLUDE_DIRECTORIES "${Charset_INCLUDE_DIR}"
  )
endif()

mark_as_advanced(
  Charset_LIBRARY
#  Charset_INCLUDE_DIR
)