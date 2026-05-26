#ifndef TODOSORTMODEL_H
#define TODOSORTMODEL_H

#include <QSortFilterProxyModel>

/*
 * ======================================================================
 * TodoSortModel.h — 排序代理模型声明
 *
 * QSortFilterProxyModel 是 Qt Model/View 架构中的一个"代理层"，它
 * 架在源模型（QStandardItemModel）与视图（QTreeView）之间，可以在
 * 不修改源数据的前提下提供排序和过滤能力。
 *
 * 为什么需要重写 lessThan()？
 *   QSortFilterProxyModel 默认的 lessThan() 直接用 DisplayRole 做
 *   字符串比较，这在纯文本列（标题）没问题，但在以下场景会出错：
 *
 *   1. 优先级列："中" < "低" < "高"（字典序）
 *      但业务期望：低 < 中 < 高（按数值 0 < 1 < 2）
 *
 *   2. 截止日期列：日期格式依赖区域设置，字符串比较不够严谨
 *
 *   3. 完成状态列："true" vs "false" 的字符串比较不符合"未完成在前"
 *
 *   因此本子类为每列指定正确的数据类型进行比较。
 * ======================================================================
 */

class TodoSortModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    /*
     * 构造函数直接转发给基类，无额外初始化逻辑。
     * explicit 防止隐式类型转换：TodoSortModel obj = somePointer; 这种写法
     * 虽然不常见，但 explicit 是一个好习惯。
     */
    explicit TodoSortModel(QObject *parent = nullptr);

protected:
    /*
     * 自定义两行之间的比较逻辑
     *
     * 返回 true 表示 left 应该排在 right 前面（升序）。
     * 注意：降序时 Qt 内部会反转结果，不需要在这里处理。
     *
     * @param left  左侧行的单元格索引
     * @param right 右侧行的单元格索引
     * @return left < right 是否成立
     */
    bool lessThan(const QModelIndex &left,
                  const QModelIndex &right) const override;
};

#endif // TODOSORTMODEL_H
