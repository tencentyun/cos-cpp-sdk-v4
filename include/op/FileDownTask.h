#pragma once

#include <pthread.h>
#include <string>
#include "op/BaseOp.h"
#include "util/FileUtil.h"
#include "util/HttpUtil.h"
#include "util/CodecUtil.h"
#include "util/HttpSender.h"
#include "util/StringUtil.h"
#include "request/CosResult.h"
#include "CosParams.h"
#include "CosDefines.h"
#include "CosConfig.h"
#include "CosSysConfig.h"

namespace qcloud_cos{

class FileDownTask {

public:
    FileDownTask(const std::string& url, const std::string& sign, uint64_t offset, 
        unsigned char* pbuf, const size_t datalen);
    FileDownTask(const std::string& url, const std::string& sign, const std::string& bucket, int appid);
    ~FileDownTask(){};
    void run();
    void downTask();
    void setDownParams(unsigned char* pdatabuf, size_t datalen, uint64_t offset);
    std::string getTaskResp();
    size_t getDownLoadLen();
    bool taskSuccess();
private:
    FileDownTask(){};

private:
    std::string m_bucket;
    uint64_t m_appid;
    std::string m_url;
    std::string m_sign;
    uint64_t m_offset;
    unsigned char*  m_pdatabuf;
    size_t m_datalen;
    std::string m_resp;
    size_t m_real_down_len;
    bool m_task_success;
};

}
