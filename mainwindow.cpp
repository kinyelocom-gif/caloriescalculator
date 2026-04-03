#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QRegularExpression>
#include <QMessageBox>
#include <QSettings>
#include <QMovie>
#include <QGraphicsDropShadowEffect>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    this->setWindowTitle("Калькулятор калорій - Профіль");
    this->setWindowIcon(QIcon(":/assets/icons/profile.png"));

    ui->notification_bar->hide();
    ui->loading_gif_label->setStyleSheet("background-color: transparent; border: none;");
    ui->loading_gif_label->setAttribute(Qt::WA_TranslucentBackground);
    ui->loading_gif_label->setAutoFillBackground(false);
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);

    shadow->setBlurRadius(20);
    shadow->setXOffset(0);
    shadow->setYOffset(4);
    shadow->setColor(QColor(0, 0, 0, 80));

    ui->notification_bar->setGraphicsEffect(shadow);

    QMovie *movie = new QMovie(":/assets/background/loading_anim.gif");
    movie->setScaledSize(QSize(30, 30));
    ui->loading_gif_label->setMovie(movie);

    QSettings settings("MyCompany", "CalorieApp");
    int lastApi = settings.value("selected_api_index", 0).toInt();
    int lastModel = settings.value("selected_model_index", 0).toInt();
    ui->apiBox->setCurrentIndex(lastApi);
    ui->modelBox->setCurrentIndex(lastModel);
    on_apiBox_currentIndexChanged(lastApi);
    on_modelBox_currentIndexChanged(lastModel);

    networkManager = new QNetworkAccessManager(this);

    apiUrl = QUrl("https://generativelanguage.googleapis.com/v1beta/models/gemini-2.0-flash:generateContent");
    apiKey = "AIzaSyD1dmaD8WuEaWk7XPurrA04LsBpsGcvzfE";

    ui->page->setAttribute(Qt::WA_StyledBackground, true);
    ui->mainStackedWidget->setCurrentIndex(0);
    ui->ckalStackedWidget->setCurrentIndex(0);
    ui->belokStackedWidget->setCurrentIndex(0);
}



MainWindow::~MainWindow()
{
    delete ui;
}



void MainWindow::on_add_button_clicked()
{
    //початок gif анімації
    ui->notification_bar->show();
    ui->loading_gif_label->movie()->start();

    QString food = ui->input_field->text();
    int gramms = ui->vagaida_field->text().toInt();
    QString userText = QString(

                           "Данні користувача: Им'я %1, Вік:%7, Вага %2 кг, Зріст %3 см, Мета: %4. "

                           "Користувач з'їв: %5. %6 грам "

                           "Розрахуй калорії та БЖВ цієї страви."

                           "ДАЙ ВІДПОВІДЬ СТРОГО ПО ШАБЛОНУ (без зайвих слів): "

                           "Страва: [Назва], ckal: [Число], proteins: [Число], fats: [Число], carbs: [Число]."

                           ).arg(userName).arg(userWeight).arg(userHeight).arg(meta).arg(food).arg(gramms).arg(userAge);
    if (userText.isEmpty()) return;
    ui->input_field->clear();
    ui->vagaida_field->clear();

    QJsonObject textPart; textPart.insert("text", userText);
    QJsonArray partsArray; partsArray.append(textPart);
    QJsonObject contentObj; contentObj.insert("parts", partsArray);
    QJsonArray contentsArray; contentsArray.append(contentObj);
    QJsonObject root; root.insert("contents", contentsArray);

    QNetworkRequest request(apiUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    request.setRawHeader("X-goog-api-key", apiKey.toUtf8());

    // Отправляем
    QNetworkReply *reply = networkManager->post(request, QJsonDocument(root).toJson());

    connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument resDoc = QJsonDocument::fromJson(reply->readAll());
            QString aiResponse = resDoc.object().value("candidates").toArray().at(0)
                                     .toObject().value("content").toObject()
                                     .value("parts").toArray().at(0)
                                     .toObject().value("text").toString();

            qDebug() << "------------------------------------";
            qDebug() << "GEMINI answers:" << aiResponse.trimmed();
            qDebug() << "------------------------------------";
            // ui->info->setText(aiResponse.trimmed());
            // ui->info->setStyleSheet("color: #000000; font-size: 14px;");

            QString strava = "Невідома страва";
            double calories = 0, proteins = 0, fats = 0, carbs = 0;

            QRegularExpression reStrava("Страва:\\s*([^,.]+)");
            QRegularExpression reCkal("ckal:\\s*(\\d+)");
            QRegularExpression reProt("proteins:\\s*(\\d+)");
            QRegularExpression reFats("fats:\\s*(\\d+)");
            QRegularExpression reCarbs("carbs:\\s*(\\d+)");

            strava = reStrava.match(aiResponse).captured(1).trimmed();
            calories = reCkal.match(aiResponse).captured(1).toDouble();
            proteins = reProt.match(aiResponse).captured(1).toDouble();
            fats = reFats.match(aiResponse).captured(1).toDouble();
            carbs = reCarbs.match(aiResponse).captured(1).toDouble();

            qDebug() << strava << calories << proteins << fats << carbs;

            cTcalories += calories;
            cTproteins += proteins;
            cTfats += fats;
            cTcarbs += carbs;

            totalCkalPercentage = qBound(0, qRound((cTcalories/userCkalScore) * 100.0), 100);
            ui->ckal_bar->setValue(totalCkalPercentage);

            totalProteinPercentage = qBound(0, qRound((cTproteins/userProteinScore) * 100.0), 100);
            ui->protein_bar->setValue(totalProteinPercentage);

            totalFatsPercentage = qBound(0, qRound((cTfats/userFatsScore) * 100.0), 100);
            ui->fats_bar->setValue(totalFatsPercentage);

            totalCarbsPercentage = qBound(0, qRound((cTcarbs/userCarbsScore) * 100.0), 100);
            ui->carbs_bar->setValue(totalCarbsPercentage);

            QString stravaLbl = QString("Страва: %1").arg(strava);
            QString caloriesLbl = QString("Калорії: %1").arg(calories);
            QString proteinLbl = QString("Білок: %1 г").arg(proteins);
            QString fatsLbl = QString("Жири: %1 г").arg(fats);
            QString carbsLbl = QString("Вуглеводи: %1 г").arg(carbs);

            ui->ckal_lbl->setText(caloriesLbl);
            ui->ckal_lbl->adjustSize();

            ui->protein_lbl->setText(proteinLbl);
            ui->protein_lbl->adjustSize();

            ui->fats_lbl->setText(fatsLbl);
            ui->fats_lbl->adjustSize();

            ui->carbs_lbl->setText(carbsLbl);
            ui->carbs_lbl->adjustSize();

            //кінець gif анімації
            ui->notification_bar->hide();
            ui->loading_gif_label->movie()->stop();


            ui->ckalStackedWidget->setCurrentIndex(1);

        } else {
            ui->notification_bar->hide();
            ui->loading_gif_label->movie()->stop();
            qDebug() << "error:" << reply->errorString();
            qDebug() << "details from server:" << reply->readAll();
        }
        reply->deleteLater();
    });
}

//кнпоки тулбара
void MainWindow::on_profile_clicked()
{
    ui->mainStackedWidget->setCurrentIndex(0);
    this->setWindowTitle("Калькулятор калорій - Профіль");
    this->setWindowIcon(QIcon(":/assets/icons/profile.png"));
}

void MainWindow::on_ckal_clicked()
{
    ui->mainStackedWidget->setCurrentIndex(1);
    this->setWindowTitle("Калькулятор калорій");
    this->setWindowIcon(QIcon(":/assets/icons/ckal.png"));
}

void MainWindow::on_belok_clicked()
{
    ui->mainStackedWidget->setCurrentIndex(2);
    this->setWindowTitle("Калькулятор калорій - Фенілоланін");
    this->setWindowIcon(QIcon(":/assets/icons/belok.png"));
}

void MainWindow::on_settings_clicked()
{
    ui->mainStackedWidget->setCurrentIndex(3);
    this->setWindowTitle("Калькулятор калорій - Налаштування");
    this->setWindowIcon(QIcon(":/assets/icons/settings.png"));
}



//фиелды с инфой s
void MainWindow::on_save_clicked()
{
    ui->notification_bar->show();
    ui->loading_gif_label->movie()->start();

    userName = ui->name_field->text();
    userAge = ui->age_field->text().toInt();
    userHeight = ui->zrist_field->text().toInt();
    userWeight = ui->vaga_field->text().toInt();
    qDebug() << "Saved:" << userName << userHeight << userWeight << "meta:" << meta;

    QString food = ui->input_field->text();
    int gramms = ui->vagaida_field->text().toInt();
    QString userText = QString(

                           "Данні користувача: Ім'я %1, Вага %2 кг, Зріст %3 см, Мета: %4. "

                           "Розрахуй цого норму калорій БЖВ опираючись на мету"

                           "ДАЙ ВІДПОВІДЬ СТРОГО ПО ШАБЛОНУ (без зайвих слів): "

                           "ckal: [Число], proteins: [Число], fats: [Число], carbs: [Число]."

                           ).arg(userName).arg(userWeight).arg(userHeight).arg(meta);
    if (userText.isEmpty()) return;
    ui->input_field->clear();
    ui->vagaida_field->clear();

    QJsonObject textPart; textPart.insert("text", userText);
    QJsonArray partsArray; partsArray.append(textPart);
    QJsonObject contentObj; contentObj.insert("parts", partsArray);
    QJsonArray contentsArray; contentsArray.append(contentObj);
    QJsonObject root; root.insert("contents", contentsArray);

    QNetworkRequest request(apiUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    request.setRawHeader("X-goog-api-key", apiKey.toUtf8());

    // Отправляем
    QNetworkReply *reply = networkManager->post(request, QJsonDocument(root).toJson());

    connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument resDoc = QJsonDocument::fromJson(reply->readAll());
            QString aiResponse = resDoc.object().value("candidates").toArray().at(0)
                                     .toObject().value("content").toObject()
                                     .value("parts").toArray().at(0)
                                     .toObject().value("text").toString();

            qDebug() << "------------------------------------";
            qDebug() << "GEMINI answers:" << aiResponse.trimmed();
            qDebug() << "------------------------------------";
            // ui->info->setText(aiResponse.trimmed());
            // ui->info->setStyleSheet("color: #000000; font-size: 14px;");

            QRegularExpression re("ckal:\\s*([\\d]+).*?proteins:\\s*([\\d]+).*?fats:\\s*([\\d]+).*?carbs:\\s*([\\d]+)");

            QRegularExpressionMatch match = re.match(aiResponse.trimmed());

            if (match.hasMatch()) {
                userCkalScore = match.captured(1).toInt();
                userProteinScore = match.captured(2).toInt();
                userFatsScore = match.captured(3).toInt();
                userCarbsScore = match.captured(4).toInt();
            }

            ui->notification_bar->hide();
            ui->loading_gif_label->movie()->stop();

            ui->mainStackedWidget->setCurrentIndex(1);

        } else {
            ui->notification_bar->hide();
            ui->loading_gif_label->movie()->stop();
            qDebug() << "error:" << reply->errorString();
            qDebug() << "details from server:" << reply->readAll();
        }
        reply->deleteLater();
    });


}

void MainWindow::on_cancel_clicked()
{
    ui->name_field->clear();
    ui->zrist_field->clear();
    ui->vaga_field->clear();
    ui->age_field->clear();
    ui->shud_btn->setChecked(false);
    ui->pidtr_btn->setChecked(false);
    ui->nabir_btn->setChecked(false);
}




void MainWindow::on_shud_btn_clicked(bool checked)
{
    shud = 1;
    ui->pidtr_btn->setChecked(false);
    ui->nabir_btn->setChecked(false);
    if (shud == 1) {
        meta = "схуднути";
    }

}


void MainWindow::on_pidtr_btn_clicked(bool checked)
{
    pidtr = 1;
    ui->shud_btn->setChecked(false);
    ui->nabir_btn->setChecked(false);
    if (pidtr == 1) {
        meta = "підтримка ваги";
    }
}


void MainWindow::on_nabir_btn_clicked(bool checked)
{
    nabir = 1;
    ui->shud_btn->setChecked(false);
    ui->pidtr_btn->setChecked(false);

    if (nabir == 1) {
        meta = "набір ваги";
    }

}

void MainWindow::on_info_btn_clicked()
{
    QString infoBoxText = QString("Ваша ціль по БЖВ на день:\n"
                               "Калорії: %1\n"
                               "Білок: %2\n"
                               "Жири: %3\n"
                               "Вуглеводи: %4").arg(userCkalScore).arg(userProteinScore).arg(userFatsScore).arg(userCarbsScore);


    QMessageBox msgBox;
    msgBox.setWindowTitle("Інформація");
    msgBox.setText(infoBoxText);
    msgBox.setIcon(QMessageBox::Information);
    msgBox.exec();
}


void MainWindow::on_apiBox_currentIndexChanged(int index)
{
    if (index == 0) {
        apiKey = "AIzaSyCqwy4vyv6egt1mOHN--I4F05uvHEFUfOo";
    } else if (index == 1) {
        apiKey = "AIzaSyD1dmaD8WuEaWk7XPurrA04LsBpsGcvzfE";
    } else if (index == 2) {
        apiKey = "AIzaSyBGxN_x2DVOfrkECPeeesk4bL8dG93YYYw";
    } else if (index == 3) {
        apiKey = "AIzaSyB3qUuK3wje4l1cK5GIo5DXZZ21bJFgQrY";
    } else if (index == 4) {
        apiKey = "AIzaSyAP0ENyy1jaPwnCPAZ_SYds8NEc-SUhG9I";
    }

    QSettings settings("MyCompany", "CalorieApp");
    settings.setValue("selected_api_index", index);

    qDebug() << "api changed: " << index;

}


void MainWindow::on_modelBox_currentIndexChanged(int index)
{
    if (index == 0) {
        apiUrl = QUrl("https://generativelanguage.googleapis.com/v1beta/models/gemini-3-flash-preview:generateContent");
        qDebug() << "model changed: gemini-3-flash-preview";
    } else if (index == 1) {
        apiUrl = QUrl("https://generativelanguage.googleapis.com/v1beta/models/gemini-3.1-flash-lite-preview:generateContent");
        qDebug() << "model changed: gemini-3.1-flash-lite-preview";
    } else if(index == 2) {
        apiUrl = QUrl("https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-flash:generateContent");
        qDebug() << "model changed: gemini-2.5-flashw";
    } else if(index == 3) {
        apiUrl = QUrl("https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-flash-lite:generateContent");
        qDebug() << "model changed: gemini-2.5-flash-lite";
    } else if(index == 4) {
        apiUrl = QUrl("https://generativelanguage.googleapis.com/v1beta/models/gemini-2.0-flash:generateContent");
        qDebug() << "model changed: gemini-2.0-flash";
    } else if(index == 5) {
        apiUrl = QUrl("https://generativelanguage.googleapis.com/v1beta/models/gemini-2.0-flash-lite:generateContent");
        qDebug() << "model changed: gemini-2.0-flash-lite";
    } else if(index == 6) {
        apiUrl = QUrl("https://generativelanguage.googleapis.com/v1beta/models/gemini-3-flash-live:generateContent");
        qDebug() << "model changed: gemini-3-flash-live";
    }

    QSettings settings("MyCompany", "CalorieApp");
    settings.setValue("selected_model_index", index);


}


void MainWindow::on_add_button_2_clicked()
{
    //початок gif анімації
    ui->notification_bar->show();
    ui->loading_gif_label->movie()->start();

    QString food = ui->input_field_2->text();
    int gramms = ui->vagaida_field_2->text().toInt();
    QString userText = QString(

                           "Користуач з'їв: %5. %6 грам"

                           "Розрахуй кількуіть білків і міліграм фінілаланіну цього блюда."

                           "ДАЙ ВІДПОВІДЬ СТРОГО ПО ШАБЛОНУ (без зайвих слів): "

                           "Страва: [Название], ckal: [Число], proteins: [Число], fenil: [Число], fats: [Число], carbs: [Число]."

                           ).arg(food).arg(gramms);
    if (userText.isEmpty()) return;
    ui->input_field_2->clear();
    ui->vagaida_field_2->clear();

    QJsonObject textPart; textPart.insert("text", userText);
    QJsonArray partsArray; partsArray.append(textPart);
    QJsonObject contentObj; contentObj.insert("parts", partsArray);
    QJsonArray contentsArray; contentsArray.append(contentObj);
    QJsonObject root; root.insert("contents", contentsArray);

    QNetworkRequest request(apiUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    request.setRawHeader("X-goog-api-key", apiKey.toUtf8());

    // відправка
    QNetworkReply *reply = networkManager->post(request, QJsonDocument(root).toJson());

    connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument resDoc = QJsonDocument::fromJson(reply->readAll());
            QString aiResponse = resDoc.object().value("candidates").toArray().at(0)
                                     .toObject().value("content").toObject()
                                     .value("parts").toArray().at(0)
                                     .toObject().value("text").toString();

            qDebug() << "------------------------------------";
            qDebug() << "GEMINI answers:" << aiResponse.trimmed();
            qDebug() << "------------------------------------";

            double calories = 0, proteins = 0, fats = 0, carbs = 0, fenil = 0;
            QString strava;

            QRegularExpression re("Блюдо:\\s*([^,]+).*?ckal:\\s*([\\d.]+).*?proteins:\\s*([\\d.]+).*?fenil:\\s*([\\d.]+).*?fats:\\s*([\\d.]+).*?carbs:\\s*([\\d.]+)");

            QRegularExpressionMatch match = re.match(aiResponse.trimmed());

            if (match.hasMatch()) {
                strava = match.captured(1);
                calories = match.captured(2).toDouble();
                proteins = match.captured(3).toDouble();
                fenil = match.captured(4).toDouble();
                fats = match.captured(5).toDouble();
                carbs = match.captured(6).toDouble();
            }

            qDebug() << strava << calories << proteins << fenil << fats << carbs;

            QString stravaLbl = QString("Страва: %1 ").arg(strava);
            QString caloriesLbl = QString("Калорії: %1 ").arg(calories);
            QString proteinLbl = QString("Білок: %1 г ").arg(proteins);
            QString fatsLbl = QString("Жири: %1 г ").arg(fats);
            QString carbsLbl = QString("Вуглеводи: %1 г ").arg(carbs);
            QString fenilLbl = QString("Фенілаланін доданої страви: %1 мг").arg(fenil);

            ui->lb1_2->setText(fenilLbl);
            ui->lb1_2->adjustSize();

            ui->ckal_lbl_2->setText(caloriesLbl);
            ui->ckal_lbl_2->adjustSize();

            ui->protein_lbl_2->setText(proteinLbl);
            ui->protein_lbl_2->adjustSize();

            ui->fats_lbl_2->setText(fatsLbl);
            ui->fats_lbl_2->adjustSize();

            ui->carbs_lbl_2->setText(carbsLbl);
            ui->carbs_lbl_2->adjustSize();

            ui->belokStackedWidget->setCurrentIndex(1);

            //кінець gif анімації
            ui->notification_bar->hide();
            ui->loading_gif_label->movie()->stop();

        } else {
            ui->notification_bar->hide();
            ui->loading_gif_label->movie()->stop();
            qDebug() << "error:" << reply->errorString();
            qDebug() << "details from server:" << reply->readAll();
        }
        reply->deleteLater();
    });
}