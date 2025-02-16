#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDir>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void updateUiState();
    void onStartStopClicked();
    void on_inputDirectoryButton_clicked();
    void on_outputDirButton_clicked();
    void openOutputDirectory();


private:
    Ui::MainWindow *ui;
    QTimer *timer;
    bool isProcessing;
    QString outputFilePath;

    // Core Logic
    void processFiles();
    void processSingleFile(const QString &inputPath, const QString &outputDir);
    bool isFileLocked(const QString &path);

    // Helpers
    QString getSelectedOperation() const;
    QString procendText;
    QByteArray getOpValue();
    bool isOverwriteEnabled() const;
    bool isdeleteInputFilesCheckBox() const;
    int processedFiles = 0;
};

#endif // MAINWINDOW_H
