#include "add.h"
//#include "ui_add.h"
//#include<QSvgWidget>
#include <QEvent>
add::add(QWidget *parent) :
    QWidget(parent)
    //ui(new Ui::add)
{
    //ui->setupUi(this);
    // 设置窗口大小
    //加上阴影边框
    //resize(400, 563);
    //阴影边距先设计20-
    shadowMargin_ = 10;
    resize(400 + 2 * shadowMargin_, 563 + 2 * shadowMargin_);  // 440 x 603
    //删除最上面一层标题栏
    setWindowFlags(Qt::FramelessWindowHint);
    //设置窗口透明
    setAttribute(Qt::WA_TranslucentBackground);

    isDarkMode_=false;
    initColorMap();
    setupShortcuts();
    setupUI();
    //初始化不能拖动
    isDragging_=false;
    //启用鼠标追踪,这样在移动是就可以触发move鼠标
    setMouseTracking(true);
    //获取边缘
    resizeEdge_ = Qt::Edges();
    isResizing_ = false;
    setMinimumSize(400, 563);
    this->installEventFilter(this);
}
add::~add()
{
    //delete ui;
}
void add:: setupUI(){

    //窗口背景设计
    // bg_white_desktop_3
     connectInit([this]() {
        // setStyleSheet(QString("add { background-color: %1; }")
        //                   .arg(colorMap["accent_blue"][isDarkMode_]));
    //     // update();
        qDebug() << "（（（（（（（（（（（（（（（（（（（";
         //repaint();
     });
       qDebug() << "hahahh";
    //创建主布局
    mainLayout_=new QVBoxLayout(this);
    //左上右下
    //mainLayout_->setContentsMargins(20, 0, 20, 24);
    //因为加上阴影扩大了，所以现在要加上阴影把他进行缩小
    mainLayout_->setContentsMargins(
        20 + shadowMargin_,
        0 + shadowMargin_,
        20 + shadowMargin_,
        24 + shadowMargin_
        );
    //后面需要自己设置垂直距离
    mainLayout_->setSpacing(0);
    //创建可以拖动的范围---------------------------------
    titleBar_ = new QWidget(this);
    titleBar_->setFixedHeight(21);
    //titleBar_->setCursor(Qt::SizeAllCursor);  // 鼠标变成拖动图标
    mainLayout_->addWidget(titleBar_);
    //************************************
    //新建任务，距离左边24，距离上边18+3--------------------------------
    //标题 "新建任务"
    //创建可拖动的标题栏

    QLabel *titleLabel = new QLabel("新建任务");
    titleLabel->setFont(createFont("Microsoft YaHei UI", 16, QFont::Bold));

    connectInit([this, titleLabel]() {
        titleLabel->setStyleSheet(QString("QLabel { color: %1; }")
                                      .arg(colorMap["base_gray_100"][isDarkMode_]));
    });
    //布局
    // //+++++标题，他距离左边是24，所以要加上
    QHBoxLayout *titleLayout = new QHBoxLayout();
    // 左边距 4px
    titleLayout->setContentsMargins(4, 0, 0, 0);
    titleLayout->setSpacing(0);
    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();
    mainLayout_->addLayout(titleLayout);
    //15
    mainLayout_->addSpacing(15);
    //********************************************
    //请仔细填写以下信息---------------------------------------
    //首先创建背景容器
    QWidget *infoFrame = createInfoFrame(
         "C:/Users/Vivasweetwu/Documents/222/resources/icons/Information1.svg",
        "请仔细填写以下信息"
        );
    //布局
    //+++++提示框
    mainLayout_->addWidget(infoFrame);
    //12
    mainLayout_->addSpacing(12);
    //*********************************************
    //基本信息------------------------------
    QLabel* sectionLabel=new QLabel("基本信息",this);
    sectionLabel->setFont(createFont("Microsoft YaHei UI", 12, QFont::Normal));
    //设置颜色
    //base_gray_060": 暗色：#AEC4C7CC，亮色：#0B21A
    connectInit([this, sectionLabel]() {
        sectionLabel->setStyleSheet(QString("QLabel { color: %1; }")
                                        .arg(colorMap["base_gray_060"][isDarkMode_]));
    });
    //布局
    //+++++基本信息
    mainLayout_->addWidget(sectionLabel);
    mainLayout_->addSpacing(16);

    //************************************************
    //主题------------------------------------
     QLabel* tasknameLabel=new QLabel("主题",this);
    tasknameLabel->setFont(createFont("Microsoft YaHei UI", 14, QFont::Normal));
    //设置颜色
    //base_gray_100:暗色：#EBEBEB,亮色：#10141A

    connectInit([this, tasknameLabel]() {
        tasknameLabel->setStyleSheet(QString("QLabel { color: %1; }")
                                         .arg(colorMap["base_gray_100"][isDarkMode_]));
    });

    tasknameLabel->setFixedWidth(42);
    //设置文本、、、、、、、、、、、、、、、、
    QLineEdit *nameEdit = createLineEdit("文本");
    //布局
    //++++++主题和输入框，水兵，其实下面这几个都是16，20
    QHBoxLayout *nameLayout = new QHBoxLayout();
    nameLayout->setContentsMargins(0, 0, 0, 0);
    //20
    nameLayout->setSpacing(20);

    nameLayout->addWidget(tasknameLabel);
    nameLayout->addWidget(nameEdit);  // 输入框自动拉伸

    mainLayout_->addLayout(nameLayout);
    //16
    mainLayout_->addSpacing(16);
   //  //**************************************************
    //分类-------------------------------------
    QLabel* categoryLabel=new QLabel("分类",this);
    categoryLabel->setFont(createFont("Microsoft YaHei UI", 14, QFont::Normal));
    //设置颜色
    //base_gray_100:暗色：#EBEBEB,亮色：#10141A
    connectInit([this, categoryLabel]() {
        categoryLabel->setStyleSheet(QString("QLabel { color: %1; }")
                                         .arg(colorMap["base_gray_100"][isDarkMode_]));
    });

    categoryLabel->setFixedWidth(42);
    //btn==================
    QPushButton *categoryBtn = createButton1("未选择");
    //布局
    //分类：标签 + 按钮，水平，这个是外部水平布局
    QHBoxLayout *categoryLayout = new QHBoxLayout();
    categoryLayout->setContentsMargins(0, 0, 0, 0);
    categoryLayout->setSpacing(20);
    //将标签 + 按钮标签 + 按钮加入到水平布局
    categoryLayout->addWidget(categoryLabel);
    categoryLayout->addWidget(categoryBtn);
    //将外部这个水平布局加入到总布局
    mainLayout_->addLayout(categoryLayout);
    //16
    mainLayout_->addSpacing(16);
    //*************************************************
    //优先级----------------------------------
    QLabel* priorityLabel=new QLabel("优先级",this);
    priorityLabel->setFont(createFont("Microsoft YaHei UI", 14, QFont::Normal));
    //设置颜色
    //base_gray_100:暗色：#EBEBEB,亮色：#10141A
    connectInit([this, priorityLabel]() {
        priorityLabel->setStyleSheet(QString("QLabel { color: %1; }")
                                         .arg(colorMap["base_gray_100"][isDarkMode_]));
    });
    priorityLabel->setFixedWidth(42);
    //优先级按钮
    QPushButton *priorityBtn = createButton1("中");
    //布局
    //+++++++优先级：标签 + 按钮
    QHBoxLayout *priorityLayout = new QHBoxLayout();
    priorityLayout->setContentsMargins(0, 0, 0, 0);
    priorityLayout->setSpacing(20);


    priorityLayout->addWidget(priorityLabel);
    priorityLayout->addWidget(priorityBtn);

    mainLayout_->addLayout(priorityLayout);
    mainLayout_->addSpacing(16);

    //****************************************************
    //+++++++++++++++++++++标签
    QLabel * labelnameLabel=new QLabel("标签",this);
    labelnameLabel->setFont(createFont("Microsoft YaHei UI", 14, QFont::Normal));
    //设置颜色
    //base_gray_100:暗色：#EBEBEB,亮色：#10141A
    connectInit([this, labelnameLabel]() {
        labelnameLabel->setStyleSheet(QString("QLabel { color: %1; }")
                                          .arg(colorMap["base_gray_100"][isDarkMode_]));
    });
    //设置文本、、、、、、、、、、、、、、、、
    QLineEdit *labelnameEdit = createLineEdit("自定义");
    labelnameLabel->setFixedWidth(42);
    //布局
    //标签
    QHBoxLayout *labelLayout = new QHBoxLayout();
    labelLayout->setContentsMargins(0, 0, 0, 0);
    labelLayout->setSpacing(20);

    labelLayout->addWidget(labelnameLabel);
    labelLayout->addWidget(labelnameEdit);

    mainLayout_->addLayout(labelLayout);
    //30
     mainLayout_->addSpacing(30);
    //*********************************************
    //更多
     QLabel* moreLabel=new QLabel("更多",this);
    moreLabel->setFont(createFont("Microsoft YaHei UI", 12, QFont::Normal));
    //设置颜色
    //base_gray_060": 暗色：#AEC4C7CC，亮色：#0B21A

    connectInit([this, moreLabel]() {
        moreLabel->setStyleSheet(QString("QLabel { color: %1; }")
                                     .arg(colorMap["base_gray_060"][isDarkMode_]));
    });

    //布局
    mainLayout_->addWidget(moreLabel);
    //16
    mainLayout_->addSpacing(16);
    //**********************************************
    //-------------
    QTextEdit *descEdit =createTextEdit("请输入任务的详细描述（选填）");
    //布局
    //++++++++描述
    mainLayout_->addWidget(descEdit);
    //37
    mainLayout_->addSpacing(37);
    //***********************************************************
    //设置按钮
    // normal/hover/press 颜色分别为:
    // 保存:"accent_blue", "accent_blue_lighten_1", "accent_blue_darken_2"
    // 取消:"base_gray_005", "base_gray_010", "base_gray_015"
    // //保存按钮
   // QPushButton *saveBtn = createButton("保存", true, "#338CFF", "#2C79DD", "#60A6FF");
    QPushButton *saveBtn = createButton2("保存");
    //取消按钮
    QPushButton *cancelBtn = createButton2("取消");
    //布局
    //按钮
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setContentsMargins(0, 0, 0, 0);
    //12
    btnLayout->setSpacing(12);

    btnLayout->addWidget(saveBtn);
    btnLayout->addWidget(cancelBtn);

    mainLayout_->addLayout(btnLayout);

    //****************************************
    //弹性空间
    mainLayout_->addStretch();
    //应用布局
    setLayout(mainLayout_);

}
//创建字体
QFont add::createFont(const QString &family, int pixelSize, QFont::Weight weight)
{
    //字型
    QFont font(family);
    //字体大小
    font.setPixelSize(pixelSize);
    //字体粗细
    font.setWeight(weight);
    return font;
}
//将分类和优先级进行封装

//创建按钮
QPushButton* add::createButton1(const QString &text)
{
    QPushButton *button = new QPushButton(this);
    button->setFixedHeight(32);
    connectInit([this, button, text]() {
        QString bgNormal = isDarkMode_ ? "#2C2C2D" : "#FFFFFF";
        QString bgHover = colorMap["base_gray_005"][isDarkMode_];
        QString bgPress = colorMap["base_gray_010"][isDarkMode_];

        button->setStyleSheet(QString(
              "QPushButton {"
              // 背景颜色
              "    background-color: %1;"
              // 圆角
              "    border-radius: 4px;"
              // 距离左上下，12，6，6
              // "    padding-left: 12px;"
              "    padding-top: 6px;"
              "    padding-bottom: 6px;"
              // 文字左对齐
              "    text-align: left;"
              // 字粗细400
              "    font-family: 'Microsoft YaHei UI';"
              "    font-weight: 400;"
              "}"
              // hover
              "QPushButton:hover {"
              "    background-color: %2;"
              "}"
              // press
              "QPushButton:pressed {"
              "    background-color: %3;"
              "}"
                ).arg(bgNormal, bgHover, bgPress));
    });
    // 创建按钮内部布局
    QHBoxLayout *btnLayout = new QHBoxLayout(button);
    // 左12，右13
    btnLayout->setContentsMargins(12, 0, 13, 0);
    btnLayout->setSpacing(0);
    //这是新添加的
    // 添加文字标签
    QLabel *textLabel = new QLabel(text, button);
    textLabel->setFont(createFont("Microsoft YaHei UI", 14, QFont::Normal));
    connectInit([this, textLabel]() {
        QString textColor = isDarkMode_ ? "#EBEBEB" : "#10141A";
        textLabel->setStyleSheet(QString("QLabel { color: %1; background: transparent; }")
                                     .arg(textColor));
    });
    btnLayout->addWidget(textLabel);
    //中间加一个弹簧
    btnLayout->addStretch();
    //创建图标
    QLabel *iconLabel = new QLabel(button);
    iconLabel->setFixedSize(10, 6);
    // 加载 SVG
    QString iconPath1 = "C:/Users/Vivasweetwu/Documents/222/resources/icons/Chevron1.svg";
    QSvgRenderer svgRenderer(iconPath1);
    // QString svgContent =
    //     "<svg width='10' height='6' viewBox='0 0 10 6' xmlns='http://www.w3.org/2000/svg'>"
    //     "<g clip-path='url(#clip0_10_100)'>"
    //     "<path d='M8.30719 1.05717C8.55112 0.813244 8.94784 0.813538 9.19196 1.05717C9.43603 1.30125 9.43603 1.69786 9.19196 1.94194L5.61871 5.51518C5.27701 5.85687 4.72214 5.85687 4.38043 5.51518L0.80719 1.94194C0.563553 1.69783 0.563259 1.30111 0.80719 1.05717C1.05112 0.813244 1.44784 0.813538 1.69196 1.05717L4.99957 4.36479L8.30719 1.05717Z' fill='#0B121A' fill-opacity='0.6'/>"
    //     "</g>"
    //     "<defs>"
    //     "<clipPath id='clip0_10_100'>"
    //     "<rect width='10' height='6' fill='white'/>"
    //     "</clipPath>"
    //     "</defs>"
    //     "</svg>";

    // QSvgRenderer svgRenderer;
    // svgRenderer.load(svgContent.toUtf8());
    QPixmap pixmap(10, 6);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    svgRenderer.render(&painter);
    painter.end();
    iconLabel->setPixmap(pixmap);
    btnLayout->addWidget(iconLabel);  // 图标放在右边
    return button;
}
QPushButton* add::createButton2(const QString &text)
{
    QPushButton *button = new QPushButton(text, this);
    button->setFixedHeight(32);
    connectInit([this, button, text]() {
        QString textColor;
        QString bgNormal, bgHover, bgPress;

        if (text == "保存") {
             // 保存按钮：白色文字
            textColor = "#FFFFFF";
            bgNormal = colorMap["accent_blue"][isDarkMode_];
            bgHover = colorMap["accent_blue_lighten_1"][isDarkMode_];
            bgPress = colorMap["accent_blue_darken_2"][isDarkMode_];
        } else {
            // 取消按钮
            textColor = colorMap["base_gray_100"][isDarkMode_];
            bgNormal = colorMap["base_gray_005"][isDarkMode_];
            bgHover = colorMap["base_gray_010"][isDarkMode_];
            bgPress = colorMap["base_gray_015"][isDarkMode_];
        }

        button->setStyleSheet(QString(
                                  "QPushButton {"
                                  "    background-color: %1;"
                                  "    border-radius: 4px;"
                                  "    color: %4;"
                                  "    font-family: 'Microsoft YaHei UI';"
                                  "    font-weight: 400;"
                                  "    font-size: 14px;"
                                  "}"
                                  "QPushButton:hover {"
                                  "    background-color: %2;"
                                  "}"
                                  "QPushButton:pressed {"
                                  "    background-color: %3;"
                                  "}"
                                  ).arg(bgNormal, bgHover, bgPress, textColor));
    });
        return button;
}
//设置lineiedt
QLineEdit* add::createLineEdit(const QString &text)
{
    QLineEdit *edit = new QLineEdit(this);
    edit->setFixedHeight(32);
    edit->setPlaceholderText(text);
    connectInit([this, edit]() {
        edit->setStyleSheet(
            QString(
                "QLineEdit {"
                "    background-color: %1;"
                //添加大边界颜色
                 "    border: 1px solid %2;"      /* 大边界颜色 */
                //圆角半径
                "    border-radius: 4px;"
                "    padding-left: 12px;"
                "    padding-top: 6px;"
                "    padding-bottom: 6px;"
                "    color: %3;"
                "    font-family: 'Microsoft YaHei UI';"
                "    font-weight: 400;"
                "}"
                //添加激活态
                "QLineEdit:focus {"
                "    border: 1px solid %4;"
                "}"
                "QLineEdit::placeholder { color: %5; }"
                ).arg(colorMap["bg_white_desktop_3"][isDarkMode_],
                     colorMap["base_gray_015"][isDarkMode_],
                     colorMap["base_gray_100"][isDarkMode_],
                     colorMap["accent_blue"][isDarkMode_],
                     colorMap["base_gray_040"][isDarkMode_])
            );
    });
    return edit;
}
//创建textedit
QTextEdit* add::createTextEdit(const QString &text)
{
    QTextEdit *descEdit = new QTextEdit(this);
    // 20, 370, 360*100
    //descEdit->setGeometry(20, 370, 360, 100);
    descEdit->setFixedHeight(100);
    descEdit->setPlaceholderText(text);
    connectInit([this, descEdit]() {
        descEdit->setFrameStyle(QFrame::NoFrame);

        descEdit->setStyleSheet(
            QString(
                // "QTextEdit {"
                // // //背景：bg_white_desktop_3：#FFFFFF
                // "    background-color: %1;"
                // //添加大边框
                // // "    border: 1px solid %2;"
                // //圆角：16px
                // "    border-radius: 16px;"
                // //内边距：12
                // "    padding: 12px;"
                // "    font-family: 'Microsoft YaHei UI';"
                // "    font-weight: 400;"
                // //字体颜色
                // "    color: %3;"
                // "}"
                "QTextEdit {"
                "    background-color: transparent;"
                "    border: none;"
                "    padding: 0;"
                "    font-family: 'Microsoft YaHei UI';"
                "    font-weight: 400;"
                "    color: %3;"
                "}"
                //这个不起效？？
                "QTextEdit::viewport {"
                "    background-color: %1;"
                "    border: 1px solid %2;"
                "    border-radius: 16px;"
                "    padding:12px;"
                "}"
                //添加激活态???????????????????????????????????
                // "QTextEdit:focus {"
                // "    border: 2px solid %4;"
                // "}"
                // "QTextEdit:focus::viewport {"
                // "    border: 1px solid %4;"
                // "}"
                "QTextEdit:focus::viewport {"
                //"    background-color: %1;"
                "    border: 1px solid %4;"
                 "    border-radius: 8px;"
                 "    padding: 150px;"
                "}"

                //占位符颜色：base_gray_040：#09111A
                "QTextEdit::placeholder { color: %5; }"
                ).arg(colorMap["bg_white_desktop_3"][isDarkMode_],
                     colorMap["base_gray_015"][isDarkMode_],
                     colorMap["base_gray_100"][isDarkMode_],
                     colorMap["accent_blue"][isDarkMode_],
                     colorMap["base_gray_040"][isDarkMode_])
            );
    });
    return descEdit;
}
void add::setupShortcuts()
{
    // 创建 Ctrl+t 快捷键
    themeShortcut_ = new QShortcut(QKeySequence("Ctrl+T"), this);
    qDebug() << "11111";
    // 连接到切换主题槽函数
    //connect(themeShortcut_, &QShortcut::activated, this, &add::toggleTheme);
    connect(themeShortcut_, &QShortcut::activated, this, [this]() {
        isDarkMode_ = !isDarkMode_;
        update();
        qDebug() << "Theme toggled to:" << (isDarkMode_ ? "Dark" : "Light");
    });
}
void add::initColorMap()
{
    //背景
    colorMap["bg_main"][false] = "#F5F5F5";
    colorMap["bg_main"][true] = "#202021";
    //新建任务，主题，分类，优先级，标签
    colorMap["base_gray_100"][false] = "#10141A";
    colorMap["base_gray_100"][true] = "#EBEBEB";
    //提示背景
    colorMap["accent_blue"][false] = "#267EF0";
    colorMap["accent_blue"][true] = "#338CFF";
    //提示字体
    colorMap["accent_blue_darken_4"][false] = "#1B539A";
    colorMap["accent_blue_darken_4"][true] = "#7BB5FF";
    //基本信息
    colorMap["base_gray_060"][false] = "#0B21A0";
    colorMap["base_gray_060"][true] = "#AEC4C7CC";
    //line
    colorMap["bg_white_desktop_3"][false] = "#FFFFFF";
    colorMap["bg_white_desktop_3"][true] = "#2C2C2D";
    //文本字体颜色:base_gray_040
    colorMap["base_gray_040"][false] = "#09111A";
    colorMap["base_gray_040"][true] = "#85ADB1B8";
    //实体文字颜色
    colorMap["base_gray_100"][false] = "#10141A";
    colorMap["base_gray_100"][true] = "#EBEBEB";

    colorMap["base_gray_015"][false] = "#26070F1A";
    colorMap["base_gray_015"][true] = "#5283888F";
    //输入框激活态：激活态下的边框颜色为:"accent_blue"
    colorMap["accent_blue"][false] = "#267EF0";
    colorMap["accent_blue"][true] = "#338CFF";
    //分类优先级按钮
    //这些btn,hover/press : "base_gray_005", "base_gray_010"
    //hover
    colorMap["base_gray_005"][false] = "#0D060F1A";
    colorMap["base_gray_005"][true] = "#3D5D6166";
    //press
    colorMap["base_gray_010"][false] = "#1A060F1A";
    colorMap["base_gray_010"][true] = "#486F747A";
    // normal/hover/press 颜色分别为
    // 保存: "accent_blue", "accent_blue_lighten_1", "accent_blue_darken_2"
    // 取消:"base_gray_005", "base_gray_010", "base_gray_015"
    //保存normal
    colorMap["accent_blue"][false] = "#267EF0";
    colorMap["accent_blue"][true] = "#338CFF";
    //保存hover
    colorMap["accent_blue_lighten_1"][false] = "#4B95F3";
    colorMap["accent_blue_lighten_1"][true] = "#2C79DD";
    //保存press
    colorMap["accent_blue_darken_2"][false] = "#1F68C5";
    colorMap["accent_blue_darken_2"][true] = "#60A6FF";
    //取消normal
    colorMap["base_gray_005"][false] = "#0D060F1A";
    colorMap["base_gray_005"][true] = "#3D5D6166";
    //取消hover
    colorMap["base_gray_010"][false] = "#1A060F1A";
    colorMap["base_gray_010"][true] = "#486F747A";
    //取消press
    colorMap["base_gray_015"][false] = "#26070F1A";
    colorMap["base_gray_015"][true] = "#5283888F";
    //描述背景
    colorMap["bg_white_desktop_3"][false] = "#FFFFFF";
    colorMap["bg_white_desktop_3"][true] = "#2C2C2D";
    //描述字体颜色
    colorMap["base_gray_100"][false] = "#10141A";
    colorMap["base_gray_100"][true] = "#EBEBEB";
    //描述占位符颜色
    colorMap["base_gray_040"][false] = "#09111A";
    colorMap["base_gray_040"][true] = "#85ADB1B8";

}
//创建提示框
QWidget* add::createInfoFrame(const QString &iconPath, const QString &text)
{
    QWidget *infoFrame=new QWidget(this);
    infoFrame->setObjectName("infoFrame");
    infoFrame->setFixedHeight(36);
    //设置背景
    //accent_blue：暗色：#338CFF，亮色：#267EF0
    //10%透明度：19
    connectInit([this, infoFrame]() {
        QColor baseColor = colorMap["accent_blue"][isDarkMode_];
        baseColor.setAlphaF(0.1);
        QString finalColor = baseColor.name(QColor::HexArgb);
        infoFrame->setStyleSheet(QString("QWidget#infoFrame { background-color: %1; border-radius: 8px; }")
                                     .arg(finalColor));
    });

    //添加水平布局
    QHBoxLayout *infoLayout = new QHBoxLayout(infoFrame);
    //好像是16，10
    infoLayout->setContentsMargins(16, 10, 16, 10);
    //9
    infoLayout->setSpacing(9);
    //添加图标
     QLabel* iconLabel=new QLabel(infoFrame);
    // //相对于infoFrame:16,10;16*16
    // //添加图标 Chevron.svg   Information.svg
    // //C:/Users/Vivasweetwu/Desktop/Chevron.svg
    // //创建一个svg渲染器并加载文件
     QSvgRenderer svgRender(iconPath);

    // QString svgContent =
    //     "<svg width='16' height='16' viewBox='0 0 16 16' xmlns='http://www.w3.org/2000/svg'>"
    //     "<g clip-path='url(#clip0_10_54)'>"
    //     "<path d='M8 0.5C12.1421 0.5 15.5 3.85786 15.5 8C15.5 12.1421 12.1421 15.5 8 15.5C3.85786 15.5 0.5 12.1421 0.5 8C0.5 3.85786 3.85786 0.5 8 0.5ZM8 1.5C4.41015 1.5 1.5 4.41015 1.5 8C1.5 11.5899 4.41015 14.5 8 14.5C11.5899 14.5 14.5 11.5899 14.5 8C14.5 4.41015 11.5899 1.5 8 1.5ZM8 7C8.55229 7 9 7.44772 9 8V11C9 11.5523 8.55229 12 8 12C7.44772 12 7 11.5523 7 11V8C7 7.44772 7.44772 7 8 7ZM8 4C8.55229 4 9 4.44772 9 5C9 5.55228 8.55229 6 8 6C7.44772 6 7 5.55228 7 5C7 4.44772 7.44772 4 8 4Z' fill='#1B539A'/>"
    //     "</g>"
    //     "<defs>"
    //     "<clipPath id='clip0_10_54'>"
    //     "<rect width='16' height='16' fill='white'/>"
    //     "</clipPath>"
    //     "</defs>"
    //     "</svg>";

    // QSvgRenderer svgRender;
    // svgRender.load(svgContent.toUtf8());
    //创建一个16*16的一个画布并填充透明背景
    QPixmap pixmap(16, 16);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    svgRender.render(&painter);
    painter.end();
    iconLabel->setPixmap(pixmap);
    //添加文字
    QLabel *infoTextLabel=new QLabel(text,infoFrame);
    infoTextLabel->setFont(createFont("Microsoft YaHei UI", 14, QFont::Normal));
    //添加
    //infoTextLabel->setAttribute(Qt::WA_TranslucentBackground);
    //设置字体颜色
    //accent_blue_darken_4: 暗色：#7BB5FF，亮色：#1B539A
    connectInit([this, infoTextLabel]() {
        infoTextLabel->setStyleSheet(QString("QLabel { color: %1; background-color: transparent;}")
                                         .arg(colorMap["accent_blue_darken_4"][isDarkMode_]));
    });
    //将控件添加到水平布局
    infoLayout->addWidget(iconLabel);
    infoLayout->addWidget(infoTextLabel);
    //添加一个弹性空间,自动占用剩余的所有空间
    infoLayout->addStretch();
    return infoFrame;
}

//connect进行抽象
void add::connectInit(std::function<void()> connectFunc)
{
    connectFunc();
    connect(themeShortcut_, &QShortcut::activated, this, connectFunc);
}
// 窗口可移动实现
void add::mousePressEvent(QMouseEvent *event)
{
    // 检测是否在可调整大小的边缘
    resizeEdge_ = hitTest(event->pos());
    if (event->button() == Qt::LeftButton) {
         resizeEdge_ = hitTest(event->pos());
        // 检查是否在标题栏区域
        if (titleBar_->geometry().contains(event->pos())) {
            isDragging_ = true;
            dragPosition_ = event->globalPos() - frameGeometry().topLeft();
        }else if(resizeEdge_ != Qt::Edges(0)){
            // 开始调整大小
            isResizing_ = true;
            resizeStart_ = event->globalPos();
            windowStart_ = geometry();
        }
    }
}

void add::mouseMoveEvent(QMouseEvent *event)
{
    if (isDragging_ && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPos() - dragPosition_);

    }else if(isResizing_){
        QPoint delta = event->globalPos() - resizeStart_;
        QRect newRect = windowStart_;
//修改窗口边缘的绝对位置
        // 处理左边缘
        if (resizeEdge_ & Qt::LeftEdge) {
            newRect.setLeft(windowStart_.left() + delta.x());
        }
        // 处理右边缘
        if (resizeEdge_ & Qt::RightEdge) {
            newRect.setRight(windowStart_.right() + delta.x());
        }
        // 处理上边缘
        if (resizeEdge_ & Qt::TopEdge) {
            newRect.setTop(windowStart_.top() + delta.y());
        }
        // 处理下边缘
        if (resizeEdge_ & Qt::BottomEdge) {
            newRect.setBottom(windowStart_.bottom() + delta.y());
        }
        //固定位置要是缩小到最小的时候
        if (newRect.width() < minimumWidth()) {
            qDebug()<<"kkk";
            if (resizeEdge_ & Qt::LeftEdge) {
                newRect.setLeft(newRect.right() - minimumWidth());
            } else {
                newRect.setWidth(minimumWidth());
            }
        }
        if (newRect.height() < minimumHeight()) {
             qDebug()<<"lll";
            if (resizeEdge_ & Qt::TopEdge) {
                newRect.setTop(newRect.bottom() - minimumHeight());
            } else {
                newRect.setHeight(minimumHeight());
            }
        }
        setGeometry(newRect);
    }else{
        //获取当前位置
        Qt::Edges edges = hitTest(event->pos());
        //设置形状
        updateCursorShape(edges);
    }
}
void add::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        isDragging_ = false;

        isResizing_ = false;
    }
}
void add::paintEvent(QPaintEvent *event)
{

    QWidget::paintEvent(event);
    static int count = 0;
    static QElapsedTimer timer;
    if (count == 0) {
        timer.start();
    }
    qDebug() << "=== paintEvent 第" << ++count << "次 ===";
    qDebug() << "时间：" << timer.elapsed() << "ms";
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);  // 抗锯齿
    //去掉阴影边距的内容框
    QRect contentRect = rect().adjusted(
        shadowMargin_,
        shadowMargin_,
        -shadowMargin_,
        -shadowMargin_
        );
     //圆角半径
    int radius = 8;

    //绘制渐进式阴影
    //注意：这里既然一设计颜色变化，那么话粗细最好不要重叠，让i+的=画笔的粗细
    for (int i = 1; i < shadowMargin_; i+=1) {
        // 计算当前阴影层的透明度
        int alpha = (int)(0.1*255.0 * (shadowMargin_ - i) / shadowMargin_);

        QColor shadowColor(0, 0, 0, alpha);
        painter.setPen(QPen(shadowColor, 1));
        // 绘制当前阴影层，向外进行扩展
        QRect shadowRect = contentRect.adjusted(-i, -i , i, i );
        //此时矩形在变大因此圆角应该也要变大,举行区域，水平圆角半径，垂直圆角半径
        painter.drawRoundedRect(shadowRect, radius + i, radius + i);
    }

    // //绘制主背景
    QPainterPath path;
    //path.addRoundedRect(contentRect, radius, radius);

    // 获取背景色
    QString strColor = colorMap["bg_white_desktop_3"][isDarkMode_];
    QColor bgColor = QColor(strColor);
    //painter.fillPath(path, bgColor);
    painter.fillRect(contentRect, bgColor);
    qDebug() << "rect: " << rect() << "color" << strColor;
    // setStyleSheet(QString("add { background-color: %1; }")
    //                  .arg(colorMap["accent_blue"][isDarkMode_]));
    //update();
    //repaint();
    //绘制边框:borderColor
    QColor borderColor ="#757575";
    borderColor.setAlphaF(0.4);
    //边框颜色和粗细
    painter.setPen(QPen(borderColor, 1));
    painter.drawRoundedRect(contentRect, radius, radius);
    // //  qDebug() << "打开了";
    // // qDebug() << "++++++++++++++++++++++++++++++++++++++++++++++++++++";
    //update(rect());
}
bool add::eventFilter(QObject *obj, QEvent *event)
{
    if(event->type() == QEvent::UpdateRequest) {
        qDebug() << "[UpdateRequest] obj=" << obj;
    }
    return QObject::eventFilter(obj, event);
}
Qt::Edges add::hitTest(const QPoint &pos) const
{
    Qt::Edges edges;
    //内容区域
    QRect contentRect = rect().adjusted(
        shadowMargin_,
        shadowMargin_,
        -shadowMargin_,
        -shadowMargin_
        );

    //只在内容区域内检测
    if (!contentRect.contains(pos)) {
        return edges;
    }
    //检测左边缘,在这个返回内
    if (pos.x()>=contentRect.left() &&
        pos.x()<=contentRect.left()+RESIZE_MARGIN) {
        edges|=Qt::LeftEdge;
    }
    //右
    if (pos.x()<=contentRect.right() &&
        pos.x()>=contentRect.right()-RESIZE_MARGIN) {
        edges|=Qt::RightEdge;
    }
    //上
    if (pos.y()>=contentRect.top() &&
        pos.y()<=contentRect.top()+RESIZE_MARGIN) {
        edges|=Qt::TopEdge;
    }
    //下
    if (pos.y()<=contentRect.bottom() &&
        pos.y()>=contentRect.bottom()-RESIZE_MARGIN) {
        edges|=Qt::BottomEdge;
    }
    return edges;
}
// 根据边缘更新光标形状
void add::updateCursorShape(Qt::Edges edges)
{
    // ↖↘，左上右下
    if (edges==(Qt::LeftEdge | Qt::TopEdge) ||
        edges==(Qt::RightEdge | Qt::BottomEdge)) {
        setCursor(Qt::SizeFDiagCursor);
    }
     // ↗↙右上左下
    else if (edges==(Qt::RightEdge|Qt::TopEdge)||
             edges==(Qt::LeftEdge|Qt::BottomEdge)) {
        setCursor(Qt::SizeBDiagCursor);
    }
    else if (edges&(Qt::LeftEdge|Qt::RightEdge)) {
        setCursor(Qt::SizeHorCursor);    // ↔
    }
    else if (edges&(Qt::TopEdge|Qt::BottomEdge)) {
        setCursor(Qt::SizeVerCursor);    // ↕
    }
    else {
        //默认箭头
        setCursor(Qt::ArrowCursor);
    }
}
