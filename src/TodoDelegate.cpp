#include "TodoDelegate.h"
#include "TodoData.h"

#include <QPainter>
#include <QComboBox>
#include <QDateEdit>
#include <QApplication>

/* ===================================================================
 * tododelegate.cpp — 自定义绘制与编辑器实现
 *
 * 绘制功能：
 *   - 优先级列：红/橙/绿 圆点 + 文字
 *   - 截止日期列：过期=红，今天=橙，三天内=浅橙，正常=默认色
 *   - 完成项：文字变灰 + 删除线
 *
 * 编辑器：
 *   - 优先级 → QComboBox 下拉
 *   - 截止日期 → QDateEdit 日历弹窗
 *   - 标题 → 默认 QLineEdit
 * =================================================================== */

// ──────────────────────────────────────────────────── 自定义绘制 ──
void TodoDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                         const QModelIndex &index) const
{
    // 拷贝一份 option 并初始化（填充默认的字体/调色板等属性）
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    painter->save();

    // 选中行 → 高亮底色；悬停行 → 交替底色
    if (opt.state & QStyle::State_Selected)
        painter->fillRect(opt.rect, opt.palette.highlight());
    else if (opt.state & QStyle::State_MouseOver)
        painter->fillRect(opt.rect, opt.palette.alternateBase());

    // 单元格绘制区域（留出 4px 边距）
    QRect r = opt.rect.adjusted(4, 2, -4, -2);

    // ------- 优先级列：色标圆点 + 文字 -------
    if (index.column() == ColPriority) {
        int prio = index.data(PriorityRole).toInt();
        QStringList labels = {"低", "中", "高"};

        // 画一个直径 14px 的实心圆
        int d = 14;
        QRect circle(r.left(), r.center().y() - d / 2, d, d);
        painter->setBrush(priorityColor(prio));
        painter->setPen(Qt::NoPen);
        painter->setRenderHint(QPainter::Antialiasing);
        painter->drawEllipse(circle);

        // 圆点右侧对齐显示优先级文字
        painter->setPen(opt.palette.text().color());
        QRect tr = r.adjusted(d + 8, 0, 0, 0);
        painter->drawText(tr, Qt::AlignVCenter | Qt::AlignLeft, labels.value(prio, "低"));
    }

    // ------- 截止日期列：过期 / 今天 / 临近 颜色预警 -------
    else if (index.column() == ColDeadline) {
        QDate deadline = index.data(DeadlineRole).toDate();
        QDate today = QDate::currentDate();

        if (deadline.isValid()) {
            QColor c;
            if (deadline < today)
                c = QColor(200, 40, 40);       // 已过期 → 红色
            else if (deadline == today)
                c = QColor(220, 140, 20);      // 今天截止 → 橙色
            else if (deadline <= today.addDays(3))
                c = QColor(210, 170, 40);      // 三天内 → 浅橙色
            else
                c = opt.palette.text().color(); // 时间充裕 → 默认色
            painter->setPen(c);
        } else {
            painter->setPen(opt.palette.text().color());
        }

        QString text = deadline.isValid() ? deadline.toString("yyyy-MM-dd") : QString();
        painter->drawText(r, Qt::AlignVCenter | Qt::AlignLeft, text);
    }

    // ------- 完成列：交给默认委托画复选框 -------
    else if (index.column() == ColCompleted) {
        painter->restore();
        QStyledItemDelegate::paint(painter, opt, index);
        return;
    }

    // ------- 标题列：普通文字 -------
    else {
        painter->setPen(opt.palette.text().color());
        painter->drawText(r, Qt::AlignVCenter | Qt::AlignLeft,
                          index.data(Qt::DisplayRole).toString());
    }

    painter->restore();
}

// ─────────────────────────────────────────────── 行高提示 ──
QSize TodoDelegate::sizeHint(const QStyleOptionViewItem &option,
                             const QModelIndex &index) const
{
    QSize s = QStyledItemDelegate::sizeHint(option, index);
    s.setHeight(qMax(s.height(), 38));   // 最小行高 38px，容纳色标圆点
    return s;
}

// ─────────────────────────────────────────── 编辑器创建 ──
QWidget *TodoDelegate::createEditor(QWidget *parent,
                                    const QStyleOptionViewItem &option,
                                    const QModelIndex &index) const
{
    // 优先级列 → 下拉框
    if (index.column() == ColPriority) {
        auto *c = new QComboBox(parent);
        c->addItems({"低", "中", "高"});
        return c;
    }
    // 截止日期列 → 日期编辑器（带日历弹窗）
    if (index.column() == ColDeadline) {
        auto *d = new QDateEdit(parent);
        d->setCalendarPopup(true);
        d->setDisplayFormat("yyyy-MM-dd");
        return d;
    }
    // 标题列 → 使用默认的 QLineEdit
    return QStyledItemDelegate::createEditor(parent, option, index);
}

// ─────────────────────────────────── 从模型读取数据到编辑器 ──
void TodoDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    // 优先级：从模型 PriorityRole 读出数值，选中对应下拉索引
    if (index.column() == ColPriority) {
        auto *c = static_cast<QComboBox *>(editor);
        c->setCurrentIndex(index.data(PriorityRole).toInt());
    }
    // 截止日期：从模型 DeadlineRole 读出 QDate，设置到日期控件
    else if (index.column() == ColDeadline) {
        auto *d = static_cast<QDateEdit *>(editor);
        d->setDate(index.data(DeadlineRole).toDate());
    }
    // 其余列：默认行为
    else {
        QStyledItemDelegate::setEditorData(editor, index);
    }
}

// ───────────────────────────── 编辑器确认后将数据写回模型 ──
void TodoDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                const QModelIndex &index) const
{
    // 优先级：同时写入 DisplayRole（文字）和 PriorityRole（数值）
    if (index.column() == ColPriority) {
        int prio = static_cast<QComboBox *>(editor)->currentIndex();
        QStringList labels = {"低", "中", "高"};
        model->setData(index, labels.value(prio), Qt::DisplayRole);
        model->setData(index, prio, PriorityRole);
    }
    // 截止日期：同时写入 DisplayRole 字符串和 DeadlineRole QDate
    else if (index.column() == ColDeadline) {
        QDate date = static_cast<QDateEdit *>(editor)->date();
        model->setData(index, date.toString("yyyy-MM-dd"), Qt::DisplayRole);
        model->setData(index, date, DeadlineRole);
    }
    // 其余列：默认行为
    else {
        QStyledItemDelegate::setModelData(editor, model, index);
    }
}
