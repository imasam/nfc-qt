#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <gps.h>

class SqliteHelper;
class NfcHelper;
class JsonHelper;
class QProcess;

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
    void updateSlot();

    void on_btnDelete_clicked();

private:
    void generateCards();                               // Generate some cards for test
    void applyNewCard(const QString& name);             // Apply new card to the real card
    void addToCardList(const QString& uid);             // Add uid to Uid List in UI
    double getDistance(GPS* gps1, GPS* gps2);
    static double rad(double d);

protected:
    void showEvent(QShowEvent *e);

private:
    Ui::MainWindow *ui;

    SqliteHelper *sqlHelper;
    NfcHelper *nfcHelper;
    JsonHelper* jsonHelper;

    QTimer* updateTimer;
    QProcess* process;
    double maxDistance;

    GPS* currentGPS;
    QString* busCardName;
    QString* subwayCardName;
    QMap<QString, GPS>* othersList;

    QString recommended;    // 系统推荐按的卡名字
};

#endif // MAINWINDOW_H
