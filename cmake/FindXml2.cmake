# GPLv2 Licence

# not in macos input-SDK

# find_path(Xml2_INCLUDE_DIR tiff.h
#   "${INPUT_SDK_PATH}/include"
#  NO_DEFAULT_PATH
# )
  
find_library(Xml2_LIBRARY 
  NAMES xml2
)

find_package_handle_standard_args(
  Xml2
  REQUIRED_VARS Xml2_LIBRARY # Xml2_INCLUDE_DIR
)

if(Xml2_FOUND AND NOT TARGET Xml2::Xml2)
  add_library(Xml2::Xml2 STATIC IMPORTED)
  set_target_properties(Xml2::Xml2 PROPERTIES
    IMPORTED_LOCATION "${Xml2_LIBRARY}"
#    INTERFACE_INCLUDE_DIRECTORIES "${Xml2_INCLUDE_DIR}"
  )
endif()

mark_as_advanced(
  Xml2_LIBRARY
#  Xml2_INCLUDE_DIR
)