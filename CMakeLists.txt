cmake_minimum_required(VERSION 3.15)

project(kerprg)

set_property(GLOBAL PROPERTY USER_FOLDERS ON)

string(REPLACE "/GS" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
string(REPLACE "/GS" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
string(REPLACE "/GR" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /GS- /GR-")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4 /GS-")

set(NTDDI_VERSION "0x0A000008" CACHE STRING "NTDDI version")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}")
find_package(WDK REQUIRED)

add_subdirectory(simple)

if(TARGET simple AND MSVC)
    get_target_property(PROP_LINK_FLAGS simple LINK_FLAGS)
    if(PROP_LINK_FLAGS)
        string(REPLACE "/ENTRY:GsDriverEntry" "" PROP_LINK_FLAGS "${PROP_LINK_FLAGS}")
        set_target_properties(simple PROPERTIES LINK_FLAGS "${PROP_LINK_FLAGS}")
    endif()

    target_link_options(simple PRIVATE
            "/ENTRY:CustomDriverEntry"
            "/NODEFAULTLIB:BufferOverflowK.lib"
            "/NODEFAULTLIB:kernel32.lib"
            "/NODEFAULTLIB:user32.lib"
    )
endif()