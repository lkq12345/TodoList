#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QStandardItemModel>
#include <QTreeView>
#include <QSortFilterProxyModel>
#include "TodoSortModel.h"
#include <QPushButton>
#include <QComboBox>
#include <QLabel>

/* ===================================================================
 * Widget.h — 主窗口声明
 *
 * 布局结构（垂直）：
 *   顶部 → 工具栏（固定高度 40px）
 *   中央 → QTreeView（Expanding 策略，伸缩因子 1）
 *
 * 功能按钮：
 *   添加 / 删除 / 排序列选择 / 升降序切换
 * =================================================================== */

class TodoDelegate;

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = nullptr);

private slots:
    void addItem();         // 弹出对话框添加新任务
    void deleteItem();      // 删除当前选中行
    void toggleSortOrder(); // 切换升序 / 降序

private:
    // ---- 工具方法 ----
    void updateSortInfo();      // 更新排序状态标签

    // ---- 模型 / 视图 / 委托 ----
    QTreeView            *m_view;
    QStandardItemModel   *m_model;       // 源模型（4 列）
    QSortFilterProxyModel *m_proxy;      // 排序代理模型
    TodoDelegate         *m_delegate;    // 自定义绘制 & 编辑器

    // ---- 工具栏控件 ----
    QPushButton *m_btnAdd;
    QPushButton *m_btnDelete;
    QComboBox   *m_cmbColumn;   // 选择排序列
    QPushButton *m_btnOrder;    // 升序 / 降序 切换
    QLabel      *m_lblSortInfo; // 显示当前排序状态
};

#endif // WIDGET_H
