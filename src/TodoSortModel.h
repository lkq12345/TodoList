#ifndef TODOSORTMODEL_H
#define TODOSORTMODEL_H

#include <QSortFilterProxyModel>

/* ===================================================================
 * todosortmodel.h — 排序代理模型
 *
 * QSortFilterProxyModel 架在源模型（QStandardItemModel）与视图之间，
 * 在不修改源数据的前提下提供排序能力。
 *
 * 本子类重写 lessThan()，使每一列按照其语义类型（int / QDate / bool）
 * 而非字符串比较，从而得到正确的排序结果。
 * =================================================================== */

class TodoSortModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit TodoSortModel(QObject *parent = nullptr);
    // 使用 QSortFilterProxyModel 的析构函数

protected:
    // 自定义两行之间的比较逻辑
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
};

#endif // TODOSORTMODEL_H
