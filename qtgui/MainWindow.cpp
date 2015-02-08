#include "MainWindow.h"
#include "../version.h"

#include "../DslWriter.h"
#include "../dictlsd/lsd.h"
#include "../dictlsd/LSAReader.h"
#include "../dictlsd/tools.h"

#include <QTableView>
#include <QAbstractListModel>
#include <QMimeData>
#include <QStringList>
#include <QByteArray>
#include <QDataStream>
#include <QPixmap>
#include <QHeaderView>
#include <QSortFilterProxyModel>
#include <QDockWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QItemSelectionModel>
#include <QMessageBox>
#include <QUrl>
#include <QFile>
#include <QFileInfo>
#include <QKeyEvent>
#include <QFileDialog>
#include <QProgressBar>
#include <QThread>

#include <vector>
#include <memory>
#include <algorithm>
#include <assert.h>

using namespace dictlsd;

class DictionaryEntry {
    std::unique_ptr<FileStream> _stream;
    QString _path;
    QString _fileName;
protected:
    std::unique_ptr<BitStreamAdapter> _adapter;
public:
    DictionaryEntry(QString path)
        : _stream(new FileStream(path.toStdString())),
          _path(path),
          _fileName(QFileInfo(path).fileName()),
          _adapter(new BitStreamAdapter(_stream.get()))
    { }
    QString path() { return _path; }
    QString fileName() { return _fileName; }
    virtual QString name() = 0;
    virtual QString source() = 0;
    virtual QString target() = 0;
    virtual unsigned entries() = 0;
    virtual QString version() = 0;
    virtual std::vector<unsigned char> const& icon() = 0;
    virtual bool supported() = 0;
    virtual void dump(QString outDir, std::function<void(int)> log) = 0;
};

class LSDDictionaryEntry : public DictionaryEntry {
    LSDDictionary _reader;
    QString printLanguage(int code) {
        return QString::fromStdString(toUtf8(langFromCode(code)));
    }
public:
    LSDDictionaryEntry(QString path)
        : DictionaryEntry(path),
          _reader(_adapter.get()) { }
    virtual QString name(){
        return QString::fromStdString(toUtf8(_reader.name()));
    }
    virtual QString source() {
        auto source = _reader.header().sourceLanguage;
        return QString("%1 (%2)").arg(source).arg(printLanguage(source));
    }
    virtual QString target() {
        auto target = _reader.header().targetLanguage;
        return QString("%1 (%2)").arg(target).arg(printLanguage(target));
    }
    virtual unsigned entries() {
        return _reader.header().entriesCount;
    }
    virtual QString version() {
        return QString("%1").arg(_reader.header().version, 1, 16);
    }
    virtual const std::vector<unsigned char> &icon() {
        return _reader.icon();
    }
    virtual bool supported() {
        return _reader.supported();
    }
    virtual void dump(QString outDir, std::function<void(int)> log) {
        writeDSL(&_reader, fileName().toStdString(), outDir.toStdString(), false, [&](int i, std::string) { log(i); });
    }
};

class LSADictionaryEntry : public DictionaryEntry {
    LSAReader _reader;
    std::vector<unsigned char> _icon = {};
public:
    LSADictionaryEntry(QString path)
        : DictionaryEntry(path),
          _reader(_adapter.get()) { }
    virtual QString name() { return ""; }
    virtual QString source() { return ""; }
    virtual QString target() { return ""; }
    virtual unsigned entries() { return _reader.entriesCount(); }
    virtual QString version() { return ""; }
    virtual const std::vector<unsigned char> &icon() { return _icon; }
    virtual bool supported() { return true; }
    virtual void dump(QString outDir, std::function<void(int)> log) {
        decodeLSA(path().toStdString(), outDir.toStdString(), log);
    }
};

class LSDListModel : public QAbstractListModel {
    std::vector<std::unique_ptr<DictionaryEntry>> _dicts;
    std::vector<QString> _columns;
public:
    LSDListModel() {
        _columns = {
            "",
            "File Name",
            "Name",
            "Source",
            "Target",
            "Entries",
            "Version"
        };
    }
    virtual Qt::DropActions supportedDropActions() const {
        return Qt::CopyAction | Qt::MoveAction;
    }
    virtual QStringList mimeTypes() const {
        QStringList types;
        types << "text/uri-list";
        return types;
    }
    virtual bool dropMimeData(const QMimeData *data, Qt::DropAction action, int, int, const QModelIndex &parent) {
        beginRemoveRows(parent, 0, _dicts.size() - 1);
        _dicts.clear();
        endRemoveRows();
        if (action == Qt::IgnoreAction)
            return true;
        for (QUrl fileUri : data->urls()) {
            QString path = fileUri.toLocalFile();
            try {
                QString ext = QFileInfo(path).suffix().toLower();
                if (ext == "lsd") {
                    _dicts.emplace_back(new LSDDictionaryEntry(path));
                } else if (ext == "lsa") {
                    _dicts.emplace_back(new LSADictionaryEntry(path));
                }
            } catch(std::exception& e) {
                QMessageBox::warning(nullptr, QString(e.what()), path);
            }
        }
        beginInsertRows(parent, 0, _dicts.size() - 1);
        endInsertRows();
        return true;
    }
    virtual Qt::ItemFlags flags(const QModelIndex &index) const {
        return QAbstractItemModel::flags(index) | (index.isValid() ? Qt::NoItemFlags : Qt::ItemIsDropEnabled);
    }
    virtual int rowCount(const QModelIndex &) const {
        return _dicts.size();
    }
    std::vector<std::unique_ptr<DictionaryEntry>>& dicts() {
        return _dicts;
    }
    virtual QVariant data(const QModelIndex &index, int role) const {
        auto& dict = _dicts.at(index.row());
        if (role == Qt::DecorationRole && index.column() == 0) {
            QPixmap icon;
            auto&& rawBytes = dict->icon();
            icon.loadFromData(rawBytes.data(), rawBytes.size());
            return QVariant(icon);
        }

        if (role == Qt::BackgroundColorRole && !dict->supported()) {
            return QColor("#fbe3e4");
        }

        if (role == Qt::DisplayRole) {
            switch(index.column()) {
            case 1: return dict->fileName();
            case 2: return dict->name();
            case 3: return dict->source();
            case 4: return dict->target();
            case 5: return dict->entries();
            case 6: return dict->version();
            }
        }
        return QVariant();
    }
    virtual int columnCount(const QModelIndex &) const {
        return _columns.size();
    }
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const {
        if (section == -1)
            return QVariant();
        if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
            return _columns.at(section);
        }
        return QVariant();
    }
};

class ConvertWithProgress : public QObject {
    Q_OBJECT
    std::vector<DictionaryEntry*> _dicts;
    QString _outDir;
signals:
    void statusUpdated(int percent);
    void nextDictionary(QString name);
    void error(QString dict, QString message);
    void done();
public:
    ConvertWithProgress(std::vector<DictionaryEntry*> dicts, QString outDir)
        : _dicts(dicts), _outDir(outDir) { }
public slots:
    void start() {
        for (DictionaryEntry* dict : _dicts) {
            emit nextDictionary(dict->fileName());
            try {
                dict->dump(_outDir, [&](int percent) {
                    emit statusUpdated(percent);
                });
            } catch (std::exception& e) {
                emit error(dict->fileName(), e.what());
                return;
            }
            emit statusUpdated(100);
        }
        emit done();
    }
};

void MainWindow::convert(bool selectedOnly) {
    QString dir = QFileDialog::getExistingDirectory(this, "Select directory to save DSL");
    if (dir.isEmpty())
        return;
    std::vector<DictionaryEntry*> dicts;
    if (selectedOnly) {
        for (auto proxyIndex : _tableView->selectionModel()->selectedRows()) {
            auto index = _proxyModel->mapToSource(proxyIndex);
            dicts.push_back(_model->dicts()[index.row()].get());
        }
    } else {
        for (auto& dict : _model->dicts()) {
            dicts.push_back(dict.get());
        }
    }
    auto it = std::remove_if(begin(dicts), end(dicts), [&](DictionaryEntry* dict) {
        return !dict->supported();
    });

    if (it != end(dicts)) {
        dicts.erase(it, end(dicts));
        QMessageBox::warning(this, "Unsupported dictionaries",
            "Some dictionaries in the list aren't supported and wont be decompiled.");
    }

    _progress->setMaximum(dicts.size() + 1);
    _progress->setValue(0);

    auto thread = new QThread();
    auto converter = new ConvertWithProgress(dicts, dir);
    converter->moveToThread(thread);

    connect(converter, &ConvertWithProgress::statusUpdated, this, [=](int percent) {
        _dictProgress->setValue(percent);
    });
    connect(converter, &ConvertWithProgress::nextDictionary, this, [=](QString name) {
        _progress->setValue(_progress->value() + 1);
        _dictProgress->setValue(0);
        _currentDict->setText(name + "...");
    });
    auto enable = [=]{
        _tableView->setEnabled(true);
        _convertAllButton->setEnabled(true);
        _convertSelectedButton->setEnabled(true);
    };
    connect(converter, &ConvertWithProgress::done, this, [=] {
        _currentDict->setText("");
        _progress->setValue(_progress->maximum());
        enable();
    });
    connect(converter, &ConvertWithProgress::error, this, [=](QString dict, QString message) {
        QMessageBox::critical(this, "An error occurred",
            QString("Decompiling of %1 failed with error\n%2").arg(dict, message));
        _progress->setValue(0);
        enable();
    });

    _tableView->setEnabled(false);
    _convertAllButton->setEnabled(false);
    _convertSelectedButton->setEnabled(false);

    connect(thread, &QThread::started, converter, &ConvertWithProgress::start);
    connect(converter, &ConvertWithProgress::done, converter, &QObject::deleteLater);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    thread->start();
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setMinimumWidth(500);
    setMinimumHeight(250);
    resize(800, 400);

    setWindowTitle(QString("lsd2dsl - %1").arg(g_version));

    auto form = new QFormLayout(this);
    auto totalLabel = new QLabel("0");
    auto selectedLabel = new QLabel("0");
    form->addRow("Total:", totalLabel);
    form->addRow("Selected:", selectedLabel);

    auto vbox = new QVBoxLayout(this);
    auto rightPanelWidget = new QWidget(this);
    rightPanelWidget->setLayout(vbox);
    vbox->addLayout(form);
    vbox->addWidget(_currentDict = new QLabel(this));
    vbox->addWidget(_dictProgress = new QProgressBar(this));
    vbox->addWidget(_progress = new QProgressBar(this));
    vbox->addWidget(_convertAllButton = new QPushButton("Convert all"));
    vbox->addWidget(_convertSelectedButton = new QPushButton("Convert selected"));
    vbox->addStretch(1);

    auto rightDock = new QDockWidget(this);
    rightDock->setTitleBarWidget(new QWidget());
    auto rightPanel = new QWidget(this);
    rightPanel->setMinimumWidth(250);
    rightPanel->setLayout(vbox);
    rightDock->setWidget(rightPanel);
    rightDock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    addDockWidget(Qt::RightDockWidgetArea, rightDock);

    auto topDock = new QDockWidget(this, Qt::FramelessWindowHint);
    topDock->setTitleBarWidget(new QWidget());
    topDock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    auto dragDropLabel = new QLabel("Drag and drop LSD/LSA files here");
    dragDropLabel->setMargin(5);
    topDock->setMinimumHeight(30);
    topDock->setMaximumHeight(30);
    topDock->setWidget(dragDropLabel);
    addDockWidget(Qt::TopDockWidgetArea, topDock);

    _tableView = new QTableView(this);
    _tableView->setDropIndicatorShown(true);
    _tableView->setAcceptDrops(true);
    _proxyModel = new QSortFilterProxyModel;
    _model = new LSDListModel;
    _proxyModel->setSourceModel(_model);
    _tableView->setModel(_proxyModel);
    _tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    _tableView->setSortingEnabled(true);
    _tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    setCentralWidget(_tableView);

    _convertAllButton->setEnabled(false);
    _convertSelectedButton->setEnabled(false);

    connect(_tableView->selectionModel(), &QItemSelectionModel::selectionChanged, [=] {
        int count = _tableView->selectionModel()->selectedRows().size();
        _convertSelectedButton->setEnabled(count > 0);
        selectedLabel->setText(QString::number(count));
    });
    auto updateRowCount = [=]() {
        totalLabel->setText(QString::number(_model->rowCount(QModelIndex())));
        _convertAllButton->setEnabled(_model->rowCount(QModelIndex()) > 0);
    };
    connect(_model, &QAbstractItemModel::rowsInserted, updateRowCount);
    connect(_model, &QAbstractItemModel::rowsRemoved, updateRowCount);
    connect(_convertAllButton, &QPushButton::clicked, [=] { convert(false); });
    connect(_convertSelectedButton, &QPushButton::clicked, [=] { convert(true); });
}

MainWindow::~MainWindow() { }

#include "MainWindow.moc"
