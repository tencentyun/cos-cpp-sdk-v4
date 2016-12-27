#include <string>
#include <iostream>
#include <fstream>
#include "CosConfig.h"
#include "CosSysConfig.h"
#include "json/json.h"

using std::string;
namespace qcloud_cos {

CosConfig::CosConfig(const string & config_file)
{
    InitConf(config_file);
}

bool CosConfig::InitConf(const std::string& config_file) {
    Json::Value root;
    Json::Reader reader;
    std::ifstream is(config_file.c_str(), std::ios::in);
    if (!is || !is.is_open()) {
        std::cout << "open config file fail " << config_file << std::endl;
        return false;
    }

    if (!reader.parse(is, root, false))
    {
        std::cout << "parse config file fail " << config_file << std::endl;
        is.close();
        return false;
    }

    is.close();

    if (root.isMember("AppID")) {
        this->appid = root["AppID"].asUInt64();
    }

    if (root.isMember("SecretID")) {
        this->secret_id  = root["SecretID"].asString();
    }

    if (root.isMember("SecretKey")) {
        this->secret_key = root["SecretKey"].asString();
    }

    //设置签名超时时间,单位:秒
    if (root.isMember("SignExpiredTime")) {
        CosSysConfig::setExpiredTime(root["SignExpiredTime"].asInt64());
    }

    //设置连接超时时间,单位:豪秒
    if (root.isMember("CurlConnectTimeoutInms")) {
        CosSysConfig::setTimeoutInms(root["CurlConnectTimeoutInms"].asInt64());
    }

    //设置全局超时时间,单位:豪秒
    if (root.isMember("CurlGlobalConnectTimeoutInms")) {
        CosSysConfig::setGlobalTimeoutInms(root["CurlGlobalConnectTimeoutInms"].asInt64());
    }
    //设置上传分片大小,默认:1M
    if (root.isMember("UploadSliceSize")) {
        CosSysConfig::setSliceSize(root["UploadSliceSize"].asInt64());
    }
    //设置L5模块MODID
    if (root.isMember("L5ModID")) {
        CosSysConfig::setL5Modid(root["L5ModID"].asInt64());
    }
    //设置L5模块CMDID
    if (root.isMember("L5CmdID")) {
        CosSysConfig::setL5Cmdid(root["L5CmdID"].asInt64());
    }
    //设置上传文件是否携带sha值,默认:不携带
    if (root.isMember("IsUploadTakeSha")) {
        CosSysConfig::setIsTakeSha(root["IsUploadTakeSha"].asInt());
    }
    //设置cos区域和下载域名:cos,cdn,innercos,自定义,默认:cos
    if (root.isMember("Region")){
        string region = root["Region"].asString();
        int type = 2;
        string domain;
        if (root.isMember("DownloadDomainType")){
            type = root["DownloadDomainType"].asInt();
            domain     = root["SelfDomain"].asString();
        }
        CosSysConfig::setRegionAndDLDomain(region, (DLDomainType)type, domain);
    }

    //设置单文件分片并发上传的线程池大小
    if (root.isMember("UploadThreadPoolSize")) {
        CosSysConfig::setUploadThreadPoolSize(root["UploadThreadPoolSize"].asInt());
    }

    //异步上传下载的线程池大小
    if (root.isMember("AsynThreadPoolSize")) {
        CosSysConfig::setAsynThreadPoolSize(root["AsynThreadPoolSize"].asInt());
    }

    //设置log输出,0:不输出, 1:屏幕,2:syslog,,默认:0
    if (root.isMember("LogoutType")) {
        CosSysConfig::setLogOutType((LOG_OUT_TYPE)(root["LogoutType"].asInt64()));
    }

    if (root.isMember("down_thread_pool_max_size")) {
        CosSysConfig::setDownThreadPoolMaxSize((root["down_thread_pool_max_size"].asUInt()));
    }

    if (root.isMember("down_slice_size")) {
        CosSysConfig::setDownSliceSize((root["down_slice_size"].asUInt()));
    }
    CosSysConfig::print_value();
    return true;
}

uint64_t CosConfig::getAppid() const
{
    return this->appid;
}

string CosConfig::getSecretId() const
{
    return this->secret_id;
}

string CosConfig::getSecretKey() const
{
    return this->secret_key;
}

}
