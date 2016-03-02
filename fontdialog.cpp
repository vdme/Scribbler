#include "fontdialog.h"
#include "ui_fontdialog.h"

FontDialog::FontDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FontDialog)
{
    ui->setupUi(this);
    buttonGroup = new QButtonGroup();
    buttonGroup->addButton(ui->drawInPointButton);
    buttonGroup->addButton(ui->drawOutPointButton);
    buttonGroup->addButton(ui->drawLimitsButton);

    contextMenu = new QMenu();
    contextMenu->addAction(tr("Delete"));
    contextMenu->addAction(tr("Copy to choosen symbol"));

    connect(ui->choosenSymbolTextEdit, SIGNAL(textChanged()),
            this, SLOT(limitTextEdit()));
    connect(ui->fontFilePushButton, SIGNAL(clicked()),
            this, SLOT(loadFont()));
    connect(ui->SymbolFilesPushButton, SIGNAL(clicked()),
            this, SLOT(loadLetters()));
    connect(ui->buttonBox, SIGNAL(accepted()),
            this, SLOT(saveFont()));
    connect(ui->buttonBox, SIGNAL(rejected()),
            this, SLOT(rejectChanges()));
    connect(ui->treeWidget, SIGNAL(itemClicked(QTreeWidgetItem*,int)),
            this, SLOT(setTextFromItem(QTreeWidgetItem*)));
    connect(ui->treeWidget, SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(showTreeWidgetContextMenu(QPoint)));
    connect(contextMenu->actions().at(0), SIGNAL(triggered(bool)),
            this, SLOT(deleteLetter()));
    connect(contextMenu->actions().at(1), SIGNAL(triggered(bool)),
            this, SLOT(copyToChoosenSymbol()));
    connect(ui->drawInPointButton, SIGNAL(toggled(bool)),
            ui->svgEditor, SLOT(enableInPointDrawing(bool)));
    connect(ui->drawOutPointButton, SIGNAL(toggled(bool)),
            ui->svgEditor, SLOT(enableOutPointDrawing(bool)));
    connect(ui->drawLimitsButton, SIGNAL(toggled(bool)),
            ui->svgEditor, SLOT(enableLimitsDrawing(bool)));

    ui->fontFileTextEdit->setLineWrapMode(QTextEdit::NoWrap);
    ui->drawInPointButton->setCheckable(true);
    ui->drawOutPointButton->setCheckable(true);
    ui->drawLimitsButton->setCheckable(true);
    contextMenu->actions().at(1)->setEnabled(false);
    ui->drawInPointButton->setIcon(QIcon("://dark_cyan_dot.png"));
    ui->drawOutPointButton->setIcon(QIcon("://dark_magnetta_dot.png"));
    ui->drawLimitsButton->setIcon(QIcon("://border.png"));
    ui->drawInPointButton->setToolTip(tr("In Point"));
    ui->drawOutPointButton->setToolTip(tr("Out Point"));
    ui->drawLimitsButton->setToolTip(tr("Limits"));
    ui->treeWidget->setColumnCount(1);
    ui->treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->splitter->setSizes(QList <int> () << 200 << 350);
    lastItem = nullptr;
}

FontDialog::~FontDialog()
{
    delete ui;
    delete buttonGroup;
    delete contextMenu;
}

void FontDialog::loadFont()
{
    QString newFileName = QFileDialog::getSaveFileName(0, tr("Choose"), "",
                                                          tr("INI") +
                                                          "(*.ini);;" +
                                                          tr("All Files") +
                                                          "(*.*)");
    if (newFileName.isEmpty())
        return;
    else
        fontFileName = newFileName;

    lastItem = nullptr;
    ui->fontFileTextEdit->setText(fontFileName);

    QSettings fontSettings(fontFileName, QSettings::IniFormat);
    fontSettings.beginGroup("Font");
    fontSettings.setIniCodec(QTextCodec::codecForName("UTF-8"));

    if (fontSettings.allKeys().size() == 0)
    {
        fontSettings.endGroup();
        return;
    }

    font.clear();
    for (const QString &key : fontSettings.childKeys())
        for (const Letter &value : fontSettings.value(key).value<QList<Letter>>())
            font.insert(key.at(0).toLower(), value);

    //It's a dirty hack, which helps to distinguish uppercase and lowercase
    //letters on freaking case-insensetive Windows
    fontSettings.beginGroup("UpperCase");
    for (const QString &key : fontSettings.childKeys())
        for (const Letter &value : fontSettings.value(key).value<QList<Letter>>())
            font.insert(key.at(0).toUpper(), value);
    fontSettings.endGroup();

    fontSettings.endGroup();

    ui->treeWidget->clear();

    for (QChar key : font.uniqueKeys())
    {
        QTreeWidgetItem *letterItem = new QTreeWidgetItem(static_cast<QTreeWidget *>(nullptr), QStringList(QString(key)));
        for (const Letter & value : font.values(key))
        {
            QTreeWidgetItem *valueItem = new QTreeWidgetItem(letterItem, QStringList(value.fileName));
            letterItem->addChild(valueItem);
        }
        ui->treeWidget->insertTopLevelItem(ui->treeWidget->topLevelItemCount(), letterItem);
    }

    ui->svgEditor->load(QString());
    enableDrawButtons(false);
}

void FontDialog::loadLetters()
{
    QStringList files = QFileDialog::getOpenFileNames(0, tr("Choose"), "",
                                                         tr("SVG") +
                                                            "(*.svg);;" +
                                                         tr("All Files") +
                                                            "(*.*)");
    if (files.isEmpty())
        return;

    QChar letter = ui->choosenSymbolTextEdit->toPlainText().at(0);

    QTreeWidgetItem *topLevelItem;

    if (font.contains(letter))
    {
        for (int i = 0; i < files.count(); i++)
            for (int j = 0; j < font.values(letter).count(); j++)
                if (QFileInfo(files.at(i)).fileName() == font.values(letter).at(j).fileName)
                {
                    files.removeAt(i);
                    i--;
                    break;
                }

        QTreeWidgetItem *letterItem = ui->treeWidget->findItems(letter, Qt::MatchFixedString).first();
        topLevelItem = ui->treeWidget->topLevelItem(ui->treeWidget->indexOfTopLevelItem(letterItem));
    }
    else
    {
        topLevelItem = new QTreeWidgetItem(static_cast<QTreeWidget *>(nullptr), QStringList(QString(letter)));
        ui->treeWidget->insertTopLevelItem(ui->treeWidget->topLevelItemCount(), topLevelItem);
    }

    for (QString fileName : files)
    {
        Letter newLetterData = { QFileInfo(fileName).fileName(),
                        QPointF(-1.0, -1.0),
                        QPointF(-1.0, -1.0),
                        QRectF(-1.0, -1.0, -1.0, -1.0) };
        font.insert(letter, newLetterData);
        QTreeWidgetItem *newLetterItem = new QTreeWidgetItem(topLevelItem, QStringList(newLetterData.fileName));
        topLevelItem->addChild(newLetterItem);
        ui->treeWidget->setCurrentItem(newLetterItem);
        setTextFromItem(newLetterItem);
    }
}

void FontDialog::saveFont()
{
    if (fontFileName.isEmpty())
        return;

    QFile file (fontFileName);
    file.remove();

    QSettings fontSettings(fontFileName, QSettings::IniFormat);
    fontSettings.beginGroup("Font");
    fontSettings.setIniCodec(QTextCodec::codecForName("UTF-8"));

    loadFromEditorToFont();

    for (QChar &key : font.keys())
        if (key.isUpper())
        {
            fontSettings.beginGroup("UpperCase");
            fontSettings.remove(key);
            fontSettings.setValue(key, QVariant::fromValue(font.values(key)));
            fontSettings.endGroup();
        }
        else
        {
            fontSettings.remove(key);
            fontSettings.setValue(key, QVariant::fromValue(font.values(key)));
        }

    fontSettings.endGroup();

    emit fontReady();
}

void FontDialog::rejectChanges()
{
    font.clear();
    fontFileName.clear();
    lastItem = nullptr;
    enableDrawButtons(false);
    ui->SymbolFilesPushButton->setEnabled(false);
    ui->choosenSymbolTextEdit->clear();
    ui->fontFileTextEdit->clear();
    ui->treeWidget->clear();
    ui->svgEditor->disableDrawing();
    ui->svgEditor->hideAll();
}

void FontDialog::limitTextEdit()
{
    QString text = ui->choosenSymbolTextEdit->toPlainText();

    if(!fontFileName.isEmpty())
    {
        ui->SymbolFilesPushButton->setEnabled(!text.isEmpty());
        contextMenu->actions().at(1)->setEnabled(!text.isEmpty());
    }

    if (text.length() > 1)
    {
        text = text.left(1);
        ui->choosenSymbolTextEdit->setText(text);
    }
}

void FontDialog::setTextFromItem(QTreeWidgetItem *item)
{
    loadFromEditorToFont();

    if (item->parent() == nullptr)
    {
        enableDrawButtons(false);
        ui->choosenSymbolTextEdit->setText(item->text(0));
        ui->svgEditor->hideAll();
        ui->svgEditor->disableDrawing();
        lastItem = nullptr;
    }
    else
    {
        enableDrawButtons(true);
        ui->choosenSymbolTextEdit->setText(item->parent()->text(0));
        ui->svgEditor->load(QFileInfo(fontFileName).path() + '/' + item->text(0));
        QList<Letter> letterList = font.values(item->parent()->text(0).at(0));

        for (const Letter &letter : letterList)
            if (letter.fileName == item->text(0))
            {
                ui->svgEditor->setLetterData(letter.inPoint, letter.outPoint, letter.limits);
                break;
            }

        lastItem = item;
    }
}

void FontDialog::loadFromEditorToFont()
{
    if (lastItem != nullptr)
    {
        Letter newLetter;
        newLetter.fileName = lastItem->text(0);
        newLetter.inPoint = ui->svgEditor->getInPoint();
        newLetter.outPoint = ui->svgEditor->getOutPoint();
        newLetter.limits = ui->svgEditor->getLimits();
        QChar key = lastItem->parent()->text(0).at(0);
        QList<Letter> letterList = font.values(key);

        for (Letter &letter : letterList)
            if (letter.fileName == lastItem->text(0))
            {
                letter = newLetter;
                break;
            }

        font.remove(key);

        for (const Letter &letter : letterList)
            font.insert(key, letter);
    }
}

void FontDialog::deleteLetter()
{
    QTreeWidgetItem *selectedItem = ui->treeWidget->itemAt(ui->treeWidget->mapFromGlobal(contextMenu->pos()));
    bool hasParent = true;

    if (selectedItem->parent() == nullptr)
        hasParent = false;

    QChar key = hasParent ? selectedItem->parent()->text(0).at(0) : selectedItem->text(0).at(0);
    QTreeWidgetItem *topLevelItem = hasParent ? selectedItem->parent() : selectedItem;

    if (!hasParent || topLevelItem->childCount() <= 1)
    {
        delete topLevelItem;
        font.remove(key);
    }
    else
    {
        QList<Letter> letterList = font.values(key);

        for (const Letter &letterData : letterList)
            if (letterData.fileName == selectedItem->text(0))
            {
                font.remove(key, letterData);
                delete selectedItem;
                break;
            }
    }

    enableDrawButtons(false);
    ui->svgEditor->hideAll();
    ui->svgEditor->disableDrawing();
    lastItem = nullptr;
}

void FontDialog::enableDrawButtons(bool enable)
{
    if (!enable && buttonGroup->checkedButton() != nullptr)
    {
        buttonGroup->setExclusive(false);
        buttonGroup->checkedButton()->setChecked(false);
        buttonGroup->setExclusive(true);
    }

    ui->drawInPointButton->setEnabled(enable);
    ui->drawOutPointButton->setEnabled(enable);
    ui->drawLimitsButton->setEnabled(enable);
}

void FontDialog::showTreeWidgetContextMenu(QPoint pos)
{
    if (ui->treeWidget->itemAt(pos) == nullptr)
        return;

    contextMenu->exec(QCursor::pos());

}

void FontDialog::copyToChoosenSymbol()
{
    if (ui->choosenSymbolTextEdit->toPlainText().isEmpty())
        return;

    QTreeWidgetItem *selectedItem = ui->treeWidget->selectedItems().at(0);
    bool hasParent = true;

    if (selectedItem->parent() == nullptr)
        hasParent = false;

    QChar key = hasParent ? selectedItem->parent()->text(0).at(0) : selectedItem->text(0).at(0);
    QChar newKey = ui->choosenSymbolTextEdit->toPlainText().at(0);
    QList<QTreeWidgetItem *> letterItemList = ui->treeWidget->findItems(newKey, Qt::MatchCaseSensitive);

    QTreeWidgetItem *topLevelItem;

    if (letterItemList.isEmpty())
    {
        topLevelItem = new QTreeWidgetItem(static_cast<QTreeWidget *>(nullptr), QStringList(QString(newKey)));
        ui->treeWidget->insertTopLevelItem(ui->treeWidget->topLevelItemCount(), topLevelItem);
    }
    else
        topLevelItem = letterItemList.first();

    if (hasParent)
    {
        for (Letter &letterData : font.values(key))
            if (letterData.fileName == selectedItem->text(0))
            {
                if (font.contains(newKey, letterData))
                    return;

                font.insert(newKey, letterData);
                QTreeWidgetItem *newLetterItem = new QTreeWidgetItem(topLevelItem, QStringList(letterData.fileName));
                topLevelItem->addChild(newLetterItem);
                break;
            }
    }
    else
    {
        for (Letter &letterData : font.values(key))
        {
            if (font.contains(newKey, letterData))
                continue;

            font.insert(newKey, letterData);
            QTreeWidgetItem *newLetterItem = new QTreeWidgetItem(topLevelItem, QStringList(letterData.fileName));
            topLevelItem->addChild(newLetterItem);
        }
    }
}
