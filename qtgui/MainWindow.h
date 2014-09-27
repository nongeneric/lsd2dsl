#pragma once

#include <QMainWindow>

class LSDListModel;
class QPushButton;
class QTableView;
class QProgressBar;
class QLabel;
class QSortFilterProxyModel;
class MainWindow : public QMainWindow {
    Q_OBJECT    
    LSDListModel* _model;
    QPushButton* _convertAllButton;
    QPushButton* _convertSelectedButton;
    QTableView* _tableView;
    QProgressBar* _progress;
    QProgressBar* _dictProgress;
    QLabel* _currentDict;
    QSortFilterProxyModel* _proxyModel;
    void convert(bool selectedOnly);
public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();
};
