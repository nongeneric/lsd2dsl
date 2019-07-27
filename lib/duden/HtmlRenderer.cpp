#include "HtmlRenderer.h"

#include <QPainter>
#include <QBuffer>
#include <QEventLoop>
#include <QWebFrame>
#include <QWebPage>
#include <QWebView>
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
        <body style="background-color:white"><div>)";

auto tail = "</div></body></html>";

}

std::vector<uint8_t> renderHtml(const std::string& html) {
    QWebPage page;
    auto frame = page.mainFrame();
    auto complete = head + html + tail;

    QEventLoop loop;
    bool alreadyLoaded = false;
    QObject::connect(&page, &QWebPage::loadFinished, &loop, [&] {
        alreadyLoaded = true;
        loop.quit();
    });
    frame->setHtml(QString::fromStdString(complete));

    if (!alreadyLoaded)
        loop.exec();

    page.setViewportSize(page.mainFrame()->contentsSize());
    auto size = page.viewportSize();
    QImage img(size, QImage::Format_RGB32);
    QPainter p(&img);
    frame->render(&p);
    p.end();

    QByteArray array;
    QBuffer buffer(&array);
    buffer.open(QIODevice::WriteOnly);
    img.save(&buffer, "BMP");
    return {array.data(), array.data() + array.size()};
}

} // namespace duden
