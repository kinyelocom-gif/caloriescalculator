/********************************************************************************
** Form generated from reading UI file 'settingswindow.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SETTINGSWINDOW_H
#define UI_SETTINGSWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_settingswindow
{
public:

    void setupUi(QWidget *settingswindow)
    {
        if (settingswindow->objectName().isEmpty())
            settingswindow->setObjectName("settingswindow");
        settingswindow->resize(400, 300);

        retranslateUi(settingswindow);

        QMetaObject::connectSlotsByName(settingswindow);
    } // setupUi

    void retranslateUi(QWidget *settingswindow)
    {
        settingswindow->setWindowTitle(QCoreApplication::translate("settingswindow", "Form", nullptr));
    } // retranslateUi

};

namespace Ui {
    class settingswindow: public Ui_settingswindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SETTINGSWINDOW_H
