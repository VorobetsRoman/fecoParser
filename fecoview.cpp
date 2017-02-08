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
        return;
    }

    // Проверка на осуществление выбора пользователем рабочей директории
    // если оператор не выбрал, рабочей директорией считается текущая программы
    QString workDir = ui->leWorkDir->text();
    if (workDir == "")
    {
        workDir = qApp->applicationDirPath();
        ui->leWorkDir->setText(workDir);
    }
    QDir::setCurrent(workDir);

    fecoParser(&inFile);
}




//====================================== Парсер FECO
void FecoView::fecoParser(QFile *_inFile)
{
    double      actualFreq      {0};
    double      actualPhi       {-1};
    qint64      infileSize      {_inFile->size()};

    QList <FileLine*> scatteringValues;
    QString     nextLine        {""};
    FileLine     *fileLine      {NULL};

    while(!_inFile->atEnd())
    {
        ui->pbRecycling->setValue(100 * _inFile->pos() / infileSize + 1);

        nextLine = _inFile->readLine();

        /// Если получаем новое значение частоты, сохраняем полученные данные в файл
        if (nextLine.contains("Frequency in Hz:"))
        {
            int newFreq = nextLine.section("=", 1).toDouble() / 1000000;

            if (actualFreq != newFreq)
            {
                if (!scatteringValues.isEmpty()) {   // проверка, если это первое значение
                    saveToFile(actualFreq, &scatteringValues);
                }
                actualFreq = newFreq;
                continue;
            }
        }

        /// Для значений
        if (nextLine.contains("THETA    PHI"))
        {
            nextLine = _inFile->readLine();
            if (nextLine.isEmpty())
            {
                QMessageBox::warning(0,
                                     QString::fromUtf8("Ошибка"),
                                     QString::fromUtf8("Невозможно прочитать очередную строку файла"));
                break;
            }

            QString phi         = nextLine.section(" ", 1, 1, QString::SectionSkipEmpty);
            QString scattering  = nextLine.section(" ", 6, 6, QString::SectionSkipEmpty);

            // Если это новое значение азимута то перейти на новую строку
            if (actualPhi != phi.toFloat())
            {
                if (!thettas.isNull()) {
                    thettasReady = true;
                }

                actualPhi = phi.toFloat();
                fileLine = new FileLine;
                fileLine->azimuth = actualPhi;
                scatteringValues.append(fileLine);
            }

            // Добавляем новые значения
            if (fileLine) {
                fileLine->values.append("\t" + scattering);
            }

            if (!thettasReady) { //дописать новое значение угла места, если массив еще не заполнен
                thettas.append(nextLine.section(" ", 0, 0, QString::SectionSkipEmpty) + "\t");
            }
        }
    }

    if (!scatteringValues.isEmpty()) {
        saveToFile(actualFreq, &scatteringValues); // записать значения для последней частоты
    }

    _inFile->close();
}




//======================================
void FecoView::saveToFile(double frequency,
                          QList <FileLine*> *newData)
{
    thettasReady = true;

    // Создаём файл для нового значения частоты
    QString fileMask    = ui->leOutfileMask->text();
    QString readed      = QString::number(int(frequency)) + "MHz"; // Наименование файла
    QFile outFile(fileMask + readed);

    if (!outFile.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(0, "Ошибка",
                             QString("Невозможно открыть файл '%1' для записи")
                             .arg(fileMask + readed));
    }

    // запись заголовка таблицы
    outFile.write("0\t");
    outFile.write(thettas);

    // Если ЭПР рассчитан не на 360 а только до 180
    if (newData->last()->azimuth == 180)
    {
        for (int i = newData->count() - 1 - 1; i >= 0; i--) //цикл с предпоследнего до первого элемента
        {
            FileLine *fileLine = new FileLine;
            fileLine->azimuth = 360 - newData->at(i)->azimuth;
            fileLine->values = newData->at(i)->values;
            newData->append(fileLine);
        }
    }

    // запись значений ЭПР
    char newLine {10};
    outFile.write((char*)&newLine, sizeof(char));
    for (FileLine *fileLine : *newData)
    {
        QByteArray ba;
        ba.append(QString::number(fileLine->azimuth));
        ba.append("\t");
        ba.append(fileLine->values);
        outFile.write(ba);
        outFile.write((char*)&newLine, sizeof(char));
    }

    outFile.write("\n");
    newData->clear();
}













