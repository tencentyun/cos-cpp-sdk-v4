/*************************************************************************
  Copyright (C) 2018 Tencent Inc.
  All rights reserved.

  > File Name: FileOpTest.cpp
  > Author: sevenyou
  > Mail: sevenyou@tencent.com
  > Created Time: Wed 24 Jan 2018 11:23:21 AM CST
  > Description:
 ************************************************************************/

#include <iostream>
#include "gtest/gtest.h"

#include "CosApi.h"
#include "CosConfig.h"
#include "op/FileOp.h"

namespace qcloud_cos {

extern "C" {

struct UnitTestData {
    std::string m_name;
    int m_age;
    UnitTestData(const std::string& name, int age) : m_name(name), m_age(age) {}

    std::string ToString() {
        std::ostringstream oss;
        oss << "name: " << m_name << ", age: " << m_age;
        return oss.str();
    }
};

// 异步下载文件的回调函数
void download_callback_with_user_data(DownloadCallBackArgs arg) {
    if (arg.m_user_data != NULL) {
        UnitTestData* tmp = static_cast<UnitTestData*>(arg.m_user_data);
        --tmp->m_age;
    }
    return;
}

// 异步上传文件的回调函数
void upload_callback_with_user_data(UploadCallBackArgs arg) {
    if (arg.m_user_data != NULL) {
        UnitTestData* tmp = static_cast<UnitTestData*>(arg.m_user_data);
        ++tmp->m_age;
    }
    return;
}

// 异步删除文件的回调函数
void delete_callback_with_user_data(DeleteCallBackArgs arg) {
    if (arg.m_user_data != NULL) {
        UnitTestData* tmp = static_cast<UnitTestData*>(arg.m_user_data);
        tmp->m_age = 0;
    }
    return;
}

}

class FileOpTest : public testing::Test {
protected:
    static void SetUpTestCase() {
        std::cout << "================SetUpTestCase Begin====================" << std::endl;
        m_config = new CosConfig("./config.json");
        m_client = new CosAPI(*m_config);
        std::cout << "================SetUpTestCase End====================" << std::endl;
    }

    static void TearDownTestCase() {
        std::cout << "================SetUpTestCase Begin====================" << std::endl;
        delete m_client;
        delete m_config;
        std::cout << "================SetUpTestCase End====================" << std::endl;
    }

protected:
    static CosConfig* m_config;
    static CosAPI* m_client;
    static std::string m_bucket_name;
};

std::string FileOpTest::m_bucket_name = "coscppsdkv4ut";
CosConfig* FileOpTest::m_config = NULL;
CosAPI* FileOpTest::m_client = NULL;

TEST_F(FileOpTest, FileUploadTest) {
    std::string src_path = "./testdata/test1";
    std::string cos_path = "/test1";
    FileUploadReq req(m_bucket_name, src_path, cos_path);
    req.setInsertOnly(0);
    std::string result = m_client->FileUpload(req);
    Json::Value value = StringUtil::StringToJson(result);
    ASSERT_TRUE(value.isObject() && value.isMember("code"));
    EXPECT_EQ(0, value["code"].asInt());
}

TEST_F(FileOpTest, FileDownloadTest) {
    std::string cos_path = "/test1";
    FileDownloadReq req(m_bucket_name, cos_path);
    int ret_code = 0;
    char buf[1024] = {0};
    uint64_t size = m_client->FileDownload(req, buf, 1024, 0, &ret_code);
    EXPECT_EQ(17, size);
    EXPECT_EQ(0, ret_code);
}

TEST_F(FileOpTest, FileStatTest) {
    std::string cos_path = "/test1";
    FileStatReq req(m_bucket_name, cos_path);
    std::string result = m_client->FileStat(req);
    Json::Value value = StringUtil::StringToJson(result);
    ASSERT_TRUE(value.isObject() && value.isMember("code"));
    EXPECT_EQ(0, value["code"].asInt());
}

TEST_F(FileOpTest, FileUpdateTest) {
    std::string cos_path = "/test1";
    std::map<std::string, std::string> custom_header;
    custom_header[PARA_CACHE_CONTROL] = "PARA_CACHE_CONTROL";
    custom_header[PARA_CONTENT_TYPE] = "PARA_CONTENT_TYPE";
    custom_header[PARA_CONTENT_DISPOSITION] = "PARA_CONTENT_DISPOSITION";
    custom_header[PARA_CONTENT_LANGUAGE] = "PARA_CONTENT_LANGUAGE";
    custom_header[PARA_X_COS_META_PREFIX + "abc"] = "PARA_X_COS_META_PREFIX";

    FileUpdateReq req(m_bucket_name, cos_path);
    std::string file_biz_attr = "file new_attribute";
    std::string auth = "eInvalid";
    req.setBizAttr(file_biz_attr);
    req.setAuthority(auth);
    req.setForbid(0);
    req.setCustomHeader(custom_header);
    std::string result = m_client->FileUpdate(req);
    Json::Value value = StringUtil::StringToJson(result);
    ASSERT_TRUE(value.isObject() && value.isMember("code"));
    EXPECT_EQ(0, value["code"].asInt());
}

TEST_F(FileOpTest, FileDeleteTest) {
    std::string cos_path = "/test1";
    FileDeleteReq req(m_bucket_name, cos_path);
    std::string result = m_client->FileDelete(req);
    Json::Value value = StringUtil::StringToJson(result);
    ASSERT_TRUE(value.isObject() && value.isMember("code"));
    EXPECT_EQ(0, value["code"].asInt());
}

TEST_F(FileOpTest, FileAsynUploadTest) {
    std::string src_path = "./testdata/asyn_test1";
    std::string cos_path = "/asyn_test1";
    FileUploadReq req(m_bucket_name, src_path, cos_path);
    req.setInsertOnly(0);
    UnitTestData test_data("sevenyou", 3);
    m_client->FileUploadAsyn(req, upload_callback_with_user_data, &test_data);

    sleep(2);
    EXPECT_EQ(4, test_data.m_age);
}

TEST_F(FileOpTest, FileAsynDownloadTest) {
    std::string cos_path = "/asyn_test1";
    FileDownloadReq req(m_bucket_name, cos_path);
    int ret_code = 0;
    char buf[1024] = {0};
    UnitTestData test_data("sevenyou", 3);
    bool ret = m_client->FileDownloadAsyn(req, buf, 1024,
            download_callback_with_user_data, &test_data);

    sleep(2);
    EXPECT_TRUE(ret);
    EXPECT_EQ(2, test_data.m_age);
    EXPECT_EQ(22, strlen(buf));
}

TEST_F(FileOpTest, FileAsynDeleteTest) {
    std::string cos_path = "/asyn_test1";
    FileDeleteReq req(m_bucket_name, cos_path);
    UnitTestData test_data("sevenyou", 3);
    bool ret = m_client->FileDeleteAsyn(req, delete_callback_with_user_data, &test_data);
    sleep(2);
    EXPECT_TRUE(ret);
    EXPECT_EQ(0, test_data.m_age);
}

#if 0
TEST_F(FileOpTest, FileUploadAsyncTest) {
}
TEST_F(FileOpTest, FileDownloadAsyncTest) {
}
#endif

} // namespace qcloud_cos
