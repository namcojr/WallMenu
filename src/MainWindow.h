#pragma once

#include "Config.h"

#include <QFutureWatcher>
#include <QImage>
#include <QMainWindow>

#include <filesystem>
#include <vector>

class ImageModel;
class QLabel;
class QListView;
class QPushButton;
class QSplitter;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(Config::Data config, QWidget* parent = nullptr);

protected:
    void resizeEvent(QResizeEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void onSelectionChanged();
    void onSetWallpaper();
    void reloadImages();
    void onScanFinished();

private:
    void buildUi();
    void setupConnections();
    void showSelectedPreview();
    void updatePreviewPixmap();
    void updateStatusForSelection();
    void updatePreviewTitle();

    Config::Data config_;
    ImageModel* model_ = nullptr;
    QListView* listView_ = nullptr;
    QLabel* previewTitleLabel_ = nullptr;
    QLabel* previewLabel_ = nullptr;
    QPushButton* setWallpaperButton_ = nullptr;

    QImage currentImage_;
    std::filesystem::path currentImagePath_;

    QFutureWatcher<std::vector<std::filesystem::path>> scanWatcher_;
};
