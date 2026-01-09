#ifndef ADD_H
#define ADD_H

#include <QWidget>
#include <QLabel>
#include<QVBoxLayout>
#include<QLineEdit>
#include<QPushButton>
#include<QSvgRenderer>
#include<QPainter>
#include<QFile>
#include<QTextEdit>
#include<QShortcut>
#include<QMouseEvent>
#include<QFileInfo>
#include<QPainterPath>
#include<QElapsedTimer>
#include<QGraphicsDropShadowEffect>
#define TRACE() qDebug() << Q_FUNC_INFO
// namespace Ui {
// class add;
// }

class add : public QWidget
{
    Q_OBJECT

public:
    explicit add(QWidget *parent = nullptr);
    ~add();
    bool eventFilter(QObject *obj, QEvent *event) override;
private:
    //Ui::add *ui;
    //主垂直布局
    QVBoxLayout *mainLayout_=nullptr;
    QShortcut *themeShortcut_ = nullptr;
    //添加拖动
    bool isDragging_;
    QPoint dragPosition_;
    QWidget *titleBar_;
    //颜色映射表
    QMap<QString, QMap<bool, QString>> colorMap;
    //设置阴影边距
    int shadowMargin_ = 20;
    //边缘长度
    static const int RESIZE_MARGIN = 8;
    //调整边缘
    Qt::Edges resizeEdge_;
    //是否正在调整边缘
    bool isResizing_;
    //开始调整时的鼠标位置
    QPoint resizeStart_;
    //开始调整时的窗口几何，左上角绝对位置和长款
    QRect windowStart_;
    //是否是暗色
    bool isDarkMode_;
    void setupUI();
    //创建字体
    QFont createFont(const QString &family, int pixelSize,QFont::Weight weight);
     //创建按钮
    QPushButton* createButton1(const QString &text);
    QPushButton* createButton2(const QString &text);
    //创建lineidet
    QLineEdit* createLineEdit(const QString &text);
    //创建textidet
    QTextEdit* createTextEdit(const QString &text);
    //通过快捷键来触发按亮色切换
    void setupShortcuts();
    //颜色映射表
    void initColorMap();
    //创建提示框
    QWidget* createInfoFrame(const QString &iconPath, const QString &text);
    //将connect统一抽象成一个函数
    void connectInit(std::function<void()> connectFunc);
    //可以移动
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    //重写paintEvent
    void paintEvent(QPaintEvent *event)override;
     //检测鼠标在哪个边缘
    Qt::Edges hitTest(const QPoint &pos) const;
    //更新光标形状
    void updateCursorShape(Qt::Edges edges);
};

#endif // ADD_H
