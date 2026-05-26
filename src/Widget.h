#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include "TodoSortModel.h"
#include <QJsonArray>

/*
 * ======================================================================
 * Widget.h — 主窗口声明
 *
 * 布局结构（由 Widget.ui 定义）：
 *   第一层 → 工具栏（固定高度 40px）
 *             包含：添加、删除、从网络加载、排序控件、状态标签
 *   第二层 → QProgressBar（默认隐藏，加载时显示）
 *   第三层 → QTreeView（Expanding 策略，占满剩余空间）
 *
 * 类职责：
 *   作为整个应用的"控制器"，持有模型、视图、委托、网络加载器，
 *   负责协调它们之间的信号/槽连接和操作流程。
 *
 * 界面元素的创建和布局由 Widget.ui 文件负责，通过 setupUi() 构建。
 * ======================================================================
 */

// 前置声明
class TodoDelegate;
class NetworkLoader;

namespace Ui {
    class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = nullptr);
    ~Widget();

private slots:
    void addItem();
    void deleteItem();
    void toggleSortOrder();
    void onLoadFromNetwork();
    void onNetworkDataReady(const QJsonArray &data);
    void onNetworkError(const QString &message);

private:
    void updateSortInfo();

    // ---- 界面（由 Widget.ui / setupUi 创建，通过 ui-> 访问） ----
    Ui::Widget *ui;

    // ---- 模型 / 排序代理 / 委托（代码中手动创建） ----
    QStandardItemModel   *m_model;       // 源模型（4 列 n 行的数据存储）
    TodoSortModel        *m_proxy;       // 排序代理模型（架在源模型和视图之间）
    TodoDelegate         *m_delegate;    // 自定义委托（控制绘制和编辑器）

    // ---- 网络加载 ----
    NetworkLoader *m_networkLoader;      // HTTP 网络请求封装
};

#endif // WIDGET_H
