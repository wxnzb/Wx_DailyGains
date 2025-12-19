#include "TaskEditDialog.h"
#include "ui_TaskEditDialog.h"

TaskEditDialog::TaskEditDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::TaskEditDialog)
{
    ui->setupUi(this);
    setupUI();
    connectSignals();
}

TaskEditDialog::TaskEditDialog(const Task& task, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::TaskEditDialog)
{
    ui->setupUi(this);
    setupUI();
    connectSignals();
    setTaskData(task); // 预填充数据
}

TaskEditDialog::~TaskEditDialog()
{
    delete ui;
}

void TaskEditDialog::setupUI()
{
    // 设置窗口标题
    setWindowTitle("任务编辑");

    // 设置优先级选项
    ui->priorityComboBox->clear();
    ui->priorityComboBox->addItem("🟢 低", Task::Low);
    ui->priorityComboBox->addItem("🟡 中", Task::Medium);
    ui->priorityComboBox->addItem("🔴 高", Task::High);

    // 设置分类选项
    ui->categoryComboBox->clear();
    ui->categoryComboBox->addItem("📁 未分类");
    ui->categoryComboBox->addItem("💼 工作");
    ui->categoryComboBox->addItem("📚 学习");
    ui->categoryComboBox->addItem("🏠 生活");

    // 设置日期范围
    ui->deadlineDateTimeEdit->setMinimumDateTime(QDateTime::currentDateTime());

    // 设置默认日期为明天
    ui->deadlineDateTimeEdit->setDateTime(QDateTime::currentDateTime().addDays(1));

    // 设置对话框大小
    resize(500, 400);
}

void TaskEditDialog::connectSignals()
{
    connect(ui->saveButton, &QPushButton::clicked, this, &TaskEditDialog::onSaveClicked);
    connect(ui->cancelButton, &QPushButton::clicked, this, &TaskEditDialog::onCancelClicked);
}

void TaskEditDialog::setTaskData(const Task& task)
{
    ui->titleLineEdit->setText(task.getTitle());
    ui->descriptionTextEdit->setPlainText(task.getDescription());
    ui->priorityComboBox->setCurrentIndex(task.getPriority());
    ui->deadlineDateTimeEdit->setDateTime(task.getDeadline());
    ui->categoryComboBox->setCurrentText(task.getCategory());
}

Task TaskEditDialog::getTaskData() const
{
    Task task;
    task.setTitle(ui->titleLineEdit->text());
    task.setDescription(ui->descriptionTextEdit->toPlainText());
    task.setPriority(static_cast<Task::Priority>(
        ui->priorityComboBox->currentData().toInt()));
    task.setDeadline(ui->deadlineDateTimeEdit->dateTime());
    task.setCategory(ui->categoryComboBox->currentText());
    return task;
}

void TaskEditDialog::onSaveClicked()
{
    // 验证必填字段
    if (ui->titleLineEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "输入错误", "任务标题不能为空！");
        return;
    }

    accept(); // 关闭对话框并返回 Accepted
}

void TaskEditDialog::onCancelClicked()
{
    reject(); // 关闭对话框并返回 Rejected
}
