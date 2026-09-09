#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QComboBox>
#include <QPushButton>
#include <QColorDialog>
#include <QMessageBox>
#include <QFileSystemWatcher>
#include <QTimer>
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QStandardPaths>
#include <QCloseEvent>
#include <QIcon>

#include <random>

static const QString CONFIG_DIR_NAME = "sticky";
static const QStringList SOCHNYE_COLORS = {
    "#FFF9C4", "#C8E6C9", "#B3E5FC", "#F8BBD0", "#E1BEE7", "#FFE0B2"
};

static QString getConfigDir() {
    static QString dir;
    if (dir.isEmpty()) {
        dir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/" + CONFIG_DIR_NAME;
    }
    return dir;
}

static QString getConfigFile() {
    return getConfigDir() + "/config.json";
}

static QString getNotePath(const QString &noteId) {
    return getConfigDir() + "/" + noteId + ".txt";
}

static QStringList getExistingNoteIds() {
    QDir dir(getConfigDir());
    if (!dir.exists()) return {};

    QStringList ids;
    for (const QFileInfo &fi : dir.entryInfoList({"*.txt"}, QDir::Files)) {
        QString base = fi.completeBaseName();
        bool ok = false;
        base.toInt(&ok);
        if (ok) ids << base;
    }
    std::sort(ids.begin(), ids.end(), [](const QString &a, const QString &b) {
        return a.toInt() < b.toInt();
    });
    return ids;
}

static QString randomSochnyColor() {
    static std::mt19937 gen{std::random_device{}()};
    std::uniform_int_distribution<int> dist(0, SOCHNYE_COLORS.size() - 1);
    return SOCHNYE_COLORS[dist(gen)];
}


class StickyApp : public QWidget {
    Q_OBJECT
public:
    StickyApp()
        : currentNoteId_("1"), isUpdating_(false), darkMode_(false),
          watcher_(nullptr), textSaveTimer_(this), settingsSaveTimer_(this)
    {
        QDir().mkpath(getConfigDir());

        textSaveTimer_.setSingleShot(true);
        connect(&textSaveTimer_, &QTimer::timeout, this, &StickyApp::doSaveNote);

        settingsSaveTimer_.setSingleShot(true);
        connect(&settingsSaveTimer_, &QTimer::timeout, this, &StickyApp::doSaveSettings);

        loadSettings();
        initUI();
        scanAndUpdateSelector();
        loadNoteData();
    }

private:
    QString currentNoteId_;
    bool isUpdating_;
    bool darkMode_;
    QJsonObject config_;
    QFileSystemWatcher *watcher_;
    QTextEdit *textEdit_;
    QComboBox *noteSelector_;
    QPushButton *themeBtn_;
    QPushButton *colorBtn_;
    QPushButton *addBtn_;
    QPushButton *deleteBtn_;
    QTimer textSaveTimer_;
    QTimer settingsSaveTimer_;

    /* ---- settings ---- */

    void loadSettings() {
        QString path = getConfigFile();
        if (QFile::exists(path)) {
            QFile f(path);
            if (f.open(QIODevice::ReadOnly)) {
                QJsonParseError err;
                QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
                if (!doc.isNull() && doc.isObject()) {
                    config_ = doc.object();
                } else {
                    QMessageBox::warning(this, "Ошибка",
                                         "Не удалось загрузить настройки:\n" + err.errorString());
                    config_ = {};
                }
            }
        }

        currentNoteId_ = config_.value("active_note_id").toString("1");
        darkMode_ = config_.value("dark_mode").toBool(false);

        if (config_.contains("window_geometry")) {
            restoreGeometry(QByteArray::fromHex(config_["window_geometry"].toString().toUtf8()));
        }

        if (!config_.contains("colors") || !config_["colors"].isObject()) {
            config_["colors"] = QJsonObject();
        }
        saveSettings();
    }

    void saveSettings() {
        config_["active_note_id"] = currentNoteId_;
        config_["dark_mode"] = darkMode_;
        settingsSaveTimer_.start(500);
    }

    void doSaveSettings() {
        QFile f(getConfigFile());
        if (!f.open(QIODevice::WriteOnly)) {
            QMessageBox::warning(this, "Ошибка", "Ошибка сохранения настроек:\n" + f.errorString());
            return;
        }
        QJsonDocument doc(config_);
        f.write(doc.toJson(QJsonDocument::Indented));
    }

    /* ---- note I/O ---- */

    void loadNoteData() {
        if (isUpdating_) return;

        QString path = getNotePath(currentNoteId_);
        QString noteText;

        if (QFile::exists(path)) {
            QFile f(path);
            if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                noteText = QString::fromUtf8(f.readAll());
            } else {
                QMessageBox::warning(this, "Ошибка", "Ошибка чтения заметки:\n" + f.errorString());
            }
        }

        int oldPos = textEdit_->textCursor().position();

        textEdit_->blockSignals(true);
        textEdit_->setPlainText(noteText);
        textEdit_->blockSignals(false);

        QTextCursor cursor = textEdit_->textCursor();
        cursor.setPosition(qMin(oldPos, noteText.length()));
        textEdit_->setTextCursor(cursor);

        setupWatcher();
        updateInterfaceColors();
    }

    void doSaveNote() {
        QString path = getNotePath(currentNoteId_);

        if (watcher_ && watcher_->files().contains(path))
            watcher_->removePath(path);

        QFile f(path);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::warning(this, "Ошибка", "Ошибка записи заметки:\n" + f.errorString());
            return;
        }
        f.write(textEdit_->toPlainText().toUtf8());

        if (watcher_)
            watcher_->addPath(path);
    }

    /* ---- watcher ---- */

    void setupWatcher() {
        QString path = getNotePath(currentNoteId_);
        if (!watcher_) {
            watcher_ = new QFileSystemWatcher(this);
            watcher_->addPath(getConfigDir());
            connect(watcher_, &QFileSystemWatcher::directoryChanged,
                    this, &StickyApp::onConfigDirChanged);
            connect(watcher_, &QFileSystemWatcher::fileChanged,
                    this, &StickyApp::onNoteFileChanged);
        } else {
            for (const QString &p : watcher_->files())
                watcher_->removePath(p);
        }
        if (QFile::exists(path))
            watcher_->addPath(path);
    }

    /* ---- UI ---- */

    void initUI() {
        setWindowTitle("Sticky!");
        setWindowFlag(Qt::WindowType::WindowStaysOnTopHint);
        resize(320, 280);
        setMinimumSize(200, 150);

        auto *mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(0);

        textEdit_ = new QTextEdit(this);
        textEdit_->setPlaceholderText("Напишите что-нибудь...");
        connect(textEdit_, &QTextEdit::textChanged, this, &StickyApp::onTextChanged);
        mainLayout->addWidget(textEdit_);

        auto *bottomBar = new QWidget(this);
        bottomBar->setObjectName("BottomBar");
        auto *bottomLayout = new QHBoxLayout(bottomBar);
        bottomLayout->setContentsMargins(5, 4, 5, 4);
        bottomLayout->setSpacing(5);

        themeBtn_ = new QPushButton(bottomBar);
        connect(themeBtn_, &QPushButton::clicked, this, &StickyApp::toggleTheme);
        bottomLayout->addWidget(themeBtn_);

        colorBtn_ = new QPushButton("🎨", bottomBar);
        colorBtn_->setToolTip("Выбрать цвет для светлой темы");
        connect(colorBtn_, &QPushButton::clicked, this, &StickyApp::chooseColor);
        bottomLayout->addWidget(colorBtn_);

        noteSelector_ = new QComboBox(bottomBar);
        connect(noteSelector_, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &StickyApp::onNoteTabChanged);
        bottomLayout->addWidget(noteSelector_, 1);

        addBtn_ = new QPushButton("+", bottomBar);
        addBtn_->setToolTip("Создать новую заметку");
        connect(addBtn_, &QPushButton::clicked, this, &StickyApp::addNewNote);
        bottomLayout->addWidget(addBtn_);

        deleteBtn_ = new QPushButton("🗑️", bottomBar);
        deleteBtn_->setToolTip("Удалить текущую заметку");
        connect(deleteBtn_, &QPushButton::clicked, this, &StickyApp::deleteCurrentNote);
        bottomLayout->addWidget(deleteBtn_);

        mainLayout->addWidget(bottomBar);
    }

    void scanAndUpdateSelector() {
        noteSelector_->blockSignals(true);
        noteSelector_->clear();

        QStringList ids = getExistingNoteIds();
        if (ids.isEmpty()) {
            ids << "1";
            QString path = getNotePath("1");
            QFile f(path);
            if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
                f.write("Новая заметка 1");
            } else {
                QMessageBox::warning(this, "Ошибка", "Ошибка создания дефолтной заметки:\n" + f.errorString());
            }
        }

        QJsonObject colors = config_["colors"].toObject();
        for (const QString &nid : ids) {
            noteSelector_->addItem("Заметка " + nid, nid);
            if (!colors.contains(nid)) {
                colors[nid] = randomSochnyColor();
            }
        }
        config_["colors"] = colors;
        saveSettings();

        if (!ids.contains(currentNoteId_)) {
            currentNoteId_ = ids.isEmpty() ? "1" : ids.first();
            saveSettings();
        }

        int idx = noteSelector_->findData(currentNoteId_);
        if (idx != -1)
            noteSelector_->setCurrentIndex(idx);

        noteSelector_->blockSignals(false);
    }

    void updateInterfaceColors() {
        themeBtn_->setText(darkMode_ ? "☀️" : "🌙");
        themeBtn_->setToolTip(darkMode_ ? "Включить светлую тему" : "Включить темную тему");

        QString bgText, fgText, bgPanel, borderColor, accentBg;
        if (darkMode_) {
            bgText = "#1e1e2e";
            fgText = "#cdd6f4";
            bgPanel = "#11111b";
            borderColor = "#313244";
            accentBg = "#313244";
        } else {
            QJsonObject colors = config_["colors"].toObject();
            bgText = colors.value(currentNoteId_).toString("#FFF9C4");
            fgText = "#111111";
            bgPanel = "rgba(0, 0, 0, 0.04)";
            borderColor = "rgba(0, 0, 0, 0.12)";
            accentBg = bgText;
        }

        setStyleSheet(
            "QTextEdit { background-color: " + bgText + "; color: " + fgText +
            "; font-size: 15px; border: none; padding: 8px; }"
            "#BottomBar { background-color: " + bgPanel +
            "; border-top: 1px solid " + borderColor + "; }"
            "#BottomBar QPushButton, #BottomBar QComboBox {"
            " background-color: " + accentBg +
            "; border: 1px solid " + borderColor +
            "; border-radius: 3px; padding: 3px; color: " + fgText +
            "; min-height: 28px; }"
            "#BottomBar QPushButton { min-width: 28px; }"
            "QComboBox::drop-down { border: none; }"
        );
        colorBtn_->setVisible(!darkMode_);
    }

    /* ---- slots ---- */

    void onTextChanged() {
        if (!isUpdating_)
            textSaveTimer_.start(500);
    }

    void onNoteTabChanged(int index) {
        if (isUpdating_ || index == -1) return;
        currentNoteId_ = noteSelector_->itemData(index).toString();
        saveSettings();
        loadNoteData();
    }

    void addNewNote() {
        QStringList existing = getExistingNoteIds();
        QList<int> intIds;
        for (const QString &s : existing) intIds << s.toInt();

        int newId = 1;
        while (intIds.contains(newId)) ++newId;

        QString strNewId = QString::number(newId);
        QString path = getNotePath(strNewId);

        QFile f(path);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::warning(this, "Ошибка", "Ошибка создания файла:\n" + f.errorString());
            return;
        }
        f.write(("Заметка " + strNewId + "\n\nНапиши тут что-нибудь...").toUtf8());

        QJsonObject colors = config_["colors"].toObject();
        colors[strNewId] = randomSochnyColor();
        config_["colors"] = colors;

        currentNoteId_ = strNewId;
        saveSettings();

        isUpdating_ = true;
        scanAndUpdateSelector();
        isUpdating_ = false;
        loadNoteData();
    }

    void deleteCurrentNote() {
        QStringList ids = getExistingNoteIds();

        if (ids.size() <= 1) {
            auto reply = QMessageBox::question(this, "Удалить заметку",
                "Нельзя удалить единственную заметку. Очистить её содержимое?",
                QMessageBox::Yes | QMessageBox::No);
            if (reply != QMessageBox::Yes) return;

            textEdit_->blockSignals(true);
            textEdit_->clear();
            textEdit_->blockSignals(false);
            textSaveTimer_.start(500);
            return;
        }

        auto reply = QMessageBox::question(this, "Удалить заметку",
            "Удалить заметку " + currentNoteId_ + "? Это действие нельзя отменить.",
            QMessageBox::Yes | QMessageBox::No);
        if (reply != QMessageBox::Yes) return;

        QString path = getNotePath(currentNoteId_);
        if (watcher_ && watcher_->files().contains(path))
            watcher_->removePath(path);

        QFile::remove(path);

        QJsonObject colors = config_["colors"].toObject();
        colors.remove(currentNoteId_);
        config_["colors"] = colors;

        isUpdating_ = true;
        scanAndUpdateSelector();
        isUpdating_ = false;
        loadNoteData();
    }

    void toggleTheme() {
        darkMode_ = !darkMode_;
        saveSettings();
        updateInterfaceColors();
    }

    void chooseColor() {
        QColor color = QColorDialog::getColor(Qt::yellow, this, "Выберите сочный цвет стикера");
        if (color.isValid()) {
            QJsonObject colors = config_["colors"].toObject();
            colors[currentNoteId_] = color.name();
            config_["colors"] = colors;
            saveSettings();
            updateInterfaceColors();
        }
    }

    void onNoteFileChanged(const QString &path) {
        if (!QFile::exists(path))
            scanAndUpdateSelector();
        loadNoteData();
    }

    void onConfigDirChanged(const QString &path) {
        Q_UNUSED(path);
        QString path2 = getNotePath(currentNoteId_);
        if (QFile::exists(path2) && (!watcher_ || !watcher_->files().contains(path2)))
            watcher_->addPath(path2);
    }

protected:
    void closeEvent(QCloseEvent *event) override {
        config_["window_geometry"] = QString::fromUtf8(saveGeometry().toHex());
        doSaveSettings();

        if (textSaveTimer_.isActive()) {
            textSaveTimer_.stop();
            doSaveNote();
        }
        QWidget::closeEvent(event);
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setStyle("Fusion");
    app.setWindowIcon(QIcon::fromTheme("sticky", QIcon(":/sticky.png")));

    StickyApp w;
    w.show();
    return app.exec();
}

#include "main.moc"
