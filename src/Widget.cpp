#include "Widget.h"
#include "TodoData.h"
#include "TodoDelegate.h"
#include "NetworkLoader.h"
#include "ui_Widget.h"

#include <QDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QDateEdit>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QJsonObject>

/*
 * ======================================================================
 * Widget.cpp — 主窗口实现
 *
 * Widget 是整个应用的"中枢神经系统"，负责：
 *   1. 搭建 UI 布局（由 Widget.ui 定义，setupUi 构建）
 *   2. 初始化 Model-View-Delegate 三层架构
 *   3. 绑定按钮事件和信号/槽连接
 *   4. 插入演示数据并在启动时展示初始排序
 *   5. 协调网络加载的操作流程
 * ======================================================================
 */

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)             // 创建 UI 对象
{
    // ══════════════════════════════════════════════════════════════════
    // 第一步：由 .ui 文件构建界面
    // ══════════════════════════════════════════════════════════════════
    /*
     * setupUi 根据 Widget.ui 生成所有界面控件并建立布局：
     *   - 工具栏（QHBoxLayout 内含按钮 + 排序控件 + 弹簧）
     *   - QProgressBar（默认隐藏，加载时显示）
     *   - QTreeView（表格视图，配置隔行变色、单选、整行选中）
     * 同时创建 ui->m_btnAdd、ui->m_view 等成员指针供后续访问。
     */
    ui->setupUi(this);

    // ══════════════════════════════════════════════════════════════════
    // 第二步：模型层初始化
    // ══════════════════════════════════════════════════════════════════

    /*
     * QStandardItemModel: 内存中的表格数据存储
     * 参数: (初始行数, 列数, 父对象)
     * 初始 0 行，4 列（由 ColCount = 4 定义），后续通过 insertRow 动态添加。
     * 设置 this 为父对象，m_model 会随 Widget 自动析构。
     */
    m_model = new QStandardItemModel(0, ColCount, this);
    m_model->setHorizontalHeaderLabels({"标题", "优先级", "截止日期", "完成"});

    /*
     * TodoSortModel: 排序代理模型（继承 QSortFilterProxyModel）
     * 架在源模型和视图之间，提供类型感知的排序能力。
     * setDynamicSortFilter(false) 关闭自动排序——排序只由用户显式触发。
     */
    m_proxy = new TodoSortModel(this);
    m_proxy->setSourceModel(m_model);
    m_proxy->setDynamicSortFilter(false);

    /*
     * TodoDelegate: 自定义单元格委托
     * 控制每列的绘制外观和双击编辑行为。
     */
    m_delegate = new TodoDelegate(this);

    // 将模型、排序代理、委托装配到视图中
    ui->m_view->setModel(m_proxy);
    ui->m_view->setItemDelegate(m_delegate);

    // ══════════════════════════════════════════════════════════════════
    // 第三步：信号/槽绑定
    // ══════════════════════════════════════════════════════════════════

    connect(ui->m_btnAdd,     &QPushButton::clicked, this, &Widget::addItem);
    connect(ui->m_btnDelete,  &QPushButton::clicked, this, &Widget::deleteItem);
    connect(ui->m_btnOrder,   &QPushButton::clicked, this, &Widget::toggleSortOrder);
    connect(ui->m_btnNetwork, &QPushButton::clicked, this, &Widget::onLoadFromNetwork);

    // 排序列选择变化 → 立即重新排序
    connect(ui->m_cmbColumn, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this] {
        m_proxy->sort(ui->m_cmbColumn->currentData().toInt(),
                      ui->m_btnOrder->text().contains("升")
                          ? Qt::AscendingOrder : Qt::DescendingOrder);
        updateSortInfo();
    });

    // ---- 网络加载器初始化 ----
    m_networkLoader = new NetworkLoader(this);
    connect(m_networkLoader, &NetworkLoader::dataReady,
            this, &Widget::onNetworkDataReady);
    connect(m_networkLoader, &NetworkLoader::errorOccurred,
            this, &Widget::onNetworkError);

    // ══════════════════════════════════════════════════════════════════
    // 第四步：插入演示数据
    // ══════════════════════════════════════════════════════════════════

    /*
     * addDemo 是一个 lambda 表达式，[&] 以引用方式捕获所有外部变量。
     * 这里主要用到 m_model（QStandardItemModel*），以引用方式捕获
     * 等价于使用指针，操作的是同一个对象。
     *
     * 5 条演示数据覆盖了不同的组合：
     *   - 优先级：高×2、中×2、低×1
     *   - 截止日期：过期的（5/10, 5/14）、当前的（5/15）、未来的（5/18, 5/20）
     *   - 完成状态：已完成的 ×1、未完成的 ×4
     *
     * 这样用户一启动程序就能看到所有功能的视觉表现。
     */
    auto addDemo = [&](const QString &title, int priority, QDate deadline, bool done) {
        int row = m_model->rowCount();
        m_model->insertRow(row);
        m_model->setData(m_model->index(row, ColTitle), title);

        QStringList priorityLabels = {"低", "中", "高"};
        m_model->setData(m_model->index(row, ColPriority),
                         priorityLabels.value(priority), Qt::DisplayRole);
        m_model->setData(m_model->index(row, ColPriority),
                         priority, PriorityRole);

        m_model->setData(m_model->index(row, ColDeadline),
                         deadline.toString("yyyy-MM-dd"), Qt::DisplayRole);
        m_model->setData(m_model->index(row, ColDeadline),
                         deadline, DeadlineRole);

        auto *checkItem = m_model->item(row, ColCompleted);
        if (!checkItem) {
            checkItem = new QStandardItem;
            m_model->setItem(row, ColCompleted, checkItem);
        }
        checkItem->setCheckable(true);
        checkItem->setCheckState(done ? Qt::Checked : Qt::Unchecked);
    };

    addDemo("完成项目报告",   PriorityHigh,   QDate(2026, 5, 15), false);
    addDemo("提交周报",       PriorityMedium, QDate(2026, 5, 14), false);
    addDemo("复习Qt基础知识", PriorityLow,    QDate(2026, 5, 20), false);
    addDemo("代码审查",       PriorityHigh,   QDate(2026, 5, 10), true);
    addDemo("更新开发环境",   PriorityMedium, QDate(2026, 5, 18), false);

    // ---- 初始排序：按截止日期升序 ----
    ui->m_cmbColumn->setCurrentIndex(ColDeadline);
    m_proxy->sort(ColDeadline, Qt::AscendingOrder);
    ui->m_btnOrder->setText("升序 ↑");
    ui->m_lblSortInfo->setText("当前排序: 截止日期 (升序)");
}

Widget::~Widget()
{
    delete ui;
}


// ══════════════════════════════════════════════════════════════════════
// 添加任务
// ══════════════════════════════════════════════════════════════════════

void Widget::addItem()
{
    // ---- 弹出对话框收集输入 ----
    QDialog dialog(this);
    dialog.setWindowTitle("添加任务");
    auto *form = new QFormLayout(&dialog);

    auto *titleInput = new QLineEdit;
    auto *priorityCombo = new QComboBox;
    priorityCombo->addItems({"低", "中", "高"});
    auto *dateInput = new QDateEdit(QDate::currentDate());
    dateInput->setCalendarPopup(true);
    dateInput->setDisplayFormat("yyyy-MM-dd");

    form->addRow("标题:",     titleInput);
    form->addRow("优先级:",   priorityCombo);
    form->addRow("截止日期:", dateInput);

    auto *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    form->addRow(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted)
        return;

    // ---- 输入验证 ----
    QString title = titleInput->text().trimmed();
    if (title.isEmpty()) {
        QMessageBox::warning(this, "提示", "标题不能为空");
        return;
    }

    // ---- 写入模型 ----
    int row = m_model->rowCount();
    m_model->insertRow(row);

    // 标题列
    m_model->setData(m_model->index(row, ColTitle), title);

    // 优先级列：同时写入显示文本和排序用数值
    QStringList priorityLabels = {"低", "中", "高"};
    m_model->setData(m_model->index(row, ColPriority),
                     priorityLabels.value(priorityCombo->currentIndex()),
                     Qt::DisplayRole);
    m_model->setData(m_model->index(row, ColPriority),
                     priorityCombo->currentIndex(),
                     PriorityRole);

    // 截止日期列：同时写入显示字符串和排序用 QDate
    m_model->setData(m_model->index(row, ColDeadline),
                     dateInput->date().toString("yyyy-MM-dd"),
                     Qt::DisplayRole);
    m_model->setData(m_model->index(row, ColDeadline),
                     dateInput->date(),
                     DeadlineRole);

    // 完成列：新建复选框 item，默认未勾选
    auto *checkItem = new QStandardItem;
    checkItem->setCheckable(true);
    checkItem->setCheckState(Qt::Unchecked);
    m_model->setItem(row, ColCompleted, checkItem);
}


// ══════════════════════════════════════════════════════════════════════
// 删除任务
// ══════════════════════════════════════════════════════════════════════

void Widget::deleteItem()
{
    /*
     * QTreeView::currentIndex() 返回的是代理模型（TodoSortModel）中的
     * 索引，不能直接用于操作源模型（QStandardItemModel）。
     * 必须通过 mapToSource() 将代理索引转换为源模型索引，再取得行号。
     *
     * 如果不做映射，删除的可能是排序后的"第 3 行"而不是源模型的"第 3 行"，
     * 导致删除错行。
     */
    QModelIndex proxyIndex = ui->m_view->currentIndex();
    if (!proxyIndex.isValid())
        return;

    int sourceRow = m_proxy->mapToSource(proxyIndex).row();
    m_model->removeRow(sourceRow);
}


// ══════════════════════════════════════════════════════════════════════
// 切换升序 / 降序
// ══════════════════════════════════════════════════════════════════════

void Widget::toggleSortOrder()
{
    bool isAscending = ui->m_btnOrder->text().contains("升");
    ui->m_btnOrder->setText(isAscending ? "降序 ↓" : "升序 ↑");

    m_proxy->sort(ui->m_cmbColumn->currentData().toInt(),
                  isAscending ? Qt::DescendingOrder : Qt::AscendingOrder);
    updateSortInfo();
}


// ══════════════════════════════════════════════════════════════════════
// 更新排序状态标签
// ══════════════════════════════════════════════════════════════════════

void Widget::updateSortInfo()
{
    QString columnName = ui->m_cmbColumn->currentText();
    QString orderText = ui->m_btnOrder->text().contains("升") ? "升序" : "降序";
    ui->m_lblSortInfo->setText(
        QString("当前排序: %1 (%2)").arg(columnName, orderText));
}


// ══════════════════════════════════════════════════════════════════════
// 从网络加载任务
// ══════════════════════════════════════════════════════════════════════

void Widget::onLoadFromNetwork()
{
    /*
     * 加载开始前的 UI 状态更新
     * 按钮置灰 + 文本变化 → 防止重复点击
     * 进度条显示 → 让用户知道有操作在后台进行
     */
    ui->m_btnNetwork->setEnabled(false);
    ui->m_btnNetwork->setText("加载中...");
    ui->m_progressBar->setVisible(true);

    /*
     * 发起 GET 请求
     * URL: JSONPlaceholder 的 /todos 接口，_limit=5 限制返回 5 条
     * 超时: 10 秒（默认 5 秒在某些网络环境下可能不够）
     */
    m_networkLoader->fetchTodos(
        QUrl("http://jsonplaceholder.typicode.com/todos?_limit=5"), 10000);
}


// ══════════════════════════════════════════════════════════════════════
// 网络数据就绪 — 解析并填充模型
// ══════════════════════════════════════════════════════════════════════

void Widget::onNetworkDataReady(const QJsonArray &data)
{
    /*
     * 方案说明：
     *   优先级（B 方案）：轮流分配 低/中/高/低/中...（按 i % 3）
     *   截止日期（B 方案）：全部设为"当前日期 + 7 天"
     * 因为 JSONPlaceholder 的待办数据本身不包含优先级和截止日期字段。
     */
    QStringList priorityLabels = {"低", "中", "高"};
    QDate defaultDeadline = QDate::currentDate().addDays(7);

    for (int i = 0; i < data.size(); ++i) {
        QJsonObject item = data[i].toObject();

        int row = m_model->rowCount();
        m_model->insertRow(row);

        // ---- 标题（直接取自 API 的 title 字段） ----
        m_model->setData(m_model->index(row, ColTitle),
                         item["title"].toString());

        // ---- 优先级：按索引轮流分配 ----
        int priority = i % 3;
        m_model->setData(m_model->index(row, ColPriority),
                         priorityLabels.value(priority), Qt::DisplayRole);
        m_model->setData(m_model->index(row, ColPriority),
                         priority, PriorityRole);

        // ---- 截止日期：固定为当前日期 + 7 天 ----
        m_model->setData(m_model->index(row, ColDeadline),
                         defaultDeadline.toString("yyyy-MM-dd"),
                         Qt::DisplayRole);
        m_model->setData(m_model->index(row, ColDeadline),
                         defaultDeadline, DeadlineRole);

        // ---- 完成状态（取自 API 的 completed 字段） ----
        auto *checkItem = new QStandardItem;
        checkItem->setCheckable(true);
        checkItem->setCheckState(
            item["completed"].toBool() ? Qt::Checked : Qt::Unchecked);
        m_model->setItem(row, ColCompleted, checkItem);
    }

    // 恢复按钮和进度条状态
    ui->m_btnNetwork->setEnabled(true);
    ui->m_btnNetwork->setText("从网络加载");
    ui->m_progressBar->setVisible(false);
}


// ══════════════════════════════════════════════════════════════════════
// 网络请求出错
// ══════════════════════════════════════════════════════════════════════

void Widget::onNetworkError(const QString &message)
{
    // 恢复 UI 状态（确保即使出错按钮也能重新使用）
    ui->m_btnNetwork->setEnabled(true);
    ui->m_btnNetwork->setText("从网络加载");
    ui->m_progressBar->setVisible(false);

    // 弹窗提示用户错误原因
    QMessageBox::warning(this, "网络错误", message);
}
