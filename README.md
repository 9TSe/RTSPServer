# 简介

Base 文件为 RTSP 的准备和知识基础
只能简单的将视频和音频搓在一起, 并不能保证音视频完全同步, 作为练手熟悉框架

主要使用技术
线程池, 定时器触发器, 回调函数, Epoll模型(不支持windows环境), chrono库, 智能指针(待更新), 对象包装器(待更新)等

---

# Linux中运行

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