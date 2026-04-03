/********************************************************************************
** Form generated from reading UI file 'ckal.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CKAL_H
#define UI_CKAL_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_ckal
{
public:

    void setupUi(QWidget *ckal)
    {
        if (ckal->objectName().isEmpty())
            ckal->setObjectName("ckal");
        ckal->resize(400, 300);

        retranslateUi(ckal);

        QMetaObject::connectSlotsByName(ckal);
    } // setupUi

    void retranslateUi(QWidget *ckal)
    {
        ckal->setWindowTitle(QCoreApplication::translate("ckal", "Form", nullptr));
    } // retranslateUi

};

namespace Ui {
    class ckal: public Ui_ckal {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CKAL_H
