#include "op/FolderOp.h"
#include "util/CodecUtil.h"
#include "util/HttpUtil.h"
#include "util/StringUtil.h"
#include "util/FileUtil.h"
#include "request/CosResult.h"
#include "CosDefines.h"
#include "Auth.h"
#include <map>
#include <string>
using std::string;
using std::map;

namespace qcloud_cos{

//目录创建
string FolderOp::FolderCreate(FolderCreateReq& req)
{
    SDK_LOG_DBG("req: %s", req.toJsonString().c_str());
    
    CosResult ret;
    if (!req.isParaValid(ret))
    {
        return ret.toJsonString();
    }

    string bucket = req.getBucket();
    string path =  req.getFormatFolderPath();
    string fullUrl = HttpUtil::GetEncodedCosUrl(getAppid(),bucket, path);
    uint64_t expired = FileUtil::GetExpiredTime();
    string sign = Auth::AppSignMuti(this->getAppid(),this->getSecretID(),
                                this->getSecretKey(),expired,bucket);

    std::map<string, string> http_headers;
    http_headers[HTTP_HEADER_AUTHORIZATION] = sign;

    std::map<string, string> http_params;
    http_params[PARA_OP] = OP_CREATE;
    http_params[PARA_BIZ_ATTR] = req.getBizAttr();
    return HttpSender::SendJsonPostRequest(
        fullUrl, http_headers, http_params);
    
}

//目录查询
string FolderOp::FolderStat(FolderStatReq& req)
{
    SDK_LOG_DBG("req: %s", req.toJsonString().c_str());
    
    CosResult ret;
    if (!req.isParaValid(ret))
    {
        return ret.toJsonString();
    }

    string folderPath = req.getFolderPath();
    return statBase(req.getBucket(), folderPath);
}

//目录列表(前缀匹配或者非前缀匹配)
string FolderOp::FolderList(FolderListReq& req)
{
    SDK_LOG_DBG("req: %s", req.toJsonString().c_str());
    
    CosResult ret;
    if (!req.isParaValid(ret))
    {
        return ret.toJsonString();
    }

    string bucket = req.getBucket();
    string folderPath = FileUtil::FormatFolderPath(req.getListPath());
    uint64_t expired = FileUtil::GetExpiredTime();
    string fullUrl = HttpUtil::GetEncodedCosUrl(getAppid(),bucket, req.getListPath());
    string sign = Auth::AppSignMuti(getAppid(),getSecretID(),getSecretKey(),
                                expired,bucket);
    std::map<string, string> http_headers;
    http_headers[HTTP_HEADER_AUTHORIZATION] = sign;

    std::map<string, string> http_params;
    http_params[PARA_OP] = OP_LIST;
    http_params[PARA_LIST_NUM] = StringUtil::IntToString(req.getListNum());
    http_params[PARA_LIST_FLAG] = StringUtil::IntToString(req.getListFlag());
    if (!req.getListContext().empty()){
        http_params[PARA_LIST_CONTEXT] = req.getListContext();
    }
    
    return HttpSender::SendGetRequest(fullUrl, http_headers, http_params);
}

//目录删除
string FolderOp::FolderDelete(FolderDeleteReq& req)
{
    SDK_LOG_DBG("req: %s", req.toJsonString().c_str());
    
    CosResult ret;
    if (!req.isParaValid(ret))
    {
        return ret.toJsonString();
    }

    return delBase(req.getBucket(), req.getFolderPath());
}

//目录更新
string FolderOp::FolderUpdate(FolderUpdateReq& req)
{
    SDK_LOG_DBG("req: %s", req.toJsonString().c_str());
    
    CosResult ret;
    if (!req.isParaValid(ret))
    {
        return ret.toJsonString();
    }

    string bucket = req.getBucket();
    string formatPath = req.getFormatFolderPath();
    string fullUrl = HttpUtil::GetEncodedCosUrl(getAppid(),bucket, formatPath);

    string sign = Auth::AppSignOnce(getAppid(),getSecretID(),getSecretKey(),
                                formatPath,bucket);

    std::map<string, string> http_headers;
    http_headers[HTTP_HEADER_AUTHORIZATION] = sign;

    std::map<string, string> http_params;
    http_params[PARA_OP] = OP_UPDATE;
    http_params[PARA_BIZ_ATTR] = req.getBizAttr();

    return HttpSender::SendJsonPostRequest(
        fullUrl, http_headers, http_params);
}

}

