# Linux 智能网关

这是一个基于 Linux、MQTT、串口和 Mesh 蓝牙模块的智能网关示例项目。网关负责把网页/MQTT 的控制命令下发给 STM32 下位机，也负责把 STM32 主动上报的数据（例如温度）发布回 MQTT。

## 串口收发改造计划

> 以下为后续可靠性改造方案，当前代码尚未实现。

### 上行：接收缓存与分帧

保留唯一串口读线程。每次 `read()` 得到的原始字节先追加到 RX 环形缓冲区，再由该线程按 `F1 DD <len> ...` 帧格式完成查找帧头、长度校验和分帧。只有完整且合法的一帧才进入 `up_buffer` 和后续 MQTT 上行流程。

该设计可正确处理半包、粘包、多帧合并、噪声字节和错误长度；未完成帧保留在环形缓冲区，等待下一次串口读取补全。

### 下行：唯一 TX 串口写线程

保留 `down_buffer`，将其作为下行 FIFO 队列。MQTT 回调只负责校验 JSON、转换业务数据并写入 `down_buffer`，随后唤醒唯一的 UART TX 线程；下行不再通过线程池提交串口写任务。

TX 线程在队列为空时通过条件变量等待；取出消息后依次执行蓝牙封包、处理部分写入或 `EINTR`/`EAGAIN`，并将完整帧写入 `/dev/ttyS1`。整个程序中只有该 TX 线程调用串口 `write()`，以保证多条 MQTT 下行命令按入队顺序发送且字节不会交叉。

关闭流程应先停止新的下行入队，唤醒并回收 TX 线程，最后关闭串口文件描述符。

> 当前项目已验证 MQTT 下行命令可以到达网关本地蓝牙模块的串口。由于目前没有 STM32 一侧的蓝牙模块，**无线链路、STM32 收包和温度上报尚未进行真机端到端验证**。
>
> **OTA 模块开发中：** 正在实现固件空中升级功能，版本比较逻辑已完成。

## 1. 硬件与通信拓扑

```text
网页 MQTT Client
       │ WebSocket：ws://192.168.50.104:8083
       ▼
 MQTT Broker：192.168.50.104:1883
       │
       ▼
 Linux 网关程序（gateway）
       │
       │ UART：/dev/ttyS1
       │ TX / RX / GND
       ▼
 网关侧 Mesh 蓝牙模块
       │
       │ 无线 Mesh / 蓝牙通信
       ▼
 STM32 侧 Mesh 蓝牙模块
       │
       │ TX / RX / GND
       ▼
 STM32F103ZET6 USART3（PB10 / PB11）
```

这里的两段 UART 飞线分别是：

```text
Linux 网关 UART  <->  网关侧蓝牙模块
STM32 USART3    <->  STM32 侧蓝牙模块
```

两块蓝牙模块之间的数据通过**无线 Mesh**传输，不需要再用串口线直接连接网关和 STM32。

### 串口接线原则

UART 必须交叉连接，并且共地：

```text
主控 TX  ->  蓝牙模块 RXD
主控 RX  <-  蓝牙模块 TXD
GND      ->  蓝牙模块 GND
```

STM32 侧参考连接：

```text
STM32 PB10 / USART3_TX  ->  蓝牙模块 RXD
STM32 PB11 / USART3_RX  <-  蓝牙模块 TXD
STM32 GND               ->  蓝牙模块 GND
```

注意确认模块供电和 IO 电平兼容，STM32F1 的 UART 电平通常为 3.3V。

## 2. 模块地址与网络配置

两块模块必须位于同一个 Mesh 网络，但每块模块的地址必须唯一。

| 节点 | NETID | MADDR | 说明 |
| --- | --- | --- | --- |
| Linux 网关侧蓝牙模块 | `1234` | `0101` | 网关代码启动时配置 |
| STM32 下位机侧蓝牙模块 | `1234` | `0088` | STM32 参考工程启动时配置 |

规则如下：

- **NETID 必须相同**：决定两个模块是否在同一个 Mesh 网络中。
- **MADDR 必须不同**：每个节点在该网络内的唯一地址，不能冲突。
- `FFFF` 通常表示广播地址：向同一个 NETID 中的所有节点发送。

网关中对应的配置位于 [app_bt.c](app/app_bt.c)：

```c
app_bt_setmaddr(device, "0101");
app_bt_setNetId(device, "1234");
```

STM32 参考工程会配置 `NETID=1234`、`MADDR=0088`。STM32 本地 USART3 使用 `9600, 8N1`；网关侧代码会把**网关与本地蓝牙模块之间**的 UART 调整为 `115200`。这两段本地 UART 的速率可以不同，只要求每一侧的主控与其本地模块参数一致。

## 3. 网关启动

正式部署只需要启动守护进程：

```sh
/usr/bin/gateway daemon
```

`daemon` 会自动创建并守护 `app` 与 `ota` 两个子进程。`app` 负责串口、蓝牙和 MQTT 网关业务，`ota` 负责固件版本检查；子进程退出后，daemon 会在下一次巡检时重新启动它。

单独执行以下命令仅用于模块调试：

```sh
/usr/bin/gateway app
/usr/bin/gateway ota
```

`app` 子进程的启动逻辑位于 [main.c](main.c) 和 [app_runner.c](app/app_runner.c)：

1. 打开串口设备 `/dev/ttyS1`；
2. 初始化线程池与 MQTT；
3. 配置本地蓝牙模块；
4. 启动串口读线程，用于上行数据；
5. 注册 MQTT 回调，用于下行数据。

串口设备宏定义在 [app_runner.h](app/app_runner.h)：

```c
#define DEVICE_FILE "/dev/ttyS1"
```

如果实际接线不是该串口，应先修改宏并重新编译部署。还需要确认运行账户有权访问该设备，例如设备节点存在、串口未被其他程序占用、账户属于相应串口用户组。

### 开发机测试模式

开发机未接入网关串口时，可使用测试模式只运行 OTA 子进程，避免 daemon 反复拉起依赖 `/dev/ttyS1` 的 `app`：

```sh
GATEWAY_TEST_MODE=1 ./gatway_test daemon
```

测试模式不会启动 `app`，因此不会建立 MQTT 或串口链路；生产部署时不要设置该环境变量，daemon 会照常管理 `app` 与 `ota`。

## 4. MQTT 配置与网页使用

网关 MQTT 配置位于 [app_mqtt.c](app/app_mqtt.c)：

| 配置 | 当前值 |
| --- | --- |
| Broker | `tcp://192.168.50.104:1883` |
| QoS | `1` |
| 下行订阅主题 | `pull` |
| 上行发布主题 | `push` |
| Client ID | `b253ba38-daf6-4b37-984f-5d8fdc6a1cfa` |

当前实现没有 TLS、用户名或密码配置；部署到真实网络前应根据实际环境补充认证与加密。

网页 Client 应：

```text
订阅 push：接收网关上报的数据
发布 pull：向网关发送控制命令
```

### 下行命令 JSON

网页点击开灯的示例：

```json
{
  "conn_type": 1,
  "id": "0088",
  "msg": "31"
}
```

网页点击关灯的示例：

```json
{
  "conn_type": 1,
  "id": "0088",
  "msg": "30"
}
```

字段含义：

| 字段 | 含义 |
| --- | --- |
| `conn_type` | 连接类型，当前代码使用 `1` |
| `id` | 目标蓝牙节点 MADDR，以每字节两个十六进制字符表示，例如 `0088` |
| `msg` | 业务载荷，以每字节两个十六进制字符表示；`31` 是 ASCII 字符 `'1'`，`30` 是 ASCII 字符 `'0'` |

因此网页发送到 `pull` 后，网关会向地址 `0088` 的下位机发送开/关灯业务字节。

## 5. 下行流程：网页到 STM32

下行路径如下：

```text
网页 publish("pull")
  -> MQTT broker
  -> app_mqtt.c: msgarrvd()
  -> app_device.c: receive_msg_func()
  -> down_buffer
  -> 线程池 write_data_task_func()
  -> app_bt_preWrite()
  -> write(/dev/ttyS1)
  -> 网关侧蓝牙模块
  -> 无线 Mesh
  -> STM32 侧蓝牙模块
  -> STM32 USART3
```

以 `id="0088"`、`msg="31"`（开灯）为例：

1. JSON 先转换成网关内部字节格式：

   ```text
   01 02 01 00 88 31
   ```

   含义是：

   ```text
   conn_type=01 | id_len=02 | msg_len=01 | id=00 88 | msg=31
   ```

2. [app_bt.c](app/app_bt.c) 的 `app_bt_preWrite()` 再把它封装为发送给本地蓝牙模块的 AT/Mesh 指令：

   ```text
   41 54 2B 4D 45 53 48 00 00 88 31 0D 0A
   A  T  +  M  E  S  H  \0  00 88 31 \r \n
   ```

   即：

   ```text
   AT+MESH + 0x00 + 目标 MADDR 两字节 + 业务载荷 + CRLF
   ```

   两个连续的 `00` 是正常的：第一个 `00` 是代码复制 `"AT+MESH"` 时带上的字符串结束符，第二个 `00` 是目标地址 `0088` 的高字节。

### 下行成功时的典型日志

```text
MQTT下行回调: {"conn_type":1,"id":"0088","msg":"31"}
准备写入下行缓冲区, len=6
已写入下行缓冲区, 准备提交串口写任务
下行串口任务开始
下行缓冲区读取完成, len=6
开始执行蓝牙预处理
蓝牙预处理完成, len=13
准备写串口, fd=3, len=13
串口write返回值=13
写入串口文件成功, len=13
```

`write()` 返回 `13` 的含义是 Linux 串口驱动已接收待发送的 13 字节；它**不能单独证明**无线模块已经送达 STM32，仍需通过接收端串口抓包或 STM32 行为确认。

## 6. 上行流程：STM32 温度到网页

上行路径如下：

```text
STM32 采集温度/主动上报
  -> STM32 USART3
  -> STM32 侧蓝牙模块
  -> 无线 Mesh
  -> 网关侧蓝牙模块
  -> /dev/ttyS1
  -> app_device.c: read_pthread_func()
  -> app_bt.c: app_bt_postRead()
  -> up_buffer
  -> 线程池 send_msg_func()
  -> app_message_chars2Json()
  -> app_mqtt_send()
  -> MQTT push
  -> 网页订阅回调 client.on("message")
```

Linux 串口读取的入口在 [app_device.c](app/app_device.c)：

```c
int size = read(device->fd, data_buffer, 125);
```

只有蓝牙模块真的从 TXD 向网关 UART 发出字节，`read()` 才会返回大于 0 的长度，之后才可能进入 `up_buffer` 和 MQTT `push`。

> 上行线程已经启动，不代表它会自动产生上行消息。上行的来源必须是下位机主动上报、对命令的响应，或其他从无线模块到达网关串口的数据。

### 网关期望的上行蓝牙接收帧

[app_bt.c](app/app_bt.c) 中的 `app_bt_postRead()` 期望本地蓝牙模块向 Linux 串口输出如下形式的无线接收帧：

```text
F1 DD <len> <source MADDR:2> <destination MADDR:2> <payload...>
```

例如下位机 `0088` 向网关 `0101` 上报 ASCII 温度 `25` 时，接收帧可表示为：

```text
F1 DD 07 00 88 01 01 32 35
```

网关解析后转换成内部格式：

```text
01 02 02 00 88 32 35
```

最终发布到 `push` 的 JSON 为：

```json
{
  "conn_type": 1,
  "id": "0088",
  "msg": "3235"
}
```

网页中的 `hexToString("3235")` 会得到字符串 `25`，因此可显示为温度值。

## 7. STM32 接收开/关灯命令的字节位置

STM32 参考工程中使用 `usart3_data[7]` 判断 `'1'` / `'0'`。这**不应直接理解为** STM32 正在解析 Linux 写出的原始 `AT+MESH` 命令。

如果 STM32 侧蓝牙模块给 USART3 输出的是与网关 `app_bt_postRead()` 一致的无线接收帧：

```text
F1 DD <len> <source:2> <destination:2> <payload...>
```

那么布局为：

```text
下标:  0  1   2      3   4      5   6       7
数据: F1 DD <len> source[0] source[1] dest[0] dest[1] payload[0]
```

此时：

```text
usart3_data[7] == '1'  -> 开灯
usart3_data[7] == '0'  -> 关灯
```

这个字节位置是合理的。

但是 STM32 当前逻辑仍应在后续改进：在访问固定下标前先验证接收长度、`F1 DD` 帧头、声明长度、地址字段和 payload 边界。由于当前没有 STM32 蓝牙模块，必须在后续真机上抓取 STM32 USART3 原始十六进制数据，确认模块实际输出帧格式后再修改协议解析代码。

## 8. 当前测试状态与建议

### 当前已验证

- 网关能够连接 MQTT Broker 并订阅 `pull`；
- 网页向 `pull` 发布命令后，网关能进入 MQTT 下行回调；
- JSON 能被转换为 6 字节内部数据；
- 数据能写入和读取 `down_buffer`；
- 蓝牙预处理可将 6 字节转换为 13 字节；
- Linux 可成功调用 `write(/dev/ttyS1, ..., 13)`，将下行命令交给本地蓝牙模块串口。
- **OTA 模块**：版本比较逻辑已实现并修复（支持 major.minor.patch 格式的版本号比较）。

### 目前无法验证

由于没有 STM32 一侧蓝牙模块，以下内容尚未完成真机验证：

- 两个模块之间是否已建立无线 Mesh 通信；
- STM32 是否收到并解析开/关灯命令；
- STM32 温度是否通过无线链路回传；
- 网关 `read(/dev/ttyS1)` 是否能收到 `F1 DD ...` 上行帧；
- 网关是否能将真实温度发布至 `push` 并由网页显示。

### 硬件到位后的验证顺序

1. 确认两侧 `NETID=1234`，网关 `MADDR=0101`，下位机 `MADDR=0088`；
2. 确认两侧 UART 接线 TX/RX 交叉、GND 共地、波特率正确；
3. 启动网关，确认出现 `MQTT initialized successfully !` 和 `蓝牙配置成功`；
4. 网页向 `pull` 发开灯/关灯命令，确认网关出现 `写入串口文件成功, len=13`；
5. 通过 STM32 USART1 调试输出或逻辑分析仪，抓取 USART3 收到的原始十六进制数据；
6. 让 STM32 周期性发送温度，查看网关是否出现“串口上行原始数据到达”；
7. 确认网关随后出现 `MQTT send successfully !`、`send message success`；
8. 确认网页订阅到 `push` 并显示温度。

## 9. 常见排查项

| 现象 | 优先检查 |
| --- | --- |
| 网页点击后没有 MQTT 下行日志 | Broker 地址、网页 WebSocket 地址、`pull` 主题、网关 Client ID 是否重复 |
| 有 MQTT 下行日志但没有串口写成功日志 | JSON 格式、`down_buffer`、线程池、`pre_write` 返回值 |
| 串口写成功但 STM32 不动作 | 两侧 NETID、目标 MADDR `0088`、模块是否处于 Mesh 工作模式、STM32 UART 接线和波特率、STM32 实际收到的帧格式 |
| 没有“串口上行原始数据到达” | 下位机未上报、STM32 侧蓝牙模块未连接、无线 Mesh 未通、网关 RX/TX/GND 或 `/dev/ttyS1` 配置错误 |
| 有原始上行但没有 MQTT `push` | `F1 DD` 帧格式/长度不匹配，或 `app_bt_postRead()` 未成功解析 |
| 网页订阅 `push` 但没有显示温度 | 检查 MQTT 主题、JSON 格式、`msg` 是否为十六进制字符串，以及网页 `hexToString()` 的处理 |

## 10. 当前实现注意事项

以下是当前代码的实现边界，后续完善时建议优先处理：

- MQTT Broker、Client ID、主题、串口设备、NETID 和 MADDR 都是硬编码；
- MQTT 暂无 TLS、用户名密码配置；连接丢失后会按退避间隔重连；
- MQTT 回调直接将 `message->payload` 当作 C 字符串使用，实际应按 `payloadlen` 复制并补 `\0` 后再解析 JSON；
- JSON 解析、字段存在性和十六进制字符串合法性校验不足；
- STM32 接收端固定读取 `usart3_data[7]`，应补充完整帧校验；
- 真实蓝牙收发帧格式需要在 STM32 侧与网关侧分别抓包确认，不能只依靠注释推断。

## 11. 关键源码位置

| 功能 | 文件 |
| --- | --- |
| 程序入口 | [main.c](main.c) |
| 启动、串口设备路径 | [app/app_runner.c](app/app_runner.c)、[app/app_runner.h](app/app_runner.h) |
| 上行/下行业务流程 | [app/app_device.c](app/app_device.c) |
| 蓝牙配置、封包、解析 | [app/app_bt.c](app/app_bt.c) |
| MQTT 连接、订阅与发布 | [app/app_mqtt.c](app/app_mqtt.c) |
| JSON 与内部字节协议转换 | [app/app_message.c](app/app_message.c) |
| 双缓冲读写 | [app/app_buffer.c](app/app_buffer.c) |
| OTA 固件升级、版本比较 | [ota/ota_version.c](ota/ota_version.c) |
