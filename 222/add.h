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
namespace Ui {
class add;
}

class add : public QWidget
{
    Q_OBJECT

public:
    explicit add(QWidget *parent = nullptr);
    ~add();

private:
    Ui::add *ui;
    // 布局
    QVBoxLayout *mainLayout;      // 主垂直布局
    //新建任务
    QLabel *titleLabel;
    void   setupUI();
};

#endif // ADD_H
