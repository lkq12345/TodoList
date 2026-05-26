#include "TodoDelegate.h"
#include "TodoData.h"

#include <QPainter>
#include <QComboBox>
#include <QDateEdit>
#include <QApplication>

/*
 * ======================================================================
 * TodoDelegate.cpp — 自定义委托实现
 *
 * 这个文件控制表格中每个单元格的"长相"和"编辑方式"。
 *
 * 绘制逻辑（paint）：
 *   表格有 4 列，每列的绘制方式完全不同：
 *   - 优先级列：不直接显示"0/1/2"或文字，而是用"彩色圆点 + 文字"
 *   - 截止日期列：按紧迫程度改变文字颜色，起到预警作用
 *   - 完成列：标准复选框，直接交给基类绘制
 *   - 标题列：纯文本
 *
 * 编辑器逻辑（createEditor / setEditorData / setModelData）：
 *   双击编辑时，3 列各有不同的编辑器：
 *   - 优先级：QComboBox 下拉选择，避免用户输入无效值
 *   - 截止日期：QDateEdit 带日历弹窗，保证日期格式正确
 *   - 标题：标准的 QLineEdit
 * ======================================================================
 */


// ══════════════════════════════════════════════════════════════════════
// 自定义单元格绘制
// ══════════════════════════════════════════════════════════════════════

void TodoDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                         const QModelIndex &index) const
{
    /*
     * 第一步：初始化样式选项
     *
     * QStyleOptionViewItem 包含了绘制需要的所有信息：
     *   - rect: 单元格的位置和大小（由 QTreeView 的布局引擎计算）
     *   - palette: 系统调色板（字体颜色、高亮色等，跟随系统主题）
     *   - state: 单元格状态（选中、悬停、禁用等，用位掩码表示）
     *   - font: 当前使用的字体
     *
     * initStyleOption() 将模型数据（DisplayRole、CheckStateRole 等）
     * 填充到 opt 中，确保默认绘制行为能正常工作。
     */
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    // 保存画笔的当前状态，最后 restore() 还原，避免影响其他绘制操作
    painter->save();

    // ---- 第二步：绘制背景 ----

    /*
     * opt.state 是一个位掩码（QFlags<QStyle::StateFlag>），用按位与
     * 测试特定位是否置位。
     *
     * 优先级：选中 > 悬停 > 无状态
     */
    if (opt.state & QStyle::State_Selected) {
        // 选中行 → 使用系统高亮色填充整行底色（通常是蓝色半透明）
        painter->fillRect(opt.rect, opt.palette.highlight());
    } else if (opt.state & QStyle::State_MouseOver) {
        // 鼠标悬停 → 使用交替行底色（浅灰色），提供视觉反馈
        painter->fillRect(opt.rect, opt.palette.alternateBase());
    }

    // ---- 第三步：裁出绘制边距 ----
    /*
     * adjusted(left, top, right, bottom) 从四个方向向内缩进。
     * 避免文字紧贴单元格边框，留出呼吸空间。
     */
    QRect contentRect = opt.rect.adjusted(4, 2, -4, -2);

    // ---- 第四步：按列类型分发绘制 ----

    // 4.1 优先级列：彩色圆点 + "低/中/高" 文字
    if (index.column() == ColPriority) {
        /*
         * 从模型中读取优先级值（int: 0=低, 1=中, 2=高）
         * 注意这里用的是 PriorityRole（自定义角色），而非 DisplayRole，
         * 因为我们存的是整数值（用于排序），DisplayRole 存的是中文文字。
         */
        int priority = index.data(PriorityRole).toInt();
        QStringList labels = {"低", "中", "高"};

        // ---------- 画圆点 ----------
        int dotDiameter = 14;                        // 圆点直径 14px
        int dotX = contentRect.left();               // 紧贴左侧绘制
        int dotY = contentRect.center().y() - dotDiameter / 2;  // 垂直居中
        QRect circleRect(dotX, dotY, dotDiameter, dotDiameter);

        // 设置填充色（红/橙/绿），去掉边框
        painter->setBrush(priorityColor(priority));
        painter->setPen(Qt::NoPen);

        // 开启抗锯齿——否则圆形边缘会出现锯齿状像素
        painter->setRenderHint(QPainter::Antialiasing, true);

        // 在正方形区域内画内切椭圆（正方形边长 = 椭圆直径 = 正圆）
        painter->drawEllipse(circleRect);

        // ---------- 画文字 ----------
        /*
         * 文字区域：圆点右侧留出 22px（14px 圆点 + 8px 间距）后开始
         * AlignVCenter | AlignLeft：垂直居中，左对齐
         */
        QRect textRect = contentRect.adjusted(dotDiameter + 8, 0, 0, 0);
        painter->setPen(opt.palette.text().color());
        painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft,
                          labels.value(priority, "低"));
    }

    // 4.2 截止日期列：按紧迫程度着色
    else if (index.column() == ColDeadline) {
        /*
         * 从 DeadlineRole 中取出 QDate 对象，与当前日期比较。
         * 四级颜色预警体系：
         *   已过期 → 红（最紧迫）
         *   今天到期 → 橙（提醒）
         *   3 天内到期 → 浅橙（注意）
         *   还有时间 → 默认色（不干扰）
         */
        QDate deadline = index.data(DeadlineRole).toDate();
        QDate today = QDate::currentDate();

        if (deadline.isValid()) {
            QColor textColor;
            if (deadline < today) {
                // 已过期：醒目红色，让用户一眼看到
                textColor = QColor(200, 40, 40);
            } else if (deadline == today) {
                // 今天截止：橙色，中度提醒
                textColor = QColor(220, 140, 20);
            } else if (deadline <= today.addDays(3)) {
                // 3 天内到期：浅橙色，轻度提醒
                textColor = QColor(210, 170, 40);
            } else {
                // 时间充裕：使用系统默认文字颜色，不增加视觉噪音
                textColor = opt.palette.text().color();
            }
            painter->setPen(textColor);
        } else {
            // 无截止日期：使用默认色
            painter->setPen(opt.palette.text().color());
        }

        // 绘制日期文本（格式：yyyy-MM-dd，无效日期显示空字符串）
        QString dateText = deadline.isValid()
                           ? deadline.toString("yyyy-MM-dd")
                           : QString();
        painter->drawText(contentRect, Qt::AlignVCenter | Qt::AlignLeft, dateText);
    }

    // 4.3 完成列：标准复选框
    else if (index.column() == ColCompleted) {
        /*
         * 复选框的绘制交给 QStyledItemDelegate::paint() 处理，
         * initStyleOption() 已经从模型读取了 CheckStateRole，
         * 基类会自动画出勾选/未勾选状态的复选框图标。
         *
         * 注意：在这里必须先 restore() 再调用基类，因为基类会
         * 自己 save()/restore()，如果先 restore() 再 return，
         * 外层的 painter->restore() 就不会执行了。
         */
        painter->restore();
        QStyledItemDelegate::paint(painter, opt, index);
        return;
    }

    // 4.4 标题列：普通文字
    else {
        painter->setPen(opt.palette.text().color());
        painter->drawText(contentRect, Qt::AlignVCenter | Qt::AlignLeft,
                          index.data(Qt::DisplayRole).toString());
    }

    // 恢复画笔到 save() 之前的状态
    painter->restore();
}


// ══════════════════════════════════════════════════════════════════════
// 行高提示
// ══════════════════════════════════════════════════════════════════════

QSize TodoDelegate::sizeHint(const QStyleOptionViewItem &option,
                             const QModelIndex &index) const
{
    /*
     * 先让基类根据字体和内容算出一个"合理的高度"，然后确保最小值
     * 不低于 38px。38px 的由来：
     *   - 优先级圆点直径 14px
     *   - 上下边距各 2px
     *   - 再留出一些呼吸空间 → 总共约 38px
     *
     * 注意：QTreeView 的行高是整行统一的，sizeHint 会对每列都调用
     * 一次，但实际行高取的是所有列中的最大值。所以虽然只有优先级列
     * 需要更多空间，整行都会被垫高到 38px。
     */
    QSize defaultSize = QStyledItemDelegate::sizeHint(option, index);
    defaultSize.setHeight(qMax(defaultSize.height(), 38));
    return defaultSize;
}


// ══════════════════════════════════════════════════════════════════════
// 编辑器创建
// ══════════════════════════════════════════════════════════════════════

QWidget *TodoDelegate::createEditor(QWidget *parent,
                                    const QStyleOptionViewItem &option,
                                    const QModelIndex &index) const
{
    // 优先级列 → QComboBox 下拉选择器
    if (index.column() == ColPriority) {
        /*
         * 下拉框提供固定的 3 个选项，用户只能从中选择，
         * 不会输入无效值。选项文字用中文"低/中/高"，
         * 下标恰好对应 Priority 枚举值（0/1/2）。
         */
        auto *comboBox = new QComboBox(parent);
        comboBox->addItems({"低", "中", "高"});
        return comboBox;
    }

    // 截止日期列 → QDateEdit 日期编辑器（带日历弹窗）
    if (index.column() == ColDeadline) {
        /*
         * QDateEdit 配置：
         *   - calendarPopup(true)：点击下拉箭头弹出日历面板
         *   - displayFormat("yyyy-MM-dd")：显示格式，固定宽度 10 字符
         */
        auto *dateEdit = new QDateEdit(parent);
        dateEdit->setCalendarPopup(true);
        dateEdit->setDisplayFormat("yyyy-MM-dd");
        return dateEdit;
    }

    // 标题列 → 由基类创建默认的 QLineEdit
    return QStyledItemDelegate::createEditor(parent, option, index);
}


// ══════════════════════════════════════════════════════════════════════
// 从模型读取数据 → 填充到编辑器
// ══════════════════════════════════════════════════════════════════════

void TodoDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    /*
     * setEditorData 在 createEditor 之后被调用，负责把模型中的数据
     * 回填到刚创建的编辑器控件中，让用户看到当前值。
     */

    // 优先级列：从 PriorityRole 读出整数值，设置下拉框的当前选择
    if (index.column() == ColPriority) {
        auto *comboBox = static_cast<QComboBox *>(editor);
        comboBox->setCurrentIndex(index.data(PriorityRole).toInt());
    }
    // 截止日期列：从 DeadlineRole 读出 QDate，设置到日期编辑器
    else if (index.column() == ColDeadline) {
        auto *dateEdit = static_cast<QDateEdit *>(editor);
        dateEdit->setDate(index.data(DeadlineRole).toDate());
    }
    // 其他列：基类默认行为（从 DisplayRole 读取字符串填充）
    else {
        QStyledItemDelegate::setEditorData(editor, index);
    }
}


// ══════════════════════════════════════════════════════════════════════
// 编辑器确认 → 将数据写回模型
// ══════════════════════════════════════════════════════════════════════

void TodoDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                const QModelIndex &index) const
{
    /*
     * 用户编辑完成后（按 Enter 或点击其他单元格），Qt 调用此函数
     * 将编辑器的值写回模型。
     *
     * 关键设计：同时写入两个角色
     *   - DisplayRole（文本）：用于表格显示
     *   - 自定义 Role（原始值）：用于排序
     * 这样无论显示还是排序，都能拿到正确数据。
     */

    // 优先级列：同时写入文字和整数值
    if (index.column() == ColPriority) {
        int priority = static_cast<QComboBox *>(editor)->currentIndex();
        QStringList labels = {"低", "中", "高"};

        // DisplayRole 存入中文文字（给用户看）
        model->setData(index, labels.value(priority), Qt::DisplayRole);

        // PriorityRole 存入整数值（给 lessThan 排序用）
        model->setData(index, priority, PriorityRole);
    }
    // 截止日期列：同时写入字符串和 QDate
    else if (index.column() == ColDeadline) {
        QDate date = static_cast<QDateEdit *>(editor)->date();

        // DisplayRole 存入格式化字符串（给用户看）
        model->setData(index, date.toString("yyyy-MM-dd"), Qt::DisplayRole);

        // DeadlineRole 存入 QDate 对象（给 lessThan 排序用）
        model->setData(index, date, DeadlineRole);
    }
    // 其他列：基类默认行为

    else {
        QStyledItemDelegate::setModelData(editor, model, index);
    }
}
