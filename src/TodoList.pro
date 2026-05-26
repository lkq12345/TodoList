# ======================================================================
# TodoList.pro — qmake 项目文件
#
# 项目名称：TodoList（待办事项桌面应用）
# 框架版本：Qt 5.12.6
# 构建工具：qmake + MinGW 7.3.0 64-bit
# C++ 标准：C++11
#
# 依赖模块：
#   core     — Qt 核心模块（QObject、事件循环等）
#   gui      — 图形界面基础（QPainter、QColor 等）
#   widgets  — 桌面控件（QWidget、QTreeView、QPushButton 等）
#   network  — 网络模块（QNetworkAccessManager、QNetworkReply 等）
# ======================================================================

QT       += core gui widgets network

CONFIG += c++11

# 目标可执行文件的名称
TARGET = TodoList

# 输出目录：项目根目录下的 bin/ 文件夹
# $$PWD 是当前 .pro 文件所在目录（src/），../bin 即项目根目录的 bin/
DESTDIR = $$PWD/../bin

# 启用 Qt 废弃 API 的编译时警告，帮助发现使用了过时函数的地方
DEFINES += QT_DEPRECATED_WARNINGS

# ---- 源文件清单 ----
# 列出所有需要编译的 .cpp 文件
SOURCES += \
    main.cpp \
    Widget.cpp \
    TodoDelegate.cpp \
    TodoSortModel.cpp \
    NetworkLoader.cpp

# ---- 头文件清单 ----
# 列出所有头文件（qmake 通过它们生成 MOC 文件以支持 Q_OBJECT 宏）
HEADERS += \
    Widget.h \
    TodoData.h \
    TodoDelegate.h \
    TodoSortModel.h \
    NetworkLoader.h

# ---- UI 表单文件 ----
# Widget.ui 定义了工具栏、进度条、表格视图的布局，
# qmake 会自动运行 uic 生成对应的 ui_Widget.h 头文件。
FORMS += \
    Widget.ui
