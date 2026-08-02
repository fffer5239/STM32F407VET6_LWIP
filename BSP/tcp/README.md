# STM32F407 LWIP TCP 适配说明

## 适配背景

CubeMX 生成的 LWIP 是 **裸机模式**（NO_SYS=1），原 BSP TCP 代码使用 **Socket API + FreeRTOS**，两者不兼容。已全部改为 **Raw API** 实现。

## 文件改动

| 文件 | 改动 |
|------|------|
| `BSP/tcp/TcpClient.h` | 接口定义，SERVER_IP、端口等宏 |
| `BSP/tcp/TcpClient.c` | Socket API → Raw API 重写 |
| `BSP/tcp/TcpServer.h` | 接口定义，去掉 FreeRTOS/cJSON 依赖 |
| `BSP/tcp/TcpServer.c` | Raw API Echo 服务器 |
| `Core/Src/main.c` | 主循环添加 `MX_LWIP_Process()` + 测试用例 |
| `Core/Src/stm32f4xx_it.c` | 添加 `ETH_IRQHandler`（之前缺失） |
| `Core/Inc/stm32f4xx_it.h` | 声明 `ETH_IRQHandler` |

## 功能说明

### TCP Server（Echo 服务器）

- 监听端口：**8888**
- 收到数据原样返回
- 电脑端用网络调试助手连接 `192.168.1.5:8888` 测试

### TCP Client（心跳发送）

- 连接目标：`192.168.1.100:1030`（可在 `TcpClient.h` 中修改）
- 每 5 秒发送 `Hello from STM32 #N`
- 断线自动重连
- 电脑端开 TCP Server 监听 `1030` 即可接收

## 网络配置

| 参数 | 值 |
|------|-----|
| STM32 IP | `192.168.1.5` |
| 子网掩码 | `255.255.255.0` |
| 网关 | `192.168.1.1` |
| 电脑 IP | `192.168.1.100`（手动设置） |

配置入口：`LWIP/App/lwip.c` → `MX_LWIP_Init()`

## 测试步骤

### 1. 编译烧录

MDK 工程需确保 `BSP/tcp` 在 include path 中。

### 2. 串口验证

连接 USART1（PA9/PA10，115200），上电后应输出：

```
========================================
  STM32F407 LWIP TCP Test
  IP: 192.168.1.5
========================================

TCP Server: Listening on port 8888
```

### 3. Ping 测试

```powershell
ping 192.168.1.5
```

### 4. TCP Server 测试

网络调试助手 → TCP Client → 连接 `192.168.1.5:8888` → 发送文字，看是否回显。

### 5. TCP Client 测试

网络调试助手 → TCP Server → 监听 `192.168.1.100:1030` → 等待接收 `Hello from STM32 #N`。

## 注意事项

1. 电脑和 STM32 必须直连或通过交换机，**不能经过路由器**（除非路由器 IP 和 STM32 同网段）
2. 电脑"以太网"网卡必须手动设置 IP 为 `192.168.1.100`
3. 如果要修改 STM32 IP，改 `LWIP/App/lwip.c` 中的 `IP_ADDRESS[]`
4. 如果要修改 TCP Client 目标，改 `TcpClient.h` 中的 `SERVER_IP`