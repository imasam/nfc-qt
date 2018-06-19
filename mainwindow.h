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

    void on_btnAdd_clicked();

private:
    void generateCards();           // Generate some cards for test

private:
    Ui::MainWindow *ui;

    SqliteHelper *sqlHelper;
    NfcHelper *nfcHelper;
    void addToCardList(const QString& uid);      // Add uid to Uid List in UI
};

#endif // MAINWINDOW_H
