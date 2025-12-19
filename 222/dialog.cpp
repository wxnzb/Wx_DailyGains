#include "dialog.h"
#include "qpushbutton.h"
#include "ui_dialog.h"

Dialog::Dialog(const QStringList& categories,QWidget *parent)
    : QDialog(parent)
    , m_categories(categories)
    , ui(new Ui::Dialog)
{
    ui->setupUi(this);
    // 设置窗口标题
    setWindowTitle("新建任务,haha");

    // 设置窗口大小
    resize(500, 400);
    //先设置垂直布局
    QVBoxLayout* mainLayout=new QVBoxLayout(this);
    //每个间距是15
    mainLayout->setSpacing(15);
    //布局与父窗口的间隙
    mainLayout->setContentsMargins(20,20,20,20);
    //基本信息框:分组框控件
    QGroupBox* basicGroup=new QGroupBox("基本信息",this);
    //后面添加到basicLayout的都实在basicGroup中
    QFormLayout* basicLayout=new QFormLayout(basicGroup);
    //它内部控件航宇航间距
    basicLayout->setSpacing(10);
    //ok,开始写信息
    //标题:使用单行文本输入框
    titleEdit=new QLineEdit(this);
    //输入为空是进行提示
    titleEdit->setPlaceholderText("请输入文本标题");
    //设置标题最小啊高度
    titleEdit->setMinimumHeight(35);
    //优化：禁用右键出菜单
    titleEdit->setContextMenuPolicy(Qt::NoContextMenu);
    basicLayout->addRow("任务标题",titleEdit);
    //分类：使用下拉选择框
    categoryCombo = new QComboBox(this);
    //这个需要自己进行设置
    if (m_categories.isEmpty()) {
        // 如果没有分类，显示提示
        categoryCombo->addItem("请先创建分类");
    } else {
        // 添加所有已有的分类
        categoryCombo->addItems(m_categories);
    }
    categoryCombo->setMinimumHeight(35);
    basicLayout->addRow("任务分类:", categoryCombo);
    //优先级：使用下拉选择框
    priorityCombo = new QComboBox(this);
    priorityCombo->addItem("高");
    priorityCombo->addItem("中");
    priorityCombo->addItem("低");
    //设置展现出来是“中”
    priorityCombo->setCurrentIndex(1);
    priorityCombo->setMinimumHeight(35);
    basicLayout->addRow("优先级:", priorityCombo);
    //截止日期：
    //创建日期选择器
    deadlineEdit=new QDateTimeEdit(this);
    //创建编辑日期的控件:默认当前日期往后推一天
    deadlineEdit->setDateTime(QDateTime::currentDateTime().addDays(1));
    //设置格式
    deadlineEdit->setDisplayFormat("yyyy-MM-dd");
    //弹出日历
    deadlineEdit->setCalendarPopup(true);
    deadlineEdit->setMinimumHeight(35);
    basicLayout->addRow("截止日期:", deadlineEdit);
    //进行显示：将basicGroup布局添加到住窗口垂直布局中
    mainLayout->addWidget(basicGroup);
    //----------------------------------------------------
   //详细描述:需要多行文本框
    QGroupBox* descGroup = new QGroupBox("详细描述", this);
    QVBoxLayout* descLayout = new QVBoxLayout(descGroup);
    descriptionEdit=new QTextEdit(this);
    descriptionEdit->setPlaceholderText("请输入任务的详细描述（可选）");
    descriptionEdit->setMinimumHeight(100);
    descLayout->addWidget(descriptionEdit);
    mainLayout->addWidget(descGroup);
    descriptionEdit->setContextMenuPolicy(Qt::NoContextMenu);
    //----------------------------
    //添加按钮
    buttonBox=new QDialogButtonBox(
        QDialogButtonBox::Ok|QDialogButtonBox::Cancel,this);
    buttonBox->button(QDialogButtonBox::Ok)->setText("保存");
    buttonBox->button(QDialogButtonBox::Cancel)->setText("取消");
    //通过在mainwin中判断来将进行处理
    //他们都返回QDialog::Accepted或者QDialog::Rejected
    connect(buttonBox, &QDialogButtonBox::accepted, this, &Dialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &Dialog::reject);
     mainLayout->addWidget(buttonBox);
}

Dialog::~Dialog()
{
    delete ui;
}
//主要是新建要用
// 获取任务标题
QString Dialog::getTitle() const
{
    return titleEdit->text().trimmed();  // 去除首尾空格
}

// 获取任务分类
QString Dialog::getCategory() const
{
    return categoryCombo->currentText();
}

// 获取优先级（字符串形式）
QString Dialog::getPriority() const
{
    return priorityCombo->currentText();  // 返回 "高"、"中"、"低"
}

// 获取优先级（枚举形式，用于创建 Task 对象）
Task::Priority Dialog::getPriorityEnum() const
{
    QString priorityText = priorityCombo->currentText();

    if (priorityText == "高") {
        return Task::High;
    } else if (priorityText == "中") {
        return Task::Medium;
    } else {
        return Task::Low;
    }
}

// 获取截止日期
QDateTime Dialog::getDeadline() const
{
    return deadlineEdit->dateTime();
}

// 获取详细描述
QString Dialog::getDescription() const
{
    return descriptionEdit->toPlainText().trimmed();
}
//开始进行编辑
void  Dialog::setTitle(const QString& title){
    titleEdit->setText(title);
}
void  Dialog::setCategory(const QString& category){
    // 在下拉框中查找对应的分类
    int index = categoryCombo->findText(category);
    if (index >= 0) {
        categoryCombo->setCurrentIndex(index);
    } else {
        // 如果分类不存在，添加它
        categoryCombo->addItem(category);
        categoryCombo->setCurrentText(category);
    }
}
void  Dialog::setPriority(Task::Priority priority){
    switch (priority) {
    case Task::High:
        priorityCombo->setCurrentIndex(0);  // "高"
        break;
    case Task::Medium:
        priorityCombo->setCurrentIndex(1);  // "中"
        break;
    case Task::Low:
        priorityCombo->setCurrentIndex(2);  // "低"
        break;
    default:
        priorityCombo->setCurrentIndex(1);  // 默认"中"
        break;
    }
}
void  Dialog::setDeadline(const QDateTime& deadline){
    if (deadline.isValid()) {
        deadlineEdit->setDateTime(deadline);
    } else {
        // 如果日期无效，设置为明天
        deadlineEdit->setDateTime(QDateTime::currentDateTime().addDays(1));
    }
}
void  Dialog::setDescription(const QString& description){
    descriptionEdit->setPlainText(description);
}
