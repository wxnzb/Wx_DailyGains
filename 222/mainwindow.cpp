#include "mainwindow.h"
#include "ui_mainwindow.h"
#include"TaskManager.h"
#include"dialog.h"
#include<QMessageBox>
#include<QInputDialog>
#include<QSplitter>
#include"add.h"
#include <QGuiApplication>
#include <QScreen>
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    ,taskManager(new TaskManager(this))
    , m_isSearching(false)
{
    ui->setupUi(this);
    // 设置整体垂直布局拉伸
    ui->mainVerticalLayout->setStretch(0, 0); // 工具栏固定高度
    ui->mainVerticalLayout->setStretch(1, 1); // 内容区域拉伸
    ui->mainVerticalLayout->setStretch(2, 0); // 状态栏固定高度

    // 设置水平分栏拉伸
    ui->contentLayout->setStretch(0, 0); // 左侧边栏固定宽度
    ui->contentLayout->setStretch(1, 1); // 右侧内容拉伸

    // 设置表格列宽策略
    ui->taskTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch); // 任务列拉伸
    ui->taskTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->taskTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    ui->taskTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    //优化,添加中间分隔++++++++++++++++++++++++++++++++++++
    setupSplitter();
    //QLineEdit默认带有右键上下文菜单，现在禁用他
    ui->searchBox->setContextMenuPolicy(Qt::NoContextMenu);
    //添加快捷键
    setupShortcuts();
    //禁用右边任务展示是双击编辑作用，改成只读模式
    ui->taskTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    //让分类文字过长时可以显示省略号
    //配置 QListWidget 的文字省略
    //使用自定义 Delegate
    ui->categoryList->setItemDelegate(new ElidedTextDelegate(this));
    //隐藏水平滚动条
    ui->categoryList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    //优化：隐藏任务第一行123445行号
    ui->taskTable->verticalHeader()->setVisible(false);
    //进行连接
    //快捷试图
    connect(ui->quickViewList, &QListWidget::itemClicked,
            this, &MainWindow::onQuickViewClicked);
    //点击分类
    connect(ui->categoryList,&QListWidget::itemClicked,
            this, &MainWindow::onCategoryClicked);
    //新建分类
    connect(ui->addCategoryBtn, &QPushButton::clicked, this, &MainWindow::onAddCategoryClicked);
    // 连接删除任务按钮
    connect(ui->deleteTaskBtn, &QPushButton::clicked, this, &MainWindow::onDeleteTaskClicked);
    //连接任务完成按钮
    connect(ui->completeTaskBtn, &QPushButton::clicked, this, &MainWindow::onCompleteTaskClicked);
    //连接新建任务按钮
    connect(ui->newTaskBtn, &QPushButton::clicked, this, &MainWindow::onAddTaskClicked);
    //连接编辑按钮
    connect(ui->editTaskBtn, &QPushButton::clicked, this, &MainWindow::onEditTaskClicked);
    //连接搜索，目前还没有实现实时变动，怎样实现？？
    connect(ui->searchBox, &QLineEdit::returnPressed,this, &MainWindow::onSearchTriggered);
    //：连接搜索框的 textChanged 信号,当他变成空的时候就应该回复原来
    connect(ui->searchBox, &QLineEdit::textChanged, this, &MainWindow::onSearchTextChanged);
    // ui->searchBox->setClearButtonEnabled(true);
    //loadTasksFromManager();
    //添加隐形爹u列为了保存唯一id
    ui->taskTable->setColumnCount(5);
    ui->taskTable->setColumnHidden(4, true); // 隐藏第5列
    //加载数据, //仅在文件不存在时创建示例数据,这样可以实现假数据的持久化
    if (!taskManager->loadFromFile("tasks.json")){
        qDebug() << "首次运行，创建示例数据";
        createSampleData();
    }
    populateCategoriesFromData();  // 加载分类列表
    updateStatusBar();              // 更新状态栏
    showAllTasks();                 // 默认显示全部任务

    qDebug() << "MainWindow 初始化完成";
}
void MainWindow::createSampleData()
{
    // 仅在首次运行时创建示例数据

    //未完成
    Task t1(
        "完成 Qt 课程设计",
        "实现 TaskManager + UI 绑定",
        Task::High,
        QDateTime::currentDateTime(),
        "学习",
        false
        );
    taskManager->addTask(t1);

    //未完成
    Task t2(
        "修复登录 Bug",
        "检查信号槽连接问题",
        Task::Medium,
        QDateTime::currentDateTime().addDays(2),
        "工作",
        false
        );
    taskManager->addTask(t2);

    //已完成
    Task t3(
        "提交实验报告",
        "实验三已完成",
        Task::Low,
        QDateTime::currentDateTime().addDays(-1),
        "学习",
        true
        );
    taskManager->addTask(t3);

    // 保存到文件
    if (taskManager->saveToFile("tasks.json")) {
        qDebug() << "示例数据已创建并保存";
    } else {
        qDebug() << "示例数据保存失败";
    }
}
MainWindow::~MainWindow()
{
    delete ui;
}
// QString getTitle() const;
// QString getDescription() const;
// Priority getPriority() const;
// QDateTime getDeadline() const;
// QString getCategory() const;
// QString getId()const;
// MainWindow.cpp


//刷新
void MainWindow::refreshCurrentView()
{
    //优化：保存当前选中的任务 ID，点击完成后不丢失焦点
    QString selectedTaskId = getCurrentSelectedTaskId();
    // 获取当前选中的快捷视图项
    QListWidgetItem* quickViewItem = ui->quickViewList->currentItem();

    // 获取当前选中的分类项
    QListWidgetItem* categoryItem = ui->categoryList->currentItem();

    if (quickViewItem) {
        // 如果选中了快捷视图，重新触发点击事件
        onQuickViewClicked(quickViewItem);
        qDebug() << "刷新快捷视图:" << quickViewItem->text();
    }
    else if (categoryItem) {
        // 如果选中了分类，重新触发点击事件
        onCategoryClicked(categoryItem);
        qDebug() << "刷新分类视图:" << categoryItem->text();
    }
    else {
        // 如果都没选中，默认显示全部任务
        showAllTasks();
        qDebug() << "刷新为全部任务";
    }
    //恢复点中状态哦
    selectTaskById(selectedTaskId);
}

// 界面展示
void MainWindow::renderTasks(const QList<Task>& tasks) {
    // 清空表格
    ui->taskTable->setRowCount(0);
    for (const Task& task : tasks) {
        // int row = ui->taskTable->rowCount();
        // ui->taskTable->insertRow(row);
        //添加任务描述行
        int row = ui->taskTable->rowCount();
        int descRow = row + 1;

        // 插入 2 行
        ui->taskTable->insertRow(row);
        ui->taskTable->insertRow(descRow);
        //标题列
        QTableWidgetItem* titleItem = new QTableWidgetItem(task.getTitle());
        //已完成任务添加删除线和灰色字体
        if (task.getStatus()) {
            QFont font = titleItem->font();
            font.setStrikeOut(true);
            titleItem->setFont(font);
            titleItem->setForeground(QColor(128, 128, 128));
        }
        ui->taskTable->setItem(row, 0, titleItem);

        //优先级列（映射为文字+颜色）
        QTableWidgetItem* priorityItem = new QTableWidgetItem;
        QString priorityStr = task.toString();
        priorityItem->setText(priorityStr);
        //优先级颜色区分
        if (task.getPriority() == Task::High) {
            priorityItem->setForeground(QColor(255, 0, 0));
        } else if (task.getPriority() == Task::Medium) {
            priorityItem->setForeground(QColor(255, 165, 0));
        } else {
            priorityItem->setForeground(QColor(0, 255, 0));
        }
        ui->taskTable->setItem(row, 1, priorityItem);

        //截止日期列
        QTableWidgetItem* deadlineItem = new QTableWidgetItem;
        if (task.getDeadline().isValid()) {
            deadlineItem->setText(task.getDeadline().toString("yyyy-MM-dd"));
        } else {
            deadlineItem->setText("无");
        }
        ui->taskTable->setItem(row, 2, deadlineItem);

        //分类列
        QTableWidgetItem* categoryItem = new QTableWidgetItem(task.getCategory());
        ui->taskTable->setItem(row, 3, categoryItem);

        //隐藏的ID列
        QTableWidgetItem* idItem = new QTableWidgetItem(task.getId());
        ui->taskTable->setItem(row, 4, idItem);
        //展示任务描述-------------------------
        //创建描述
        QString description = task.getDescription();
        if (description.isEmpty()) {
            description = "（无描述）";
        }

        //只在第一列创建项
        QTableWidgetItem* descItem = new QTableWidgetItem(description);

        //设置描述行的样式
        descItem->setForeground(QColor(100, 100, 100));  // 灰色字体
        QFont descFont = descItem->font();
        descFont.setItalic(true);     // 斜体
        descFont.setPointSize(descFont.pointSize() - 1);  // 字体小一号
        descItem->setFont(descFont);
        //设置描述行的高度
        // ui->taskTable->setRowHeight(descRow, 10);
        //设置背景色，暂时用默认
        //descItem->setBackground(QColor(100, 245, 245));

        ui->taskTable->setItem(descRow, 0, descItem);

        //合并单元格
        ui->taskTable->setSpan(descRow, 0, 1, 4);  // 参数：行，列，行数，列数

        //描述行的隐藏ID列（用于保持一致性）
        // QTableWidgetItem* descIdItem = new QTableWidgetItem(task.getId());
        // ui->taskTable->setItem(descRow, 4, descIdItem);


        qDebug() << "渲染任务:" << task.getTitle() << "| 描述:" << description;
    }
}

//界面显示
//从数据中自动提取分类,显示分类
void MainWindow::populateCategoriesFromData()
{
    ui->categoryList->clear();
    QStringList categories = taskManager->getCategories();
    for (const QString& cat : categories) {
        ui->categoryList->addItem(cat);
    }
    qDebug() << "从TaskManager加载分类：" << categories;
}

//状态栏显示
void MainWindow::updateStatusBar()
{
    int total = taskManager->getTotalTaskCount();
    int completed = taskManager->getCompletedTaskCount();
    int todayDue = taskManager->getDueTodayTaskCount();


    QString statusText = tr("共 %1 项任务 | 已完成 %2 项 | 今日待办 %3 项")
                             .arg(total)
                             .arg(completed)
                             .arg(todayDue);

    ui->statusLabel->setText(statusText);

    qDebug() << "状态栏更新:" << statusText;
}
//左边
//-------------------------------------------------------
void MainWindow::onQuickViewClicked(QListWidgetItem *item)
{
    qDebug() << "用户点击了:" << item->text();

    // 根据点击的项显示不同的数据
    if (item->text().contains("全部任务")) {
        showAllTasks();
    } else if (item->text().contains("重要")) {
        showImportantTasks();
    } else if (item->text().contains("今日待办")) {
        showTodayTasks();
    }
}
// 显示所有任务
void MainWindow::showAllTasks()
{
    qDebug() << "显示所有任务";

    // 清空表格
    ui->taskTable->setRowCount(0);
    QList<Task> allTasks = taskManager->getAllTasks();
    renderTasks(allTasks);
    qDebug() << "所有任务显示完成";
}
// 显示重要任务
void MainWindow::showImportantTasks()
{
    qDebug() << "显示重要任务";

    // 清空表格
    ui->taskTable->setRowCount(0);

    // 只显示高优先级的任务
    QList<Task> highTasks = taskManager->getTaskByPriority(Task::High);
    renderTasks(highTasks);

    qDebug() << "重要任务显示完成";
}
// 显示今日待办任务
void MainWindow::showTodayTasks()
{
    qDebug() << "显示今日待办任务";

    // 清空表格
    ui->taskTable->setRowCount(0);

    // 获取今天的日期
    QString today = QDate::currentDate().toString("yyyy-MM-dd");

    // 只显示今天到期的任务
    QList<Task> todayTasks = taskManager->getDueTodayTasks();
    renderTasks(todayTasks);
    qDebug() << "今日待办任务显示完成，显示了" << ui->taskTable->rowCount() << "个任务";
}
//开始写分类

//-------------------------------------------------------
void MainWindow::onCategoryClicked(QListWidgetItem *item)
{
    QString categoryName = item->text();
    qDebug() << "用户点击了分类:" << categoryName;
    showTasksByCategory(categoryName);
}

// 同时确保 showTasksByCategory 函数也存在
void MainWindow::showTasksByCategory(const QString &category)
{
    qDebug() << "按分类显示:" << category;
    ui->taskTable->setRowCount(0);

    QList<Task> categoryTasks = taskManager->getTaskByCategory(category);
    renderTasks(categoryTasks);
    qDebug() << category << "分类任务显示完成，显示了" << ui->taskTable->rowCount() << "个任务";
}
//-------------------------------------------------------
//新建分类
void MainWindow::onAddCategoryClicked()
{
    // 弹出一个输入对话框，让用户输入新分类的名称
    bool ok;
    QString categoryName = QInputDialog::getText(this, // 父窗口
                                                 tr("新建分类"),   // 对话框标题
                                                 tr("请输入分类名称:"), // 提示文字
                                                 QLineEdit::Normal, // 输入模式
                                                 "", // 默认文本
                                                 &ok // 用于接收用户点击了“确定”还是“取消”
                                                 );

    // 如果用户点击了“确定”并且输入不为空
    if (ok && !categoryName.trimmed().isEmpty()) {
        addCategory(categoryName.trimmed());
    }
}

// 添加分类函数
void MainWindow::addCategory(const QString &categoryName)
{
    // 先检查 TaskManager 中是否已存在
    if (taskManager->isHasCategory(categoryName)) {
        QMessageBox::information(this, tr("提示"),
                                 tr("分类 \"%1\" 已存在！").arg(categoryName));
        return;
    }

    // 添加到 TaskManager
    taskManager->addCategory(categoryName);

    // 保存到文件
    if (taskManager->saveToFile("tasks.json")) {
        qDebug() << "分类已保存到文件";
    } else {
        qDebug() << "分类保存失败";
    }

    // 刷新 UI 的分类列表
    populateCategoriesFromData();

    // 显示成功提示
    ui->statusbar->showMessage(tr("分类 \"%1\" 已添加").arg(categoryName), 3000);
    qDebug() << "成功添加新分类:" << categoryName;
}
//用id来进行判断!!!!!!!!!!!!!!!!!!
//删除任务或者选中分类里面的所有任务
//应该首先判断任务是否被选中，要是被选中，那么直接删除就好了，要是任务没被选中，那么就看是否分类被选中，那么就可以直接删除，因为任务选中一定是你出任务，没有选中才删除分类
void MainWindow::onDeleteTaskClicked()
{
    QListWidgetItem* selectedCategory = ui->categoryList->currentItem();


    //情况：选中了任务
    int selectedRow = ui->taskTable->currentRow();
    //没有选中任务
    if (selectedRow == -1) {
        if (selectedCategory != nullptr) {
            // 情况1：分类列表有选中项 → 删除分类
            qDebug() << "当前选中了分类，准备删除分类";
            deleteCategoryAndTasks();
            return;
        }else{
            QMessageBox::warning(this, "提示", "请先选中要删除的任务或者分类！！！！");
            return;
        }
    }
    //这里要是点击了描述，就-1返回任务
    selectedRow=getTaskRowFromTableRow(selectedRow);
    // 获取任务信息
    QString taskId = ui->taskTable->item(selectedRow, 4)->text();
    QString taskName = ui->taskTable->item(selectedRow, 0)->text();

    // 二次确认
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "确认删除",
        QString("确定要删除任务 \"%1\" 吗？").arg(taskName),
        QMessageBox::Yes | QMessageBox::No
        );

    if (reply != QMessageBox::Yes) {
        return;
    }

    // 删除任务
    if (taskManager->removeTask(taskId)) {
        //保存到文件
        taskManager->saveToFile("tasks.json");

        // 刷新当前视图（而不是重新加载所有数据）
        refreshCurrentView();

        // 更新状态栏
        updateStatusBar();

        ui->statusbar->showMessage(QString("任务 \"%1\" 已删除").arg(taskName), 3000);
        qDebug() << "已删除任务 ID:" << taskId;
    } else {
        QMessageBox::critical(this, "错误", "任务删除失败！");
    }
}
void  MainWindow::deleteCategoryAndTasks(){
    QListWidgetItem* selectedCategory = ui->categoryList->currentItem();
    QString categoryName = selectedCategory->text();
    qDebug() << "准备删除分类:" << categoryName;

    //获取该分类下的任务数量
    QList<Task> categoryTasks = taskManager->getTaskByCategory(categoryName);
    int taskCount = categoryTasks.size();
    QString confirmMessage;
    if (taskCount > 0) {
        confirmMessage = QString(
                             "确定要删除分类 \"%1\" 吗？\n\n"
                             "警告：该分类下有 %2 个任务，\n"
                             "删除分类后，这些任务也会一并删除！\n\n"
                             "此操作不可恢复！"
                             ).arg(categoryName).arg(taskCount);
    } else {
        confirmMessage = QString(
                             "确定要删除分类 \"%1\" 吗？\n\n"
                             "该分类下没有任务。"
                             ).arg(categoryName);
    }

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "确认删除分类",
        confirmMessage,
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
        );

    if (reply != QMessageBox::Yes) {
        qDebug() << "用户取消了删除分类操作";
        return;
    }

    //删除该分类下的所有任务
    for (const Task& task : categoryTasks) {
        taskManager->removeTask(task.getId());
        qDebug() << "已删除任务:" << task.getTitle();
    }
    //删除分类
    taskManager->removeCategory(categoryName);
    qDebug() << "分类已删除:" << categoryName;
    //保存到文件
    taskManager->saveToFile("tasks.json");
    qDebug() << "更改已保存到文件";
    // 刷新分类列表
    populateCategoriesFromData();
    //默认显示全部任务
    showAllTasks();
    // 更新状态栏
    updateStatusBar();
    //显示成功提示
    QString successMessage = QString(
                                 "分类 \"%1\" 已删除\n"
                                 "共删除了 %2 个任务"
                                 ).arg(categoryName).arg(taskCount);

    ui->statusbar->showMessage(
        QString("分类 \"%1\" 已删除（共 %2 个任务）")
            .arg(categoryName).arg(taskCount),
        5000
        );

    QMessageBox::information(this, "删除成功", successMessage);

    qDebug() << "删除分类成功 -" << categoryName << "| 删除了" << taskCount << "个任务";

}
//任务完成
void MainWindow::onCompleteTaskClicked()
{ int selectedRow = ui->taskTable->currentRow();
    if (selectedRow == -1) {
        QMessageBox::warning(this, "提示", "请先选中要标记的任务！");
        return;
    }
    //这里要是点击了描述，就-1返回任务
    selectedRow=getTaskRowFromTableRow(selectedRow);

    QString taskId = ui->taskTable->item(selectedRow, 4)->text();
    QString taskName = ui->taskTable->item(selectedRow, 0)->text();

    Task* task = taskManager->getTask(taskId);
    if (!task) {
        QMessageBox::critical(this, "错误", "未找到该任务！");
        return;
    }

    bool newStatus = !task->getStatus();
    task->setStatus(newStatus);

    // 更新任务
    if (taskManager->updateTask(taskId, *task)) {
        // 保存到文件
        taskManager->saveToFile("tasks.json");

        // 刷新当前视图
        refreshCurrentView();

        //更新状态栏
        updateStatusBar();

        // 显示提示信息
        QString message = newStatus
                              ? QString("任务 \"%1\" 已完成").arg(taskName)
                              : QString("任务 \"%1\" 已取消完成").arg(taskName);

        ui->statusbar->showMessage(message, 3000);
        qDebug() << message;
    } else {
        QMessageBox::critical(this, "错误", "任务更新失败！");
    }
}
//新建任务实现
void MainWindow::onAddTaskClicked()
{
    qDebug() << "用户点击了新建任务按钮";

    // // 创建分类需要用自己的
    // QStringList categories;
    // for (int i = 0; i < ui->categoryList->count(); ++i) {
    //     QListWidgetItem* item = ui->categoryList->item(i);
    //     categories.append(item->text());
    // }
    // // 创建对话框时传入分类列表
    // Dialog dialog(categories, this);

    // // 显示对话框（模态）
    // if (dialog.exec()==QDialog::Accepted){
    //     //获取对话框中的数据
    //     //在diallog中进行定义
    //     QString title = dialog.getTitle();
    //     QString category = dialog.getCategory();
    //     Task::Priority priority = dialog.getPriorityEnum();
    //     QDateTime deadline = dialog.getDeadline();
    //     QString description = dialog.getDescription();
    //     if (title.isEmpty()) {
    //         QMessageBox::warning(this, "提示", "任务标题不能为空！");
    //         return;
    //     }
    //     //将他保存在文件中并进行显示
    //     Task newTask(
    //         title,
    //         description,
    //         priority,
    //         deadline,
    //         category,
    //         false  // 新建任务默认未完成
    //         );
    //     QString newTaskId = taskManager->addTask(newTask);


    //     //保存到文件
    //     taskManager->saveToFile("tasks.json");

    //     //刷新当前视图
    //     refreshCurrentView();

    //     //更新状态栏
    //     updateStatusBar();

    //     //显示成功提示
    //     ui->statusbar->showMessage(
    //         QString("任务 \"%1\" 已创建").arg(title),
    //         3000
    //         );

    //     qDebug() << "新任务已创建:" << title;

    // }
    //    qDebug() << "对话框已关闭";
    //-----------------------------------------
    // //创建 add 窗口
    // add *addDialog = new add(this);
    //修改：不设置父窗口（nullptr），让 add 窗口独立显示
    add *addDialog = new add(nullptr);
    //显示窗口
    addDialog->show();
    qDebug() << "add界面显示";
}
//编辑
void MainWindow::onEditTaskClicked()
{
    qDebug() << "用户点击了编辑任务按钮";

    // 获取当前选中的行
    int selectedRow = ui->taskTable->currentRow();
    if (selectedRow == -1) {
        QMessageBox::warning(this, "提示",
                             "请先选中要编辑的任务！\n\n"
                             "提示：点击任务列表中的某一行即可选中。");
        return;
    }
    //这里要是点击了描述，就-1返回任务
    selectedRow=getTaskRowFromTableRow(selectedRow);
    // 获取任务ID
    QTableWidgetItem* idItem = ui->taskTable->item(selectedRow, 4);
    if (!idItem) {
        QMessageBox::critical(this, "错误", "无法获取任务ID！");
        return;
    }
    QString taskId = idItem->text();
    qDebug() << "正在编辑任务，ID:" << taskId;

    // 从 TaskManager 获取任务对象
    Task* task = taskManager->getTask(taskId);
    if (!task) {
        QMessageBox::critical(this, "错误", "未找到该任务！");
        return;
    }

    //获取当前所有分类
    QStringList categories;
    for (int i = 0; i < ui->categoryList->count(); ++i) {
        categories.append(ui->categoryList->item(i)->text());
    }

    //检查是否有分类
    if (categories.isEmpty()) {
        QMessageBox::warning(this, "提示",
                             "请先创建至少一个分类！\n\n"
                             "提示：点击左侧的 ➕ 按钮可以新建分类。");
        return;
    }

    //创建编辑对话框
    Dialog dialog(categories, this);
    dialog.setWindowTitle("编辑任务");  // 修改窗口标题

    //预填充现有数据（这是关键！）
    dialog.setTitle(task->getTitle());
    dialog.setCategory(task->getCategory());
    dialog.setPriority(task->getPriority());
    dialog.setDeadline(task->getDeadline());
    dialog.setDescription(task->getDescription());

    qDebug() << "对话框已预填充数据 - 标题:" << task->getTitle();

    //显示对话框（模态）
    if (dialog.exec() == QDialog::Accepted) {

        //获取用户修改后的数据
        QString newTitle = dialog.getTitle();

        // 验证标题不为空
        if (newTitle.isEmpty()) {
            QMessageBox::warning(this, "提示", "任务标题不能为空！");
            return;
        }

        //更新任务对象的数据
        task->setTitle(newTitle);
        task->setCategory(dialog.getCategory());
        task->setPriority(dialog.getPriorityEnum());
        task->setDeadline(dialog.getDeadline());
        task->setDescription(dialog.getDescription());

        //更新到 TaskManager
        if (taskManager->updateTask(taskId, *task)) {

            //保存到文件
            if (taskManager->saveToFile("tasks.json")) {
                qDebug() << "任务已保存到文件";
            } else {
                qDebug() << "任务保存失败";
            }

            //刷新当前视图
            refreshCurrentView();

            //更新状态栏
            updateStatusBar();

            // 显示成功提示
            ui->statusbar->showMessage(
                QString("任务 \"%1\" 已更新").arg(newTitle),
                3000
                );

            qDebug() << "任务已更新 - ID:" << taskId << "| 新标题:" << newTitle;

        } else {
            QMessageBox::critical(this, "错误", "任务更新失败！");
            qDebug() << "任务更新失败 - ID:" << taskId;
        }
    } else {
        qDebug() << "用户取消了编辑";
    }
}
//搜素框实现
void MainWindow::onSearchTriggered()
{
    //.trimmed去掉字符串两端的空白字符
    QString keyword = ui->searchBox->text().trimmed();

    if (keyword.isEmpty()) {
        QMessageBox::information(this, "提示", "请输入搜索关键词");
        return;
    }
    //优化：搜索前，保存当前视图
    if (!m_isSearching) {
        saveCurrentView();
        m_isSearching = true;
    }
    qDebug() << "开始搜索关键词:" << keyword;
    performSearch(keyword);
}


//执行搜索
void MainWindow::performSearch(const QString& keyword)
{
    if (keyword.isEmpty()) {
        return;
    }

    //获取所有任务
    QList<Task> allTasks = taskManager->getAllTasks();

    //筛选包含关键词的任务
    QList<Task> matchedTasks;

    for (const Task& task : allTasks) {
        // Qt::CaseInsensitive设置成不区分大小写的匹配
        //在标题中搜索
        bool titleMatch = task.getTitle().contains(keyword, Qt::CaseInsensitive);

        // 在描述中搜索
        bool descMatch = task.getDescription().contains(keyword, Qt::CaseInsensitive);

        // 如果标题或描述中包含关键词，就添加到结果中
        if (titleMatch || descMatch) {
            matchedTasks.append(task);

            // 调试信息：显示匹配位置
            if (titleMatch) {
                qDebug() << "标题匹配:" << task.getTitle();
            }
            if (descMatch) {
                qDebug() << "描述匹配:" << task.getDescription();
            }
        }
    }

    //显示搜索结果
    qDebug() << "搜索完成，找到" << matchedTasks.size() << "个匹配任务";

    // 清空表格并显示结果
    ui->taskTable->setRowCount(0);
    renderTasks(matchedTasks);

    //清空左侧选中状态，防止看到左侧点击的时候出现的是搜索的结果
    //清除快捷试图
    ui->quickViewList->clearSelection();
    //清除分类列表
    ui->categoryList->clearSelection();
    //对于分类试用
    // m_currentSelectedCategory.clear();

    // 5. 在状态栏显示搜索结果统计
    QString statusMessage = QString("搜索 \"%1\"：找到 %2 个匹配任务")
                                .arg(keyword)
                                .arg(matchedTasks.size());
    ui->statusbar->showMessage(statusMessage, 5000);

    //如果没有找到任何匹配
    if (matchedTasks.isEmpty()) {
        QMessageBox::information(this, "搜索结果",
                                 QString("未找到包含 \"%1\" 的任务\n\n"
                                         "搜索范围：任务标题和详细描述").arg(keyword));
    }
}
//优化
void MainWindow::setupSplitter()
{
    // 创建 QSplitter
    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setChildrenCollapsible(false);  // 防止拖过头导致隐藏
    splitter->setHandleWidth(1);  // 分割线宽度

    // 将左右控件从原布局中移除
    ui->contentLayout->removeWidget(ui->leftSidebarWidget);
    ui->contentLayout->removeWidget(ui->rightContentWidget);

    // 添加到 QSplitter
    splitter->addWidget(ui->leftSidebarWidget);
    splitter->addWidget(ui->rightContentWidget);

    //设置初始大小比例（左侧：右侧 = 1:3）
    splitter->setStretchFactor(0, 1);  // 左侧占 1 份
    splitter->setStretchFactor(1, 3);  // 右侧占 3 份

    //设置左侧的宽度范围
    ui->leftSidebarWidget->setMinimumWidth(150);   // 最小宽度 150px
    ui->leftSidebarWidget->setMaximumWidth(500);   // 最大宽度 400px

    //将 QSplitter 添加到主布局
    ui->contentLayout->addWidget(splitter);

    qDebug() << "分割线已设置";
}
//设置快捷键
void MainWindow::setupShortcuts()
{
    qDebug() << "开始设置快捷键";

    //Ctrl+N：新建任务
    QShortcut *newTaskShortcut = new QShortcut(QKeySequence("Ctrl+N"), this);
    connect(newTaskShortcut, &QShortcut::activated, this, &MainWindow::onAddTaskClicked);
    qDebug() << "Ctrl+N → 新建任务";

    //Delete：删除任务/分类
    QShortcut *deleteShortcut = new QShortcut(QKeySequence("Delete"), this);
    connect(deleteShortcut, &QShortcut::activated, this, &MainWindow::onDeleteTaskClicked);
    qDebug() << "Delete → 删除任务";

    //Ctrl+F：聚焦搜索框
    QShortcut *searchShortcut = new QShortcut(QKeySequence("Ctrl+F"), this);
    connect(searchShortcut, &QShortcut::activated, [=]() {
        ui->searchBox->setFocus();       // 聚焦到搜索框
        ui->searchBox->selectAll();      // 全选已有文字
        qDebug() << "Ctrl+F 触发 → 聚焦到搜索框";
    });


    //Ctrl+E：编辑任务
    QShortcut *editShortcut = new QShortcut(QKeySequence("Ctrl+E"), this);
    connect(editShortcut, &QShortcut::activated, this, &MainWindow::onEditTaskClicked);
    qDebug() << "Ctrl+E";

    // Ctrl+D / Space：标记任务完成
    QShortcut *completeShortcut1 = new QShortcut(QKeySequence("Ctrl+D"), this);
    connect(completeShortcut1, &QShortcut::activated, this, &MainWindow::onCompleteTaskClicked);
    qDebug() << "Ctrl+D";

    QShortcut *completeShortcut2 = new QShortcut(QKeySequence("Space"), this);
    connect(completeShortcut2, &QShortcut::activated, this, &MainWindow::onCompleteTaskClicked);
    qDebug() << "Space";
    //Esc：清空搜索框
    QShortcut *escShortcut = new QShortcut(QKeySequence("Esc"), this);
    connect(escShortcut, &QShortcut::activated, [=]() {
        if (!ui->searchBox->text().isEmpty()) {
            ui->searchBox->clear();            // 清空搜索框
            refreshCurrentView();              // 恢复默认视图
            qDebug() << "Esc";
        }
    });

    //Ctrl+Q：退出程序
    QShortcut *quitShortcut = new QShortcut(QKeySequence("Ctrl+Q"), this);
    connect(quitShortcut, &QShortcut::activated, this, &QMainWindow::close);
    qDebug() << "Ctrl+Q";

    qDebug() << "快捷键设置完成！";
}
//实现添加描述行
int MainWindow::getTaskRowFromTableRow(int tableRow)const {
    if (tableRow % 2 == 0) {
        return tableRow;
    } else {
        return tableRow - 1;
    }
}
//获取当前选中的任务 ID
QString MainWindow::getCurrentSelectedTaskId() const
{
    int selectedRow = ui->taskTable->currentRow();

    // 获取任务行
    int taskRow = getTaskRowFromTableRow(selectedRow);

    // 获取隐藏列的任务 ID
    QTableWidgetItem* idItem = ui->taskTable->item(taskRow, 4);

    if (idItem) {
        return idItem->text();
    }

    return QString();
}

//根据任务 ID 重新选中行
void MainWindow::selectTaskById(const QString& taskId)
{
    if (taskId.isEmpty()) {
        return;  // 没有可选中的任务
    }

    // 遍历表格，找到对应的任务
    for (int row = 0; row < ui->taskTable->rowCount(); row += 2) {
        // 只检查任务行，跳过描述行,因为你是根据任务id
        QTableWidgetItem* idItem = ui->taskTable->item(row, 4);

        if (idItem && idItem->text() == taskId) {
            // 找到了,重新选中这一行
            ui->taskTable->setCurrentCell(row, 0);
            qDebug() << "已重新选中任务;";
            return;
        }
    }

    qDebug() << "未找到任务";
}
//实现就是搜索框没内容时应该回复原来的样子
// 监听搜索框内容变化
void MainWindow::onSearchTextChanged(const QString& text)
{
    qDebug() << "搜索框内容变化:" << text << "| 当前是否在搜索:" << m_isSearching;

    // 如果搜索框变成空，并且之前处于搜索状态
    if (text.trimmed().isEmpty()) {
        qDebug() << "检测到搜索框清空，准备恢复之前的视图";
        // 恢复搜索前的视图
        restoreViewBeforeSearch();
        // 标记搜索结束
        m_isSearching = false;

        // 清空状态栏的搜索提示
        updateStatusBar();

        qDebug() << "已恢复到搜索前的视图";
    }
}

//保存当前视图状态
void MainWindow::saveCurrentView()
{
    // 获取当前选中的快捷视图项
    QListWidgetItem* quickViewItem = ui->quickViewList->currentItem();

    // 获取当前选中的分类项
    QListWidgetItem* categoryItem = ui->categoryList->currentItem();

    if (quickViewItem) {
        // 保存快捷视图
        m_viewBeforeSearch = "quickView:" + quickViewItem->text();
        qDebug() << "保存视图状态 - 快捷视图:" << quickViewItem->text();
    }
    else if (categoryItem) {
        // 保存分类
        m_viewBeforeSearch = "category:" + categoryItem->text();
        qDebug() << "保存视图状态 - 分类:" << categoryItem->text();
    }
}
//恢复搜索前的视图
void MainWindow::restoreViewBeforeSearch()
{
    if (m_viewBeforeSearch.isEmpty()) {
        // 如果没有保存的视图，默认显示全部任务
        showAllTasks();
        qDebug() << "没有保存的视图，显示全部任务";
        return;
    }

    qDebug() << "准备恢复视图:" << m_viewBeforeSearch;

    if (m_viewBeforeSearch.startsWith("quickView:")) {
        // 恢复快捷视图
        QString viewName = m_viewBeforeSearch.mid(10);  // 去掉 "quickView:" 前缀

        // 在快捷视图列表中找到对应的项
        for (int i = 0; i < ui->quickViewList->count(); ++i) {
            QListWidgetItem* item = ui->quickViewList->item(i);
            if (item->text() == viewName) {
                // 重新选中这一项
                ui->quickViewList->setCurrentItem(item);

                // 触发点击事件
                onQuickViewClicked(item);

                qDebug() << "已恢复快捷视图:" << viewName;
                return;
            }
        }
    }
    else if (m_viewBeforeSearch.startsWith("category:")) {
        // 恢复分类视图
        QString categoryName = m_viewBeforeSearch.mid(9);  // 去掉 "category:" 前缀

        // 在分类列表中找到对应的项
        for (int i = 0; i < ui->categoryList->count(); ++i) {
            QListWidgetItem* item = ui->categoryList->item(i);
            if (item->text() == categoryName) {
                // 重新选中这一项
                ui->categoryList->setCurrentItem(item);

                // 触发点击事件
                onCategoryClicked(item);

                qDebug() << "已恢复分类视图:" << categoryName;
                return;
            }
        }
    }
}
