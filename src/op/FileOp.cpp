#include <string.h>
#include <stdint.h>
#include <map>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string>
#include "op/FileOp.h"
#include "Auth.h"
#include <boost/bind.hpp>
#include "threadpool/boost/threadpool.hpp"

using std::string;
using std::map;
namespace qcloud_cos{

extern boost::threadpool::pool* threadpool;

//文件下载
int FileOp::FileDownload(FileDownloadReq& req, string localPath, int* ret_code)
{
    SDK_LOG_DBG("req:%s",req.toJsonString().c_str());

    //获取文件的大小
    string bucket = req.getBucket();
    string filepath = req.getFilePath();
    FileStatReq statreq(bucket, filepath);
    string statrsp = FileStat(statreq);
    Json::Reader reader;
    Json::Value  statrsp_json;
    if (!reader.parse(statrsp, statrsp_json)){
        SDK_LOG_ERR("stat bucket parse, error, rsp:%s",statrsp.c_str());
        *ret_code = -1;
        return 0;
    }

    if (!statrsp_json.isMember("code") || statrsp_json["code"] != 0) {
        SDK_LOG_ERR("stat bucket error, rsp:%s",statrsp.c_str());
        *ret_code = -1;
        return 0;
    }
    
    uint64_t filesize = 0;
    Json::Value &data = statrsp_json["data"];
    if (!data.isMember("filesize")) {
        SDK_LOG_ERR("stat bucket error, rsp:%s",statrsp.c_str());
        *ret_code = -1;
        return 0;
    }
    filesize = data["filesize"].asUInt64();

    //多次签名
    uint64_t expired = FileUtil::GetExpiredTime();
    string sign = Auth::AppSignMuti(getAppid(),getSecretID(),getSecretKey(),
                                    expired, req.getBucket());
    //构造URL
    string url = "";
    if (CosSysConfig::getDownloadDomainType() == DOMAIN_COS){
        url = HttpUtil::GetEncodedDownloadCosUrl(getAppid(),req.getBucket(),req.getFilePath(),
                                CosSysConfig::getDownloadDomain(),sign);
    } else if (CosSysConfig::getDownloadDomainType() == DOMAIN_CDN) {
        url = HttpUtil::GetEncodedDownloadCosUrl(getAppid(),req.getBucket(),req.getFilePath(),
                                CosSysConfig::getDownloadDomain(),sign);
    } else if (CosSysConfig::getDownloadDomainType() == DOMAIN_SELF_DOMAIN) {
        url = HttpUtil::GetEncodedDownloadCosUrl(CosSysConfig::getDownloadDomain(),req.getFilePath(),sign);
    }

    unsigned pool_size = CosSysConfig::getDownThreadPoolMaxSize();
    unsigned slice_size = CosSysConfig::getDownSliceSize();
    unsigned max_task_num = filesize / slice_size + 1;
    if (max_task_num < pool_size) {
        pool_size = max_task_num;
    }

    unsigned char ** fileContentBuf = new unsigned char *[pool_size];
    for(unsigned i =0;i<pool_size;i++){
        fileContentBuf[i] = new unsigned char[slice_size];
    }

    FileDownTask **pptaskArr = new FileDownTask*[pool_size];
    for (int i=0; i<pool_size;i++)
    {
        pptaskArr[i] = new FileDownTask(url, sign, bucket,getAppid());
    }

    SDK_LOG_DBG("upload data,url=%s, poolsize=%u,slice_size=%u,filesize=%lu",url.c_str(), pool_size, slice_size, filesize);

    int fd = open(localPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);    
    if (fd == -1) {
        SDK_LOG_ERR("open file(%s) fail, errno=%d",localPath.c_str(), errno);
        *ret_code = -1;
        return 0;
    }

    std::vector<uint64_t> vec_offset;
    vec_offset.reserve(pool_size);
    boost::threadpool::pool tp(pool_size);
    uint64_t offset =0;
    bool task_fail_flag = false;
    unsigned down_times = 0;
    while(offset < filesize)
    {
        SDK_LOG_DBG("down data, offset=%lu, filesize=%lu", offset, filesize);
        unsigned task_index = 0;
        vec_offset.clear();
        for (; task_index<pool_size && (offset<filesize); ++task_index) {
            SDK_LOG_DBG("down data, task_index=%d, filesize=%lu, offset=%lu", task_index, filesize, offset);
            FileDownTask *ptask = pptaskArr[task_index];
            ptask->setDownParams(fileContentBuf[task_index], slice_size, offset);
            tp.schedule(boost::bind(&FileDownTask::run, ptask) );
            vec_offset[task_index] = offset;
            offset += slice_size;
            ++down_times;
        }

        unsigned task_num = task_index;

        tp.wait();

        for (task_index = 0; task_index < task_num; ++task_index) {
            FileDownTask *ptask = pptaskArr[task_index];
            if (ptask->taskSuccess() == false) {
                SDK_LOG_ERR("down data, down task fail, rsp:%s", ptask->getTaskResp().c_str());
                task_fail_flag = true;
                break;
            } else {
                if (-1 == lseek(fd, vec_offset[task_index], SEEK_SET)) {
                    SDK_LOG_ERR("down data, lseek ret=%d, offset=%lu", errno, vec_offset[task_index]);
                    task_fail_flag = true;
                    break;
                }
                if (-1 == write(fd, fileContentBuf[task_index], ptask->getDownLoadLen())) {
                    SDK_LOG_ERR("down data, write ret=%d, len=%lu", errno, ptask->getDownLoadLen());
                    task_fail_flag = true;
                    break;
                }
                
                SDK_LOG_DBG("down data, down_times=%u,task_index=%d, filesize=%lu, offset=%lu, downlen:%lu ", down_times,task_index, filesize, vec_offset[task_index], ptask->getDownLoadLen());
            }
        }

        if (task_fail_flag) {
            break;
        }
    }

    //释放所有资源
    close(fd);
    delete [] pptaskArr;
    delete [] fileContentBuf;
    *ret_code = 0;
    if (task_fail_flag) {
        *ret_code = -1;
		return 0;
    }
    return offset > filesize ? filesize : offset;
}

int FileOp::FileDownload(FileDownloadReq& req, char* buffer, size_t bufLen, uint64_t offset, int* ret_code)
{
    SDK_LOG_DBG("req:%s",req.toJsonString().c_str());
    CosResult ret;
    if (!req.isParaValid(ret))
    {
        SDK_LOG_ERR("para not valid, error");
        *ret_code = -1;
        return 0;
    }

    if (!buffer)
    {
        SDK_LOG_ERR("buffer is null, error");
        *ret_code = -2;
        return 0;
    }

    //多次签名
    uint64_t expired = FileUtil::GetExpiredTime();
    string sign = Auth::AppSignMuti(getAppid(),getSecretID(),getSecretKey(),
                                    expired, req.getBucket());
    //构造URL
    string url = "";
    if (CosSysConfig::getDownloadDomainType() == DOMAIN_COS){
        url = HttpUtil::GetEncodedDownloadCosUrl(getAppid(),req.getBucket(),req.getFilePath(),
                                CosSysConfig::getDownloadDomain(),sign);
    } else if (CosSysConfig::getDownloadDomainType() == DOMAIN_CDN) {
        url = HttpUtil::GetEncodedDownloadCosUrl(getAppid(),req.getBucket(),req.getFilePath(),
                                CosSysConfig::getDownloadDomain(),sign);
    } else if (CosSysConfig::getDownloadDomainType() == DOMAIN_SELF_DOMAIN) {
        url = HttpUtil::GetEncodedDownloadCosUrl(CosSysConfig::getDownloadDomain(),req.getFilePath(),sign);
    }

    SDK_LOG_DBG("url:%s", url.c_str());

    string rspbuf;
    std::map<string, string> user_params;
    std::map<string, string> http_params;
    //文件下载,架平需要host头域中带'-'
    char host_head[1024];
    memset(host_head, 0, sizeof(host_head));
    snprintf(host_head, sizeof(host_head),"%s-%lu.%s", req.getBucket().c_str(), getAppid(),
                         CosSysConfig::getDownloadDomain().c_str());
    http_params["host"] = host_head;
    //SDK_LOG_DBG("host head:%s", host_head);
    char range_head[128];
    memset(range_head, 0, sizeof(range_head));
        snprintf(range_head,sizeof(range_head),"bytes=%lu-%lu", offset, (offset+bufLen - 1));
    //增加Range头域，避免大文件时将整个文件下载
    http_params["Range"] = range_head;
    int http_code = HttpSender::SendGetRequest(&rspbuf, url, http_params, user_params);
    //当实际长度小于请求的数据长度时httpcode为206
    if (http_code != 200 && http_code != 206)
    {
        SDK_LOG_ERR("FileDownload: url(%s) fail, ret: %s",url.c_str(), rspbuf.c_str());
        *ret_code = http_code;
        return -3;
    }

    size_t buf_max_size = bufLen;
    size_t len = MIN(rspbuf.length(), buf_max_size);
    memcpy(buffer, rspbuf.c_str(), len);
    *ret_code = 0;
    return len;
}

//文件上传
string FileOp::FileUpload(FileUploadReq& req)
{
    if (req.getFileSize() < SINGLE_FILE_SIZE) {
        return FileUploadSingle(req);
    } else {
        return FileUploadSlice(req);
    }
}

//文件下载(异步)
bool FileOp::FileDownloadAsyn(FileOp& op,
                              const FileDownloadReq& req,
                              char* buffer, size_t bufLen,
                              DownloadCallback callback,
                              void* user_data)
{
    Download_Asyn_Arg arg(&op, req, buffer, bufLen, callback, user_data);
    if (threadpool){
        threadpool->schedule(boost::bind(FileDownload_Asyn_Thread, arg));//会复制一份arg
    } else {
        SDK_LOG_ERR("threadpool is null, error");
        return false;
    }

    return true;
}

//文件下载(异步)
extern "C"
void FileDownload_Asyn_Thread(Download_Asyn_Arg arg)
{

    FileOp* op = arg.m_op;
    FileDownloadReq  req = arg.m_req;
    DownloadCallback callback = arg.m_callback;
    void* user_data = arg.m_user_data;

    char *buffer = arg.m_buffer;
    size_t bufLen = arg.m_bufLen;

    string succ_msg = "success";

    if (!op){
        SDK_LOG_ERR("op or req is null");

        if (callback){
            string fail_msg = "download("+ req.getFilePath() + ") fail";
            DownloadCallBackArgs cb_arg(req, buffer, bufLen, 0, -1, fail_msg, user_data);
            callback(cb_arg);
        } else {
            SDK_LOG_WARN("download call back null");
        }

        return;
    }

    size_t data_size = 0;
    int ret_code = 0;
    DownloadCallBackArgs cb_arg(req, buffer, bufLen, data_size, 0, succ_msg, user_data);
    data_size = op->FileDownload(req, buffer, bufLen, 0, &ret_code);
    if (ret_code == 0){
        succ_msg = "download file(" + req.getFilePath() + "), success, size=" + StringUtil::IntToString(data_size);
        cb_arg.m_data_size = data_size;
        SDK_LOG_DBG("%s",succ_msg.c_str());
    } else {
        SDK_LOG_ERR("download file %s fail",req.getFilePath().c_str());
        cb_arg.m_data_size = 0;
        cb_arg.m_code = ret_code;
        cb_arg.m_message = "download("+ req.getFilePath() + ") fail";
    }

    if (callback){
        callback(cb_arg);
    } else {
        SDK_LOG_WARN("download call back null");
    }

    return;
}

extern "C"
void FileUpload_Asyn_Thread(Upload_Asyn_Arg arg)
{
    FileOp* op              = arg.m_op;
    FileUploadReq  req      = arg.m_req;
    UploadCallback callback = arg.m_callback;
    void* user_data         = arg.m_user_data;

    Json::Value rsp_json;
    rsp_json["code"] = 0;
    rsp_json["message"] = "";
    rsp_json["data"] = "";

    if (!op){
        SDK_LOG_ERR("op is null");
        rsp_json["code"] = -2;
        rsp_json["message"] = "sdk inter error, fileUpload(" + req.getDstPath() + ") fail";;
        if (callback){
            UploadCallBackArgs cb_arg(req, StringUtil::JsonToString(rsp_json),
                                      -2, user_data);
            callback(cb_arg);
        } else {
            SDK_LOG_WARN("download call back null");
        }

        return;
    }

    string rsp;
    if (req.getFileSize() < SINGLE_FILE_SIZE) {
        rsp = op->FileUploadSingle(req);
    } else {
        rsp = op->FileUploadSlice(req);
    }

    Json::Value root_json = StringUtil::StringToJson(rsp);
    int code = -1;
    if (root_json.isMember("code")) {
        code = root_json["code"].asInt();
    }

    if (callback){
        UploadCallBackArgs cb_arg(req, rsp, code, user_data);
        callback(cb_arg);
    } else {
        SDK_LOG_ERR("upload callback null");
    }

    return;
}

extern "C"
void FileDelete_Asyn_Thread(Delete_Asyn_Arg arg)
{
    FileOp* op              = arg.m_op;
    FileDeleteReq  req      = arg.m_req;
    DeleteCallback callback = arg.m_callback;
    void* user_data         = arg.m_user_data;

    Json::Value rsp_json;
    rsp_json["code"] = 0;
    rsp_json["message"] = "";
    rsp_json["data"] = "";

    if (!op){
        SDK_LOG_ERR("op is null");
        rsp_json["code"] = -2;
        rsp_json["message"] = "sdk inter error, fileDelete(" + req.getFormatPath() + ") fail";;
        if (callback){
            DeleteCallBackArgs cb_arg(req, StringUtil::JsonToString(rsp_json), -2, user_data);
            callback(cb_arg);
        } else {
            SDK_LOG_WARN("download call back null");
        }

        return;
    }

    string rsp = op->FileDelete(req);
    Json::Value root_json = StringUtil::StringToJson(rsp);
    int code = -1;
    if (root_json.isMember("code")) {
        code = root_json["code"].asInt();
    }

    if (callback){
        DeleteCallBackArgs cb_arg(req, rsp, code, user_data);
        callback(cb_arg);
    } else {
        SDK_LOG_ERR("upload callback null");
    }
    return;
}

//文件上传(异步)
bool FileOp::FileUploadAsyn(FileOp& op,
                            const FileUploadReq& req,
                            UploadCallback callback,
                            void* user_data)
{
    Upload_Asyn_Arg arg(&op, req, callback, user_data);
    if (threadpool) {
        threadpool->schedule(boost::bind(FileUpload_Asyn_Thread, arg));
    } else {
        SDK_LOG_ERR("threadpool is null, error");
        return false;
    }

    return true;
}

//单文件上传
string FileOp::FileUploadSingle(FileUploadReq& req)
{
    SDK_LOG_DBG("req: %s", req.toJsonString().c_str());
    CosResult ret;
    if (!req.isParaValid(ret))
    {
        return ret.toJsonString();
    }

    //读取文件内容
    const unsigned char * pbuf = NULL;
    unsigned int len = 0;
    string fileContent;
    if (!req.isBufferUpload())
    {
        fileContent = FileUtil::getFileContent(req.getSrcPath());
        pbuf = (unsigned char *)fileContent.c_str();
        len = fileContent.length();
    } else {
        pbuf = (unsigned char *)req.getBuffer();
        len  = req.getBufferLen();
    }

    //多次签名
    uint64_t expired = FileUtil::GetExpiredTime();
    string sign = Auth::AppSignMuti(getAppid(),getSecretID(),getSecretKey(),
                                    expired, req.getBucket());
    map<string,string> user_headers;
    user_headers[HTTP_HEADER_AUTHORIZATION] = sign;
    map<string,string> user_params;
    user_params[PARA_OP] = OP_UPLOAD;
    user_params[PARA_INSERT_ONLY] = StringUtil::IntToString(req.getInsertOnly());
    if (CosSysConfig::isTakeSha()){
        user_params[PARA_SHA] = req.getSha();;
    }
    if (!req.getBizAttr().empty()){
        user_params[PARA_BIZ_ATTR] = req.getBizAttr();
    }

    //构造URL
    string url = HttpUtil::GetEncodedCosUrl(getAppid(),req.getBucket(),req.getDstPath());

    //发送上传消息
    return HttpSender::SendSingleFilePostRequest(url,user_headers,user_params, pbuf, len);

}

//分片文件上传
string FileOp::FileUploadSlice(FileUploadReq& req)
{
    SDK_LOG_DBG("req: %s", req.toJsonString().c_str());
    CosResult ret;
    if (!req.isParaValid(ret))
    {
        return ret.toJsonString();
    }

    //暂时不支持从内存中进行分片上传
    if (req.isBufferUpload())
    {
        ret.setCode(PARA_ERROR_CODE);
        ret.setMessage("not support buffer upload larger than 8M");
        return ret.toJsonString();
    }

    string initRsp = FileUploadSliceInit(req);
    Json::Value initRspJson = StringUtil::StringToJson(initRsp);
    if (initRspJson["code"] != 0)
    {
        return initRsp;
    }

    SDK_LOG_DBG("slice init resp: %s", initRsp.c_str());
    // 表示秒传，则结束
    if (initRspJson["data"].isMember("access_url")) {
        return initRsp;
    }

    string dataRsp = FileUploadSliceData(req, initRsp);
    SDK_LOG_DBG("slice data resp: %s", dataRsp.c_str());
    Json::Value dataRspJson = StringUtil::StringToJson(dataRsp);
    if (dataRspJson["code"] != 0)
    {
        return dataRsp;
    }

    string finishRsp = FileUploadSliceFinish(req, dataRsp);
    return finishRsp;
}

string FileOp::FileUploadSliceInit(FileUploadReq& req)
{
    SDK_LOG_DBG("req: %s", req.toJsonString().c_str());
    //计算sha值
    string sha1Digest = req.getSha();
    string uploadparts;

    if (CosSysConfig::isTakeSha()){
        string sha1;
        CodecUtil::conv_file_to_upload_parts(req.getSrcPath(),req.getSliceSize(),uploadparts, sha1);
        //设置,后面在data和finish中调用
        req.setSha(sha1);
    }

    //多次签名
    uint64_t expired = FileUtil::GetExpiredTime();
    string sign = Auth::AppSignMuti(getAppid(),getSecretID(),getSecretKey(),expired,req.getBucket());
    map<string,string> user_headers;
    user_headers[HTTP_HEADER_AUTHORIZATION] = sign;

    map<string,string> user_params;
    user_params[PARA_OP] = OP_UPLOAD_SLICE_INIT;
    user_params[PARA_INSERT_ONLY] =StringUtil::IntToString(req.getInsertOnly());
    user_params[PARA_SLICE_SIZE] = StringUtil::IntToString(req.getSliceSize());
    user_params[PARA_FILE_SIZE] = StringUtil::Uint64ToString(req.getFileSize());
    if (CosSysConfig::isTakeSha()){
        user_params[PARA_UPLOAD_PARTS] = uploadparts;
        user_params[PARA_SHA] = req.getSha();
    }
    if (!req.getBizAttr().empty()){
        user_params[PARA_BIZ_ATTR] = req.getBizAttr();
    }

    //构造URL
    string url = HttpUtil::GetEncodedCosUrl(getAppid(),req.getBucket(),req.getDstPath());

    //发送上传消息
    return HttpSender::SendSingleFilePostRequest(url,user_headers,user_params, NULL, 0);
}

string FileOp::FileUploadSliceData(FileUploadReq& req, string& initRsp)
{
    SDK_LOG_DBG("initRsp: %s", initRsp.c_str());
    string bucket = req.getBucket();
    Json::Value initRspJson = StringUtil::StringToJson(initRsp);
    if (!initRspJson.isMember("data")){
        SDK_LOG_ERR("FileUploadSliceData: json not data, %s", initRsp.c_str());
        return CosResult(PARA_ERROR_CODE, PARA_ERROR_DESC).toJsonString();
    }
    int slice_size = initRspJson["data"]["slice_size"].asInt();
    string sessionid = initRspJson["data"]["session"].asString();
    int serial_upload = 0;
    if (initRspJson["data"].isMember("serial_upload")) {
        serial_upload = initRspJson["data"]["serial_upload"].asInt();
    }

    uint64_t expired = FileUtil::GetExpiredTime();
    string sign = Auth::AppSignMuti(getAppid(),getSecretID(),getSecretKey(),expired, bucket);

    //构造URL
    string url = HttpUtil::GetEncodedCosUrl(getAppid(),req.getBucket(),req.getDstPath());

    std::ifstream fin(req.getSrcPath().c_str(), std::ios::in | std::ios::binary);
    if (!fin.is_open()){
        SDK_LOG_ERR("FileUploadSliceData: file open fail, %s", req.getSrcPath().c_str());
        return CosResult(LOCAL_FILE_NOT_EXIST_CODE,LOCAL_FILE_NOT_EXIST_DESC).toJsonString();
    }

    uint64_t filesize = FileUtil::getFileLen(req.getSrcPath());
    uint64_t offset = 0;

    int pool_size = CosSysConfig::getUploadThreadPoolSize();
    if (serial_upload == 1) {
        pool_size = 1;
    }

    bool task_fail_flag = false;

    unsigned char ** fileContentBuf = new unsigned char *[pool_size];
    for(int i =0;i<pool_size;i++){
        fileContentBuf[i] = new unsigned char[slice_size];
    }

    FileUploadTask **pptaskArr = new FileUploadTask*[pool_size];
    for (int i=0; i<pool_size;i++)
    {
        pptaskArr[i] = new FileUploadTask(url, sessionid, sign, req.getSha());
    }


    SDK_LOG_DBG("upload data,url=%s, poolsize=%d,slice_size=%d, serial_upload=%d",url.c_str(), pool_size, slice_size, serial_upload);
    string taskResp;//记录线程中返回的错误响应消息
    boost::threadpool::pool tp(pool_size);
    while(offset < filesize)
    {
        int task_index = 0;
        for (; task_index < pool_size; ++task_index) {

            //unsigned char * fileContentBuf = new unsigned char[slice_size];
            fin.read((char *)fileContentBuf[task_index],slice_size);
            size_t read_len = fin.gcount();
            if (read_len == 0 && fin.eof()) {
                SDK_LOG_DBG("read over, task_index: %d", task_index);
                break;
            }

            SDK_LOG_DBG("upload data, task_index=%d, filesize=%lu, offset=%lu,len=%lu", task_index, filesize, offset, read_len);

            FileUploadTask *ptask = pptaskArr[task_index];
            ptask->setOffset(offset);
            ptask->setBufdata(fileContentBuf[task_index], read_len);
            tp.schedule(boost::bind(&FileUploadTask::run, ptask));

            offset += read_len;
        }

        int max_task_num = task_index;

        tp.wait();
        for (task_index = 0; task_index < max_task_num; ++task_index) {
            FileUploadTask *ptask = pptaskArr[task_index];
            if (ptask->taskSuccess() == false) {
                taskResp = ptask->getTaskResp();
                task_fail_flag = true;
                break;
            }
        }

        if (task_fail_flag) {
            break;
        }
        if (taskResp.empty()) {
            taskResp = (pptaskArr[max_task_num - 1])->getTaskResp();
        }
    }

    if (task_fail_flag) {
        tp.clear();
    }

    //释放相关资源
    fin.close();

    for (int i=0; i<pool_size;i++)
    {
        delete pptaskArr[i];
    }
    delete [] pptaskArr;

    //释放二维数组空间
    for (int i=0;i<pool_size;i++) {
        delete [] fileContentBuf[i];
    }
    delete [] fileContentBuf;

    return taskResp;
}

string FileOp::FileUploadSliceFinish(FileUploadReq& req, string& dataRspJson)
{
    SDK_LOG_DBG("dataRspJson: %s", dataRspJson.c_str());

    Json::Value rspJson = StringUtil::StringToJson(dataRspJson);
    string sessionid = rspJson["data"]["session"].asString();
    //uint64_t filesize = FileUtil::getFileLen(req.getSrcPath());

    //多次签名
    uint64_t expired = FileUtil::GetExpiredTime();
    string sign = Auth::AppSignMuti(getAppid(),getSecretID(),getSecretKey(),expired, req.getBucket());
    map<string,string> user_headers;
    user_headers[HTTP_HEADER_AUTHORIZATION] = sign;

    map<string,string> user_params;
    user_params[PARA_OP] = OP_UPLOAD_SLICE_FINISH;
    user_params[PARA_SESSION] = sessionid;
    user_params[PARA_FILE_SIZE] = StringUtil::Uint64ToString(req.getFileSize());
    if (CosSysConfig::isTakeSha()){
        user_params[PARA_SHA] = req.getSha();
    }

    //构造URL
    string url = HttpUtil::GetEncodedCosUrl(getAppid(),req.getBucket(),req.getDstPath());

    //发送上传消息
    string finishRsp = HttpSender::SendSingleFilePostRequest(url,user_headers,
        user_params, NULL, 0);
    return finishRsp;
}

string FileOp::FileUploadSliceList(FileUploadReq& req)
{
    SDK_LOG_DBG("req: %s", req.toJsonString().c_str());

    string bucket = req.getBucket();
    //多次签名
    uint64_t expired = FileUtil::GetExpiredTime();
    string sign = Auth::AppSignMuti(getAppid(),getSecretID(),getSecretKey(),expired, bucket);
    map<string,string> user_headers;
    user_headers[HTTP_HEADER_AUTHORIZATION] = sign;

    map<string,string> user_params;
    user_params[PARA_OP] = OP_UPLOAD_SLICE_LIST;

    //构造URL
    string url = HttpUtil::GetEncodedCosUrl(getAppid(),req.getBucket(),req.getDstPath());

    //发送上传消息
    string listRsp = HttpSender::SendSingleFilePostRequest(url,user_headers,
        user_params, NULL, 0);
    return listRsp;
}

string FileOp::FileUpdate(FileUpdateReq& req)
{
    SDK_LOG_DBG("req: %s", req.toJsonString().c_str());

    CosResult ret;
    if (!req.isParaValid(ret))
    {
        return ret.toJsonString();
    }

    string bucket = req.getBucket();
    string formatPath = req.getFormatPath();
    string fullUrl = HttpUtil::GetEncodedCosUrl(getAppid(),bucket, formatPath);
    string sign = Auth::AppSignOnce(getAppid(),getSecretID(),getSecretKey(),
                                formatPath,bucket);

    std::map<string, string> http_headers;
    http_headers[HTTP_HEADER_AUTHORIZATION] = sign;

    Json::Value body;
    body[PARA_OP] = OP_UPDATE;
    if (req.isUpdateForBid()){
        body[PARA_FORBID] = req.getForbid();
    }
    if (req.isUpdateAuthority()){
        body[PARA_AUTHORITY] = req.getAuthority();
    }
    if (req.isUpdateBizAttr()){
        body[PARA_BIZ_ATTR] = req.getBizAttr();
    }
    if (req.isUpdateCustomHeader()){
        map<string,string> map_custom_headers = req.getCustomHeaders();
        map<string,string>::const_iterator iter = map_custom_headers.begin();
        for (;iter != map_custom_headers.end(); iter++)
        {
            body[PARA_CUSTOM_HEADERS][iter->first] = iter->second;
        }
    }
    string bodyJson = StringUtil::JsonToString(body);
    return HttpSender::SendJsonBodyPostRequest(fullUrl, bodyJson, http_headers);
}

string FileOp::FileStat(FileStatReq& req)
{
    SDK_LOG_DBG("req: %s", req.toJsonString().c_str());

    CosResult ret;
    if (!req.isParaValid(ret))
    {
        return ret.toJsonString();
    }

    string bucket = req.getBucket();
    string formatPath = req.getFormatPath();

    return statBase(bucket, formatPath);
}

string FileOp::FileDelete(FileDeleteReq& req)
{
    SDK_LOG_DBG("req: %s", req.toJsonString().c_str());

    CosResult ret;
    if (!req.isParaValid(ret))
    {
        return ret.toJsonString();
    }
    return delBase(req.getBucket(), req.getFilePath());
}

bool FileOp::FileDeleteAsyn(FileOp& op,
                            const FileDeleteReq& req,
                            DeleteCallback callback,
                            void* user_data) {
    Delete_Asyn_Arg arg(&op, req, callback, user_data);
    if (threadpool) {
        threadpool->schedule(boost::bind(FileDelete_Asyn_Thread, arg));
    } else {
        SDK_LOG_ERR("threadpool is null, error");
        return false;
    }

    return true;
}

string FileOp::FileMove(FileMoveReq& req)
{
    SDK_LOG_DBG("req: %s", req.toJsonString().c_str());

    CosResult ret;
    if (!req.isParaValid(ret))
    {
        return ret.toJsonString();
    }

    string bucket = req.getBucket();
    string formatPath = req.getFormatPath();

    string sign = Auth::AppSignOnce(getAppid(),getSecretID(),getSecretKey(),
                                    formatPath,bucket);

    string fullUrl = HttpUtil::GetEncodedCosUrl(getAppid(),bucket, formatPath);

    std::map<string, string> http_headers;
    http_headers[HTTP_HEADER_AUTHORIZATION] = sign;
    std::map<string, string> http_params;
    http_params[PARA_OP] = OP_MOVE;
    http_params[PARA_MOVE_DST_FILEID] = req.getDstPath();
    http_params[PARA_MOVE_OVER_WRITE] = StringUtil::IntToString(req.getOverWrite());
    return HttpSender::SendJsonPostRequest(fullUrl, http_headers, http_params);
}

}

