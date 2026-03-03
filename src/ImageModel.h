#pragma once

#include <QAbstractListModel>
#include <filesystem>
#include <vector>

class ImageModel : public QAbstractListModel {
    Q_OBJECT

public:
    explicit ImageModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    void setImages(std::vector<std::filesystem::path> images);
    std::filesystem::path imagePathAt(int row) const;

    static std::vector<std::filesystem::path> discoverImages(
        const std::filesystem::path& root,
        bool recursive
    );

private:
    std::vector<std::filesystem::path> images_;
};
