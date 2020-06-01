#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QFileDialog>
#include <QMainWindow>

#include "broth.hpp"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

    public:
        MainWindow(QWidget *parent = nullptr);
        ~MainWindow();

        void Reconstruct();

        QWidget *maxIterationsSpinBox;
        QWidget *threadCountSpinBox;
        QWidget *zoomRatioSpinBox;
        QWidget *drawWhileUpdatingCheckBox;
        QWidget *modeSelectComboBox;

        QWidget *ResolutionWSpinBox;
        QWidget *ResolutionHSpinBox;

        QWidget *lastUpdateLabel;
        QWidget *pixelRatioLabel;
        QWidget *positionLabel;

    private slots:
        void on_maxIterationsSpinBox_valueChanged(int arg1);

        void on_threadCountSpinBox_valueChanged(int arg1);

        void on_zoomRatioSpinBox_valueChanged(double arg1);

        void on_drawWhileUpdatingCheckBox_stateChanged(int arg1);

        void on_modeSelectComboBox_activated(int index);

        void on_QuitButton_clicked();

        void on_RelaunchButton_clicked();

        void on_ReconstructButton_clicked();

        void on_FileDialogOpenerButton_clicked();

        void on_ResolutionWSpinBox_valueChanged(int arg1);

        void on_ResolutionHSpinBox_valueChanged(int arg1);

private:
        Ui::MainWindow *ui;
        unsigned int targetW = 640;
        unsigned int targetH = 480;
        std::unique_ptr<Broth> broth;

        void UpdateUI();
        void NotifyCallback();

};
#endif // MAINWINDOW_H
