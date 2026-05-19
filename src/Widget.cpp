#include "Widget.h"
#include "TodoData.h"
#include "TodoDelegate.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QDateEdit>
#include <QDialogButtonBox>
#include <QMessageBox>

/* ===================================================================
 * Widget.cpp — 主窗口实现
 *
 * 负责：
 *   1. 搭建 工具栏 + QTreeView 布局
 *   2. 初始化 QStandardItemModel 与 QSortFilterProxyModel
 *   3. 绑定按钮事件（添加 / 删除 / 排序）
 *   4. 插入演示数据
 * =================================================================== */

Widget::Widget(QWidget *parent)
    : QWidget(parent)
{
    // ═══════════════════════════════════════════ 模型层初始化 ════
    m_model = new QStandardItemModel(0, ColCount, this);
    m_model->setHorizontalHeaderLabels({"标题", "优先级", "截止日期", "完成"});

    // 排序代理模型（架在源模型与视图之间）
    m_proxy = new TodoSortModel(this);
    m_proxy->setSourceModel(m_model);
    m_proxy->setDynamicSortFilter(false);  // 关闭自动重排，用户显式触发排序

    // 自定义委托
    m_delegate = new TodoDelegate(this);

    // ═══════════════════════════════════════════ 界面搭建 ════
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(6);

    // -- 工具栏（固定高度 40px） --
    auto *bar = new QWidget;
    bar->setFixedHeight(40);
    auto *hb = new QHBoxLayout(bar);
    hb->setContentsMargins(0, 0, 0, 0);
    hb->setSpacing(6);

    m_btnAdd    = new QPushButton("添加");
    m_btnDelete = new QPushButton("删除");
    hb->addWidget(m_btnAdd);
    hb->addWidget(m_btnDelete);
    hb->addSpacing(12);

    hb->addWidget(new QLabel("排序列:"));
    m_cmbColumn = new QComboBox;
    m_cmbColumn->addItem("标题",     ColTitle);
    m_cmbColumn->addItem("优先级",   ColPriority);
    m_cmbColumn->addItem("截止日期", ColDeadline);
    m_cmbColumn->addItem("完成状态", ColCompleted);
    hb->addWidget(m_cmbColumn);

    m_btnOrder = new QPushButton("升序 ↑");
    hb->addWidget(m_btnOrder);

    m_lblSortInfo = new QLabel;
    m_lblSortInfo->setStyleSheet("color: #666;");
    hb->addWidget(m_lblSortInfo);
    hb->addStretch();

    // -- 中央表格视图 --
    m_view = new QTreeView;
    m_view->setModel(m_proxy);
    m_view->setItemDelegate(m_delegate);
    m_view->setRootIsDecorated(false);           // 扁平列表，无展开箭头
    m_view->setAlternatingRowColors(true);       // 隔行变色
    m_view->setSelectionBehavior(QAbstractItemView::SelectRows);  // 整行选中
    m_view->setSelectionMode(QAbstractItemView::SingleSelection); // 单选
    m_view->header()->setStretchLastSection(true);  // 最后一列自适应
    m_view->header()->setDefaultAlignment(Qt::AlignCenter);
    m_view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // 布局：工具栏固定高度，表格用 stretch=1 占满剩余空间
    root->addWidget(bar);
    root->addWidget(m_view, 1);

    setWindowTitle("TodoList");
    resize(720, 500);

    // ═══════════════════════════════════════════ 信号绑定 ════
    connect(m_btnAdd,    &QPushButton::clicked, this, &Widget::addItem);
    connect(m_btnDelete, &QPushButton::clicked, this, &Widget::deleteItem);
    connect(m_btnOrder,  &QPushButton::clicked, this, &Widget::toggleSortOrder);

    connect(m_cmbColumn, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this] {
        m_proxy->sort(m_cmbColumn->currentData().toInt(),
                      m_btnOrder->text().contains("升")
                          ? Qt::AscendingOrder : Qt::DescendingOrder);
        updateSortInfo();
    });

    // ═══════════════════════════════════════════ 演示数据 ════
    auto addDemo = [&](const QString &title, int prio, QDate deadline, bool done) {
        int r = m_model->rowCount();
        m_model->insertRow(r);
        m_model->setData(m_model->index(r, ColTitle), title);

        QStringList pl = {"低", "中", "高"};
        m_model->setData(m_model->index(r, ColPriority), pl.value(prio), Qt::DisplayRole);
        m_model->setData(m_model->index(r, ColPriority), prio, PriorityRole);

        m_model->setData(m_model->index(r, ColDeadline), deadline.toString("yyyy-MM-dd"), Qt::DisplayRole);
        m_model->setData(m_model->index(r, ColDeadline), deadline, DeadlineRole);

        auto *ci = m_model->item(r, ColCompleted);
        if (!ci) { ci = new QStandardItem; m_model->setItem(r, ColCompleted, ci); }
        ci->setCheckable(true);
        ci->setCheckState(done ? Qt::Checked : Qt::Unchecked);
    };
    addDemo("完成项目报告",     PriorityHigh,   QDate(2026, 5, 15), false);
    addDemo("提交周报",         PriorityMedium, QDate(2026, 5, 14), false);
    addDemo("复习Qt基础知识",   PriorityLow,    QDate(2026, 5, 20), false);
    addDemo("代码审查",         PriorityHigh,   QDate(2026, 5, 10), true);
    addDemo("更新开发环境",     PriorityMedium, QDate(2026, 5, 18), false);

    // 初始排序：按截止日期升序
    m_cmbColumn->setCurrentIndex(m_cmbColumn->findData(ColDeadline));
    m_proxy->sort(ColDeadline, Qt::AscendingOrder);
    m_btnOrder->setText("升序 ↑");
    m_lblSortInfo->setText("当前排序: 截止日期 (升序)");
}

// ────────────────────────────────────────────── 添加任务 ──
void Widget::addItem()
{
    // 弹出模态对话框收集输入
    QDialog dlg(this);
    dlg.setWindowTitle("添加任务");
    auto *form = new QFormLayout(&dlg);

    auto *titleEdit = new QLineEdit;
    auto *prioCmb   = new QComboBox;
    prioCmb->addItems({"低", "中", "高"});
    auto *dateEdit  = new QDateEdit(QDate::currentDate());
    dateEdit->setCalendarPopup(true);
    dateEdit->setDisplayFormat("yyyy-MM-dd");

    form->addRow("标题:",     titleEdit);
    form->addRow("优先级:",   prioCmb);
    form->addRow("截止日期:", dateEdit);

    auto *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    form->addRow(bb);
    connect(bb, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted) return;

    // 验证
    QString title = titleEdit->text().trimmed();
    if (title.isEmpty()) {
        QMessageBox::warning(this, "提示", "标题不能为空");
        return;
    }

    // 写入模型
    int row = m_model->rowCount();
    m_model->insertRow(row);

    m_model->setData(m_model->index(row, ColTitle), title);

    QStringList pl = {"低", "中", "高"};
    m_model->setData(m_model->index(row, ColPriority), pl.value(prioCmb->currentIndex()), Qt::DisplayRole);
    m_model->setData(m_model->index(row, ColPriority), prioCmb->currentIndex(), PriorityRole);

    m_model->setData(m_model->index(row, ColDeadline), dateEdit->date().toString("yyyy-MM-dd"), Qt::DisplayRole);
    m_model->setData(m_model->index(row, ColDeadline), dateEdit->date(), DeadlineRole);

    auto *ci = new QStandardItem;
    ci->setCheckable(true);
    ci->setCheckState(Qt::Unchecked);
    m_model->setItem(row, ColCompleted, ci);
}

// ────────────────────────────────────────────── 删除任务 ──
void Widget::deleteItem()
{
    QModelIndex idx = m_view->currentIndex();     // 代理模型中的索引
    if (!idx.isValid()) return;
    // 代理 → 源模型映射，再删除对应行
    m_model->removeRow(m_proxy->mapToSource(idx).row());
}

// ─────────────────────────────────────────── 切换升降序 ──
void Widget::toggleSortOrder()
{
    bool asc = m_btnOrder->text().contains("升");
    m_btnOrder->setText(asc ? "降序 ↓" : "升序 ↑");
    m_proxy->sort(m_cmbColumn->currentData().toInt(),
                  asc ? Qt::DescendingOrder : Qt::AscendingOrder);
    updateSortInfo();
}

// ─────────────────────────────────────── 更新排序状态标签 ──
void Widget::updateSortInfo()
{
    QString colName = m_cmbColumn->currentText();
    QString order   = m_btnOrder->text().contains("升") ? "升序" : "降序";
    m_lblSortInfo->setText(QString("当前排序: %1 (%2)").arg(colName, order));
}
