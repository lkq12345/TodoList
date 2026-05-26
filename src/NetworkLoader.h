#ifndef NETWORKLOADER_H
#define NETWORKLOADER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonArray>
#include <QTimer>

/*
 * ======================================================================
 * NetworkLoader.h — 网络请求封装类声明
 *
 * 职责：封装 HTTP GET 请求的生命周期管理，包括：
 *   1. 发起请求（fetchTodos）
 *   2. 超时控制（QTimer）
 *   3. 响应处理（JSON 解析 + 错误检测）
 *   4. 取消正在进行的请求（cancel）
 *
 * 使用 QNetworkAccessManager 而非 QTcpSocket 的原因是：JSONPlaceholder
 * 提供的是标准的 RESTful HTTP API，QNetworkAccessManager 自动处理了
 * HTTP 协议细节（请求行、响应头、状态码等），如果手写 QTcpSocket 需要
 * 自己拼 HTTP 报文，徒增复杂度。
 *
 * 设计为独立的类而非直接在 Widget 中写网络代码，是为了保持单一职责
 * 原则——Widget 只关心 UI 和业务逻辑，不关心网络细节。
 * ======================================================================
 */

class NetworkLoader : public QObject
{
    Q_OBJECT

public:
    /*
     * @param parent QObject 父对象，用于内存管理
     *               NetworkLoader 的所有子对象（m_manager, m_timer）
     *               也会被自动纳入 Qt 的对象树管理。
     */
    explicit NetworkLoader(QObject *parent = nullptr);

    /*
     * 析构时取消正在进行的请求，避免回调操作已销毁的对象。
     */
    ~NetworkLoader();

    /*
     * 发起 GET 请求获取待办任务数据
     *
     * 每次调用前会自动取消上一次未完成的请求（通过 cancel()），
     * 防止重复点击导致多个请求同时进行。
     *
     * @param url       目标 URL（当前使用 JSONPlaceholder 的 HTTP 端点）
     * @param timeoutMs 超时毫秒数，默认 5000ms（可通过参数覆盖）
     */
    void fetchTodos(const QUrl &url, int timeoutMs = 5000);

    /*
     * 取消当前正在进行的请求（如果有）
     *
     * 安全地终止请求并清理资源：
     *   1. 调用 reply->abort() 终止底层 TCP 连接
     *   2. reply->deleteLater() 延迟销毁 reply 对象
     *   3. 停止超时定时器
     *
     * 注意：abort() 后 reply 仍会发射 finished() 信号，但此时
     * m_reply 已置为 nullptr，onReplyFinished() 会在空值检查时
     * 直接返回，不会重复处理。
     */
    void cancel();

signals:
    /*
     * 数据加载完成信号
     * @param data 解析后的 JSON 数组，每条是一个任务对象
     *             （含 title、completed 等字段）
     */
    void dataReady(const QJsonArray &data);

    /*
     * 错误发生信号
     * @param message 中文错误描述，直接用于 QMessageBox 展示
     */
    void errorOccurred(const QString &message);

private slots:
    /*
     * QNetworkReply::finished 信号的响应槽
     * 处理：网络错误检查 → JSON 解析 → 数据校验 → 发射信号
     */
    void onReplyFinished();

    /*
     * QTimer::timeout 信号的响应槽
     * 超时时取消当前请求并发射错误信号
     */
    void onTimeout();

private:
    QNetworkAccessManager *m_manager;   // HTTP 客户端（Qt 自带的异步网络引擎）
    QNetworkReply         *m_reply;     // 当前活动的请求回复对象
    QTimer                *m_timer;     // 超时定时器（单次触发）
};

#endif // NETWORKLOADER_H
