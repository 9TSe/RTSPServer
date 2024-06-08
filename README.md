# 简介

Base 文件为 RTSP 的准备和知识基础, 包括RTCP

主项目将视频和音频搓在一起, 不保证音视频完全同步, 作为练手熟悉框架

项目涉及到
cpp, cmake, 线程池, 原子变量, 锁, 定时器触发器, Epoll模型, chrono库, 智能指针, 对象包装器, 右值引用, 较多回调函数等

---

# Linux中运行

## 1. 配置SDL环境

[SDL-1.2.15.tar.gz-一键下寨!](https://sourceforge.net/projects/libsdl/files/SDL/1.2.15/SDL-1.2.15.tar.gz/download)

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

## 2.5 aac源和h264源

```bash
//两种方法使用ffmpeg提出MP4视频源(h264)
//1.
ffmpeg -i source.mp4 -codec copy -bsf: h264_mp4toannexb -f h264 source.h264
//2. 
ffmpeg -i source.mp4 -an -vcodec copy -f h264 source.h264


//使用ffmpeg提出MP4音频源(aac)
ffmpeg -i source.mp4  -vn -acodec aac source.aacffmepg 

```


## 3. 运行

```bash
#cd 到main.cpp的目录后
mkdir build
cd build
cmake ..
make
./RTSPServer
```

另起一个终端

```bash
#over tcp
ffplay -i -rtsp_transport tcp  rtsp://127.0.0.1:8554/test

#over udp
ffplay -i rtsp://127.0.0.1:8554/test
```

ps: 源码使用视频 b站视频号: `BV1jF4m1P7J5`