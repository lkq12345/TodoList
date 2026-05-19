#ifndef TODODATA_H
#define TODODATA_H

#include <QColor>

/* ===================================================================
 * tododata.h — 数据常量定义
 *
 * 集中管理整个程序用到的枚举常量、角色 ID 和工具函数，
 * 避免各文件硬编码魔数（magic number），方便统一调整。
 * =================================================================== */

// ---- 自定义数据角色 ----
// QModelIndex::data(role) 通过不同的 role 取出不同类型的数据。
// Qt::UserRole 之后的值供用户自定义使用。
enum TodoRole {
    PriorityRole = Qt::UserRole + 1,   // 优先级数值（int，0=低 1=中 2=高）
    DeadlineRole                        // 截止日期（QDate）
};

// ---- 优先级枚举 ----
// 直接与 QComboBox 的索引对应，0 低 / 1 中 / 2 高
enum Priority {
    PriorityLow    = 0,
    PriorityMedium = 1,
    PriorityHigh   = 2
};

// ---- 表格列索引 ----
// QStandardItemModel 的列号，方便按列读写数据
enum Column {
    ColTitle     = 0,
    ColPriority  = 1,
    ColDeadline  = 2,
    ColCompleted = 3,
    ColCount     = 4     // 总列数，用于循环遍历
};

// ---- 优先级色标 ----
// 根据优先级返回对应的圆点颜色：
//   高 → 红   中 → 橙   低 → 绿
inline QColor priorityColor(int priority) {
    switch (priority) {
        case PriorityHigh:   return QColor(220, 50, 50);
        case PriorityMedium: return QColor(230, 150, 30);
        case PriorityLow:    return QColor(80, 180, 60);
        default:             return Qt::gray;
    }
}

#endif // TODODATA_H
