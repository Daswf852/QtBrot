#include "mainwindow.hpp"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow) {

    ui->setupUi(this);

    targetW = 640;
    targetH = 480;

    maxIterationsSpinBox = this->findChild<QSpinBox *>("maxIterationsSpinBox");
    assert(maxIterationsSpinBox);
    threadCountSpinBox = this->findChild<QSpinBox *>("threadCountSpinBox");
    assert(threadCountSpinBox);
    zoomRatioSpinBox = this->findChild<QDoubleSpinBox *>("zoomRatioSpinBox");
    assert(zoomRatioSpinBox);
    drawWhileUpdatingCheckBox = this->findChild<QCheckBox *>("drawWhileUpdatingCheckBox");
    assert(drawWhileUpdatingCheckBox);
    modeSelectComboBox = this->findChild<QComboBox *>("modeSelectComboBox");
    assert(modeSelectComboBox);

    ResolutionWSpinBox = this->findChild<QSpinBox *>("ResolutionWSpinBox");
    assert(ResolutionWSpinBox);
    ResolutionHSpinBox = this->findChild<QSpinBox *>("ResolutionHSpinBox");
    assert(ResolutionHSpinBox);

    lastUpdateLabel = this->findChild<QLabel *>("lastUpdateLabel");
    assert(lastUpdateLabel);
    pixelRatioLabel = this->findChild<QLabel *>("pixelRatioLabel");
    assert(pixelRatioLabel);
    positionLabel = this->findChild<QLabel *>("positionLabel");
    assert(positionLabel);

    Reconstruct();
    UpdateUI();
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::Reconstruct() {
    broth.reset();
    broth = std::make_unique<Broth>(targetW, targetH);
    broth->SetNotifyCallback(std::bind(&MainWindow::NotifyCallback, this));
}

void MainWindow::UpdateUI() {

    static_cast<QSpinBox *>(maxIterationsSpinBox)->setValue(broth->GetMaxIterations()); //unless SetMaxIterations is called in some weird way this won't overflow
    static_cast<QSpinBox *>(threadCountSpinBox)->setValue(broth->GetThreads()); //same as above /shrug
    static_cast<QDoubleSpinBox *>(zoomRatioSpinBox)->setValue(broth->GetZoomRatio()); //this very well might overflow
    static_cast<QCheckBox *>(drawWhileUpdatingCheckBox)->setDown(broth->GetDrawWhileUpdating());

    static_cast<QSpinBox *>(ResolutionWSpinBox)->setValue(targetW);
    static_cast<QSpinBox *>(ResolutionHSpinBox)->setValue(targetH);

    int index = 0;
    switch (broth->GetProcessMethod()) {
        case Broth::None: index = 0;
            break;
        case Broth::BasicMT: index = 1;
            break;
        case Broth::OptMT: index = 2;
            break;
        case Broth::OpenCL: index = 3;
            break;
        default:
            break;

    }
    static_cast<QComboBox *>(modeSelectComboBox)->setCurrentIndex(index);

    std::stringstream ss;
    ss<<"Last update took "<<broth->GetPassedTime()<<" seconds";
    static_cast<QLabel *>(lastUpdateLabel)->setText(ss.str().c_str());
    ss.str("");

    ss<<"Currently at: "<<broth->GetCenterCoordinates();
    static_cast<QLabel *>(positionLabel)->setText(ss.str().c_str());
    ss.str("");

    ss<<"One number = "<<broth->GetPixelRatio()<<" pixels";
    static_cast<QLabel *>(pixelRatioLabel)->setText(ss.str().c_str());
    ss.str("");
}

void MainWindow::NotifyCallback() {
    UpdateUI();
}


void MainWindow::on_maxIterationsSpinBox_valueChanged(int arg1) {
    if (arg1 <= 0) arg1 = 256;
    if (broth->AreWorkersDone()) {
        broth->SetMaxIterations(arg1);
    }
    UpdateUI();
}

void MainWindow::on_threadCountSpinBox_valueChanged(int arg1) {
    if (arg1 <= 0 || (unsigned int)arg1 > std::thread::hardware_concurrency()) arg1 = 1;
    if (broth->AreWorkersDone()) {
        broth->SetThreads(arg1);
    }
    UpdateUI();
}

void MainWindow::on_zoomRatioSpinBox_valueChanged(double arg1) {
    if (arg1 <= 0.f || arg1 > 100.f) arg1 = 10.f;
    if (broth->AreWorkersDone()) {
        broth->SetZoomRatio(arg1);
    }
    UpdateUI();
}

void MainWindow::on_drawWhileUpdatingCheckBox_stateChanged(int arg1) {
    if (broth->AreWorkersDone()) {
        broth->SetDrawWhileUpdating(arg1);
    }
    UpdateUI();
}

void MainWindow::on_modeSelectComboBox_activated(int index) {
    if (broth->AreWorkersDone()) {
        switch (index) {
            case 0:
                broth->SetProcessMethod(Broth::None);
                break;
            case 1:
                broth->SetProcessMethod(Broth::BasicMT);
                break;
            case 2:
                broth->SetProcessMethod(Broth::OptMT);
                break;
            case 3:
                broth->SetProcessMethod(Broth::OpenCL);
                break;
            default:
                break;
        }
    }
}

void MainWindow::on_QuitButton_clicked() {
    this->close();
}

void MainWindow::on_RelaunchButton_clicked() {
    broth->RestartWorkers();
}

void MainWindow::on_ReconstructButton_clicked() {
    Reconstruct();
}

void MainWindow::on_FileDialogOpenerButton_clicked() {
    if (!broth->AreWorkersDone()) return;

    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Kernel"), ".", tr("OpenCL kernel files (*.cl)"));

    broth->SetOpenCLKernelPath(fileName.toStdString());
}

void MainWindow::on_ResolutionWSpinBox_valueChanged(int arg1) {
    targetW = arg1;
}

void MainWindow::on_ResolutionHSpinBox_valueChanged(int arg1) {
    targetH = arg1;
}
