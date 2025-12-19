#include "add.h"
#include "ui_add.h"
//#include<QSvgWidget>
add::add(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::add)
{   //将颜色变为白色
    //setStyleSheet("add { background-color: white; }");
    ui->setupUi(this);
    // 设置窗口标题
    // setWindowTitle("新建任务");
    // 设置窗口大小
    resize(400, 563);
    setupUI();
}
add::~add()
{
    delete ui;
}
void add:: setupUI(){
    //新建任务，距离左边24，距离上边18+3--------------------------------
    //标题 "新建任务"
    titleLabel = new QLabel("新建任务", this);
    //距离左边，上边，控件大小[不太需要控件大小]
    //titleLabel->setGeometry(24, 21, 64, 22);
    titleLabel->move(24, 21);
    //设置字体信息
    // 设置字体
    //使用Microsoft YaHei UI
    QFont titleFont("Microsoft YaHei UI");
    //大小
    titleFont.setWeight(QFont::Bold);
    //应用
    titleLabel->setFont(titleFont);
    //请仔细填写以下信息---------------------------------------
    //首先创建背景容器
    QWidget *infoFrame=new QWidget(this);
    infoFrame->setGeometry(20,58,350,36);
    //启用自动填充背景
    infoFrame->setAutoFillBackground(true);
    //设置背景
    //accent_blue：暗色：#338CFF，亮色：#267EF0
    //10%透明度：19
    infoFrame->setStyleSheet("QWidget{"
                             //"background-color:#19338CFF;"
                             "background-color:rgba(51, 140, 255, 0.1);"
                             "border-radius:8px;"
                             "}"
                             );

    //添加图标
    QLabel* iconLabel=new QLabel(infoFrame);
    //相对于infoFrame:16,10;16*16
    iconLabel->setGeometry(16,10,16,16);
    //添加图标 Chevron.svg   Information.svg
    //C:/Users/Vivasweetwu/Desktop/Chevron.svg
    QString iconPath=":/../../Desktop/Information.svg";

    //创建一个svg渲染器并加载文件
    QSvgRenderer svgRender(iconPath);
    //创建一个16*16的一个画布并填充透明背景
    QPixmap pixmap(16, 16);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    svgRender.render(&painter);
    painter.end();
    iconLabel->setPixmap(pixmap);
    // QSvgWidget *svgWidget = new QSvgWidget;
    // svgWidget->load("C:/Users/Vivasweetwu/Desktop/Information.svg");
    //添加文字
    QLabel *infoTextLabel=new QLabel("请仔细填写以下信息",infoFrame);
    //40,8,184*20
    //infoTextLabel->setGeometry(40,8,184,20);不能用因为有双重背景
    // 只设置位置
    infoTextLabel->move(40, 8);

    // 根据文字内容自动调整大小
    infoTextLabel->adjustSize();
    QFont infoFont("Microsoft YaHei UI");
    //粗细
    infoFont.setWeight(QFont::Bold);
    infoTextLabel->setFont(infoFont);
    //设置字体颜色
    //accent_blue_darken_4: 暗色：#7BB5FF，亮色：#1B539A
    infoTextLabel->setStyleSheet("QLabel{"
                                 "color:#7BB5FF;"
                                 "}"
                                 );
    //基本信息------------------------------
    QLabel* sectionLabel=new QLabel("基本信息",this);
    //20,106
    sectionLabel->move(20, 106);
    QFont sectionFront("Microsoft YaHei UI");
    //设置粗细
    // Normal = 400,
    sectionFront.setWeight(QFont::Normal);
    sectionLabel->setFont(sectionFront);
    //设置颜色
    //base_gray_060": 暗色：#AEC4C7CC，亮色：#0B21A
    sectionLabel->setStyleSheet("QLabel{"
                                "color:#AEC4C7CC;"
                                "}"
                                );
    //主题------------------------------------
    QLabel* tasknameLabel=new QLabel("主题",this);
    //20，144
    tasknameLabel->move(20,144);
    QFont tasknameFont("Microsoft YaHei UI");
    //设置粗细
    //400
    tasknameFont.setWeight(QFont::Normal);
    tasknameLabel->setFont(tasknameFont);
    //设置颜色
    //base_gray_100:暗色：#EBEBEB,亮色：#10141A
    tasknameLabel->setStyleSheet("QLabel{"
                                 "color:#EBEBEB;"
                                 "}"
                                 );
    //设置文本、、、、、、、、、、、、、、、、
    QLineEdit * nameEdit=new QLineEdit(this);
    //82,138,298*32
    nameEdit->setGeometry(82,138,298,32);
    //设置占位符文本
    nameEdit->setPlaceholderText("文本");
    //文本距左边和上边和下边12，6,6
    //设置样式
    nameEdit->setStyleSheet(
        "QLineEdit {"
        //背景颜色， "bg_white_desktop_3":暗色： "#2C2C2D",亮色：#FFFFFF
        "    background-color:#2C2C2D;"
        // 圆角
        "    border-radius: 1px;"
        //距离左上下，12，6，6
        "    padding-left: 12px;"
        "    padding-top: 6px;"
        "    padding-bottom: 6px;"
        //文字颜色:??？？？
        "    font-family: 'Microsoft YaHei UI';"
        //字体粗细：Normal
        "    font-weight: 400;"
        "}"
        //占位符===============
        "QLineEdit::placeholder {"
        //占位符颜色：base_gray_040：暗色：#85ADB1B8，亮色：#09111A
        "    color: #85ADB1B8;"
        //占位符粗细：400
        "}"
        );
    //分类-------------------------------------
    QLabel* categoryLabel=new QLabel("分类",this);
    //20，192
    categoryLabel->move(20,192);
    QFont categoryFont("Microsoft YaHei UI");
    //设置粗细
    //400
    categoryFont.setWeight(QFont::Normal);
    categoryLabel->setFont(categoryFont);
    //设置颜色
    //base_gray_100:暗色：#EBEBEB,亮色：#10141A
    categoryLabel->setStyleSheet("QLabel{"
                                 "color:#EBEBEB;"
                                 "}"
                                 );
    //btn==================
    //82,186,298*32
    QPushButton *priorityBtn=new QPushButton("未选择",this);
    priorityBtn->setGeometry(82, 186, 298, 32);
    //未选择左，上下，12，6，6

    // 设置样式
    priorityBtn->setStyleSheet(
        "QPushButton {"
         //背景颜色， "bg_white_desktop_3":暗色： "#2C2C2D",亮色：#FFFFFF
        "    background-color: #2C2C2D;"
        // 圆角
        "    border-radius: 4px;"
        //距离左上下，12，6，6
        "    padding-left: 12px;"
        "    padding-top: 6px;"
        "    padding-bottom: 6px;"
        // 文字左对齐
        "    text-align: left;"
        // 文字颜色
        //base_gray_100:暗色：#EBEBEB,亮色：#10141A
        "    color: #EBEBEB;"
        // 字粗细400
        "    font-family: 'Microsoft YaHei UI';"
        "    font-weight: 400;"
        "}"
        //这些btn,hover/press : "base_gray_005", "base_gray_010"
        //hover
        //base_gray_005:暗色：#3D5D6166，亮色：
        "QPushButton:hover {"
        "    background-color: #3D5D6166;"
        "}"
        //press
        //base_gray_010:暗色：#486F747A，亮色：
        "QPushButton:pressed {"
        "    background-color: #486F747A;"
        "}"
        );
    //优先级----------------------------------
    QLabel* priorityLabel=new QLabel("优先级",this);
    //20，240
    priorityLabel->move(20,240);
    QFont priorityFont("Microsoft YaHei UI");
    //设置粗细
    //400
    priorityFont.setWeight(QFont::Normal);
    priorityLabel->setFont(priorityFont);
    //设置颜色
    //base_gray_100:暗色：#EBEBEB,亮色：#10141A
    priorityLabel->setStyleSheet("QLabel{"
                                 "color:#EBEBEB;"
                                 "}"
                                 );
    //更多
    QLabel* moreLabel=new QLabel("更多",this);
    //20，338
    moreLabel->move(20,338);
    QFont moreFont("Microsoft YaHei UI");
    //设置粗细
    //400
    moreFont.setWeight(QFont::Normal);
    moreLabel->setFont(moreFont);
    //设置颜色
    //base_gray_060": 暗色：#AEC4C7CC，亮色：#0B21A
    moreLabel->setStyleSheet("QLabel{"
                             "color:#AEC4C7CC;"
                             "}"
                             );
    //设置按钮

}
