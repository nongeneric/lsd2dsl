#include "HtmlRenderer.h"

#include <QPainter>
#include <QBuffer>
#include <QEventLoop>
#include <QWebEnginePage>
#include <QWebEngineView>
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
    QWebEnginePage page;
    auto complete = head + html + tail;

    QEventLoop loop;
    bool alreadyLoaded = false;
    QObject::connect(&page, &QWebEnginePage::loadFinished, &loop, [&] {
        alreadyLoaded = true;
        loop.quit();
    });
    page.setHtml(QString::fromStdString(complete));

    if (!alreadyLoaded)
        loop.exec();
    assert(alreadyLoaded);

    QWebEngineView view(&page);
    view.setHtml(QString::fromStdString(complete));
    view.show();

    //auto size = page.contentsSize();
    QImage img(100,100, QImage::Format_RGB32);
    QPainter p;
    p.begin(&img);
    view.render(&p);
    p.end();

    QByteArray array;
    QBuffer buffer(&array);
    buffer.open(QIODevice::WriteOnly);
    img.save(&buffer, "BMP");
    return {array.data(), array.data() + array.size()};
}

} // namespace duden
