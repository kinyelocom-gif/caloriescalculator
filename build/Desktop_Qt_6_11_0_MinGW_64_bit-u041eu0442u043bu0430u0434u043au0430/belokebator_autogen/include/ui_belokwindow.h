/********************************************************************************
** Form generated from reading UI file 'belokwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_BELOKWINDOW_H
#define UI_BELOKWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_belokwindow
{
public:

    void setupUi(QWidget *belokwindow)
    {
        if (belokwindow->objectName().isEmpty())
            belokwindow->setObjectName("belokwindow");
        belokwindow->resize(400, 300);

        retranslateUi(belokwindow);

        QMetaObject::connectSlotsByName(belokwindow);
    } // setupUi

    void retranslateUi(QWidget *belokwindow)
    {
        belokwindow->setWindowTitle(QCoreApplication::translate("belokwindow", "Form", nullptr));
    } // retranslateUi

};

namespace Ui {
    class belokwindow: public Ui_belokwindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_BELOKWINDOW_H
