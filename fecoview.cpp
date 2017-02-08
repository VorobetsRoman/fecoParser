#include "fecoview.h"
#include "ui_fecoview.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>
#include <QTextCodec>




//====================================== Конструктор
FecoView::FecoView(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FecoView)
{
    ui->setupUi(this);
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
    setWindowTitle(QString::fromUtf8("Преобразование файла *.out"));

    connect(ui->tbInputName,    &QToolButton    ::pressed,
            this,               &FecoView       ::getInputFileNameSlot      );
    connect(ui->pbRun,          &QPushButton    ::pressed,
            this,               &FecoView       ::runRecyclingSlot          );
    connect(ui->tbWorkDir,      &QToolButton    ::pressed,
            this,               &FecoView       ::getWorkingDirectorySlot   );

    ui->leOutfileMask->setText("Воздушный_объект_");
}




//====================================== Деструктор
FecoView::~FecoView()
{
    delete ui;
}




//====================================== Получение имени входного файла
void FecoView::getInputFileNameSlot()
{
    QString fileName ("");
    fileName = QFileDialog::getOpenFileName(0,
                                            "Выбрать файл FECO(*.out)",
                                            "",
                                            "*.out");
    ui->leInputFile->setText(fileName);
}




//====================================== Выбор рабочей директории
void FecoView::getWorkingDirectorySlot()
{
    QString workingDirectory("");
    workingDirectory = QFileDialog::getExistingDirectory(0,
                                                         "Выберите директорию для сохранения результата",
                                                         "",
                                                         0);
    ui->leWorkDir->setText(workingDirectory);
}




//====================================== Запуск обработки
void FecoView::runRecyclingSlot()
{
    /// Производится проверка верности введенных пользователем данных
    // Проверка на осуществление выбора пользователем входного файла
    QString inFileName  = ui->leInputFile->text();
    if (inFileName == "")
    {
        QMessageBox::warning(0,
                             "Ошибка",
                             "Неверное имя входного файла");
        ui->leInputFile->setFocus();
        return;
    }

    // Проверка на возможность открытия входного файла
    QFile inFile(inFileName);
    if (!inFile.open(QIODevice::ReadOnly))
    {
        QMessageBox::warning(0,
                             "Ошибка",
                             "Oшибка открытия входного файла");
    }

    // Проверка на осуществление выбора пользователем рабочей директории
    // если пользователь не выбрал, рабочей директорией считается текущая программы
    QString workDir = ui->leWorkDir->text();
    if (workDir == "")
    {
        workDir = qApp->applicationDirPath();
        ui->leWorkDir->setText(workDir);
    }
    QDir::setCurrent(workDir);

    QString fileMask = ui->leOutfileMask->text();

    fecoParser(&inFile, fileMask);
}




//====================================== Парсер FECO
void FecoView::fecoParser(QFile *_inFile,
                          QString _outFileMask)
{
    double      actualFreq      {0};
    double      actualPhi       {-1};
    QFile       outFile;
    qint64      infileSize = _inFile->size();

    QByteArray  thettas, results;
    bool        thettasReady    {false};
    QByteArray  nextLine;
    QString     str             {""};

    while(!_inFile->atEnd())
    {
        ui->pbRecycling->setValue(100 * _inFile->pos() / infileSize + 1);

        nextLine = _inFile->readLine();
        str(nextLine);

        /// Если получаем новое значение частоты, закрываем старый файл,
        /// создаём новый

        if (str.contains("Frequency in Hz:"))
        {
            QString readed = str.section("=", 1);
            int newFreq = readed.toDouble() / 1000000;

            if (actualFreq != newFreq)
            {
                if (outFile.isOpen())
                {
                    outFile.write(thettas);
                    thettasReady = true;
                    outFile.write(results);
                    results.clear();
                    char data {10};
                    outFile.write((char*)&data, sizeof(char));
                    outFile.close();
                }
                // Создаём файл для нового значения частоты
                actualFreq = newFreq;
                readed = QString::number(int(actualFreq)) + "MHz"; // Наименование файла

                outFile.setFileName(_outFileMask + readed);
                if (!outFile.open(QIODevice::WriteOnly))
                {
                    QMessageBox::warning(0, "Ошибка",
                                         QString("Невозможно открыть файл '%1' для записи")
                                         .arg(_outFileMask + readed));
                    break;
                }
                outFile.write("0\t");
            }
        }

        /// Для значений
        if (str.contains("THETA    PHI"))
        {
            QString nextStr = _inFile->readLine();
            if (nextStr.isEmpty())
            {
                QMessageBox::warning(0,
                                     QString::fromUtf8("Ошибка"),
                                     QString::fromUtf8("Невозможно прочитать очередную строку файла"));
                break;
            }

            QString phi         = nextStr.section(" ", 1, 1, QString::SectionSkipEmpty);
            QString scattering  = nextStr.section(" ", 6, 6, QString::SectionSkipEmpty);

            // Если это новое значение азимута то перейти на новую строку
            if (actualPhi != phi.toFloat())
            {
                if (!thettas.isNull())
                    thettasReady = true;
                results.append("\n" + phi);
                actualPhi = phi.toFloat();
            }
            results.append("\t" + scattering);

            if (!thettasReady) {
                QString theta = nextStr.section(" ", 0, 0, QString::SectionSkipEmpty);
                qDebug() << theta;
                thettas.append(theta + "\t");
            }
        }
    }

    _inFile->close();
}














