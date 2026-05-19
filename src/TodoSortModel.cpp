#include "TodoSortModel.h"
#include "TodoData.h"

#include <QDate>

/* ===================================================================
 * todosortmodel.cpp — 按语义类型排序的实现
 *
 * 默认的 lessThan 直接用 DisplayRole 做字符串比较，
 * 会导致优先级 "中" < "低" 这类错误。
 * 这里为每列指定正确的数据类型进行比较。
 * =================================================================== */

TodoSortModel::TodoSortModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
}

bool TodoSortModel::lessThan(const QModelIndex &left,
                             const QModelIndex &right) const
{
    int col = left.column();

    // ---- 优先级：按 int 数值比较（低=0 < 中=1 < 高=2） ----
    if (col == ColPriority) {
        int l = left.data(PriorityRole).toInt();
        int r = right.data(PriorityRole).toInt();
        return l < r;
    }

    // ---- 截止日期：按 QDate 先后比较 ----
    if (col == ColDeadline) {
        QDate l = left.data(DeadlineRole).toDate();
        QDate r = right.data(DeadlineRole).toDate();
        return l < r;
    }

    // ---- 完成状态：未完成排在已完成前面 ----
    if (col == ColCompleted) {
        bool l = (left.data(Qt::CheckStateRole).toInt() == Qt::Checked);
        bool r = (right.data(Qt::CheckStateRole).toInt() == Qt::Checked);
        return l < r;
    }

    // ---- 标题列：使用默认的字符串比较 ----
    return QSortFilterProxyModel::lessThan(left, right);
}
