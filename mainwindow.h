#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QNetworkAccessManager>
#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void on_cancel_clicked();

    void on_add_button_clicked();

    void on_profile_clicked();

    void on_ckal_clicked();

    void on_belok_clicked();

    void on_settings_clicked();

    void on_save_clicked();

    void on_shud_btn_clicked(bool checked);

    void on_pidtr_btn_clicked(bool checked);

    void on_nabir_btn_clicked(bool checked);

    void on_info_btn_clicked();

    void on_apiBox_currentIndexChanged(int index);

    void on_modelBox_currentIndexChanged(int index);

    void on_add_button_2_clicked();



    void on_saveDel_clicked();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    Ui::MainWindow *ui;
    QString userName;
    int userHeight;
    int userWeight;
    int userAge;
    QNetworkAccessManager *networkManager;
    QUrl apiUrl;
    QString apiKey;
    QString paidApiKey;

    QString meta;
    bool shud;
    bool pidtr;
    bool nabir;
    double userCkalScore = 0;
    double userProteinScore = 0, userFatsScore = 0, userCarbsScore = 0;
    double cTfenil = 0.0;

    double ckalPercentage = 0, proteinPercentage = 0, fatsPercentage = 0, carbsPercentage = 0;
    int totalCkalPercentage = 0, totalProteinPercentage = 0, totalFatsPercentage = 0, totalCarbsPercentage = 0;
    double cTcalories = 0, cTproteins = 0, cTfats = 0, cTcarbs = 0;

    void saveFullSession();
    void loadFullSession();

    QString stravaLbl, proteinLbl, fatsLbl, carbsLbl, caloriesLbl;
};


#endif // MAINWINDOW_H
