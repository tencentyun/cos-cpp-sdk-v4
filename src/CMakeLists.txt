cmake_minimum_required(VERSION 2.8)
CMAKE_policy(SET CMP0015 NEW)

project(cos-cpp-sdk)
SET(CMAKE_C_COMPILER g++)
set(LIBRARY_OUTPUT_PATH ../lib)
include_directories(../include)
link_directories(../lib)
#file(GLOB_RECURSE cossdk_source_files "*.cpp")
if (OPENSSL_VERSION VERSION_LESS 1.1.0)
    message("old openssl version less than 1.1.0")
    set(cossdk_source_files Auth.cpp CosApi.cpp CosConfig.cpp CosSysConfig.cpp op/BaseOp.cpp
        op/FileDownTask.cpp op/FileOp.cpp op/FileUploadTask.cpp op/FolderOp.cpp
        request/CosResult.cpp request/FileDeleteReq.cpp request/FileDownloadReq.cpp request/FileMoveReq.cpp
        request/FileReq.cpp request/FileStatReq.cpp request/FileUpdateReq.cpp request/FileUploadReq.cpp
        request/FolderCreateReq.cpp request/FolderDeleteReq.cpp request/FolderListReq.cpp request/FolderReq.cpp
        request/FolderStatReq.cpp request/FolderUpdateReq.cpp request/ReqBase.cpp util/CodecUtil.cpp util/FileUtil.cpp
        util/HttpSender.cpp util/HttpUtil.cpp util/l5_endpoint_provider.cpp util/Sha1.cpp util/StringUtil.cpp util/true_random.cpp)
ELSE()
    message("new version upper than 1.1.0")
    set(cossdk_source_files Auth.cpp CosApi.cpp CosConfig.cpp CosSysConfig.cpp op/BaseOp.cpp
        op/FileDownTask.cpp op/FileOp.cpp op/FileUploadTask.cpp op/FolderOp.cpp
        request/CosResult.cpp request/FileDeleteReq.cpp request/FileDownloadReq.cpp request/FileMoveReq.cpp
        request/FileReq.cpp request/FileStatReq.cpp request/FileUpdateReq.cpp request/FileUploadReq.cpp
        request/FolderCreateReq.cpp request/FolderDeleteReq.cpp request/FolderListReq.cpp request/FolderReq.cpp
        request/FolderStatReq.cpp request/FolderUpdateReq.cpp request/ReqBase.cpp util/CodecUtilHighOpenssl.cpp util/FileUtil.cpp
        util/HttpSender.cpp util/HttpUtil.cpp util/l5_endpoint_provider.cpp util/Sha1.cpp util/StringUtil.cpp util/true_random.cpp)
ENDIF()

add_library(cossdk STATIC ${cossdk_source_files})
if (__USE_L5)
    MESSAGE("use l5")
    add_definitions(-D__USE_L5)
    target_link_libraries(cossdk curl ssl rt jsoncpp qos_client stdc++ pthread z boost_thread)
ELSE()
    target_link_libraries(cossdk curl ssl rt jsoncpp stdc++ pthread z boost_thread)
ENDIF()

set_target_properties(cossdk PROPERTIES OUTPUT_NAME "cossdk")
