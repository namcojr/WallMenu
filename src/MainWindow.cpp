#include "MainWindow.h"

#include "ImageModel.h"
#include "WallpaperSetter.h"

#include <QFileInfo>
#include <QFont>
#include <QFuture>
#include <QApplication>
#include <QLabel>
#include <QKeyEvent>
#include <QListView>
#include <QMessageBox>
#include <QPushButton>
#include <QResizeEvent>
#include <QShortcut>
#include <QSplitter>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QWidget>
#include <QtConcurrent/QtConcurrent>

namespace {
QFont buildListFont(const Config::Data& config) {
    QFont font(config.listFontName, config.listFontSize);
    return font;
}

QFont buildTitleFont(const Config::Data& config) {
    QFont font(config.titleFontName, config.titleFontSize);
    switch (config.titleFontStyle) {
    case Config::FontStyle::Normal:
        font.setBold(false);
        font.setItalic(false);
        break;
    case Config::FontStyle::Bold:
        font.setBold(true);
        font.setItalic(false);
        break;
    case Config::FontStyle::Italic:
        font.setBold(false);
        font.setItalic(true);
        break;
    case Config::FontStyle::BoldItalic:
        font.setBold(true);
        font.setItalic(true);
        break;
    }
    return font;
}

QString formatFileSize(qint64 bytes) {
    if (bytes < 0) {
        return "0kB";
    }
    if (bytes < 1000) {
        return QString("%1B").arg(bytes);
    }
    const qint64 kb = (bytes + 500) / 1000;
    return QString("%1kB").arg(kb);
}
}

MainWindow::MainWindow(Config::Data config, QWidget* parent)
    : QMainWindow(parent), config_(std::move(config)) {
    buildUi();
    setupConnections();
    reloadImages();
}

void MainWindow::buildUi() {
    setWindowTitle("Wallmenu");
    resize(1600, 960);

    auto* splitter = new QSplitter(Qt::Horizontal, this);

    auto* leftContainer = new QWidget(splitter);
    auto* leftLayout = new QVBoxLayout(leftContainer);
    leftLayout->setContentsMargins(8, 8, 8, 8);

    model_ = new ImageModel(this);
    listView_ = new QListView(leftContainer);
    listView_->setModel(model_);
    listView_->setUniformItemSizes(true);
    listView_->setSelectionMode(QAbstractItemView::SingleSelection);
    listView_->setFont(buildListFont(config_));

    setWallpaperButton_ = new QPushButton("Set Wallpaper", leftContainer);

    leftLayout->addWidget(listView_);
    leftLayout->addWidget(setWallpaperButton_);

    auto* rightContainer = new QWidget(splitter);
    auto* rightLayout = new QVBoxLayout(rightContainer);
    rightLayout->setContentsMargins(8, 8, 8, 8);
    rightLayout->setSpacing(4);

    previewTitleLabel_ = new QLabel(rightContainer);
    previewTitleLabel_->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    previewTitleLabel_->setFont(buildTitleFont(config_));
    previewTitleLabel_->setText("No image selected");

    previewLabel_ = new QLabel(rightContainer);
    previewLabel_->setAlignment(Qt::AlignCenter);
    previewLabel_->setMinimumSize(320, 240);
    previewLabel_->setText("Select an image to preview");

    rightLayout->addWidget(previewTitleLabel_, 0, Qt::AlignHCenter);
    rightLayout->addSpacing(2);
    rightLayout->addWidget(previewLabel_, 1);

    splitter->addWidget(leftContainer);
    splitter->addWidget(rightContainer);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);

    setCentralWidget(splitter);
    statusBar()->showMessage("Ready");
}

void MainWindow::setupConnections() {
    connect(listView_->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &MainWindow::onSelectionChanged);

    connect(setWallpaperButton_, &QPushButton::clicked, this, &MainWindow::onSetWallpaper);

    connect(&scanWatcher_, &QFutureWatcher<std::vector<std::filesystem::path>>::finished,
            this, &MainWindow::onScanFinished);

    auto* reloadShortcut = new QShortcut(QKeySequence(Qt::Key_R), this);
    reloadShortcut->setContext(Qt::ApplicationShortcut);
    connect(reloadShortcut, &QShortcut::activated, this, &MainWindow::reloadImages);

    auto* quitShortcut = new QShortcut(QKeySequence(Qt::Key_Q), this);
    quitShortcut->setContext(Qt::ApplicationShortcut);
    connect(quitShortcut, &QShortcut::activated, this, []() {
        QApplication::quit();
    });
}

void MainWindow::reloadImages() {
    if (scanWatcher_.isRunning()) {
        return;
    }

    setWallpaperButton_->setEnabled(false);
    statusBar()->showMessage("Scanning images...");

    const auto root = config_.wallpaperDir;
    const bool recursive = config_.recursive;

    QFuture<std::vector<std::filesystem::path>> future = QtConcurrent::run([root, recursive]() {
        return ImageModel::discoverImages(root, recursive);
    });
    scanWatcher_.setFuture(future);
}

void MainWindow::onScanFinished() {
    auto images = scanWatcher_.result();
    model_->setImages(std::move(images));

    if (model_->rowCount() > 0) {
        listView_->setCurrentIndex(model_->index(0, 0));
    } else {
        currentImage_ = {};
        currentImagePath_.clear();
        previewTitleLabel_->setText("No image selected");
        previewLabel_->setPixmap({});
        previewLabel_->setText("No images found");
        statusBar()->showMessage("No images found", 3000);
    }

    setWallpaperButton_->setEnabled(model_->rowCount() > 0);
}

void MainWindow::onSelectionChanged() {
    showSelectedPreview();
}

void MainWindow::showSelectedPreview() {
    const QModelIndex index = listView_->currentIndex();
    if (!index.isValid()) {
        return;
    }

    const std::filesystem::path imagePath = model_->imagePathAt(index.row());
    if (imagePath.empty()) {
        return;
    }

    currentImagePath_ = imagePath;

    // TODO: Introduce thumbnail caching to avoid repeatedly decoding full-sized images.
    QImage image(QString::fromStdString(imagePath.string()));
    if (image.isNull()) {
        previewLabel_->setPixmap({});
        previewLabel_->setText("Failed to load image");
        statusBar()->showMessage("Failed to load selected image", 3000);
        return;
    }

    currentImage_ = std::move(image);
    updatePreviewTitle();
    updatePreviewPixmap();
    updateStatusForSelection();
}

void MainWindow::updatePreviewTitle() {
    if (currentImage_.isNull() || currentImagePath_.empty()) {
        previewTitleLabel_->setText("No image selected");
        return;
    }

    const QFileInfo fileInfo(QString::fromStdString(currentImagePath_.string()));
    previewTitleLabel_->setText(QString("%1 · %2 · %3x%4")
                                    .arg(fileInfo.fileName())
                                    .arg(formatFileSize(fileInfo.size()))
                                    .arg(currentImage_.width())
                                    .arg(currentImage_.height()));
}

void MainWindow::updatePreviewPixmap() {
    if (currentImage_.isNull()) {
        return;
    }

    const QSize targetSize = previewLabel_->size();
    if (targetSize.width() <= 0 || targetSize.height() <= 0) {
        return;
    }

    const QPixmap scaled = QPixmap::fromImage(currentImage_.scaled(
        targetSize,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
    ));
    previewLabel_->setPixmap(scaled);
}

void MainWindow::updateStatusForSelection() {
    if (currentImage_.isNull() || currentImagePath_.empty()) {
        statusBar()->showMessage("Ready");
        return;
    }

    const QFileInfo fileInfo(QString::fromStdString(currentImagePath_.string()));
    const qint64 bytes = fileInfo.size();
    const double kib = static_cast<double>(bytes) / 1024.0;

    statusBar()->showMessage(QString("%1x%2 | %3 KiB")
                                 .arg(currentImage_.width())
                                 .arg(currentImage_.height())
                                 .arg(QString::number(kib, 'f', 1)));
}

void MainWindow::onSetWallpaper() {
    const QModelIndex index = listView_->currentIndex();
    if (!index.isValid()) {
        return;
    }

    const std::filesystem::path imagePath = model_->imagePathAt(index.row());
    if (imagePath.empty()) {
        return;
    }

    QString error;
    if (!WallpaperSetter::setWallpaper(QString::fromStdString(imagePath.string()), &error)) {
        QMessageBox::warning(this, "Set Wallpaper", QString("Failed to set wallpaper.\n%1").arg(error));
        return;
    }

    statusBar()->showMessage("Wallpaper updated", 2500);
}

void MainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
    updatePreviewPixmap();
}

void MainWindow::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        onSetWallpaper();
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_R) {
        reloadImages();
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_Q) {
        QApplication::quit();
        event->accept();
        return;
    }

    QMainWindow::keyPressEvent(event);
}
