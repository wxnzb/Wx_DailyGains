#pragma once
//管理多个任务
#include "Task.h"
#include <QObject>
class TaskManager:public QObject
{
    Q_OBJECT
public:
    explicit TaskManager(QObject *parent = nullptr);
    //1:任务个管理方法
    QString addTask(const Task& task);
    bool removeTask(const QString& taskId);
    bool updateTask(const QString& taskId,const Task& task);
    Task* getTask(const QString& taskId);
    QList<Task> getAllTasks()const;
    //2:任务查询
    //按照分类查询
    QList<Task> getTaskByCategory(const QString&category)const;
    //按照优先级查询
    QList<Task> getTaskByPriority(Task::Priority priority)const;
    //按照是否完成查询
    QList<Task> getTaskByCompletion(bool status)const;
    //搜索关键词查询任务
    QList<Task> searchTasks(const QString& keyword)const;
    //按照时间筛选
    QList<Task> getOverdueTasks()const;
    QList<Task> getDueTodayTasks()const;
    //3:分类管理方法
    QStringList getCategories()const;
    void addCategory(const QString& category);
    void removeCategory(const QString& category);
    bool isHasCategory(const QString& category)const;
    //4:统计信息方法
    int getTotalTaskCount()const;
    int getCompletedTaskCount()const;
    int getPendingTaskCount()const;
    int getOverdueTaskCount()const;
    int getDueTodayTaskCount()const;
    //数据持久化
    bool saveToFile(const QString& filename);
    bool loadFromFile(const QString& filename);
    //设置成公共的
    int findTaskIndex(const QString& taskId)const;
private:
    //任务列表
    QList<Task> m_tasks;
    //所有分类名称
    QStringList m_categories;

    //确保至少有一个默认分类
    void ensureDefaultCategories();
    QString generateTaskId()const;
};

