#ifndef FECOVIEW_H
#define FECOVIEW_H

#include <QWidget>
#include <QFile>
#include <QDir>

struct FileLine {
    int azimuth;
    QString values;
};


namespace Ui {
class FecoView;
}

class FecoView : public QWidget
{
    Q_OBJECT

public:
    explicit FecoView               (QWidget *parent = 0);
            ~FecoView               ();

private:
    Ui::FecoView *ui {NULL};
    QByteArray  thettas;
    bool        thettasReady    {false};

    void    fecoParser              (QFile      *_inFile);
    void    saveToFile              (const double frequency,
                                     QList <FileLine*> *newData);

private slots:
    void    getInputFileNameSlot    ();
    void    getWorkingDirectorySlot ();
    void    runRecyclingSlot        ();

};

#endif // FECOVIEW_H
