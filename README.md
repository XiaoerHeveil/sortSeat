# 排个座（sortSeat）

一个基于**领域专用语言（DSL）规则引擎**的智能排座位程序，提供 wxWidgets 图形界面。你只需用一组简单、可读的规则（如「张三必须坐在第 3 列第 5 行」「李四和王五当同桌」「成绩高的带成绩差的当同桌」），程序就能自动生成一张满足约束的座位表，并支持导出为 TXT / Excel 文件。

## 功能特性

- **DSL 排座规则引擎**：支持指定座位、相邻/不相邻、同桌、限定行列、男女分组、全局男女排列、条件/分组条件（基于成绩等数值）等多种约束。
- **双进程架构**：GUI 前端（`sorSeatUI`）与业务后端（`sortSeat`）通过 **Windows 命名管道（Named Pipe）** 通信，职责分离、稳定可靠。
- **启动检测与存活监控**：前端拉起后端后自动握手，心跳检测后端存活；后端异常退出时前端会提示并退出。
- **异步日志**：基于 spdlog 的异步日志，GUI 与后端日志统一写入可执行文件旁的 `log/` 目录，按天滚动、保留 30 天自动清理。
- **格式校验**：前端输入自动规范化（全角/半角符号、一行多人按分号拆分等），后端统一校验后再进入计算逻辑。
- **导出**：支持导出 TXT（座位文本）、Excel（`.xlsx`）；导出日志到指定目录。

## 技术栈

- **语言/标准**：C++20
- **构建系统**：CMake（≥3.15）+ vcpkg
- **GUI**：wxWidgets（MSW）
- **Excel**：OpenXLSX
- **日志**：spdlog（异步日志器）
- **IPC**：Windows 命名管道（全双工、message-mode）
- **编译器**：MSVC（Visual Studio，`/utf-8`）

## 目录结构

```
sortSeat/
├── CMakeLists.txt          # 构建脚本（sorSeatCore 静态库 + 两个可执行程序）
├── src/                    # 业务核心代码（与 GUI 共用）
│   ├── sorSeat.cpp         # 后端入口（IPC 服务端 + --test 命令行测试）
│   ├── SeatEngine.*        # 排序引擎：读文件 → 执行规则 → 填充 → 输出
│   ├── sorting.*           # DSL 规则解析与执行
│   ├── student.*           # 学生数据结构
│   ├── fileInput.*         # 文件读取（TXT/CSV/XLSX）
│   ├── Validate.*          # 前端输入格式校验与规范化
│   ├── IpcCommon.h         # IPC 协议定义（操作码/消息帧/字段序列化）
│   ├── NamedPipe.*         # 命名管道封装（服务端/客户端）
│   ├── Log.*               # spdlog 异步日志 + 30 天清理
│   └── exceptions.h        # 自定义异常类型
├── sorSeatUI/              # GUI 前端代码
│   ├── sorSeatApp.*        # wxApp 入口（拉起后端 + 握手 + 心跳）
│   ├── MainFrame.*         # 主窗口（输入/结果/设置/信息页）
│   ├── SeatPanel.*         # 结果座位面板
│   ├── SequenceButton.*    # 序列帧按钮
│   ├── sorButton.*         # 自定义按钮
│   ├── IpcClient.*         # 后端客户端（拉起 sortSeat.exe + 收发消息）
│   └── resource/           # 图片等资源（构建时复制到可执行文件旁）
├── docs/                   # 文档
│   ├── 排座表达式.md         # DSL 规则语法与异常说明
│   └── 项目文档.md           # 本项目文档与注解
└── test/                   # 测试数据（学生名单/规则样例）
```

## 构建

前置条件：

- 已安装 [vcpkg](https://github.com/microsoft/vcpkg)。
- 已通过 vcpkg 安装 `wxWidgets`、`OpenXLSX`、`spdlog`（x64-windows）。

```bash
# 配置（生成 VS 工程）
cmake -S . -B build -A x64

# 编译 Debug
cmake --build build --config Debug

# 编译 Release
cmake --build build --config Release
```

产物：

- `build/Debug/sortSeat.exe` / `build/Release/sortSeat.exe`（后端）
- `build/Debug/sorSeatUI.exe` / `build/Release/sorSeatUI.exe`（前端 GUI）
- 资源文件自动复制到可执行文件旁的 `resource/` 目录。

## 快速开始

1. 双击运行 `sorSeatUI.exe`（或从命令行启动），程序会自动拉起同目录下的 `sortSeat.exe` 后端并完成握手。
2. 在「设置」页手动输入人员名单（`姓名：性别`，每行一人；也可用分号分隔多人），或导入学生名单文件（TXT / CSV / XLSX）。
3. 可选：输入排座规则（DSL），或导入规则文件。
4. 设置座位列数与每组列数，点击「开始」。
5. 在「结果」页查看生成的座位表，可导出 TXT / Excel / 日志。

## 排座 DSL 规则

规则语法详见 [docs/排座表达式.md](docs/排座表达式.md)。支持的函数：

| 函数 | 说明 |
| --- | --- |
| `setSeat("name", x, y)` | 指定某人坐在第 x 列第 y 行 |
| `adjacent("center", radius, "name1", ...)` | 以某人为中心，周围半径内相邻/不相邻 |
| `on("name1", ..., condition1, condition2)` | 限定某人在特定行/列范围 |
| `deskmate("name1", "name2")` | 指定两人当同桌 |
| `setGender(male/female, row/col)` | 指定某行/列全为某性别 |
| `overallSituation(男, 男, ...)` | 全局按性别排列每组各列 |
| `condition("title", greater/less, lead/cooperate, min, max)` | 按数值列条件排序（需 CSV/XLSX） |
| `groupCondition("title", greater/less, scope, base, ...)` | 按数值列条件分组优先排序（需 CSV/XLSX） |

特殊关键字：`not`（取反）、`and`、`or`。

## 标准测试流程

### 1. 后端 CLI 独立测试

```bash
sortSeat --test test/班级.txt test/ABCD.txt 6 4
```

- 参数依次为：学生名单文件、规则文件、座位列数、每组列数。
- 输出座位布局与结果文本（`,` = 同桌相邻，空格 = 空一格，换行 = 另起一行）。

### 2. IPC 端到端联调

1. 启动 `sorSeatUI.exe`，观察后端 `sortSeat.exe` 被自动拉起。
2. 查看可执行文件旁 `log/` 目录，应看到 `sorSeatUI_*.log` 与 `sortSeat_*.log`，且包含「与后端握手成功」「前端已连接」等记录。
3. 输入名单/规则 → 点击「开始」→ 结果页正确渲染座位表。
4. 分别点击导出 TXT / Excel / 日志，验证文件写入指定目录。

### 3. 异常场景

- 手动结束 `sortSeat.exe` 进程 → 前端应弹出「主进程意外退出」并退出。
- 后端无法启动且重启超过 3 次 → 前端应弹出「你的计算机硬件配置过低或此程序与你的计算机不兼容！」。
