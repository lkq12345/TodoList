#include "TodoSortModel.h"
#include "TodoData.h"

#include <QDate>

/*
 * ======================================================================
 * TodoSortModel.cpp — 类型感知排序实现
 *
 * 当用户在界面选择排序列并点击升序/降序时，QTreeView 调用
 * QSortFilterProxyModel::sort()，而 sort() 内部通过不断调用
 * lessThan() 来决定两行的先后顺序。
 *
 * 默认的 lessThan() 用 left.data(Qt::DisplayRole).toString() 比较，
 * 这会导致：
 *   - 优先级列："中" < "低" < "高"（字典序），明显错误
 *   - 截止日期列：虽碰巧正确但不可靠
 *   - 完成状态列：未完成的 "Unchecked" 和已完成的 "Checked" 字符串
 *     比较结果不符合业务需求
 *
 * 本重写版本为每列指定正确的数据类型进行比较。
 *
 * 注意：lessThan 只负责判断"左 < 右 是否成立"，不关心排序方向。
 * 升序/降序由 QSortFilterProxyModel::sort() 的第二个参数控制，
 * Qt 内部在降序时会反转 lessThan 的结果，无需在此处理。
 * ======================================================================
 */

TodoSortModel::TodoSortModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    // 构造函数无需额外初始化，所有配置通过 setter 方法在 Widget 中完成
}

bool TodoSortModel::lessThan(const QModelIndex &left,
                             const QModelIndex &right) const
{
    // 获取当前排序列的索引
    int column = left.column();

    // ---- 优先级列：按整数值比较（低=0 < 中=1 < 高=2） ----
    /*
     * PriorityRole 存储优先级对应的整数值（0/1/2），这个值在写入
     * 模型时由 TodoDelegate::setModelData() 或 Widget::addItem()
     * 负责设置。直接比较整数即可得到正确的排序：
     *   0（低） < 1（中） < 2（高）
     *
     * 如果用 DisplayRole 的字符串"低/中/高"比较，结果是：
     *   "中" < "低" < "高"（汉字 Unicode 编码顺序）
     * 这完全不符合业务语义。
     */
    if (column == ColPriority) {
        int leftVal = left.data(PriorityRole).toInt();
        int rightVal = right.data(PriorityRole).toInt();
        return leftVal < rightVal;
    }

    // ---- 截止日期列：按 QDate 先后比较 ----
    /*
     * DeadlineRole 存储 QDate 对象（而非字符串），QDate::operator<
     * 基于内部儒略日（Julian Day）数值比较，结果绝对准确。
     *
     * 如果用 DisplayRole 的字符串"2026-5-14"比较：
     *   - 碰巧固定宽度 "yyyy-MM-dd" 格式下字符串排序和日期排序一致
     *   - 但如果格式变化（如 "yyyy/M/d"），字符串排序就会出错
     *   - 直接用 QDate 比较更严谨
     */
    else if (column == ColDeadline) {
        QDate leftDate = left.data(DeadlineRole).toDate();
        QDate rightDate = right.data(DeadlineRole).toDate();
        return leftDate < rightDate;
    }

    // ---- 完成状态列：未完成排在已完成前面 ----
    /*
     * 将 CheckStateRole 转成 bool，false < true，所以：
     *   - 未完成（false）排在已完成（true）的前面
     * 这样用户首先看到还未完成的任务。
     *
     * 业务逻辑解释："未完成的比已完成更重要"——这是待办事项应用
     * 的常见需求，把注意力引导到未完成的工作上。
     */
    else if (column == ColCompleted) {
        bool leftChecked = (left.data(Qt::CheckStateRole).toInt() == Qt::Checked);
        bool rightChecked = (right.data(Qt::CheckStateRole).toInt() == Qt::Checked);
        return leftChecked < rightChecked;
        /*
         * C++ 的 bool 比较规则：
         *   false < true  → 未完成排在前面（升序）
         *   如果降序，Qt 取反 → 已完成排在前面
         * 这两种排序都有意义，由用户通过升降序按钮选择。
         */
    }

    // ---- 标题列：使用默认的字符串比较 ----
    /*
     * 标题是纯文本，默认的 DisplayRole 字符串比较完全够用。
     * 调用基类的 lessThan 保留标准行为。
     *
     * 注意：列号相等检查（left.column() == right.column()）由基类
     * 负责，我们不需要在参数上做预处理。
     */
    return QSortFilterProxyModel::lessThan(left, right);
}
