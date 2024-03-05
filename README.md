# 简介

1-5 文件分别为 RTSP 的准备和知识基础(即使相较于RTSP_Server难度差距较大)

只能简单的将视频和音频搓在一起, 并不能保证音视频完全同步, 只是作为练手熟悉基本框架

---

# Linux环境中运行

## 1. 配置SDL环境

(SDL-1.2.15.tar.gz-一键下载)[https://sourceforge.net/projects/libsdl/files/SDL/1.2.15/SDL-1.2.15.tar.gz/download]

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