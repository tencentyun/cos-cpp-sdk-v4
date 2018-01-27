/*************************************************************************
  Copyright (C) 2018 Tencent Inc.
  All rights reserved.

  > File Name: FolderOpTest.cpp
  > Author: sevenyou
  > Mail: sevenyou@tencent.com
  > Created Time: Wed 24 Jan 2018 11:23:21 AM CST
  > Description:
 ************************************************************************/

#include <iostream>

#include "gtest/gtest.h"

#include "Common.h"
#include "CosApi.h"
#include "CosConfig.h"
#include "op/FolderOp.h"

namespace qcloud_cos {

class FolderOpTest : public testing::Test {
protected:
    static void SetUpTestCase() {
        std::cout << "================SetUpTestCase Begin====================" << std::endl;
        m_config = new CosConfig("./config.json");
        std::string appid = GetEnv("CPP_SDK_V4_APPID");
        m_config->setAppid(strtoull(appid.c_str(), NULL, 10));
        m_config->setSecretId(GetEnv("CPP_SDK_V4_SECRET_ID"));
        m_config->setSecretKey(GetEnv("CPP_SDK_V4_SECRET_KEY"));
        m_bucket_name += GetEnv("COS_CPP_V4_TAG");
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

std::string FolderOpTest::m_bucket_name = "coscppsdkv4ut";
CosConfig* FolderOpTest::m_config = NULL;
CosAPI* FolderOpTest::m_client = NULL;

TEST_F(FolderOpTest, FolderCreateTest) {
    std::string cos_path = "/testdata/";
    std::string folder_biz_attr = "folder attribute";
    FolderCreateReq req(m_bucket_name, cos_path, folder_biz_attr);
    std::string result = m_client->FolderCreate(req);
    std::cout << result << std::endl;
    Json::Value value = StringUtil::StringToJson(result);
    ASSERT_TRUE(value.isObject() && value.isMember("code"));
    EXPECT_EQ(0, value["code"].asInt());
}

TEST_F(FolderOpTest, FolderStatTest) {
    std::string cos_path = "/testdata/";
    FolderStatReq req(m_bucket_name, cos_path);
    std::string result = m_client->FolderStat(req);
    Json::Value value = StringUtil::StringToJson(result);
    ASSERT_TRUE(value.isObject() && value.isMember("code") && value["code"].isInt());
    EXPECT_EQ(0, value["code"].asInt());
    ASSERT_TRUE(value.isMember("data") && value["data"].isMember("biz_attr")
            && value["data"]["biz_attr"].isString());
    EXPECT_EQ("folder attribute", value["data"]["biz_attr"].asString());
}

TEST_F(FolderOpTest, FolderUpdateTest) {
    std::string folder_biz_attr = "new folder attribute";

    // 1. Update
    {
        std::string cos_path = "/testdata/";
        FolderUpdateReq req(m_bucket_name, cos_path, folder_biz_attr);
        std::string result = m_client->FolderUpdate(req);
        Json::Value value = StringUtil::StringToJson(result);
        ASSERT_TRUE(value.isObject() && value.isMember("code") && value["code"].isInt());
        EXPECT_EQ(0, value["code"].asInt());
    }

    // 2. Stat
    {
        std::string cos_path = "/testdata/";
        FolderStatReq req(m_bucket_name, cos_path);
        std::string result = m_client->FolderStat(req);
        Json::Value value = StringUtil::StringToJson(result);
        ASSERT_TRUE(value.isObject() && value.isMember("code") && value["code"].isInt());
        EXPECT_EQ(0, value["code"].asInt());
        ASSERT_TRUE(value.isMember("data") && value["data"].isMember("biz_attr")
                && value["data"]["biz_attr"].isString());
        EXPECT_EQ(folder_biz_attr, value["data"]["biz_attr"].asString());
    }
}

TEST_F(FolderOpTest, FolderListTest) {
    {
        std::string cos_path = "/testdata/";
        FolderListReq req(m_bucket_name, cos_path);
        std::string result = m_client->FolderList(req);
        std::cout << result << std::endl;
        Json::Value value = StringUtil::StringToJson(result);
        ASSERT_TRUE(value.isObject() && value.isMember("code") && value["code"].isInt());
        EXPECT_EQ(0, value["code"].asInt());
    }

    // 上传文件
    {
        for (int idx = 0; idx != 10; ++idx) {
            std::string src_path = "./testdata/test1";
            std::stringstream ss;
            ss << "/testdata/test_" << idx;
            std::string cos_path = ss.str();
            FileUploadReq req(m_bucket_name, src_path, cos_path);
            req.setInsertOnly(0);
            std::string result = m_client->FileUpload(req);
            Json::Value value = StringUtil::StringToJson(result);
            ASSERT_TRUE(value.isObject() && value.isMember("code"));
            EXPECT_EQ(0, value["code"].asInt());
        }
    }

    {
        std::string cos_path = "/testdata/";
        FolderListReq req(m_bucket_name, cos_path);
        std::string result = m_client->FolderList(req);
        Json::Value value = StringUtil::StringToJson(result);
        ASSERT_TRUE(value.isObject() && value.isMember("code") && value["code"].isInt());
        EXPECT_EQ(0, value["code"].asInt());
    }
}

TEST_F(FolderOpTest, FolderDeleteTest) {
    std::string cos_path = "/testdata/";

    // 1. 清空目录下所有文件
    {
        for (int idx = 0; idx != 10; ++idx) {
            std::stringstream ss;
            ss << "/testdata/test_" << idx;
            std::string cos_path = ss.str();
            FileDeleteReq req(m_bucket_name, cos_path);
            std::string result = m_client->FileDelete(req);
            Json::Value value = StringUtil::StringToJson(result);
            ASSERT_TRUE(value.isObject() && value.isMember("code"));
            EXPECT_EQ(0, value["code"].asInt());
        }
    }

    // 2. 目录空
    // 注意：如果未清空目录下的所有文件, 也会返回删除成功, 但由于目录下的文件未删除, 控制台依旧可以看到该目录)
    {
        FolderDeleteReq req(m_bucket_name, cos_path);
        std::string result = m_client->FolderDelete(req);
        Json::Value value = StringUtil::StringToJson(result);
        ASSERT_TRUE(value.isObject() && value.isMember("code"));
        EXPECT_EQ(0, value["code"].asInt());
    }
}

} // namespace qcloud_cos
