#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <QMessageBox>
#include <QSettings>
#include <QMovie>
#include <QGraphicsDropShadowEffect>
#include <QFile>
#include <QTextStream>
#include <QProcess>
#include <QCoreApplication>
#include <QCloseEvent>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Завантажуємо попередню сесію користувача
    loadFullSession();

    this->setWindowTitle("Калькулятор калорій - Профіль");
    this->setWindowIcon(QIcon(":/assets/icons/profile.png"));

    // Налаштування візуалу для панелі завантаження та тіней
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

    // Підключення GIF-анімації
    QMovie *movie = new QMovie(":/assets/background/loading_anim.gif");
    movie->setScaledSize(QSize(30, 30));
    ui->loading_gif_label->setMovie(movie);

    // Зчитування ключа доступу до API з файлу
    QFile keyFile(":/assets/api/apikey.txt");
    if (keyFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&keyFile);
        paidApiKey = in.readLine().trimmed();
        keyFile.close();
    }

    // Завантаження останніх обраних налаштувань API та моделі
    QSettings settings("MyCompany", "CalorieApp");
    int lastApi = settings.value("selected_api_index", 0).toInt();
    int lastModel = settings.value("selected_model_index", 0).toInt();

    ui->apiBox->blockSignals(true);
    ui->modelBox->blockSignals(true);

    ui->apiBox->setCurrentIndex(lastApi);
    ui->modelBox->setCurrentIndex(lastModel);

    ui->apiBox->blockSignals(false);
    ui->modelBox->blockSignals(false);

    on_apiBox_currentIndexChanged(lastApi);
    on_modelBox_currentIndexChanged(lastModel);

    networkManager = new QNetworkAccessManager(this);
    ui->page->setAttribute(Qt::WA_StyledBackground, true);
}

MainWindow::~MainWindow()
{
    delete ui;
}

// Додавання звичайної страви та розрахунок загального БЖВ
void MainWindow::on_add_button_clicked()
{
    ui->add_button->setEnabled(false);

    // Початок анімації завантаження
    ui->notification_bar->show();
    ui->loading_gif_label->movie()->start();

    QString food = ui->input_field->text();
    int gramms = ui->vagaida_field->text().toInt();

    // Формуємо текстовий запит для нейромережі
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

    // Збираємо JSON об'єкт для відправки на сервер
    QJsonObject textPart; textPart.insert("text", userText);
    QJsonArray partsArray; partsArray.append(textPart);
    QJsonObject contentObj; contentObj.insert("parts", partsArray);
    QJsonArray contentsArray; contentsArray.append(contentObj);
    QJsonObject root; root.insert("contents", contentsArray);

    QNetworkRequest request(apiUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    if(apiKey.isEmpty()) {
        return;
    }

    request.setRawHeader("X-goog-api-key", apiKey.toUtf8());

    // Відправляємо POST-запит
    QNetworkReply *reply = networkManager->post(request, QJsonDocument(root).toJson());

    connect(reply, &QNetworkReply::finished, this, [=]() {
        ui->add_button->setEnabled(true);
        if (reply->error() == QNetworkReply::NoError) {

            // Парсинг відповіді від Gemini
            QJsonDocument resDoc = QJsonDocument::fromJson(reply->readAll());
            QString aiResponse = resDoc.object().value("candidates").toArray().at(0)
                                     .toObject().value("content").toObject()
                                     .value("parts").toArray().at(0)
                                     .toObject().value("text").toString();

            QString strava = "Невідома страва";
            double calories = 0, proteins = 0, fats = 0, carbs = 0;

            // Витягуємо конкретні числа за допомогою регулярних виразів
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

            // Оновлюємо накопичені дані
            cTcalories += calories;
            cTproteins += proteins;
            cTfats += fats;
            cTcarbs += carbs;

            // Розраховуємо відсотки для прогрес-барів
            totalCkalPercentage = qBound(0, qRound((cTcalories/userCkalScore) * 100.0), 100);
            ui->ckal_bar->setValue(totalCkalPercentage);

            totalProteinPercentage = qBound(0, qRound((cTproteins/userProteinScore) * 100.0), 100);
            ui->protein_bar->setValue(totalProteinPercentage);

            totalFatsPercentage = qBound(0, qRound((cTfats/userFatsScore) * 100.0), 100);
            ui->fats_bar->setValue(totalFatsPercentage);

            totalCarbsPercentage = qBound(0, qRound((cTcarbs/userCarbsScore) * 100.0), 100);
            ui->carbs_bar->setValue(totalCarbsPercentage);

            // Оновлюємо текст під барами
            stravaLbl = QString("Страва: %1").arg(strava);
            caloriesLbl = QString("Калорії: %1").arg(calories);
            proteinLbl = QString("Білок: %1 г").arg(proteins);
            fatsLbl = QString("Жири: %1 г").arg(fats);
            carbsLbl = QString("Вуглеводи: %1 г").arg(carbs);

            ui->ckal_lbl->setText(caloriesLbl);
            ui->ckal_lbl->adjustSize();

            ui->protein_lbl->setText(proteinLbl);
            ui->protein_lbl->adjustSize();

            ui->fats_lbl->setText(fatsLbl);
            ui->fats_lbl->adjustSize();

            ui->carbs_lbl->setText(carbsLbl);
            ui->carbs_lbl->adjustSize();

            // Вимикаємо анімацію та переходимо на вкладку результатів
            ui->notification_bar->hide();
            ui->loading_gif_label->movie()->stop();
            ui->ckalStackedWidget->setCurrentIndex(1);

        } else {
            ui->notification_bar->hide();
            ui->loading_gif_label->movie()->stop();
        }
        reply->deleteLater();
    });
}

// Кнопки бокового меню для навігації
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

// Збереження профілю та розрахунок загальних норм БЖВ
void MainWindow::on_save_clicked()
{
    ui->notification_bar->show();
    ui->loading_gif_label->movie()->start();

    userName = ui->name_field->text();
    userAge = ui->age_field->text().toInt();
    userHeight = ui->zrist_field->text().toInt();
    userWeight = ui->vaga_field->text().toInt();

    // Визначаємо допуск фенілаланіну залежно від обраної дієти
    int dietIndex = ui->fenilBox->currentIndex();
    int tolerance = 25;

    if (dietIndex == 0) tolerance = 15;      // Строгая
    else if (dietIndex == 1) tolerance = 25; // Умеренная
    else if (dietIndex == 2) tolerance = 40; // Мягкая

    QSettings settings("MyCompany", "CalorieApp");
    settings.setValue("selected_diet_index", dietIndex);

    userFenilScore = userWeight * tolerance;
    userProteinPKYScore = (double)userFenilScore / 50.0;

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

    QNetworkReply *reply = networkManager->post(request, QJsonDocument(root).toJson());

    connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->error() == QNetworkReply::NoError) {

            QJsonDocument resDoc = QJsonDocument::fromJson(reply->readAll());
            QString aiResponse = resDoc.object().value("candidates").toArray().at(0)
                                     .toObject().value("content").toObject()
                                     .value("parts").toArray().at(0)
                                     .toObject().value("text").toString();

            // Зчитуємо розраховані норми
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
        }
        reply->deleteLater();
    });
}

// Очищення полів вводу профілю
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

// Обробка вибору мети користувача
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

// Інформаційне вікно для звичайного БЖВ
void MainWindow::on_info_btn_clicked()
{
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Інформація про БЖВ");
    msgBox.setIcon(QMessageBox::NoIcon);

    QString infoText = QString(
        "<b>ℹ️ Ваша ціль по БЖВ на день:</b><br><br>"
        "🔥 Калорії: <b>%1 ккал</b><br>"
        "💪 Білок: <b>%2 г</b><br>"
        "🥑 Жири: <b>%3 г</b><br>"
        "🍞 Вуглеводи: <b>%4 г</b>"
        ).arg(userCkalScore).arg(userProteinScore).arg(userFatsScore).arg(userCarbsScore);

    msgBox.setText(infoText);
    msgBox.setStyleSheet(
        "QLabel { min-width: 300px; color: #121212; font-size: 14px; }"
        "QPushButton { width: 80px; padding: 5px; }"
        );

    msgBox.exec();
}

// Вибір API ключа та моделі Gemini
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
        apiKey = paidApiKey;
    }

    QSettings settings("MyCompany", "CalorieApp");
    settings.setValue("selected_api_index", index);
}

void MainWindow::on_modelBox_currentIndexChanged(int index)
{
    if (index == 0) {
        apiUrl = QUrl("https://generativelanguage.googleapis.com/v1beta/models/gemini-3-flash-preview:generateContent");
    } else if (index == 1) {
        apiUrl = QUrl("https://generativelanguage.googleapis.com/v1beta/models/gemini-3.1-flash-lite-preview:generateContent");
    } else if(index == 2) {
        apiUrl = QUrl("https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-flash:generateContent");
    } else if(index == 3) {
        apiUrl = QUrl("https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-flash-lite:generateContent");
    } else if(index == 4) {
        apiUrl = QUrl("https://generativelanguage.googleapis.com/v1beta/models/gemini-2.0-flash:generateContent");
    } else if(index == 5) {
        apiUrl = QUrl("https://generativelanguage.googleapis.com/v1beta/models/gemini-2.0-flash-lite:generateContent");
    } else if(index == 6) {
        apiUrl = QUrl("https://generativelanguage.googleapis.com/v1beta/models/gemini-3-flash-live:generateContent");
    }

    QSettings settings("MyCompany", "CalorieApp");
    settings.setValue("selected_model_index", index);
}

// Додавання страви для дієти ФКУ (з розрахунком фенілаланіну)
void MainWindow::on_add_button_2_clicked()
{
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

    QNetworkReply *reply = networkManager->post(request, QJsonDocument(root).toJson());

    connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->error() == QNetworkReply::NoError) {

            QJsonDocument resDoc = QJsonDocument::fromJson(reply->readAll());
            QString aiResponse = resDoc.object().value("candidates").toArray().at(0)
                                     .toObject().value("content").toObject()
                                     .value("parts").toArray().at(0)
                                     .toObject().value("text").toString();

            double calories = 0, proteins = 0, fats = 0, carbs = 0, fenil = 0;
            QString strava;

            QRegularExpression re("Страва:\\s*([^,]+).*?ckal:\\s*([\\d.]+).*?proteins:\\s*([\\d.]+).*?fenil:\\s*([\\d.]+).*?fats:\\s*([\\d.]+).*?carbs:\\s*([\\d.]+)");
            QRegularExpressionMatch match = re.match(aiResponse.trimmed());

            if (match.hasMatch()) {
                strava = match.captured(1);
                calories = match.captured(2).toDouble();
                proteins = match.captured(3).toDouble();
                fenil = match.captured(4).toDouble();
                fats = match.captured(5).toDouble();
                carbs = match.captured(6).toDouble();
            }

            // Оновлюємо текст з результатами страви
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

            cTfenil += fenil;
            cTPKYproteins += proteins;

            double currentFenilNorm = userFenilScore;

            // Оновлюємо прогрес-бари ФКУ
            int totalFenilPercentage = qBound(0, qRound((cTfenil / currentFenilNorm) * 100.0), 100);
            ui->fenil_bar->setValue(totalFenilPercentage);

            totalPKYProteinPercentage = qBound(0, qRound((cTPKYproteins / userProteinPKYScore) * 100.0), 100);
            ui->protein_bar_2->setValue(totalPKYProteinPercentage);

            ui->belokStackedWidget->setCurrentIndex(1);

            ui->notification_bar->hide();
            ui->loading_gif_label->movie()->stop();

        } else {
            ui->notification_bar->hide();
            ui->loading_gif_label->movie()->stop();
        }
        reply->deleteLater();
    });
}

// Захист даних при закритті програми
void MainWindow::closeEvent(QCloseEvent *event)
{
    saveFullSession();
    event->accept();
}

// Запис всіх параметрів користувача у JSON файл
void MainWindow::saveFullSession()
{
    QJsonObject root;

    QJsonObject profile;
    profile["name"] = userName;
    profile["age"] = userAge;
    profile["height"] = userHeight;
    profile["weight"] = userWeight;
    profile["meta"] = meta;
    root["profile"] = profile;

    QJsonObject progress;
    progress["cTcalories"] = cTcalories;
    progress["cTproteins"] = cTproteins;
    progress["cTPKYproteins"] = cTPKYproteins;
    progress["cTfenil"] = cTfenil;
    progress["cTfats"] = cTfats;
    progress["cTcarbs"] = cTcarbs;
    root["progress"] = progress;

    QJsonObject targets;
    targets["calScore"] = userCkalScore;
    targets["protScore"] = userProteinScore;
    targets["proteinPKYScore"] = userProteinPKYScore;
    targets["fenilScore"] = userFenilScore;
    targets["fatsScore"] = userFatsScore;
    targets["carbsScore"] = userCarbsScore;
    root["targets"] = targets;

    QJsonObject bjv;
    bjv["caloriesLbl"] = caloriesLbl;
    bjv["proteinLbl"] = proteinLbl;
    bjv["fatsLbl"] = fatsLbl;
    bjv["carbsLbl"] = carbsLbl;
    bjv["fenilLbl_2"] = ui->lb1_2->text();
    bjv["caloriesLbl_2"] = ui->ckal_lbl_2->text();
    bjv["proteinLbl_2"] = ui->protein_lbl_2->text();
    root["bjv"] = bjv;

    root["lastPageIndex"] = ui->mainStackedWidget->currentIndex();
    root["lastCkalPage"] = ui->ckalStackedWidget->currentIndex();
    root["lastBelokPage"] = ui->belokStackedWidget->currentIndex();

    QFile file("session.json");
    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(root);
        file.write(doc.toJson());
        file.close();
    }
}

// Зчитування та відновлення інтерфейсу з JSON файлу
void MainWindow::loadFullSession()
{
    ui->mainStackedWidget->setCurrentIndex(0);
    ui->ckalStackedWidget->setCurrentIndex(0);
    ui->belokStackedWidget->setCurrentIndex(0);

    QFile file("session.json");
    if (!file.exists()) return;

    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        file.close();

        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonObject root = doc.object();

        QJsonObject profile = root["profile"].toObject();
        userName = profile["name"].toString();
        userAge = profile["age"].toInt();
        userHeight = profile["height"].toInt();
        userWeight = profile["weight"].toInt();
        meta = profile["meta"].toString();

        ui->shud_btn->setChecked(meta == "схуднути");
        ui->pidtr_btn->setChecked(meta == "підтримка ваги");
        ui->nabir_btn->setChecked(meta == "набір ваги");

        ui->name_field->setText(userName);
        ui->age_field->setText(userAge > 0 ? QString::number(userAge) : "");
        ui->zrist_field->setText(userHeight > 0 ? QString::number(userHeight) : "");
        ui->vaga_field->setText(userWeight > 0 ? QString::number(userWeight) : "");

        QJsonObject targets = root["targets"].toObject();
        userCkalScore = targets["calScore"].toDouble();
        userProteinScore = targets["protScore"].toDouble();
        userProteinPKYScore = targets["proteinPKYScore"].toDouble();
        userFenilScore = targets["fenilScore"].toDouble();
        userFatsScore = targets["fatsScore"].toDouble();
        userCarbsScore = targets["carbsScore"].toDouble();

        QJsonObject progress = root["progress"].toObject();
        cTcalories = progress["cTcalories"].toDouble();
        cTproteins = progress["cTproteins"].toDouble();
        cTPKYproteins = progress["cTPKYproteins"].toDouble();
        cTfenil = progress["cTfenil"].toDouble();
        cTfats = progress["cTfats"].toDouble();
        cTcarbs = progress["cTcarbs"].toDouble();

        if (userCkalScore > 0) ui->ckal_bar->setValue(qBound(0, qRound((cTcalories / userCkalScore) * 100.0), 100));
        if (userProteinScore > 0) ui->protein_bar->setValue(qBound(0, qRound((cTproteins / userProteinScore) * 100.0), 100));
        if (userFenilScore > 0) ui->fenil_bar->setValue(qBound(0, qRound((cTfenil / userFenilScore) * 100.0), 100));
        if (userProteinPKYScore > 0) ui->protein_bar_2->setValue(qBound(0, qRound((cTPKYproteins / userProteinPKYScore) * 100.0), 100));
        if (userFatsScore > 0) ui->fats_bar->setValue(qBound(0, qRound((cTfats / userFatsScore) * 100.0), 100));
        if (userCarbsScore > 0) ui->carbs_bar->setValue(qBound(0, qRound((cTcarbs / userCarbsScore) * 100.0), 100));

        QJsonObject bjv = root["bjv"].toObject();
        ui->ckal_lbl->setText(bjv["caloriesLbl"].toString());
        ui->protein_lbl->setText(bjv["proteinLbl"].toString());
        ui->fats_lbl->setText(bjv["fatsLbl"].toString());
        ui->carbs_lbl->setText(bjv["carbsLbl"].toString());
        ui->lb1_2->setText(bjv["fenilLbl_2"].toString());
        ui->ckal_lbl_2->setText(bjv["caloriesLbl_2"].toString());
        ui->protein_lbl_2->setText(bjv["proteinLbl_2"].toString());

        ui->mainStackedWidget->setCurrentIndex(root["lastPageIndex"].toInt());
        ui->ckalStackedWidget->setCurrentIndex(root["lastCkalPage"].toInt());
        ui->belokStackedWidget->setCurrentIndex(root["lastBelokPage"].toInt());
    }
}

// Видалення збережених даних та перезапуск додатка
void MainWindow::on_saveDel_clicked()
{
    userName = "";
    userAge = 0;
    userHeight = 0;
    userWeight = 0;
    meta = "";
    cTcalories = 0;
    cTproteins = 0;
    cTfats = 0;
    cTcarbs = 0;
    cTfenil = 0;

    totalCkalPercentage = 0;
    totalProteinPercentage = 0;
    totalFatsPercentage = 0;
    totalCarbsPercentage = 0;

    QFile::remove("session.json");

    QString program = QCoreApplication::applicationFilePath();
    QProcess::startDetached(program, QCoreApplication::arguments());
    QCoreApplication::exit(0);
}

// Інформаційне вікно з нормами ФКУ
void MainWindow::on_info_btn_2_clicked()
{
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Денна норма ФКУ");

    msgBox.setIcon(QMessageBox::NoIcon);

    QString infoText = QString(
        "<b>ℹ️ Ваші ФКУ-ліміти на день:</b><br><br>"
        "🧬 Фенілаланін: <b>%1 мг</b><br>"
        "🥩 Натуральний білок: <b>%2 г</b><br>"
        "⚖️ Ваша вага: <b>%3 кг</b><br><br>"
        "<small>Розраховано на основі обраної дієти.</small>"
        ).arg(userFenilScore).arg(userProteinPKYScore, 0, 'f', 1).arg(userWeight);

    msgBox.setText(infoText);

    msgBox.setStyleSheet(
        "QLabel { "
        "min-width: 350px; "
        "color: #121212; "
        "font-size: 14px; "
        "qproperty-alignment: 'AlignLeft | AlignTop'; "
        "}"
        "QPushButton { width: 80px; padding: 5px; }"
        );

    msgBox.exec();
}