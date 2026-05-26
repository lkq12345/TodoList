#include "Widget.h"
#include <QApplication>
#include <QDebug>
#include <cstdio>

/*
 * ======================================================================
 * main.cpp — 应用程序入口
 *
 * 程序启动流程：
 *   1. qInstallMessageHandler 安装自定义日志处理器，过滤掉 Qt 内置
 *      图标引起的 libpng iCCP 刷屏警告（不影响功能但污染控制台）
 *   2. 创建 QApplication 对象，管理应用程序全局资源（字体、样式等）
 *   3. 构造主窗口 Widget（同时也是 TodoSortModel、TodoDelegate 等
 *      所有核心对象的创建时机）
 *   4. 显示窗口并进入 Qt 事件循环（等待用户操作）
 * ======================================================================
 */

/*
 * 自定义 Qt 消息处理器
 *
 * Qt 的日志系统通过 qInstallMessageHandler 安装的处理函数来分发
 * qDebug() / qWarning() / qCritical() 的输出。默认行为是全部打印
 * 到 stderr。
 *
 * 本项目只需要过滤一类噪声：
 *   "libpng warning: iCCP" —— 这是 Qt 5 内置的某些图标 PNG 文件
 *   包含了 iCCP 色彩配置信息，libpng 会发出警告。该警告完全无害，
 *   但每次鼠标悬浮/点击都会触发，造成大量刷屏。
 */
static void messageHandler(QtMsgType type, const QMessageLogContext &ctx,
                           const QString &msg)
{
    // 抑制参数未使用的编译器警告（这些参数在过滤器场景下不需要）
    Q_UNUSED(type);
    Q_UNUSED(ctx);

    // 过滤 libpng 的 iCCP 色彩配置警告
    if (msg.contains("libpng warning: iCCP"))
        return;

    // 其他消息（包括真正的错误、我们自己输出的 qDebug 等）正常打印
    fprintf(stderr, "%s\n", qPrintable(msg));
}

int main(int argc, char *argv[])
{
    // 安装自定义消息处理器（必须在 QApplication 创建之前调用）
    qInstallMessageHandler(messageHandler);

    // 创建应用程序对象（管理事件循环、样式、字体等全局资源）
    QApplication app(argc, argv);

    // 构造主窗口（内部会完成模型、视图、委托、网络加载器的全部初始化）
    Widget mainWindow;
    mainWindow.show();

    // 进入事件循环 —— 此后程序由用户操作和信号驱动，不会再返回
    return app.exec();
}
