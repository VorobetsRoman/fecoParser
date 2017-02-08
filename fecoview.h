#ifndef FECOVIEW_H
#define FECOVIEW_H

#include <QWidget>
#include <QFile>
#include <QDir>

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
    void    fecoParser              (QFile      *_inFile,
                                     QString    _outFileMask);

private slots:
    void    getInputFileNameSlot    ();
    void    getWorkingDirectorySlot ();
    void    runRecyclingSlot        ();

};

#endif // FECOVIEW_H
