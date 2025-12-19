#include "TaskManager.h"
#include<QDateTime>
#include<QRandomGenerator>
#include<QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include<QByteArray>
TaskManager::TaskManager(QObject* parent):QObject(parent) {
    ensureDefaultCategories();
    qDebug() << "TaskManager 初始化完成";

}
//1:任务个管理方法
QString TaskManager::addTask(const Task& task) {
    Task newTask = task;
    QString newId = generateTaskId();
    newTask.setId(newId);
    m_tasks.append(newTask);
    return newId;
}
bool TaskManager::removeTask(const QString& taskId) {
    int i = findTaskIndex(taskId);
    if (i == -1) {
        return false;
    }
    m_tasks.removeAt(i);
    return true;
}
bool TaskManager::updateTask(const QString& taskId, const Task& task) {
    int i = findTaskIndex(taskId);
    if (i == -1) {
        return false;
    }
    Task updatedTask = task;
    updatedTask.setId(taskId);
    m_tasks[i] = updatedTask;
    return true;
}
Task* TaskManager::getTask(const QString& taskId) {
    int i = findTaskIndex(taskId);
    if (i == -1) {
        return nullptr;
    }
    return &m_tasks[i];
}
QList<Task> TaskManager::getAllTasks()const {
    return m_tasks;
}
//2:任务查询
//按照分类查询
QList<Task> TaskManager::getTaskByCategory(const QString& category)const {
    QList<Task>r;
    for (const Task& t : m_tasks) {
        if (t.getCategory() == category) {
            r.append(t);
        }
    }
    return r;
}
//按照优先级查询
QList<Task> TaskManager::getTaskByPriority(Task::Priority priority)const {
    QList<Task>r;
    for (const Task& t : m_tasks) {
        if (t.getPriority() == priority) {
            r.append(t);
        }
    }
    return r;
}
//按照是否完成查询
QList<Task> TaskManager::getTaskByCompletion(bool status)const {
    QList<Task>r;
    for (const Task& t : m_tasks) {
        if (t.getStatus() == status) {
            r.append(t);
        }
    }
    return r;
}
//搜索关键词查询任务
QList<Task> TaskManager::searchTasks(const QString& keyword)const {
    QList<Task>r;
    if (keyword.isEmpty()) {
        return r;
    }
    for (const Task& t : m_tasks) {
        if (t.getTitle().contains(keyword,Qt::CaseInsensitive)
            || t.getDescription().contains(keyword, Qt::CaseInsensitive)
            || t.getCategory().contains(keyword, Qt::CaseInsensitive)) {
            r.append(t);
        }
    }
    return r;
}
//按照时间筛选
QList<Task> TaskManager::getOverdueTasks()const {
    QList<Task>r;
    for (const Task& t : m_tasks) {
        if (t.isOverdue()) {
            r.append(t);
        }
    }
    return r;
}
QList<Task> TaskManager::getDueTodayTasks()const {
    QList<Task>r;
    for (const Task& t : m_tasks) {
        if (t.isDueToday()) {
            r.append(t);
        }
    }
    return r;
}
//3:分类管理方法
QStringList TaskManager::getCategories()const {
    return m_categories;
}
void TaskManager::addCategory(const QString& category) {
    if (!m_categories.contains(category)) {
        m_categories.append(category);
    }
}
void TaskManager::removeCategory(const QString& category) {
    if (m_categories.contains(category) && category != "未分类") {
        m_categories.removeAll(category);
        //这里删除分类的任务移动到未删除
        for (Task& t : m_tasks) {
            if (t.getCategory() == category) {
                t.setCategory("未分类");
            }
        }
    }
}
bool TaskManager::isHasCategory(const QString& category)const {
    return m_categories.contains(category);
}
//4:统计信息方法
int TaskManager::getTotalTaskCount()const {
    return m_tasks.size();
}
int TaskManager::getCompletedTaskCount()const {
    int cnt = 0;
    for (const Task& t : m_tasks) {
        if (t.getStatus() ) {
            cnt++;
        }
    }
    return cnt;
}
int TaskManager::getPendingTaskCount()const {
    return getTotalTaskCount() - getCompletedTaskCount();
}
int TaskManager::getOverdueTaskCount()const {
    return getOverdueTasks().size();
}
int TaskManager::getDueTodayTaskCount()const {
    return getDueTodayTasks().size();
}
//数据持久化：json保存在本地文件
bool TaskManager::saveToFile(const QString& filename) {
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "Failed to open file for writing: " << file.errorString();
        return false;
    }
    QJsonObject root;
    QJsonArray categoriesArray;
    for (const QString& category : m_categories) {
        categoriesArray.append(category);
    }
    QJsonArray tasksArray;
    root["categories"] = categoriesArray;
    for (const Task& task : m_tasks) {
        QJsonObject taskObj;
        taskObj["id"] = task.getId();
        taskObj["title"] = task.getTitle();
        taskObj["description"] = task.getDescription();
        taskObj["priority"] = task.getPriority();
        taskObj["deadline"] = task.getDeadline().toString(Qt::ISODate);
        taskObj["category"] = task.getCategory();
        taskObj["status"] = task.getStatus();
        tasksArray.append(taskObj);
    }
    root["tasks"]=tasksArray;
    QJsonDocument doc(root);
    file.write(doc.toJson());
    file.close();
    qDebug() << "数据已保存到文件:" << filename;
    return true;
}
//从json文件读取数据
bool TaskManager::loadFromFile(const QString& filename) {
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to open file for writing: " << file.errorString();
        return false;
    }
    //读数据到data
    QByteArray data = file.readAll();
    file.close();
    //测试u
    qDebug() << "========== tasks.json RAW ==========";
    qDebug().noquote() << data;
    qDebug() << "========== END ==========";

    //将json数据解析成一个doc
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull()) {
        qDebug() << "JSON document is null";
        return false;
    }
    //获取doc最顶层的Json结构
    QJsonObject root = doc.object();
    //加载分类:清空现有的重新进行加载
    //1
    m_categories.clear();
    QJsonArray categoriesArray = root["categories"].toArray();
    for (const QJsonValue& value : categoriesArray) {
        m_categories.append(value.toString());
    }
    m_tasks.clear();
    QJsonArray tasksArray = root["tasks"].toArray();
    for (const QJsonValue& value : tasksArray) {
        QJsonObject o = value.toObject();

        Task t(
            o["title"].toString(),
            o["description"].toString(),
            static_cast<Task::Priority>(o["priority"].toInt()),
            QDateTime::fromString(o["deadline"].toString(), Qt::ISODate),
            o["category"].toString(),
            o["status"].toBool()
            );

        t.setId(o["id"].toString());

        m_tasks.append(t);
    }
    return true;
}

//私有方法
void TaskManager::ensureDefaultCategories() {
    if (m_categories.isEmpty())
        m_categories << "未分类" << "工作" << "学习" << "生活";
}
QString TaskManager::generateTaskId()const{
    return QString("task_%1_%2")
    .arg(QDateTime::currentDateTime().toMSecsSinceEpoch()).arg(QRandomGenerator::global()->bounded(1000, 9999));
}
int TaskManager::findTaskIndex(const QString& taskId)const {
    for (int i = 0; i < m_tasks.size(); i++) {
        if (m_tasks[i].getId() == taskId) {
            return i;
        }
    }
    return -1;
}

