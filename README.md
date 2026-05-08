# 🛡️ AI 智能人脸识别监控看板 (Enterprise AI Facial Recognition Dashboard)

![C++](https://img.shields.io/badge/C++-17-blue.svg) ![Qt](https://img.shields.io/badge/Qt-5.15+-41CD52.svg) ![License](https://img.shields.io/badge/License-MIT-green.svg)

基于 **C++17** 与 **Qt 5** 开发的企业级 AI 人脸识别桌面监控终端。本项目深度集成了百度 AI 开放平台视觉算法，实现了从底层硬件设备捕获、高并发异步网络通信到前端 QSS 定制化渲染的完整桌面端业务流。

## ✨ 核心特性 (Core Features)

- **🎥 实时视频流接入**：基于 `QCamera` 与 `QCameraImageCapture` 实现的多设备自适应视频流捕获。
- **🧠 结构化 AI 分析**：集成百度 AI 接口，实时精准输出（年龄、性别、情绪、颜值、口罩佩戴、眼镜款式、活体检测）等多维结构化数据。
- **🚀 高性能并发架构**：彻底摒弃传统 `QThread` 的笨重开销，采用 **QtConcurrent 线程池** 结合 Lambda 表达式，实现图像 Base64 编码与网络 I/O 的完全异步化，主 GUI 线程零阻塞，监控画面丝滑流畅。
- **🎨 企业级 Dashboard UI**：完全脱离 Qt Designer 的原生灰白束缚，纯代码重构响应式分栏布局，并注入全局深色科技风 QSS (Qt Style Sheets) 主题。
- **📈 极速渲染优化**：引入数据帧双缓冲与文本绘制缓存机制，避免每秒数十次的重复字符串拼接，大幅降低 CPU/GPU 绘制开销。
- **🔒 商业级工程规范**：修复了原生多态指针带来的内存泄漏风险；将敏感 API Key 抽离至本地 `config.ini` 动态读取，兼顾了安全性与可维护性。

## 🛠️ 技术栈 (Tech Stack)

- **核心语言**: C++17
- **GUI 框架**: Qt 5 (Widgets, Multimedia, Network, Concurrent)
- **数据交互**: HTTPS (QSslConfiguration), QJsonDocument, RESTful API
- **多线程模型**: Thread Pool (`QtConcurrent::run`, `QFutureWatcher`)
- **视觉定制**: QSS, QPainter 自定义绘制

## 🚀 快速开始 (Quick Start)

### 1. 环境依赖
- Qt 5.15.0 或更高版本
- 编译器：MinGW 64-bit 或 MSVC
- OpenSSL (用于 HTTPS 请求)

### 2. 编译与构建
1.使用 Qt Creator 打开 image_recognition.pro 文件。

2.右键项目选择 “执行 qmake”，然后点击运行。
##⚙️ 配置指南 (Configuration)
  为保障账号与数据安全，本程序源码中不内置任何第三方 API 密钥。你需要进行以下简单配置：
    1.注册获取密钥：前往 百度智能云控制台 - 人脸识别，创建应用并获取免费的 API Key 和 Secret Key。
    2.生成配置文件：首次运行本软件时，若检测不到密钥，系统会在可执行文件同级目录（如 build-xxx-Debug/debug/）下自动生成一个 config.ini 模板文件，并在软件界面给出路径提示。
    3.填入密钥：使用文本编辑器打开生成的 config.ini，填入你的密钥：
        client_id=在此填入你的_API_KEY
        client_secret=在此填入你的_SECRET_KEY
      
3.重启软件：保存文件后重新打开本软件，即可正常进行鉴权与识别。

##📖 使用说明 (Usage)
本系统采用全自动化识别流设计，操作极为简便：
1.系统初始化：打开软件后，右侧日志窗口会显示网络连接状态。若配置正确，系统会自动获取 Token 并显示 【系统就绪】Token 鉴权成功。
2.选择监控设备：在右侧控制栏的下拉菜单中，选择你想要调用的摄像头设备。系统支持热切换，切换时会自动安全释放旧设备资源。
3.自动化实时分析：
  目标进入画面后，系统每秒会自动在后台线程池发起异步 AI 分析请求。
  识别成功后，左侧视频流画面中将以动态线框实时锁定人脸，并在人脸旁渲染核心数据。
  右侧“AI 结构化数据分析”面板会同步滚动输出详细的活体分数与多维特征报告。
4.手动抓拍 (可选)：点击 [拍照] 按钮，可强制触发一次即时的人脸检测分析，适用于需要立即确认数据的场景。

##🤝 参与贡献
欢迎提交 Issue 探讨问题，或发起 Pull Request 来帮助完善这个项目！
