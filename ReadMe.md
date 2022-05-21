# 简介
salt是一个基于c++ asio的网络框架，计划支持tcp的服务端和客户端。

# 开发计划
服务器端
- [x] 监听端口，接受新客户端
- [x] 基本的数据传输功能
- [x] 基于包头、包内容的拆包器（用于接收数据）

客户端
- [x] 连接服务器端
- [x] 发送，接收数据
- [x] 连接失败重试
- [ ] 基于服务名称的轮询负载均衡
- [x] 基于包头、包内容的拆包器（用于接收数据）

其它
- [ ] 可扩展的日志接口
- [x] 持续集成
- [x] cmake Config-file package
- [ ] 文档
- [ ] 使用string_view优化内存拷贝

# 编译
```bash
git clone https://github.com/Tyzual/salt.git
cd salt
cmake -S . -B build
cmake --build build
```