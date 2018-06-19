/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 5.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralWidget;
    QVBoxLayout *verticalLayout_2;
    QHBoxLayout *horizontalLayout;
    QLabel *label;
    QLabel *lblCurrentCard;
    QVBoxLayout *verticalLayout;
    QLabel *label_2;
    QListWidget *lstUid;
    QHBoxLayout *horizontalLayout_2;
    QPushButton *btnAdd;
    QPushButton *btnRestore;
    QPushButton *btnApply;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QStringLiteral("MainWindow"));
        MainWindow->resize(542, 337);
        centralWidget = new QWidget(MainWindow);
        centralWidget->setObjectName(QStringLiteral("centralWidget"));
        verticalLayout_2 = new QVBoxLayout(centralWidget);
        verticalLayout_2->setSpacing(6);
        verticalLayout_2->setContentsMargins(11, 11, 11, 11);
        verticalLayout_2->setObjectName(QStringLiteral("verticalLayout_2"));
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSpacing(6);
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        label = new QLabel(centralWidget);
        label->setObjectName(QStringLiteral("label"));

        horizontalLayout->addWidget(label);

        lblCurrentCard = new QLabel(centralWidget);
        lblCurrentCard->setObjectName(QStringLiteral("lblCurrentCard"));
        QSizePolicy sizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(lblCurrentCard->sizePolicy().hasHeightForWidth());
        lblCurrentCard->setSizePolicy(sizePolicy);
        lblCurrentCard->setMinimumSize(QSize(150, 0));

        horizontalLayout->addWidget(lblCurrentCard);


        verticalLayout_2->addLayout(horizontalLayout);

        verticalLayout = new QVBoxLayout();
        verticalLayout->setSpacing(6);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        label_2 = new QLabel(centralWidget);
        label_2->setObjectName(QStringLiteral("label_2"));

        verticalLayout->addWidget(label_2);

        lstUid = new QListWidget(centralWidget);
        lstUid->setObjectName(QStringLiteral("lstUid"));

        verticalLayout->addWidget(lstUid);


        verticalLayout_2->addLayout(verticalLayout);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setSpacing(6);
        horizontalLayout_2->setObjectName(QStringLiteral("horizontalLayout_2"));
        btnAdd = new QPushButton(centralWidget);
        btnAdd->setObjectName(QStringLiteral("btnAdd"));

        horizontalLayout_2->addWidget(btnAdd);

        btnRestore = new QPushButton(centralWidget);
        btnRestore->setObjectName(QStringLiteral("btnRestore"));

        horizontalLayout_2->addWidget(btnRestore);

        btnApply = new QPushButton(centralWidget);
        btnApply->setObjectName(QStringLiteral("btnApply"));

        horizontalLayout_2->addWidget(btnApply);


        verticalLayout_2->addLayout(horizontalLayout_2);

        MainWindow->setCentralWidget(centralWidget);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QApplication::translate("MainWindow", "MainWindow", nullptr));
        label->setText(QApplication::translate("MainWindow", "Current Card: ", nullptr));
        lblCurrentCard->setText(QApplication::translate("MainWindow", "None", nullptr));
        label_2->setText(QApplication::translate("MainWindow", "Card List:", nullptr));
        btnAdd->setText(QApplication::translate("MainWindow", "Add New Card", nullptr));
        btnRestore->setText(QApplication::translate("MainWindow", "Restore Deafult", nullptr));
        btnApply->setText(QApplication::translate("MainWindow", "Apply Selected Card", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
