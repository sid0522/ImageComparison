#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDir>
#include "opencv2/core/core.hpp"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:

    void on_pushButton_2_clicked();

    void on_pushButton_3_clicked();

    void on_horizontalSlider_valueChanged(int value);

    void on_pushButton_4_clicked();

private:
    Ui::MainWindow *ui;

    QString directory_;
    QDir currentDir;
    QStringList label;
    QVector<QString> matchedfileNames;
    QVector<QString> matchedfilePath;
    QWidget* checkBoxWidget;

public:
    void DisplayFiles(const QStringList& path);
    double runSIFT(cv::Mat firstImg, cv::Mat secondImg);
    bool fileExists(const QString& str);
};

#endif // MAINWINDOW_H
