#pragma once
#include <QString>
#include <QDateTime>
#include <QDate>

class Task
{
public:
    // 优先级枚举
    enum Priority {
        Low,
        Medium,
        High,
    };
    Task();
    explicit Task(const QString& title);
    Task(const QString& title, const QString& description, Priority priority,
         const QDateTime& deadline, const QString& category, bool status);

    //属性获取方法
    QString getTitle() const;
    QString getDescription() const;
    Priority getPriority() const;
    QDateTime getDeadline() const;
    QString getCategory() const;
    QString getId()const;

    //状态表示是否完成
    bool getStatus() const;
    //属性设置方法
    void setTitle(const QString& title);
    void setDescription(const QString& description);
    void setPriority(Priority priority);
    void setDeadline(const QDateTime& deadline);
    void setCategory(const QString& category);
    void setStatus(bool status);
    void setId(const QString& id);

    //工具
    QString toString() const;
    Priority toPriority(const QString& priorityString);
    //是否已经过期
    bool isOverdue() const;
    //是否今天过期
    bool  isDueToday()const;

private:
    //任务包含属性：
    QString m_title;
    QString m_description;
    Priority m_priority;
    QDateTime m_deadline;
    //分类
    QString  m_category;
    //0"未完成" 1"已完成"
    bool m_status;
    QString m_id;
};
