#include <string.h>
#include <stdint.h>
#include <map>
#include <string>
#include "op/FileOp.h"
#include "Auth.h"
#include "op/FileDownTask.h"

using std::string;
using std::map;
namespace qcloud_cos{

FileDownTask::FileDownTask(const string& url, const string& sign, uint64_t offset, 
        unsigned char* pbuf, const size_t datalen) : m_url(url), m_sign(sign), 
        m_offset(offset), m_pdatabuf(pbuf), m_datalen(datalen), m_resp(""), m_task_success(false)
{
}

FileDownTask::FileDownTask(const string& url, const string& sign, const string& bucket, int appid) : 
    m_url(url), m_sign(sign), 
    m_offset(0), m_pdatabuf(NULL), 
    m_datalen(0), m_resp(""), m_task_success(false)
{
    m_bucket = bucket;
    m_appid = appid;
}

void FileDownTask::run()
{
    m_resp = "";
    m_task_success = false;
    downTask();
}

void FileDownTask::setDownParams(unsigned char* pbuf, size_t datalen, uint64_t offset)
{
    m_pdatabuf = pbuf;
    m_datalen  = datalen;
    m_offset = offset;
}

size_t FileDownTask::getDownLoadLen()
{
    return m_real_down_len;
}

bool FileDownTask::taskSuccess() {
    return m_task_success;
}

string FileDownTask::getTaskResp() {
    return m_resp;
}

void FileDownTask::downTask()
{
    SDK_LOG_DBG("FileDownTask:url:%s", m_url.c_str());
    std::map<string, string> user_params;
    std::map<string, string> http_params;
    //文件下载,架平需要host头域中带'-'
    char host_head[256];
    memset(host_head, 0, sizeof(host_head));
    snprintf(host_head, sizeof(host_head),"%s-%lu.%s", m_bucket.c_str(), m_appid,
                         CosSysConfig::getDownloadDomain().c_str());
    http_params["host"] = host_head;
    char range_head[128];
    memset(range_head, 0, sizeof(range_head));
    snprintf(range_head,sizeof(range_head),"bytes=%lu-%lu", m_offset, (m_offset+m_datalen - 1));
    //增加Range头域，避免大文件时将整个文件下载
    http_params["Range"] = range_head;
    //http_params["Authorization"] = m_sign;
    int http_code = HttpSender::SendGetRequest(&m_resp, m_url, http_params, user_params);
    //当实际长度小于请求的数据长度时httpcode为206
    if (http_code != 200 && http_code != 206)
    {
        SDK_LOG_ERR("FileDownload: url(%s) fail, httpcode:%d, ret: %s", m_url.c_str(), http_code, m_resp.c_str());
        m_task_success = false;
        m_real_down_len = 0;
        return;
    }

    size_t buf_max_size = m_datalen;
    size_t len = MIN(m_resp.length(), buf_max_size);
    memcpy(m_pdatabuf, m_resp.c_str(), len);
    m_real_down_len = len;
    m_task_success = true;
    m_resp = "";
    return;
}

}

