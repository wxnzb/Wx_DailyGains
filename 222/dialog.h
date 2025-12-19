#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QTextEdit>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include"Task.h"
namespace Ui {
class Dialog;
}

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(const QStringList& categories,QWidget *parent = nullptr);
    ~Dialog();
    //目前要给新建用，你保存的时候他要获取这些逆天的信息
    QString getTitle() const;           // 获取任务标题
    QString getCategory() const;        // 获取任务分类
    QString getPriority() const;        // 获取优先级（返回字符串）
    Task::Priority getPriorityEnum() const;  // 获取优先级（返回枚举）
    QDateTime getDeadline() const;      // 获取截止日期
    QString getDescription() const;     // 获取详细描述
    //这个主要是给编辑用的
    void setTitle(const QString& title);
    void setCategory(const QString& category);
    void setPriority(Task::Priority priority);
    void setDeadline(const QDateTime& deadline);
    void setDescription(const QString& description);
private:
    Ui::Dialog *ui;
    // 控件成员变量
    QLineEdit* titleEdit;           // 任务标题
    QComboBox* categoryCombo;       // 任务分类
    QComboBox* priorityCombo;       // 优先级
    QDateTimeEdit* deadlineEdit;    // 截止日期
    QTextEdit* descriptionEdit;     // 详细描述
    QDialogButtonBox* buttonBox;    // 按钮组

    //分类列表
    QStringList m_categories;
    // 初始化 UI
    void setupUI();
};

#endif // DIALOG_H
