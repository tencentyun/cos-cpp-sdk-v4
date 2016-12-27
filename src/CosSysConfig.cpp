#include "CosSysConfig.h"
#include "CosDefines.h"
#include <iostream>
#include <stdint.h>

namespace qcloud_cos {

//上传文件是否携带sha值,默认不携带
bool CosSysConfig::takeSha = false;
//上传文件分片大小,默认1M
uint64_t CosSysConfig::sliceSize = SLICE_SIZE_1M;
//签名超时时间,默认60秒
uint64_t CosSysConfig::expire_in_s = 60;
//HTTP连接时间(秒)
uint64_t CosSysConfig::timeout_in_ms = 5 * 1000;
//全局超时时间(毫秒)
uint64_t CosSysConfig::timeout_for_entire_request_in_ms = 300 * 1000;
//L5的模块ID信息
int64_t CosSysConfig::l5_modid = -1;
int64_t CosSysConfig::l5_cmdid = -1;
string CosSysConfig::m_region = "";
string CosSysConfig::m_uploadDomain = "";
unsigned CosSysConfig::m_threadpool_size = DEFAULT_THREAD_POOL_SIZE_UPLOAD_SLICE;
unsigned CosSysConfig::m_asyn_threadpool_size = DEFAULT_POOL_SIZE;
//日志输出
LOG_OUT_TYPE CosSysConfig::log_outtype = COS_LOG_STDOUT;

const string DOWN_ULR_CDN = "file.myqcloud.com";
const string DOWN_ULR_INNER_COS = "innercos.myqcloud.com";

//下载文件到本地线程池大小
unsigned CosSysConfig::m_down_thread_pool_max_size = 10;
//下载文件到本地,每次下载字节数
unsigned CosSysConfig::m_down_slice_size = 4*1024*1024;

//下载域名类型,用于构造下载url,默认为cdn
DLDomainType  CosSysConfig::domainType = DOMAIN_CDN;
string  CosSysConfig::downloadDomain = DOWN_ULR_CDN;

void CosSysConfig::print_value()
{
    std::cout << "region:" << m_region << std::endl;
    std::cout << "takeSha:" << takeSha << std::endl;
    std::cout << "sliceSize:" << sliceSize << std::endl;
    std::cout << "expire_in_s:" << expire_in_s << std::endl;
    std::cout << "timeout_in_ms:" << timeout_in_ms << std::endl;
    std::cout << "timeout_for_entire_request_in_ms:" << timeout_for_entire_request_in_ms << std::endl;
    std::cout << "l5_modid:" << l5_modid << std::endl;
    std::cout << "l5_cmdid:" << l5_cmdid << std::endl;
    std::cout << "m_uploadDomain:" << m_uploadDomain << std::endl;
    std::cout << "m_threadpool_size:" << m_threadpool_size << std::endl;
    std::cout << "log_outtype:" << log_outtype << std::endl;
    std::cout << "domainType:" << domainType << std::endl;
    std::cout << "downloadDomain:" << downloadDomain << std::endl; 
    std::cout << "threadpool_size:" << m_threadpool_size << std::endl;
    std::cout << "asyn_threadpool_size:" << m_asyn_threadpool_size << std::endl;
    std::cout << "log_outtype:" << log_outtype << std::endl;
    std::cout << "down_thread_pool_max_size:" << m_down_thread_pool_max_size << std::endl;
    std::cout << "down_slice_size:" << m_down_slice_size << std::endl;

}

void CosSysConfig::setIsTakeSha(bool isTakeSha)
{
    takeSha = isTakeSha;
}

void CosSysConfig::setSliceSize(uint64_t slice_size)
{
    sliceSize = slice_size;
}

void CosSysConfig::setDownThreadPoolMaxSize(unsigned size)
{
    if (size > 10) {
        m_down_thread_pool_max_size = 10;
    } else if (size < 1) {
        m_down_thread_pool_max_size = 1;        
    } else {
        m_down_thread_pool_max_size = size;
    }
}

void CosSysConfig::setDownSliceSize(unsigned slice_size)
{
    if (slice_size < 4 * 1024) {
        m_down_slice_size = 4* 1024;
    } else if (slice_size > 20 * 1024 * 1024) {
        m_down_slice_size = 20 * 1024 * 1024;
    } else {
        m_down_slice_size = slice_size;
    }
}

unsigned CosSysConfig::getDownThreadPoolMaxSize()
{
    return m_down_thread_pool_max_size;
}

unsigned CosSysConfig::getDownSliceSize()
{
    return m_down_slice_size;
}

void CosSysConfig::setExpiredTime(uint64_t time)
{
    expire_in_s = time;
}
void CosSysConfig::setTimeoutInms(uint64_t time)
{
    timeout_in_ms = time;
}
void CosSysConfig::setGlobalTimeoutInms(uint64_t time)
{
    timeout_for_entire_request_in_ms = time;
}
void CosSysConfig::setL5Modid(int64_t modid)
{
    l5_modid = modid;
}
void CosSysConfig::setL5Cmdid(int64_t cmdid)
{
    l5_cmdid = cmdid;
}

void CosSysConfig::setUploadThreadPoolSize(unsigned size)
{
    if ( size > MAX_THREAD_POOL_SIZE_UPLOAD_SLICE ) {
        m_threadpool_size = MAX_THREAD_POOL_SIZE_UPLOAD_SLICE;
    } else if (size < MIN_THREAD_POOL_SIZE_UPLOAD_SLICE) {
        m_threadpool_size = MIN_THREAD_POOL_SIZE_UPLOAD_SLICE;
    } else {
        m_threadpool_size = size;
    }
}

void CosSysConfig::setAsynThreadPoolSize(unsigned size)
{
    if ( size > MAX_THREAD_POOL_SIZE_UPLOAD_SLICE ) {
        m_asyn_threadpool_size = MAX_THREAD_POOL_SIZE_UPLOAD_SLICE;
    } else if (size < MIN_THREAD_POOL_SIZE_UPLOAD_SLICE) {
        m_asyn_threadpool_size = MIN_THREAD_POOL_SIZE_UPLOAD_SLICE;
    } else {
        m_asyn_threadpool_size = size;
    }
}

unsigned CosSysConfig::getAsynThreadPoolSize()
{
    return m_asyn_threadpool_size;
}

unsigned CosSysConfig::getUploadThreadPoolSize()
{
    return m_threadpool_size;
}

void CosSysConfig::setLogOutType(LOG_OUT_TYPE log)
{
    log_outtype = log;
}

int CosSysConfig::getLogOutType()
{
    return (int)log_outtype;
}

void CosSysConfig::setRegionAndDLDomain(string region, DLDomainType type, string domain)
{
    m_region = region;
    m_uploadDomain = "http://" + region + ".file.myqcloud.com/files/v2/";
    CosSysConfig::domainType = type;
    switch (type){
        case DOMAIN_CDN: 
            downloadDomain = DOWN_ULR_CDN;
            break;
        case DOMAIN_COS: 
            downloadDomain = "cos" + region + ".myqcloud.com";
            break;
        case DOMAIN_INNER_COS: 
            downloadDomain = DOWN_ULR_INNER_COS;
            break;
        case DOMAIN_SELF_DOMAIN: 
            downloadDomain = domain;
            break;
        default:
            break;
    }

    return;
}

string CosSysConfig::getDownloadDomain()
{
    return downloadDomain;
}

string CosSysConfig::getUploadDomain()
{
    return m_uploadDomain;
}

DLDomainType CosSysConfig::getDownloadDomainType()
{
    return domainType;
}

bool CosSysConfig::isTakeSha()
{
    return takeSha;
}

uint64_t CosSysConfig::getSliceSize()
{
    return sliceSize;
}

uint64_t CosSysConfig::getExpiredTime()
{
    return expire_in_s;
}

uint64_t CosSysConfig::getTimeoutInms()
{
    return timeout_in_ms;
}
uint64_t CosSysConfig::getGlobalTimeoutInms()
{
    return timeout_for_entire_request_in_ms;
}
int64_t CosSysConfig::getL5Modid()
{
    return l5_modid;
}
int64_t CosSysConfig::getL5Cmdid()
{
    return l5_cmdid;
}
}
