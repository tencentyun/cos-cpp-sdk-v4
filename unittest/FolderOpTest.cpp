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

#include "CosApi.h"
#include "CosConfig.h"
#include "op/FolderOp.h"

namespace qcloud_cos {

class FolderOpTest : public testing::Test {
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

std::string FolderOpTest::m_bucket_name = "coscppsdkv4ut";
CosConfig* FolderOpTest::m_config = NULL;
CosAPI* FolderOpTest::m_client = NULL;

TEST_F(FolderOpTest, FolderCreateTest) {
    std::string cos_path = "/testdata/";
    std::string folder_biz_attr = "folder attribute";
    FolderCreateReq req(m_bucket_name, cos_path, folder_biz_attr);
    std::string result = m_client->FolderCreate(req);
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


} // namespace qcloud_cos
