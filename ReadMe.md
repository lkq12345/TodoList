# TodoList

一个基于 **Qt 5.12.6 / C++11** 的待办事项桌面应用，采用经典的 **Model-View-Delegate（MVC）** 架构实现。

---

## 功能一览

### 1. 四列表格展示

| 列 | 说明 |
|---|---|
| 标题 | 任务名称，纯文本 |
| 优先级 | 低 / 中 / 高，带彩色圆点标识（绿 / 橙 / 红） |
| 截止日期 | yyyy-MM-dd 格式，根据紧迫程度自动着色 |
| 完成 | 复选框，勾选标记任务完成 |

### 2. 添加任务

点击"添加"按钮弹出模态对话框，收集以下信息：

- **标题** — 文本输入，提交时校验非空
- **优先级** — 下拉选择（低 / 中 / 高）
- **截止日期** — 日期选择器，支持日历弹窗

### 3. 删除任务

选中一行后点击"删除"，从模型中移除该行。

### 4. 行内编辑

双击任意单元格进入编辑模式：

- **标题列** — `QLineEdit` 文本编辑框
- **优先级列** — `QComboBox` 下拉选择
- **截止日期列** — `QDateEdit` 日历弹窗
- **完成列** — 直接点击复选框切换

### 5. 类型感知排序

通过下拉框选择排序列，按钮切换升序 / 降序。排序逻辑覆盖默认的字符串比较，按各列语义类型排序：

- **优先级** — 按数值比较（低=0 < 中=1 < 高=2）
- **截止日期** — 按 `QDate` 先后排序
- **完成状态** — 未完成排前面
- **标题** — 按字符串排序

### 6. 截止日期预警色

- 已过期 → **红色**
- 今天截止 → **橙色**
- 三天内到期 → **浅橙色**
- 时间充裕 → 默认色

### 7. 从网络加载任务

点击"从网络加载"按钮，异步请求 JSONPlaceholder API 获取远程待办数据：

- 请求期间显示不确定模式进度条动画
- 按钮置灰防止重复点击
- 10 秒超时控制，超时自动取消并提示
- 错误弹窗显示具体原因（无网络、DNS 解析失败等）
- 远程数据的优先级轮流分配（低/中/高），截止日期统一设为当前日期 + 7 天

---

## 技术要点

### 架构设计

```
┌─────────────────────────────────────────────────────────┐
│                    Widget (主窗口)                        │
│         持有: ui, m_model, m_proxy, m_delegate           │
├───────────┬──────────────┬──────────────┬────────────────┤
│ Ui::Widget│QStandardItem │ TodoSortModel│  QTreeView    │
│ (.ui 布局) │ Model (数据)  │ (排序代理)    │  (可视化)      │
├───────────┴──────────────┴──────────────┴────────────────┤
│                    TodoDelegate                          │
│              (自定义绘制 & 单元格编辑器)                   │
├─────────────────────────────────────────────────────────┤
│                    NetworkLoader                         │
│          QNetworkAccessManager + QTimer 超时控制          │
└─────────────────────────────────────────────────────────┘
```

### 核心模块

| 文件 | 职责 |
|---|---|
| `main.cpp` | 应用程序入口；安装自定义日志处理器，过滤 libpng iCCP 警告 |
| `Widget.h / .cpp` | 主窗口：模型/视图/委托初始化、信号/槽连接、增删操作 |
| `Widget.ui` | UI 布局定义（工具栏、进度条、表格视图），由 Qt Designer 可视化编辑 |
| `TodoData.h` | 纯头文件，集中管理枚举常量、自定义数据角色和工具函数 |
| `TodoDelegate.h / .cpp` | 自定义 `QStyledItemDelegate`，控制单元格绘制和编辑器 |
| `TodoSortModel.h / .cpp` | 排序代理模型，按语义类型重写 `lessThan()` 比较逻辑 |
| `NetworkLoader.h / .cpp` | 网络请求封装（异步 GET、超时控制、JSON 解析） |

### Qt Model-View-Delegate 详解

#### Model（数据模型）— `QStandardItemModel`

- 4 列 n 行的表格结构，作为数据存储中心
- 使用自定义角色 `PriorityRole`（`Qt::UserRole+1`）存储优先级的整数值
- 使用自定义角色 `DeadlineRole`（`Qt::UserRole+2`）存储截止日期的 `QDate` 对象
- 角色分离：同一单元格同时持有 `DisplayRole`（显示用字符串）和自定义角色（排序用原始数据）

#### View（视图）— `QTreeView`

- 配置为扁平列表：`setRootIsDecorated(false)` 禁用展开箭头
- 交替行底色：`setAlternatingRowColors(true)`
- 整行选中 + 单选模式
- 绑定 `TodoSortModel` 作为数据源，而非直接绑定源模型

#### Delegate（委托）— `TodoDelegate`

继承 `QStyledItemDelegate`，重写 5 个虚方法：

- **`paint()`** — 自定义单元格绘制：
  - 优先级列：14px 直径彩色圆点 + 文字标签
  - 截止日期列：按过期 / 今天 / 三天内动态选色
  - 完成列：委托基类绘制标准复选框
- **`sizeHint()`** — 强制最小行高 38px
- **`createEditor()`** — 按列类型创建编辑器（ComboBox / DateEdit / LineEdit）
- **`setEditorData()`** — 从模型读取数据填充编辑器
- **`setModelData()`** — 编辑确认后将数据写回模型（同时写入 DisplayRole + 自定义 Role）

#### Proxy Model（排序代理）— `TodoSortModel`

继承 `QSortFilterProxyModel`，在源模型和视图之间插入一层：

- 不修改源数据，只改变视图呈现顺序
- 关闭动态排序（`setDynamicSortFilter(false)`），用户显式触发排序

### 网络层 — `NetworkLoader`

封装 `QNetworkAccessManager` 的异步 HTTP 请求生命周期：

- 每次请求前自动取消上一次未完成的请求（防止重复点击叠加请求）
- `QTimer` 实现超时控制（默认 5 秒，Widget 调用时覆盖为 10 秒）
- 析构时自动取消请求，避免 use-after-free
- 信号/槽与 Widget 解耦：只发射 `dataReady(QJsonArray)` 和 `errorOccurred(QString)`

### UI 布局 — `Widget.ui`

使用 Qt Designer 定义界面布局，通过 `setupUi()` 在运行时构建：

- 工具栏（QHBoxLayout）：按钮组 → 固定间距 → 排序控件 → 伸缩弹簧
- 进度条：默认隐藏，网络加载时显示不确定模式动画
- 表格视图（QTreeView）：占用剩余全部空间，表头居中、末列自动拉伸

---

## 构建方式

### 环境要求

- Qt 5.12.6+
- 编译器：MinGW 7.3.0 64-bit（与 Qt 配套）
- qmake 构建系统

### 构建步骤

```bash
cd src
qmake TodoList.pro
mingw32-make -j4
```

可执行文件输出到 `bin/TodoList.exe`。

### QMake 配置

```pro
QT       += core gui widgets network
CONFIG   += c++11
TARGET   = TodoList
DESTDIR  = $$PWD/../bin
```

---

## 项目结构

```
TodoList/
├── CLAUDE.md                    # Claude Code 项目指令
├── README.md                    # 项目文档
├── bin/                         # 编译产物
│   └── TodoList.exe
└── src/                         # 源代码
    ├── TodoList.pro             # qmake 项目文件
    ├── main.cpp                 # 入口点
    ├── Widget.h / Widget.cpp    # 主窗口
    ├── Widget.ui                # UI 布局定义（Qt Designer）
    ├── TodoData.h               # 共享常量与枚举
    ├── TodoDelegate.h / .cpp    # 自定义委托
    ├── TodoSortModel.h / .cpp   # 排序代理模型
    └── NetworkLoader.h / .cpp   # 网络请求封装
```

---

## 已知局限

- **MinGW 链接问题** — 在 Git Bash 中编译时需手动加 `-fno-use-linker-plugin` 绕过 LTO 插件缺失问题
- **HTTPS 不支持** — Qt 5.12.6 未捆绑 OpenSSL DLL，网络请求使用 HTTP
- **无持久化** — 数据仅存在于内存中的 `QStandardItemModel`，程序退出后丢失
- **无搜索 / 过滤** — 不支持按关键词或条件筛选任务
- **无分类 / 标签** — 不支持对任务分组管理
- **无数据导出** — 不支持打印或导出为文件
