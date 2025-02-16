#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QDesktopServices>
#include <QFile>
#include <QTimer>
#include <QDir>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow), timer(new QTimer(this)), isProcessing(false)
{

    ui->setupUi(this);
    ui->operationsComboBox->addItems({"XOR", "AND", "OR", "NOT"});
    connect(timer, &QTimer::timeout, this, &MainWindow::processFiles);
    connect(ui->processFileButton, &QPushButton::clicked, this, &MainWindow::onStartStopClicked);
    connect(ui->openOutputDirectoryButton, &QPushButton::clicked, this, &MainWindow::openOutputDirectory);
//    connect(ui->radioOnce, &QRadioButton::toggled, this, &MainWindow::updateUiState);
    connect(ui->radioTimer, &QRadioButton::toggled, this, &MainWindow::updateUiState);
    updateUiState();

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::updateUiState() {
    bool isTimerMode = ui->radioTimer->isChecked();
    ui->spinInterval->setEnabled(isTimerMode);
    ui->processFileButton->setText(isTimerMode ? "Старт" : "Модифицировать файлы");
    ui->procfilesText->setText("Файлов обработано:");

}

void MainWindow::onStartStopClicked() {
    if (isProcessing) {
        ui->radioTimer->setEnabled(true);
        ui->spinInterval->setEnabled(true);
        ui->overWriteCheckBox->setEnabled(true);
        ui->deleteInputFilesCheckBox->setEnabled(true);
        timer->stop();
        isProcessing = false;
        ui->processFileButton->setText("Старт");
    } else {
        // Проверка режима
        if (ui->radioTimer->isChecked()) {
            ui->radioTimer->setEnabled(false);
            ui->spinInterval->setEnabled(false);
            ui->overWriteCheckBox->setEnabled(false);
            ui->deleteInputFilesCheckBox->setEnabled(false);
            processedFiles = 0;
            int intervalSec = ui->spinInterval->value();
            timer->start(intervalSec * 1000); // Конвертация в мс
            isProcessing = true;
            ui->processFileButton->setText("Стоп");
        } else {
            processedFiles = 0;
            processFiles();
        }
    }
}

void MainWindow::on_inputDirectoryButton_clicked()
{
    const QString dir = QFileDialog::getExistingDirectory(this, "Выбрать папку", QDir::homePath());
        if (!dir.isEmpty()) {
            ui->inputDirEdit->setText(dir);
        }
}

QString MainWindow::getSelectedOperation() const {
    return ui->operationsComboBox->currentText();
}

QByteArray MainWindow::getOpValue() {
    QString input = ui->opValueLineEdit->text().trimmed();

    if (input.isEmpty() && getSelectedOperation() != "NOT") {
            QMessageBox::warning(this, "Ошибка", "Введите значение для операции!");
            return QByteArray();
    }

    if (input.startsWith("0x", Qt::CaseInsensitive)) {
            input = input.mid(2);
    }

    if (input.size() % 2 != 0) {
            input.prepend('0');
    }

    if (input.size() != 16 && getSelectedOperation() != "NOT") {
            QMessageBox::critical(this, "Ошибка", "Требуется 8-байтное значение (16 HEX символов)");
            return QByteArray();
    }

    return QByteArray::fromHex(input.toLatin1());
}

bool MainWindow::isOverwriteEnabled() const {
    return ui->overWriteCheckBox->isChecked(); // Возвращает true, если флажок установлен
}

bool MainWindow::isdeleteInputFilesCheckBox() const {
    return ui->deleteInputFilesCheckBox->isChecked(); // Возвращает true, если флажок установлен

}

void MainWindow::processFiles(){

    const QString inputDir = ui->inputDirEdit->text();
    const QString outputDir = ui->outputDirEdit->text();

    if (inputDir.isEmpty() || outputDir.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Укажите входную и выходную директории!");
        return;
    }

    QStringList masks = ui->maskEdit->text().split(",", Qt::SkipEmptyParts);
    for (QString &mask : masks) {
        mask = mask.trimmed(); // Удаляем пробелы в начале и конце
    }
    const QDir dir(inputDir);


    if (dir.entryList(masks, QDir::Files | QDir::NoDotAndDotDot).isEmpty()){
        QMessageBox::warning(this, "Ошибка", "В директории не содержится подходящих файлов!");
        if (ui->radioTimer->isChecked()){
            ui->radioTimer->setEnabled(true);
            ui->spinInterval->setEnabled(true);
            ui->overWriteCheckBox->setEnabled(true);
            ui->deleteInputFilesCheckBox->setEnabled(true);
            timer->stop();
            isProcessing = false;
            ui->processFileButton->setText("Старт");
        }
        return;
    }

    for (const QString &file : dir.entryList(masks, QDir::Files)) {
        const QString inputPath = dir.absoluteFilePath(file);

        if (isFileLocked(inputPath)) {
            qDebug() << "Файл заблокирован:" << inputPath;
            continue;
        }
        processSingleFile(inputPath, outputDir);
        processedFiles++;
        procendText = QString("Файлов обработано: %1").arg(processedFiles);
        ui->procfilesText->setText(procendText);
    }
}
void MainWindow::processSingleFile(const QString &inputPath, const QString &outputDir) {

    QFile inputFile(inputPath);
    if (!inputFile.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "Ошибка", "Не удалось открыть файл: " + inputPath);
        return;
    }

    QByteArray data = inputFile.readAll();
    inputFile.close();

    const QString operation = getSelectedOperation();
    const QByteArray opValue = getOpValue();


    if (operation != "NOT" && opValue.isEmpty()) {
        return;
    }

    // Применение операции
    for (int i = 0; i < data.size(); ++i) {
        if (operation == "XOR") {
            data[i] ^= opValue[i % opValue.size()];
        } else if (operation == "AND") {
            data[i] &= opValue[i % opValue.size()];
        } else if (operation == "OR") {
            data[i] |= opValue[i % opValue.size()];
        } else if (operation == "NOT") {
            data[i] = ~data[i];
        }
    }
    QString outputFileName = "modified_" + QFileInfo(inputPath).fileName();
    QString outputPath = QDir(outputDir).filePath(outputFileName);

    if (!ui->overWriteCheckBox->isChecked()) {
        int counter = 1;
        while (QFile::exists(outputPath)) {
            outputPath = QDir(outputDir).filePath(
                QString("modified_%1_%2").arg(counter++).arg(QFileInfo(inputPath).fileName()));
        }
    }

    // Сохранение файла
    QFile outputFile(outputPath);
    if (outputFile.open(QIODevice::WriteOnly)) {
        outputFile.write(data); // Сохраняем бинарные данные
        outputFilePath = outputPath; // Сохраняем последний путь

        if (ui->deleteInputFilesCheckBox->isChecked()) {
            QFile::remove(inputPath);
        }
    } else {
        QMessageBox::critical(this, "Ошибка", "Ошибка сохранения файла: " + outputPath);
    }
}

bool MainWindow::isFileLocked(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadWrite | QIODevice::ExistingOnly)) {
        return true;
    }
    file.close();
    return false;
}

void MainWindow::openOutputDirectory() {

    if (!outputFilePath.isEmpty()) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(outputFilePath).absolutePath()));
    }

}

void MainWindow::on_outputDirButton_clicked()
{
    const QString dir = QFileDialog::getExistingDirectory(this, "Выбрать папку", QDir::homePath());
        if (!dir.isEmpty()) {
            ui->outputDirEdit->setText(dir);
        }
}

