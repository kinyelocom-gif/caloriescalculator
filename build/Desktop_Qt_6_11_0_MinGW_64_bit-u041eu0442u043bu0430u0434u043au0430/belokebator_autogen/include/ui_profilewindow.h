/********************************************************************************
** Form generated from reading UI file 'profilewindow.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_PROFILEWINDOW_H
#define UI_PROFILEWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFrame>
#include <QtWidgets/QLabel>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_profilewindow
{
public:
    QFrame *info_container;
    QLabel *label;

    void setupUi(QWidget *profilewindow)
    {
        if (profilewindow->objectName().isEmpty())
            profilewindow->setObjectName("profilewindow");
        profilewindow->resize(800, 600);
        profilewindow->setStyleSheet(QString::fromUtf8("QWidget { background: transparent; }"));
        info_container = new QFrame(profilewindow);
        info_container->setObjectName("info_container");
        info_container->setGeometry(QRect(20, 111, 760, 458));
        info_container->setMaximumSize(QSize(760, 458));
        info_container->setStyleSheet(QString::fromUtf8("background-image: url(:/assets/background/info_container.svg);"));
        info_container->setFrameShape(QFrame::Shape::StyledPanel);
        info_container->setFrameShadow(QFrame::Shadow::Raised);
        label = new QLabel(info_container);
        label->setObjectName("label");
        label->setGeometry(QRect(280, 220, 49, 16));
        label->setStyleSheet(QString::fromUtf8("color: black\n"
""));
        label->setTextFormat(Qt::TextFormat::AutoText);

        retranslateUi(profilewindow);

        QMetaObject::connectSlotsByName(profilewindow);
    } // setupUi

    void retranslateUi(QWidget *profilewindow)
    {
        profilewindow->setWindowTitle(QCoreApplication::translate("profilewindow", "Form", nullptr));
        label->setText(QCoreApplication::translate("profilewindow", "\321\205\321\203\320\265\321\201\320\276\321\201", nullptr));
    } // retranslateUi

};

namespace Ui {
    class profilewindow: public Ui_profilewindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_PROFILEWINDOW_H
