#ifndef TODODELEGATE_H
#define TODODELEGATE_H

#include <QStyledItemDelegate>

/*
 * ======================================================================
 * TodoDelegate.h — 自定义委托声明
 *
 * QStyledItemDelegate 是 Qt Model/View 架构中的"表现层"，负责控制
 * 每个单元格的：
 *   ① 绘制外观（paint）         → 优先级圆点、日期颜色预警
 *   ② 编辑控件（createEditor）  → 优先级用下拉框、日期用日历弹窗
 *   ③ 编辑器与模型的数据交互    → setEditorData / setModelData
 *   ④ 行高（sizeHint）          → 确保 38px 最小高度
 *
 * 本委托对 4 列做了差异化的定制：
 *   - 标题列：沿用 QStyledItemDelegate 默认的 QLineEdit 编辑器
 *   - 优先级列：QComboBox 下拉选择 + 彩色圆点绘制
 *   - 截止日期列：QDateEdit 日历弹窗 + 时间预警色
 *   - 完成列：标准复选框（Qt 默认绘制，无需额外定制）
 * ======================================================================
 */

/*
 * TodoDelegate — 自定义单元格委托
 *
 * 使用 "using QStyledItemDelegate::QStyledItemDelegate" 继承基类
 * 的构造函数，因为本类没有自己的成员变量需要初始化，手写构造函数
 * 只是重复基类的签名，用继承构造函数更简洁。
 */
class TodoDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    // 继承 QStyledItemDelegate 的构造函数，等价于手写：
    //   TodoDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}
    using QStyledItemDelegate::QStyledItemDelegate;

    /*
     * 自定义单元格绘制函数
     * 每列的绘制逻辑：
     *   - 优先级列：彩色圆点（14px 直径）+ "低/中/高" 文字
     *   - 截止日期列：根据距离今天的远近选择文字颜色
     *   - 完成列：直接调用基类绘制标准复选框
     *   - 标题列：普通文本
     */
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;

    /*
     * 行高提示函数
     * 确保每行至少 38px 高，容纳优先级列的色标圆点和充足间距。
     */
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;

    /*
     * 创建单元格编辑器控件
     * 按列类型返回不同的编辑器：
     *   优先级列 → QComboBox（下拉选择"低/中/高"）
     *   截止日期列 → QDateEdit（日历弹窗，yyyy-MM-dd 格式）
     *   其他列（标题） → QLineEdit（由基类负责创建）
     */
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;

    /*
     * 从模型读取数据，填充到编辑器控件中
     * 按列类型从对应的自定义 role 中读取原始数据。
     */
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;

    /*
     * 编辑器编辑完成后，将数据写回模型
     * 按列类型同时写入 DisplayRole（显示文本）和自定义 role（排序用原始数据）。
     */
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const override;
};

#endif // TODODELEGATE_H
