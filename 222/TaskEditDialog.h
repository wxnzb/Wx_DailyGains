#pragma once

#include <QDialog>
#include "Task.h"

namespace Ui {
    class TaskEditDialog;
}

class TaskEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TaskEditDialog(QWidget* parent = nullptr);
    explicit TaskEditDialog(const Task& task, QWidget* parent = nullptr);
    ~TaskEditDialog();

    Task getTaskData() const;
    void setTaskData(const Task& task);

private slots:
    void onSaveClicked();
    void onCancelClicked();

private:
    Ui::TaskEditDialog* ui;
    void setupUI();
    void connectSignals();
};