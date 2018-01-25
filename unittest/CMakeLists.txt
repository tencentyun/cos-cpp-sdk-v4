# CMakeLists.txt for directory unittest

IF(ENABLE_UNITTEST)
    ADD_EXECUTABLE(file_op_test FileOpTest.cpp)
    TARGET_LINK_LIBRARIES(file_op_test cossdk curl ssl crypto rt jsoncpp qos_client stdc++ pthread z boost_system boost_thread gtest gtest_main)

    ADD_EXECUTABLE(folder_op_test FolderOpTest.cpp)
    TARGET_LINK_LIBRARIES(folder_op_test cossdk curl ssl crypto rt jsoncpp qos_client stdc++ pthread z boost_system boost_thread gtest gtest_main)
ENDIF()
