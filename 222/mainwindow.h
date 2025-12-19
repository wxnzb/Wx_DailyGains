#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidgetItem>
#include<QSplitter>
#include "TaskManager.h"
#include"Task.h"
#include <QShortcut>
#include <QStyledItemDelegate>
#include <QPainter>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class ElidedTextDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit ElidedTextDelegate(QObject *parent = nullptr)
        : QStyledItemDelegate(parent) {}

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        //获取文字
         QString text = index.data(Qt::DisplayRole).toString();
        //优化：点击分类中状态先绘制背景

         //绘制背景：圆角矩形
         QRect bgRect = option.rect.adjusted(1, 1, -1, -1);  // 留出外边距

         if (option.state & QStyle::State_Selected) {
             // 选中状态
             if (option.state & QStyle::State_MouseOver) {
                 // 选中 + 悬停
                 painter->setBrush(QColor(255, 255, 255, 51));  // rgba(255,255,255,0.2)
             } else {
                 // 仅选中
                 painter->setBrush(QColor(255, 255, 255, 38));  // rgba(255,255,255,0.15)
             }
             painter->setPen(Qt::NoPen);
             painter->drawRoundedRect(bgRect, 6, 6);  // 圆角 6pxadd
         }
         else if (option.state & QStyle::State_MouseOver) {
             // 仅悬停
             painter->setBrush(QColor(255, 255, 255, 25));  // rgba(255,255,255,0.1)
             painter->setPen(Qt::NoPen);
             painter->drawRoundedRect(bgRect, 6, 6);
         }

        //设置绘制区域
         QRect textRect = option.rect.adjusted(8, 0, -8, 0);

        //计算省略后的文字
        QFontMetrics fm(option.font);
        QString elidedText = fm.elidedText(text, Qt::ElideRight, textRect.width());

        // 绘制背景
            painter->setPen(option.palette.text().color());

        //绘制文字
        painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, elidedText);
    }
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override
    {
        QSize size = QStyledItemDelegate::sizeHint(option, index);
        size.setHeight(qMax(size.height(), 28));
        return size;
    }
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
private slots:
    //点击快捷视图
    void onQuickViewClicked(QListWidgetItem *item);
    //点击分类
    void onCategoryClicked(QListWidgetItem *item);
    //新建分类
    void onAddCategoryClicked();//现在还没有实现存储功能!!!!!!!!!!!!!!!!!!!!!!
    //删除任务
    void onDeleteTaskClicked();//现在还没有实现从存储中真是删除的功能!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //任务已完成
     void onCompleteTaskClicked();
    //新建任务
      void onAddTaskClicked();
     //编辑
      void onEditTaskClicked();
      //搜索：在搜索框按enter之后就可以进行展示
     void onSearchTriggered();
      //优化：一直监听，要是变成空的就应该回复原来的视图
      void onSearchTextChanged(const QString& text);
private:
    Ui::MainWindow *ui;
    TaskManager *taskManager;
    //新增：保存分割线指针
    QSplitter *m_splitter;
    //保存原来左边选中的一个东西
    QString m_viewBeforeSearch;
     // 标记当前是否处于搜索状态
    bool m_isSearching;
    void createSampleData();
    void refreshCurrentView();  // 刷新当前视图
    // //存储假数据的列表
    //   QList<QStringList> fakeTasks;
    //   void loadTasksFromManager();
     //界面展示统一函数
      void renderTasks(const QList<Task>& tasks);
    // //先填充假数据
    //  void fillTableWithFakeData();
     // 显示所有任务
     void showAllTasks();
     // 显示重要任务
     void showImportantTasks();
      // 显示今日待办
     void showTodayTasks();
     //分类
     // 显示分类
     void populateCategoriesFromData();
     //点击分类内容后显示相应的分类
     void showTasksByCategory(const QString &category);
     //添加分类
     void addCategory(const QString &categoryName);
     //显示状态栏
     void updateStatusBar();
     // 执行搜索
     void performSearch(const QString& keyword);
     //点击分类删除时删除她下面的所有任务
     void deleteCategoryAndTasks();
     //优化+++++++++++++++++++++++++++++++++++++++
     //设置左右分割先
     void setupSplitter();
     //添加快捷键
     void setupShortcuts();
     //任务描述，用来返回在递减任务描述时返回任务行
     int getTaskRowFromTableRow(int tableRow)const;
     //在点击任务然后点击一完成时不应该丢失焦点
     //获取当前选中的任务 ID
     QString getCurrentSelectedTaskId() const;
     //根据任务 ID 重新选中行
     void selectTaskById(const QString& taskId);
     //在搜索里面没有内容时应该回复原来的视图
     // 保存当前视图状态
     void saveCurrentView();
     // 恢复搜索前的视图
     void restoreViewBeforeSearch();

};

#endif // MAINWINDOW_H
