#include "HtmlRenderer.h"

#include "common/Stopwatch.h"

#include <QBuffer>
#include <QEventLoop>
#include <QPainter>
#include <QPdfDocument>
#include <QWebEnginePage>

#include <qcolor.h>
#include <qimage.h>
#include <qpagelayout.h>
#include <qpageranges.h>
#include <qpdfdocumentrenderoptions.h>
#include <qsize.h>

#include <fmt/format.h>

#include <boost/range/irange.hpp>
#include <chrono>
#include <fstream>

namespace duden {

namespace {

auto head = R"(
    <!DOCTYPE html>
    <html>
        <head>
        <meta http-equiv="content-type" content="text/html; charset=utf-8">
        <style>
            table {
                border-collapse: collapse;
                width: 720px;
            }
            table, th, td {
                border: 1px solid black;
            }
            th, td {
                padding: 5px;
            }
            div {
                background-color: transparent;
                font-size: 14px;
            }
        </style>
    </head>
    <body style="background-color:white"><div>
)";

auto tail = "</div></body></html>";

std::array<unsigned, 3> constexpr MarkerColors{0xff1122, 0x33ff44, 0x4455ff};

std::string const pageBreakMarker = fmt::format(R"(
    <svg>
        <rect id="box" x="0" y="10" width="5" height="5" fill="#{:06x}"/>
        <rect id="box" x="5" y="10" width="5" height="5" fill="#{:06x}"/>
        <rect id="box" x="10" y="10" width="5" height="5" fill="#{:06x}"/>
    </svg>
)", MarkerColors[0], MarkerColors[1], MarkerColors[2]);

} // namespace

enum class CropType { Vertical, Horizontal };
enum class PieceType { PageBreak, TableBreak };

class HtmlRenderer {
    using Batch = std::vector<std::string const*>;
    using Batches = std::vector<Batch>;

    unsigned _appendMs = 0;
    unsigned _cropMs = 0;
    unsigned _printPdfMs = 0;
    unsigned _renderPdfMs = 0;
    unsigned _loadHtmlMs = 0;
    Log& _log;

    QImage crop(QImage image, CropType type) {
        Stopwatch sw;
        assert(image.format() != QImage::Format_Invalid);
        auto isWhiteOrTransparent = [&](int x, int y) {
            auto rgb = image.pixel(x, y);
            return qAlpha(rgb) == 0 || (rgb & 0xffffff) == 0xffffff;
        };

        auto isRowWhite = [&](int y) {
            for (int x = 0; x < image.width(); ++x)
                if (!isWhiteOrTransparent(x, y))
                    return false;
            return true;
        };

        auto isColumnWhite = [&](int x) {
            for (int y = 0; y < image.height(); ++y)
                if (!isWhiteOrTransparent(x, y))
                    return false;
            return true;
        };

        int x1 = 0;
        int x2 = image.width() - 1;
        int y1 = 0;
        int y2 = image.height() - 1;
        if (type == CropType::Vertical) {
            for (; y1 <= y2 && isRowWhite(y1); ++y1) ;
            for (; y2 >= y1 && isRowWhite(y2); --y2) ;
        } else {
            for (; x1 <= x2 && isColumnWhite(x1); ++x1) ;
            for (; x2 >= x1 && isColumnWhite(x2); --x2) ;
        }
        auto copy =  image.copy(x1, y1, x2 - x1 + 1, y2 - y1 + 1);
        _cropMs += sw.elapsedMs();
        return copy;
    }

    QImage append(std::vector<QImage> const& images) {
        Stopwatch sw;
        int width = 0;
        int height = 0;
        for (auto const& image : images) {
            height += image.height();
            width = std::max(width, image.width());
        }

        if (width == 0 || height == 0)
            return images.front();

        QImage res(width, height, QImage::Format_ARGB32);
        res.fill(Qt::transparent);
        {
            QPainter painter(&res);
            int y = 0;
            for (auto const& image : images) {
                painter.drawImage(0, y, image);
                y += image.height();
            }
        }
        _appendMs += sw.elapsedMs();
        return res;
    }

    unsigned countRows(std::string const& table) {
        unsigned count = 0;
        size_t pos = 0;
        for (;;) {
            pos = table.find("<tr>", pos);
            if (pos == std::string::npos)
                break;
            pos++;
            count++;
        }
        return count;
    }

    Batches groupTables(Batch allTables, unsigned batchSize) {
        Batches batches;
        Batch currentBatch;
        unsigned currentBatchRows = 0;
        for (auto table : allTables) {
            currentBatchRows += countRows(*table);
            currentBatch.push_back(table);
            if (currentBatchRows > batchSize) {
                batches.push_back(std::move(currentBatch));
                currentBatchRows = 0;
            }
        }
        if (currentBatchRows > 0)
            batches.push_back(std::move(currentBatch));
        return batches;
    }

    std::vector<QImage> printPages(QString const& completeHtml) {
        Stopwatch sw;
        QWebEnginePage page;
        QEventLoop loop;
        bool alreadyLoaded = false;
        bool pdfSaved = false;
        QObject::connect(&page, &QWebEnginePage::loadFinished, &loop, [&] {
            alreadyLoaded = true;
        });

        page.setHtml(completeHtml);

        while (!alreadyLoaded)
            loop.processEvents();
        assert(!page.isLoading());

        _loadHtmlMs += sw.elapsedMs();
        sw = {};

        QPageSize pageSize(QPageSize::A4Plus);
        QPageLayout layout;

        QByteArray pdfBytes;
        page.printToPdf([&](QByteArray const& array) {
            pdfBytes = array;
            pdfSaved = true;
        });
        while (!pdfSaved)
            loop.processEvents();

        _printPdfMs += sw.elapsedMs();
        sw = {};

        QPdfDocument pdf;
        QBuffer pdfBytesBuffer(&pdfBytes);
        pdfBytesBuffer.open(QIODeviceBase::ReadOnly);
        pdf.load(&pdfBytesBuffer);

        std::vector<QImage> printedPages;
        for (int p = 0; p < pdf.pageCount(); ++p) {
            auto image = pdf.render(p, pdf.pagePointSize(p).toSize() * 2);
            printedPages.push_back(std::move(image));
        }

        _renderPdfMs += sw.elapsedMs();
        return printedPages;
    }

    std::vector<QImage> splitByMarker(QImage const& image) {
        assert(image.format() != QImage::Format_Invalid);
        auto isMarkerRow = [&](int y) {
            auto pattern = MarkerColors.begin();
            std::optional<unsigned> prevColor;
            for (int x = 0; x < image.width(); ++x) {
                auto color = image.pixel(x, y) & 0xffffff;
                if (color == prevColor)
                    continue;
                if (color == *pattern)
                    pattern++;
                else
                    pattern = MarkerColors.begin();
                if (pattern == end(MarkerColors))
                    return true;
                prevColor = color;
            }
            return false;
        };

        std::vector<QImage> pieces;
        auto addPieceIfNotEmpty = [&] (int y1, int y2) {
            if (y2 - y1 > 0)
                pieces.push_back(image.copy(0, y1, image.width(), y2 - y1));
        };

        int tableY = 0;
        for (int y = 0; y < image.height(); ++y) {
            if (isMarkerRow(y)) {
                addPieceIfNotEmpty(tableY, y);
                tableY = y + 1;
            }
        }
        addPieceIfNotEmpty(tableY, image.height());
        return pieces;
    }

    void splitPages(std::vector<QImage> const& pages,
                    std::function<void(QImage const&, PieceType)> handler) {
        for (auto const& page : pages) {
            auto pieces = splitByMarker(page);
            assert(!pieces.empty());
            for (size_t i = 0; i < pieces.size() - 1; ++i) {
                handler(pieces[i], PieceType::TableBreak);
            }
            handler(pieces.back(), PieceType::PageBreak);
        }
    }

public:
    explicit HtmlRenderer(Log& log) : _log(log) {}

    void render(std::vector<std::string const*> htmls,
                std::function<void(QImage const&)> handler,
                unsigned batchSize) {
        auto batches = groupTables(htmls, batchSize);
        _log.verbose("split %d htmls into %d batches\n", htmls.size(), batches.size());

        for (auto const& batch : batches) {
            QString completeHtml = head;
            for (auto html : batch) {
                completeHtml += *html + pageBreakMarker;
            }
            completeHtml += tail;

            std::vector<QImage> tableImage;
            auto printedPages = printPages(completeHtml);

            splitPages(printedPages, [&] (QImage const& piece, PieceType type) {
                tableImage.push_back(crop(piece, CropType::Vertical));
                if (type == PieceType::TableBreak) {
                    handler(crop(append(tableImage), CropType::Horizontal));
                    tableImage.clear();
                }
            });

            _log.verbose("appendMs = %d\n", _appendMs);
            _log.verbose("cropMs = %d\n", _cropMs);
            _log.verbose("printPdfMs = %d\n", _printPdfMs);
            _log.verbose("renderPdfMs = %d\n", _renderPdfMs);
            _log.verbose("loadHtmlMs = %d\n", _loadHtmlMs);
        }
    }
};

void renderHtml(std::function<void(QImage const&)> handler,
                std::vector<std::string const*> htmls,
                Log& log,
                unsigned batchSize) {
    HtmlRenderer(log).render(htmls, handler, batchSize);
}

std::vector<uint8_t> qImageToPng(QImage const& image) {
    QByteArray array;
    QBuffer buffer(&array);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    return {array.data(), array.data() + array.size()};
}

} // namespace duden
