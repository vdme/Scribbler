#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QtCore/QSettings>
#include <QtCore/QSignalMapper>
#include <QtWidgets/QDialog>
#include <QtWidgets/QColorDialog>

namespace Ui {
class PreferencesDialog;
}

class PreferencesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PreferencesDialog(QWidget *parent = 0);
    ~PreferencesDialog();

public slots:
    void loadSettingsToFile();
    void loadSettingsFromFile();
    bool alternateMargins();

signals:
    void settingsChanged();

private:
    Ui::PreferencesDialog *ui;
    QSignalMapper *sheetSizeSignalMapper;  //is necessary to connect QRadioButtons
                                            //from the sheetSize group to setSheetSize()
    enum class SheetSize {
        A4,
        A5,
        Custom
    };

    bool changedByProgram;

private slots:
    void setSheetSize(int size);
    void setColor();
    void changeSheetOrientation();
};

#endif // PREFERENCESDIALOG_H
