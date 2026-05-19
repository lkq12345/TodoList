#include "Widget.h"
#include <QApplication>
#include <QDebug>
#include <cstdio>

/*
 * main.cpp — 应用程序入口
 *
 * 1. 创建 QApplication 对象（管理全局资源）
 * 2. 构造主窗口 Widget
 * 3. 显示窗口并进入事件循环
 */

// 自定义消息处理器：过滤 libpng iCCP 刷屏警告
static void messageHandler(QtMsgType type, const QMessageLogContext &ctx, const QString &msg)
{
    Q_UNUSED(type);
    Q_UNUSED(ctx);

    // 跳过 libpng 的 iCCP 色彩配置警告（Qt 内置图标引起，不影响功能）
    if (msg.contains("libpng warning: iCCP"))
        return;
    // 其余消息由 Qt 默认方式处理
    fprintf(stderr, "%s\n", qPrintable(msg));
}

int main(int argc, char *argv[])
{
    qInstallMessageHandler(messageHandler);

    QApplication a(argc, argv);
    Widget w;
    w.show();
    return a.exec();
}
