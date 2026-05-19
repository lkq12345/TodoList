#ifndef TODODELEGATE_H
#define TODODELEGATE_H

#include <QStyledItemDelegate>

/* ===================================================================
 * tododelegate.h — 自定义委托
 *
 * QStyledItemDelegate 负责在 Model/View 架构中控制每个单元格的
 *   ① 绘制外观（paint）
 *   ② 编辑控件（createEditor / setEditorData / setModelData）
 *   ③ 行高（sizeHint）
 *
 * 本委托针对「优先级」「截止日期」「完成」三列做了定制化，
 * 标题列沿用默认的 QLineEdit 编辑器。
 * =================================================================== */

class TodoDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    using QStyledItemDelegate::QStyledItemDelegate;

    // ---- 自定义单元格绘制 ----
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;

    // ---- 行高提示 ----
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;

    // ---- 编辑器创建 ----
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;

    // ---- 从模型读出数据填充到编辑器 ----
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;

    // ---- 编辑器确认后将数据写回模型 ----
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const override;
};

#endif // TODODELEGATE_H
