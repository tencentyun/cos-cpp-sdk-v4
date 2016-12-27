1、开发环境
依赖静态库: curl jsoncpp boost_system boost_thread  (在lib文件夹下)
依赖动态库: ssl crypto rt z  (需要安装)
(1)安装openssl的库和头文件 http://www.openssl.org/source/ 
(2)安装curl的库和头文件 http://curl.haxx.se/download/curl-7.43.0.tar.gz 
(3)安装jsoncpp的库和头文件 https://github.com/open-source-parsers/jsoncpp 
(4)安装boost的库和头文件 http://www.boost.org/ 
(5)安装cmake工具 http://www.cmake.org/download/ 

2、本地编译说明：
修改CMakeList.txt文件中，指定本地boost头文件路径，修改如下语句：
SET(BOOST_HEADER_DIR "/root/boost_1_61_0")

3、配置文件说明
"Region":"sh",                    //所属COS区域，上传下载操作的URL均与该参数有关
"SignExpiredTime":360,            //签名超时时间，单位：秒
"CurlConnectTimeoutInms":10000,   //CURL连接超时时间，单位：毫秒
"CurlGlobalConnectTimeoutInms":360000, //CURL连接执行最大时间，单位：毫秒
"UploadSliceSize":1048576,        //分片大小，单位：字节，可选的有512k,1M,2M,3M(需要换算成对应字节数)
"IsUploadTakeSha":0,              //上传文件时是否需要携带sha值
"DownloadDomainType":2,           //下载域名类型：1: cdn, 2: cos, 3: innercos, 4: self domain
"SelfDomain":"",                  //自定义域名
"UploadThreadPoolSize":5          //单文件分片上传线程池大小
"AsynThreadPoolSize":2            //异步上传下载线程池大小
"LogoutType":0                    //打印输出，0:不输出,1:输出到屏幕,2:打印syslog
"down_thread_pool_max_size":10    //下载文件到本地的线程池的最大大小,默认值为10,有效值的范围:(0,10]
"down_slice_size":4194304         //下载文件到本地的分片大小,range的范围,默认值4M，有效值范围:(4k,20M]