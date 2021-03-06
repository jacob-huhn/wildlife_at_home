CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(Wildlife_Client)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake/Modules/")

MESSAGE(STATUS "BUILD TYPE: ${CMAKE_BUILD_TYPE}")

IF(OpenCV_DIR)
    MESSAGE(STATUS "OpenCV_DIR: " ${OpenCV_DIR})
ELSE()
    IF(APPLE)
        set(OpenCV_DIR ~/Dropbox/osx/opencv/share/OpenCV)
        #set(OpenCV_DIR)
    ELSEIF(MSVC)
        set(GLUT_ROOT_PATH ~/Dropbox/Windows/freeglut)
        set(FREETYPE_DIR ~/Dropbox/Windows/freetype)
        set(FTGL_DIR ~/Dropbox/Windows/ftgl)
        set(OpenCV_DIR ~/Dropbox/Windows/opencv/build)
    ELSE()
        set(OpenCV_DIR ~/opencv/share/OpenCV)
    ENDIF()
    MESSAGE(STATUS "OpenCV_DIR: " ${OpenCV_DIR})
ENDIF()

IF(APPLE)
    MESSAGE(STATUS "Setting FLAGS for APPLE")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mmacosx-version-min=10.5")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mmacosx-version-min=10.5")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -Wall")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -Wall")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -O3 -DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O3 -DNDEBUG")
ELSEIF(UNIX)
    MESSAGE(STATUS "Seting FLAGS for UNIX")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mieee-fp")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mieee-fp")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -O1 -DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O1 -DNDEBUG")
ENDIF()

# Executables
#add_executable(wildlife_surf_collect wildlife_surf_collect)

set(Boost_USE_MULTITHREADED OFF)

#find_package(OpenCV REQUIRED COMPONENTS nonfree core)
find_package(OpenCV REQUIRED nonfree)
find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)
find_package(Freetype REQUIRED)
find_package(BOINC)
find_package(FTGL)
find_package(Boost COMPONENTS system filesystem program_options)

MESSAGE(STATUS "OpenCV Include Dirs: ${OpenCV_INCLUDE_DIRS}")
MESSAGE(STATUS "OpenCV Libs: ${OpenCV_LIBS}")

MESSAGE(STATUS "Boost Include Dirs: ${Boost_INCLUDE_DIRS}")
MESSAGE(STATUS "Boost Libs: ${Boost_LIBRARIES}")

#include_directories("${PROJECT_SOURCE_DIR}/../include/" ${OpenCV_INCLUDE_DIRS} ${BOINC_INCLUDE_DIR})
#target_link_libraries(wildlife_surf_collect pthread ${OpenCV_LIBS} ${BOINC_APP_LIBRARIES})

if(BOINC_APP_FOUND AND OpenCV_DIR)
    add_executable(wildlife_surf_collect
        wildlife_surf_collect
        ../source/Event
        ../source/EventType
        ../source/VideoType
        ../source/utils
        ../source/boinc_utils)
    include_directories("${PROJECT_SOURCE_DIR}/../include/" ${OpenCV_INCLUDE_DIRS} ${BOINC_INCLUDE_DIR})
    IF(MSVC)
        target_link_libraries(wildlife_surf_collect ${OpenCV_LIBS} ${BOINC_APP_LIBRARIES} ${BOINC_GRAPHICS_LIBRARY})
    ELSE()
        target_link_libraries(wildlife_surf_collect ${OpenCV_LIBS} ${BOINC_APP_LIBRARIES})
    ENDIF()

    add_executable(wildlife_surf_predict
        wildlife_surf_predict
        ../source/Event
        ../source/EventType
        ../source/VideoType
        ../source/utils
        ../source/boinc_utils
        ../source/svm
        ../source/cvplot)
    include_directories("${PROJECT_SOURCE_DIR}/../include/" ${OpenCV_INCLUDE_DIRS} ${BOINC_INCLUDE_DIR})
    IF(MSVC)
        target_link_libraries(wildlife_surf_predict ${OpenCV_LIBS} ${BOINC_APP_LIBRARIES} ${BOINC_GRAPHICS_LIBRARY})
    ELSE()
        target_link_libraries(wildlife_surf_predict ${OpenCV_LIBS} ${BOINC_APP_LIBRARIES})
    ENDIF()

    #add_executable(wildlife_surf_collect wildlife_surf_collect2)
    #include_directories("${PROJECT_SOURCE_DIR}/../include/" ${OpenCV_INCLUDE_DIR} ${BOINC_INCLUDE_DIR})
    #target_link_libraries(wildlife_surf_collect ${OpenCV_LIBS} ${BOINC_API_LIBRARY} ${BOINC_APP_LIBRARIES})

    IF(FTGL_FOUND)
        add_executable(wildlife_graphics_app
            wildlife_graphics_app
            ../source/utils
            ../source/boinc_utils)
        include_directories ("${PROJECT_SOURCE_DIR}/../include/" ${OPENGL_INCLUDE_DIRS} ${GLUT_INCLUDE_DIRS} ${FREETYPE_INCLUDE_DIRS} ${FTGL_INCLUDE_DIRS})
        target_link_libraries(wildlife_graphics_app
            ${OpenCV_LIBS}
            ${BOINC_APP_LIBRARIES}
            ${BOINC_GRAPHICS_LIBRARY}
            ${OPENGL_LIBRARIES}
            ${GLUT_LIBRARY}
            ${FREETYPE_LIBS}
            ${FTGL_LIBRARIES})
    ENDIF()
ENDIF()

IF(Boost_FOUND AND NOT(MSVC OR APPLE))
    add_executable(wildlife_surf_combine
        wildlife_surf_combine
        ../source/EventType)
    include_directories("${PROJECT_SOURCE_DIR}/../include/" ${Boost_INCLUDE_DIR})
    target_link_libraries(wildlife_surf_combine
        ${Boost_LIBRARIES}
        ${OpenCV_LIBS})

    add_executable(wildlife_surf_svm_generate
        wildlife_surf_svm_generate
        ../source/EventType
        ../source/utils)
    include_directories("${PROJECT_SOURCE_DIR}/../include/" ${Boost_INCLUDE_DIR})
    target_link_libraries(wildlife_surf_svm_generate
        ${Boost_LIBRARIES}
        ${OpenCV_LIBS})
ENDIF()
