#ifndef TODODATA_H
#define TODODATA_H

#include <QColor>

/*
 * ======================================================================
 * TodoData.h — 全局数据常量定义
 *
 * 所有需要在多个文件之间共享的枚举、角色 ID 和工具函数都集中在这里，
 * 避免各 .cpp 文件硬编码魔数（magic number），方便统一调整。
 *
 * 核心概念——自定义数据角色（Custom Data Roles）：
 *   Qt 的 Model/View 框架中，每个单元格可以通过不同的 role 取出不同
 *   类型的数据。Qt 预留了 DisplayRole（显示文本）、CheckStateRole（复
 *   选框状态）等标准 role，Qt::UserRole 之后的值供用户自定义。
 *
 *   本项目用两个自定义 role 分别存储"给人看的数据"和"给排序用的数据"：
 *     - DisplayRole → "高"  （用户友好的中文文本）
 *     - PriorityRole → 2    （排序用的原始整数值）
 *   这就是"角色分离"——同一单元格里的数据，按用途分开放。
 * ======================================================================
 */

/*
 * 自定义数据角色枚举
 *
 * 这些值作为 QModelIndex::data(role) 的参数，指定要取出哪种数据。
 * Qt::UserRole 是 Qt 预留的起始值（= 0x0100 = 256），我们在此基础上
 * 递增，避免和标准 role 冲突。
 */
enum TodoRole {
    PriorityRole = Qt::UserRole + 1,   // 优先级数值（int: 0=低, 1=中, 2=高）
    DeadlineRole                        // 截止日期（QDate）
};

/*
 * 优先级枚举
 *
 * 特意设计为与 QComboBox 的下标一致，这样 currentIndex 可以直接
 * 当作 PriorityRole 的值存入模型，无需额外转换。
 */
enum Priority {
    PriorityLow    = 0,       // 低优先级 → 绿色圆点
    PriorityMedium = 1,       // 中优先级 → 橙色圆点
    PriorityHigh   = 2        // 高优先级 → 红色圆点
};

/*
 * 表格列索引枚举
 *
 * QStandardItemModel 是一个扁平的 4 列表格，用这些常量按名引用各列，
 * 比硬编码 0/1/2/3 更可读，后续如果增删列也只需改这里。
 */
enum Column {
    ColTitle     = 0,          // 标题列
    ColPriority  = 1,          // 优先级列（含彩色圆点）
    ColDeadline  = 2,          // 截止日期列（含时间预警色）
    ColCompleted = 3,          // 完成状态列（复选框）
    ColCount     = 4           // 总列数，用于循环遍历等场景
};

/*
 * 优先级 → 颜色映射函数
 *
 * 根据优先级返回对应的色标颜色，用于在 TodoDelegate::paint() 中
 * 绘制优先级列的彩色圆点：
 *   高 → 红 (220,50,50)     紧迫/重要
 *   中 → 橙 (230,150,30)    中等
 *   低 → 绿 (80,180,60)     宽松/不重要
 *
 * 设计为 inline 函数是因为它定义在头文件中，inline 可以避免多重
 * 定义链接错误（多个 .cpp 包含此头文件时）。
 */
inline QColor priorityColor(int priority) {
    switch (priority) {
        case PriorityHigh:   return QColor(220, 50, 50);
        case PriorityMedium: return QColor(230, 150, 30);
        case PriorityLow:    return QColor(80, 180, 60);
        default:             return Qt::gray;
    }
}

#endif // TODODATA_H
