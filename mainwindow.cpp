#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <iostream>
#include <algorithm>
#include <QFileDialog>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QPixmap>
#include <QGraphicsItem>
#include <QTextBrowser>
#include <QtDebug>
#include <QTableWidgetItem>
#include <QDirIterator>
#include <QLCDNumber>
#include <QMessageBox>
#include <QCheckBox>
#include <QProgressDialog>
#include "opencv2/xfeatures2d.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    MainWindow::setWindowTitle("Image Comparison");
}

MainWindow::~MainWindow()
{
    delete ui;
}

double MainWindow::runSIFT(cv::Mat firstImg, cv::Mat secondImg)
{
    // Parameters ...
    int nfeatures = 0;
    int nOctaveLayers = 3;
    double contrastThreshold = 0.04;
    double edgeThreshold = 10.0;
    double sigma = 1.6;

    cv::Ptr<cv::Feature2D> siftDetector = cv::xfeatures2d::SIFT::create(nfeatures, nOctaveLayers, contrastThreshold, edgeThreshold, sigma);

    if((firstImg.cols < 100) && (firstImg.rows < 100))
    {
        cv::resize(firstImg, firstImg, cv::Size(), 200/firstImg.rows, 200/firstImg.cols);
    }

    if((secondImg.cols < 100) && (secondImg.rows < 100))
    {
        cv::resize(secondImg, secondImg, cv::Size(), 200/secondImg.rows, 200/secondImg.cols);
    }

    // Check if the Images are loaded correctly ...
    if(firstImg.empty() || secondImg.empty())
    {
        qDebug() << "Error while trying to read one of the input files!";
        return 0;
    }

    // Keypoints Vectors for the First & Second Image ...
    std::vector<cv::KeyPoint> firstImgKeypoints, secondImgKeypoints;

    // Detecting Keypoints ...
    siftDetector->detect(firstImg, firstImgKeypoints);

    siftDetector->detect(secondImg, secondImgKeypoints);

    // Descriptors for the First & Second Image ...
    cv::Mat firstImgDescriptor, secondImgDescriptor;

    // Computing the descriptors
    siftDetector->compute(firstImg, firstImgKeypoints, firstImgDescriptor);
    siftDetector->compute(secondImg, secondImgKeypoints, secondImgDescriptor);

    // Find the matching points
    cv::DescriptorMatcher *matcher;
    matcher = new cv::BFMatcher(cv::NORM_L1);

    std::vector< cv::DMatch > firstMatches, secondMatches;
    matcher->match( firstImgDescriptor, secondImgDescriptor, firstMatches );
    matcher->match( secondImgDescriptor, firstImgDescriptor, secondMatches );

    delete matcher;

    int bestMatchesCount = 0;
    std::vector< cv::DMatch > bestMatches;

    for(uint i = 0; i < firstMatches.size(); ++i)
    {
        cv::Point matchedPt1 = firstImgKeypoints[i].pt;
        cv::Point matchedPt2 = secondImgKeypoints[firstMatches[i].trainIdx].pt;

        bool foundInReverse = false;

        for(uint j = 0; j < secondMatches.size(); ++j)
        {
            cv::Point tmpSecImgKeyPnt = secondImgKeypoints[j].pt;
            cv::Point tmpFrstImgKeyPntTrn = firstImgKeypoints[secondMatches[j].trainIdx].pt;
            if((tmpSecImgKeyPnt == matchedPt2) && ( tmpFrstImgKeyPntTrn == matchedPt1))
            {
                foundInReverse = true;
                break;
            }
        }
        if(foundInReverse)
        {
            bestMatches.push_back(firstMatches[i]);
            bestMatchesCount++;
        }
    }
    double minKeypoints = firstImgKeypoints.size() <= secondImgKeypoints.size() ? firstImgKeypoints.size() : secondImgKeypoints.size();

    qDebug() << "Probability = " + QString::number((bestMatchesCount / minKeypoints) * 100);

    double number = ((bestMatchesCount/minKeypoints) * 100);
    return number;
}

void MainWindow::on_pushButton_2_clicked() //open folder button
{
    directory_ = QDir::toNativeSeparators(QFileDialog::getExistingDirectory(this, tr("Select images/folder"), QDir::currentPath()));
    currentDir = directory_;

    QVector<QString> fileNames;
    QVector<QString> fileSize;
    QVector<QString> file_dir;
    QStringList filter;
    filter << "*.jpg" << "*.jpeg" << "*.png" << "*.bmp";
    QDirIterator it(directory_, QStringList() << filter, QDir::Files, QDirIterator::Subdirectories);
    while(it.hasNext())
    {
        it.next();
        qDebug() << it.fileName() << "\n";
        QFileInfo fileInfo(it.fileName());
        QString fileNameStr(fileInfo.fileName());
        fileNames.append(fileNameStr);

        QString file_path(it.filePath());
        QFile file_size(file_path);
        long long img_size = 0;
        img_size = file_size.size();
        fileSize.append(QString::number(img_size));
        file_dir.append(file_path);

        ui->tableWidget->setRowCount(fileNames.size());
        for(int i = 0; i < fileNames.size(); ++i)
        {
            label << "File Name" << "File Size";
            ui->tableWidget->setColumnCount(2);
            ui->tableWidget->setHorizontalHeaderLabels(label);
            ui->tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
            //ui->tableWidget->verticalHeader()->hide();
            ui->tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);

            ui->tableWidget->setItem(i, 0, new QTableWidgetItem(fileNames[i]));
            ui->tableWidget->setItem(i, 1, new QTableWidgetItem(fileSize[i]));
        }
    }
}

void MainWindow::on_pushButton_3_clicked() //find button
{
    QStringList files;
    QVector<QString> fileNames;
    QStringList filter;
    double match_per = 0.0;
    double match = 0;
    QProgressDialog progress(this);
    progress.setCancelButtonText(tr("&Cancel"));

    filter << "*.jpg" << "*.jpeg" << "*.png" << "*.bmp";
    QDirIterator it(directory_, QStringList() << filter, QDir::Files, QDirIterator::Subdirectories);
    while(it.hasNext())
    {
        files << it.next();
        QFileInfo fileInfo(it.fileName());
        QString fileNameStr(fileInfo.fileName());
        fileNames.push_back(fileNameStr);
    }
    for(int i = 0; i < fileNames.size(); ++i)
    {
        progress.setRange(0, fileNames.size());
        progress.setWindowTitle(tr("Finding Match"));
        progress.setValue(i);
        progress.setLabelText(tr("Searching... file %1 of %n", 0, fileNames.size()).arg(i));
        QCoreApplication::processEvents();

        if(!files.at(i).isNull())
        {
            cv::Mat first_img = cv::imread(files.at(i).toStdString());

            for(int j = i + 1; j < fileNames.size(); ++j)
            {
                if(!files.at(j).isNull())
                {
                    cv::Mat sec_img = cv::imread(files.at(j).toStdString());

                    match = runSIFT(first_img, sec_img);

                    match_per = ui->lcdNumber->value();
                }
                if(match >= match_per)
                {
                    matchedfileNames.push_back(fileNames[j]);
                    matchedfilePath.push_back(files.at(j));
                }
            }
        }
    }
    std::sort(matchedfilePath.begin(), matchedfilePath.end());
    matchedfilePath.erase(std::unique(matchedfilePath.begin(), matchedfilePath.end()), matchedfilePath.end());

    std::sort(matchedfileNames.begin(), matchedfileNames.end());
    matchedfileNames.erase(std::unique(matchedfileNames.begin(), matchedfileNames.end()), matchedfileNames.end());

    if(matchedfileNames.isEmpty())
    {
        QMessageBox Msgbox;
        Msgbox.setStyleSheet("QLabel{min-width: 300px;}");
        Msgbox.setText("No match found");
        Msgbox.exec();
    }
    if(!matchedfileNames.isEmpty())
    {
        QMessageBox Msgbox;
        Msgbox.setStyleSheet("QLabel{min-width: 300px;}");
        Msgbox.setText("Match found");
        ui->tableWidget->clearContents();
        Msgbox.exec();
    }
    for(int i = 0; i < matchedfileNames.size(); ++i)
    {
        ui->tableWidget->setRowCount(matchedfileNames.size());
        qDebug() << "matched files" << matchedfileNames[i];
        label.insert(1, "Select Files");
        ui->tableWidget->setColumnCount(2);
        ui->tableWidget->setHorizontalHeaderLabels(label);
        ui->tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
        //ui->tableWidget->verticalHeader()->hide();
        ui->tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);

        ui->tableWidget->setItem(i, 0, new QTableWidgetItem(matchedfileNames[i]));

        checkBoxWidget = new QWidget;
        QCheckBox* checkBox = new QCheckBox;
        QHBoxLayout* layoutCheckBox = new QHBoxLayout(checkBoxWidget);
        layoutCheckBox->addWidget(checkBox);
        layoutCheckBox->setAlignment(Qt::AlignCenter);
        layoutCheckBox->setContentsMargins(0, 0, 0, 0);
        ui->tableWidget->setCellWidget(i, 1, checkBoxWidget);
    }
}

bool MainWindow::fileExists(const QString& str)
{
    if(std::find(matchedfileNames.begin(), matchedfileNames.end(), str) != matchedfileNames.end())
        return true;
    else
        return false;
}


void MainWindow::on_horizontalSlider_valueChanged(int value) //slider
{
    ui->lcdNumber->display(value);
}

void MainWindow::on_pushButton_4_clicked() //Delete button
{
    for(int i = 0; i < matchedfileNames.size(); ++i)
    {
        QWidget* item = (ui->tableWidget->cellWidget(i, 1));
        QCheckBox* checkB = qobject_cast<QCheckBox*>(item->layout()->itemAt(0)->widget());
        if(checkB->isChecked())
        {
            QDir file;
            file.remove(matchedfilePath[i]);
        }
    }
    QMessageBox Msgbox;
    Msgbox.setStyleSheet("QLabel{min-width: 300px;}");
    Msgbox.setText("File deleted sucessfully");
    Msgbox.exec();
    ui->tableWidget->clearContents();
    ui->tableWidget->setColumnCount(0);
    ui->tableWidget->verticalHeader()->hide();
}
