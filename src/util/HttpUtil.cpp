#include "util/Sha1.h"
#include "util/HttpUtil.h"
#include "util/FileUtil.h"
#include "CosSysConfig.h"
#include "CosDefines.h"
#include "util/l5_endpoint_provider.h"
#include <string>

using std::string;
namespace qcloud_cos {

string HttpUtil::GetEncodedCosUrl(const string& endpoint,
                                      const string& bucketName,
                                      const string& dstPath,
                                      uint64_t  appid) 
{
    char urlBytes[10240];
    snprintf(urlBytes, sizeof(urlBytes),
#if __WORDSIZE == 64
     "/%lu/%s/%s",
#else
     "/%llu/%s/%s",
#endif
     appid,
     bucketName.c_str(),
     dstPath.c_str());

    string url_str(urlBytes);
    return endpoint + FileUtil::EncodePath(url_str);
}

string HttpUtil::GetEncodedCosUrl(uint64_t appid, const std::string& bucket_name,
                                     const std::string& path)
{
#ifdef __USE_L5
    uint64_t l5_modid = CosSysConfig::getL5Modid();
    uint64_t l5_cmdid = CosSysConfig::getL5Cmdid();
    if (l5_modid == -1 && l5_cmdid == -1) {
        return GetEncodedCosUrl(CosSysConfig::getUploadDomain(), bucket_name, path, appid);
    }
    // l5地址
    std::string endpoint;
    if (!L5EndpointProvider::GetEndPoint(l5_modid,l5_cmdid, &endpoint)) {
        return GetEncodedCosUrl(CosSysConfig::getUploadDomain(),bucket_name, path, appid);
    }
    return GetEncodedCosUrl(endpoint, bucket_name, path, appid);
#else
    return GetEncodedCosUrl(CosSysConfig::getUploadDomain(), bucket_name, path, appid);
#endif
}

string HttpUtil::GetEncodedDownloadCosUrl(
                    const string& domain,
                    const string& dstPath,
                    const string& sign) 
{
    string url_str = domain + dstPath;
    string url = "http:/" + FileUtil::EncodePath(url_str) + "?sign=" + sign;
    return url;
}

string HttpUtil::GetEncodedDownloadCosUrl(
                    uint64_t  appid,
                    const string& bucketName,
                    const string& dstPath,
                    const string& domain,
                    const string& sign) 
{
    char urlBytes[10240];
    snprintf(urlBytes, sizeof(urlBytes),
#if __WORDSIZE == 64
     "%s-%lu.%s/%s",
#else
     "%s-%llu.%s/%s",
#endif
     bucketName.c_str(),
     appid,
     domain.c_str(),
     dstPath.c_str()
    );

    string url_str(urlBytes);
    string url = "http:/" + FileUtil::EncodePath(url_str) + "?sign=" + sign;
    return url;
}

string HttpUtil::GetEncodedDownloadCosCdnUrl(
                    uint64_t  appid,
                    const string& bucketName,
                    const string& dstPath,
                    const string& domain,
                    const string& sign) 
{
    char urlBytes[10240];
    snprintf(urlBytes, sizeof(urlBytes),
#if __WORDSIZE == 64
     "%s/files/v2/%lu/%s/%s",
#else
     "%s/files/v2/%llu/%s/%s",
#endif
     domain.c_str(),
     appid,
     bucketName.c_str(),
     dstPath.c_str()
    );

    string url_str(urlBytes);
    string url = "http:/" + FileUtil::EncodePath(url_str) + "?sign=" + sign;
    return url;
}

}
