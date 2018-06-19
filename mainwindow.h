#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class SqliteHelper;
class NfcHelper;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_btnApply_clicked();

private:
    void test();

private:
    Ui::MainWindow *ui;

    SqliteHelper *sqlHelper;
    NfcHelper *nfcHelper;
};

#endif // MAINWINDOW_H
