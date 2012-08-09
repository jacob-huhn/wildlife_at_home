include_directories(${OpenCV_INCLUDE_DIR})
include_directories(${OpenCV_INCLUDE_DIR}/../)
include_directories(${BOINC_INCLUDE_DIR})
include_directories(${BOINC_INCLUDE_DIR}/api)
include_directories(${BOINC_INCLUDE_DIR}/lib)

add_executable(wildlife wildlife)
target_link_libraries(wildlife ${OpenCV_LIBRARIES} ${BOINC_APP_LIBRARIES})