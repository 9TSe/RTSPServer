# 简介

Base 文件为 RTSP 的准备和知识基础(即使相较于RTSP_Server难度差距较大)

只能简单的将视频和音频搓在一起, 并不能保证音视频完全同步, 只是作为练手熟悉基本框架

RTSPClient的作用是从RTSPServer拉流(项目中相互的推拉流目前未尝试, Client使用的server是zlm)

以下介绍的是RTSPServer

---

# Linux环境中运行

## 1. 配置SDL环境

[SDL-1.2.15.tar.gz-一键下载](https://sourceforge.net/projects/libsdl/files/SDL/1.2.15/SDL-1.2.15.tar.gz/download)

```bash
tar -zxvf SDL-1.2.15.tar.gz

cd SDL-1.2.15/

./configure --prefix=/usr/local

make

make install
```


## 2. 安装ffmpeg库

```bash
sudo apt install ffmpeg
```

## 3. 整理文件

先切换到RTSP_Server目录中

```bash
mkdir src
mv *.cpp src

mkdir include
mv *.h include

mkdir build
```

准备好 .aac 和 .h264 文件放置RTSP_Server目录中

## 4. 运行

```bash
cd build
cmake ..
make
cd ..
./app
```

另起一个终端

```bash
#over tcp
ffplay -i -rtsp_transport tcp  rtsp://127.0.0.1:8554/test

#over udp
ffplay -i rtsp://127.0.0.1:8554/test
```


---

# 对源码框架的简单解读

0. 初始时间

1. 启用网络编程库. 实例化select模型. 启动定时器. 设置时间管理器回调函数(根据时间信息执行函数),触发条件(m_timerManager_readCallback)

2. 等待添加任务执行线程

3. new MediaSessionManager() 

4. new usageenvironment (eventscheduler,threadpool)

5. 初始化 Ipv4Address 设置ip和端口

6. 创建rtsp 利用sockets 进行 bind, 传入文件描述符创建ioevent和triggerevent
ioevent 实例的回调函数用于 accept 触发条件(m_readCallback)
triggerevent实例的回调函数用于关闭链接(delete rtspconnection) 触发条件(m_triggerCallback)

7. 初始化 mediasession("test")

8. H264FileMediaSource::Create_New(env, "daliu.h264"); 将h264源填入线程队列,线程得以执行, 处理h264格式放入输出队列

9. 设置h264执行的帧率, 为计时器提供信息,insert.  new timerevent
设置回调函数(将输出队列中的h264提取,send_frame->Send_RtpPacket) 触发条件 (m_timeoutCallback)timerevent handle

10. 设置回调函数,用于执行h264信息,传输rtp可tcp,udp, 触发条件 Send_RtpPacket(send,上一步)
    
11. 类似同8

12. 类似9, 不同的信息载入相同的类,初始化后强制转换. 加入同一队列, 重载send_frame->Send_RtpPacket
    
13. 同10, 区别取决于trackid

14. 将10,13会话加入到会话管理器(sessionmanager)
    
15. 开启监听, 跳到select模型进行"初始化"

16.  loop(1. m_timerManager_readCallback触发,6.  m_triggerCallback触发,6.  m_readCallback, m_..Callback触发 9. m_timeoutCallback 通过 m_timerManager_readCallback触发)