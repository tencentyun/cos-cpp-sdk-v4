#include <pthread.h>
#include <string>
#include "curl/curl.h"
#include "threadpool/boost/threadpool.hpp"
#include "CosApi.h"
using std::string;
namespace qcloud_cos {

int CosAPI::g_init = 0;
int CosAPI::cos_obj_num = 0;
SimpleMutex CosAPI::init_mutex = SimpleMutex();
boost::threadpool::pool *threadpool = NULL;

int CosAPI::FileDownload(FileDownloadReq& request, char* buffer, size_t bufLen, uint64_t offset, int* ret_code)
{
    return fileOp.FileDownload(request, buffer, bufLen, offset, ret_code);
}

int CosAPI::FileDownload(FileDownloadReq& request, string localPath, int* ret_code)
{
    return fileOp.FileDownload(request, localPath, ret_code);
}

bool CosAPI::FileDownloadAsyn(FileDownloadReq& request, char* buffer, size_t bufLen,DownloadCallback callback, void* user_data)
{
    return fileOp.FileDownloadAsyn(fileOp, request, buffer, bufLen, callback, user_data);
}

bool CosAPI::FileUploadAsyn(FileUploadReq& request, UploadCallback callback, void* user_data)
{
    return fileOp.FileUploadAsyn(fileOp, request, callback, user_data);
}

string CosAPI::FileUpload(FileUploadReq& request)
{
    return fileOp.FileUpload(request);
}

string CosAPI::FileStat(FileStatReq& request)
{
    return fileOp.FileStat(request);
}

string CosAPI::FileDelete(FileDeleteReq& request)
{
    return fileOp.FileDelete(request);
}

string CosAPI::FileMove(FileMoveReq& request)
{
    return fileOp.FileMove(request);
}

bool CosAPI::FileDeleteAsyn(FileDeleteReq& req, DeleteCallback callback, void* user_data) {
    return fileOp.FileDeleteAsyn(fileOp, req, callback, user_data);
}

string CosAPI::FileUpdate(FileUpdateReq& request)
{
    return fileOp.FileUpdate(request);
}
string CosAPI::FolderCreate(FolderCreateReq& request)
{
    return folderOp.FolderCreate(request);
}
string CosAPI::FolderStat(FolderStatReq& request)
{
    return folderOp.FolderStat(request);
}
string CosAPI::FolderDelete(FolderDeleteReq& request)
{
    return folderOp.FolderDelete(request);
}
string CosAPI::FolderUpdate(FolderUpdateReq& request)
{
    return folderOp.FolderUpdate(request);
}
string CosAPI::FolderList(FolderListReq& request)
{
    return folderOp.FolderList(request);
}

CosAPI::CosAPI(CosConfig& config):fileOp(config),folderOp(config)
{
    COS_Init();
}

CosAPI::~CosAPI()
{
    COS_UInit();
}

int CosAPI::COS_Init() {
    SimpleMutexLocker locker(&init_mutex);
    ++cos_obj_num;
    if (!g_init) {
        g_init = true;
        CURLcode retCode = curl_global_init(CURL_GLOBAL_ALL);
        if (CURLE_OK != retCode){
            SDK_LOG_ERR("curl_global_init error, retcode=%d",retCode);
            return -1;
        }

        threadpool = new boost::threadpool::pool(CosSysConfig::getAsynThreadPoolSize());
    }

    return 0;
}

void CosAPI::COS_UInit() {
    SimpleMutexLocker locker(&init_mutex);
    --cos_obj_num;
    if (g_init && cos_obj_num == 0) {
        curl_global_cleanup();

        if (threadpool){
            threadpool->wait();
            delete threadpool;
        }

        g_init = false;
    }
}

}
