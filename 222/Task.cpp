#include "Task.h"
Task::Task() :m_title(""), m_description(""), m_priority(Low), m_deadline(QDateTime()), m_category("未分类"), m_status(false),m_id(""){}
Task::Task(const QString &title) :m_title(title), m_description(""), m_priority(Low), m_deadline(QDateTime()), m_category("未分类"), m_status(false),m_id(""){}
Task::Task(const QString& title, const QString& description, Priority priority,
           const QDateTime& deadline, const QString& category, bool status)
    : m_id(""),
    m_title(title),
    m_description(description),
    m_priority(priority),
    m_deadline(deadline),
    m_category(category),
    m_status(status)
{
}
//get方法
QString Task::getTitle()const {
    return m_title;
}
QString Task::getDescription()const {
    return m_description;
}
Task::Priority Task::getPriority() const {
    return m_priority;
}
QDateTime Task::getDeadline() const {
    return m_deadline;
}
QString Task::getCategory() const {
    return m_category;
}

QString Task::getId()const {
    return m_id;
}
//set方法
bool Task::getStatus() const {
    return m_status;
}
void Task::setTitle(const QString& title) {
    m_title = title;
}
void Task::setDescription(const QString& description) {
    m_description = description;
}
void Task::setPriority(Priority priority) {
    m_priority = priority;
}
void Task::setDeadline(const QDateTime& deadline) {
    m_deadline = deadline;
}
void Task::setCategory(const QString& category) {
    m_category = category;
}
void Task::setStatus(bool status) {
    m_status = status;
}
void Task::setId(const QString& id) {
    m_id = id;
}
//工具
QString Task::toString() const {
    switch (m_priority) {
    case High:return "High";break;
    case Medium:return "Medium";break;
    case Low:return "Low";break;
    default:return "Low";break;
    }
}
Task::Priority Task::toPriority(const QString& str) {
    if (str=="High")
        return High;
    else if (str=="Medium")
        return Medium;
    else
        return Low;
}
//判断是否过期：未完成、有截止日期、截止日期小于当前日期
bool Task::isOverdue()const {
    return !m_status && m_deadline.isValid() && m_deadline < QDateTime::currentDateTime();
}
//判断今天是否到期
bool  Task::isDueToday()const {
    return !m_status && m_deadline.isValid() && m_deadline.date()==QDate::currentDate();
}
