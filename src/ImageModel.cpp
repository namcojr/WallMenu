#include "ImageModel.h"

#include <QFileInfo>

#include <algorithm>
#include <cctype>
#include <system_error>

namespace {
bool isSupportedExtension(const std::filesystem::path& path) {
    const std::string ext = path.extension().string();
    std::string lower;
    lower.reserve(ext.size());
    for (char c : ext) {
        lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }

    return lower == ".jpg" || lower == ".jpeg" || lower == ".png" || lower == ".webp";
}
}

ImageModel::ImageModel(QObject* parent)
    : QAbstractListModel(parent) {}

int ImageModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(images_.size());
}

QVariant ImageModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(images_.size())) {
        return {};
    }

    const auto& path = images_[static_cast<std::size_t>(index.row())];
    if (role == Qt::DisplayRole) {
        return QString::fromStdString(path.filename().string());
    }
    if (role == Qt::ToolTipRole) {
        return QString::fromStdString(path.string());
    }
    return {};
}

void ImageModel::setImages(std::vector<std::filesystem::path> images) {
    beginResetModel();
    images_ = std::move(images);
    endResetModel();
}

std::filesystem::path ImageModel::imagePathAt(int row) const {
    if (row < 0 || row >= static_cast<int>(images_.size())) {
        return {};
    }
    return images_[static_cast<std::size_t>(row)];
}

std::vector<std::filesystem::path> ImageModel::discoverImages(const std::filesystem::path& root, bool recursive) {
    std::vector<std::filesystem::path> discovered;
    std::error_code ec;
    if (!std::filesystem::exists(root, ec) || !std::filesystem::is_directory(root, ec)) {
        return discovered;
    }

    if (recursive) {
        for (std::filesystem::recursive_directory_iterator it(root, ec), end; it != end; it.increment(ec)) {
            if (ec) {
                continue;
            }
            if (!it->is_regular_file(ec)) {
                continue;
            }
            const auto& path = it->path();
            if (isSupportedExtension(path)) {
                discovered.push_back(path);
            }
        }
    } else {
        for (std::filesystem::directory_iterator it(root, ec), end; it != end; it.increment(ec)) {
            if (ec) {
                continue;
            }
            if (!it->is_regular_file(ec)) {
                continue;
            }
            const auto& path = it->path();
            if (isSupportedExtension(path)) {
                discovered.push_back(path);
            }
        }
    }

    std::sort(discovered.begin(), discovered.end(), [](const auto& a, const auto& b) {
        QString lhs = QString::fromStdString(a.filename().string()).toLower();
        QString rhs = QString::fromStdString(b.filename().string()).toLower();
        if (lhs == rhs) {
            return a.string() < b.string();
        }
        return lhs < rhs;
    });

    return discovered;
}
