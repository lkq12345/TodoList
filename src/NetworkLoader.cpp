#include "NetworkLoader.h"
#include <QJsonDocument>
#include <QNetworkRequest>

/*
 * ======================================================================
 * NetworkLoader.cpp — 网络请求封装类实现
 *
 * 核心逻辑：
 *   fetchTodos() 发起 GET 请求
 *       ↓
 *   onReplyFinished() 响应到达
 *       ├─ 网络错误 → emit errorOccurred(错误描述)
 *       ├─ JSON 解析失败 → emit errorOccurred("JSON 解析失败")
 *       └─ 成功 → emit dataReady(QJsonArray)
 *
 *   超时控制：
 *   onTimeout() 定时器触发 → abort() → emit errorOccurred("请求超时")
 *
 * 线程安全说明：
 *   Qt 的 QNetworkAccessManager 是异步的，但所有信号都在主线程发射，
 *   NetworkLoader 和 Widget 也在主线程，所以不存在竞态条件。
 * ======================================================================
 */

NetworkLoader::NetworkLoader(QObject *parent)
    : QObject(parent)                           // 将自身挂到父对象（Widget）的对象树上
    , m_manager(new QNetworkAccessManager(this)) // 创建 HTTP 客户端，随 NetworkLoader 自动析构
    , m_reply(nullptr)                           // 初始无请求在运行
    , m_timer(new QTimer(this))                  // 创建超时定时器，随 NetworkLoader 自动析构
{
    // 定时器设为单次触发模式：start() 后只响应一次 timeout()，不会循环触发
    m_timer->setSingleShot(true);

    // 连接定时器的超时信号到超时处理槽
    connect(m_timer, &QTimer::timeout,
            this, &NetworkLoader::onTimeout);
}

NetworkLoader::~NetworkLoader()
{
    /*
     * 析构时取消正在进行的请求，确保 reply 的回调不会在对象销毁后
     * 继续执行（避免 use-after-free）。
     *
     * cancel() 内部会调用 reply->abort() 和 reply->deleteLater()，
     * 后者将 reply 的销毁推迟到事件循环的下一次处理，确保当前栈帧
     * 安全退出后才会释放。
     */
    cancel();
}

/*
 * 发起 GET 请求
 *
 * @param url       远程 API 地址
 * @param timeoutMs 超时时间（毫秒），默认 5000，Widget 调用时传入 10000
 */
void NetworkLoader::fetchTodos(const QUrl &url, int timeoutMs)
{
    // 取消上一次请求（如果有），避免多个请求叠加
    cancel();

    // 配置 HTTP 请求
    QNetworkRequest request(url);

    /*
     * 设置 User-Agent 请求头
     *
     * 某些 HTTP API 会拒绝没有 User-Agent 的请求（返回 403/429），
     * 或者返回不同格式的数据。显式设置可以避免此类问题。
     */
    request.setRawHeader("User-Agent", "TodoList/1.0");

    // 发起异步 GET 请求（不会阻塞主线程）
    m_reply = m_manager->get(request);

    /*
     * 连接 reply 的 finished 信号到处理槽
     *
     * finished 信号在以下任一情况发生时发射：
     *   ① 服务器响应全部到达
     *   ② 网络连接断开
     *   ③ 请求被 abort()
     *   ④ SSL 握手失败
     * 因此 onReplyFinished() 必须检查 reply->error() 来区分成功/失败。
     */
    connect(m_reply, &QNetworkReply::finished,
            this, &NetworkLoader::onReplyFinished);

    // 启动超时定时器
    m_timer->start(timeoutMs);
}

/*
 * 取消当前请求
 *
 * 这个方法是"幂等"的——没有请求在运行时调用它也没有副作用。
 */
void NetworkLoader::cancel()
{
    if (m_reply) {
        // 断开 finished 连接，避免已取消的请求还触发回调
        disconnect(m_reply, &QNetworkReply::finished,
                   this, &NetworkLoader::onReplyFinished);

        // 终止底层 TCP 连接
        m_reply->abort();

        // 延迟销毁 reply 对象（确保所有信号处理完毕后再释放）
        m_reply->deleteLater();
        m_reply = nullptr;
    }

    // 停止定时器（如果正在计时）
    m_timer->stop();
}

/*
 * 服务器响应到达后的处理槽
 *
 * 注意：即使请求被 abort()，finished 信号依然会发射。所以必须通过
 * reply->error() 判断是正常完成还是异常终止，不能仅凭信号到达就处理数据。
 */
void NetworkLoader::onReplyFinished()
{
    // 先停止超时定时器——无论成功还是失败，响应已经到达，不需要等超时了
    m_timer->stop();

    /*
     * 空指针保护
     *
     * 如果 cancel() 在 onReplyFinished() 之前被调用（比如用户关闭窗口），
     * m_reply 已经被置为 nullptr 并 deleteLater()，此时直接返回。
     */
    if (!m_reply)
        return;

    /*
     * 将 m_reply 转移到局部变量，然后立即将 m_reply 置空。
     * 这样做的好处是：即使在处理过程中再次调用 cancel()，也不会
     * 重复操作这个 reply 对象。
     */
    QNetworkReply *reply = m_reply;
    m_reply = nullptr;

    // 将 reply 的销毁推迟到事件循环的下一轮，确保当前处理完成
    reply->deleteLater();

    // ---- 第一步：检查网络错误 ----
    if (reply->error() != QNetworkReply::NoError) {
        /*
         * QNetworkReply::errorString() 返回的错误描述可能包含英文
         * 技术细节（如 "Host not found"、"Connection refused"），
         * 但对于调试已经足够。
         *
         * 常见错误：
         *   QNetworkReply::HostNotFoundError    → 域名无法解析（无网络？）
         *   QNetworkReply::ConnectionRefusedError → 服务器拒绝连接
         *   QNetworkReply::TimeoutError          → 连接/读取超时
         *   QNetworkReply::SslHandshakeFailedError → SSL 证书问题
         */
        emit errorOccurred(reply->errorString());
        return;
    }

    // ---- 第二步：读取响应体 ----
    QByteArray responseData = reply->readAll();
    if (responseData.isEmpty()) {
        emit errorOccurred("服务器返回的数据为空");
        return;
    }

    // ---- 第三步：解析 JSON ----
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        // JSON 格式错误（如缺少引号、花括号不匹配等）
        emit errorOccurred("JSON 解析失败: " + parseError.errorString());
        return;
    }

    // ---- 第四步：校验数据格式 ----
    if (!doc.isArray()) {
        // JSONPlaceholder 的 /todos 端点应该返回数组，如果是其他结构说明有问题
        emit errorOccurred("数据格式错误：期望返回 JSON 数组，但收到 " +
                           QString(doc.isObject() ? "对象" : "其他类型"));
        return;
    }

    // ---- 第五步：发射成功信号 ----
    /*
     * 注意：这里直接传递 QJsonArray，Widget 收到信号后再逐条解析。
     * 不在 NetworkLoader 中做业务解析（如提取 title、completed），
     * 是为了保持 NetworkLoader 的通用性——它只关心"获取数据 + 解析
     * 成 JSON"，不关心 JSON 的具体结构。
     */
    emit dataReady(doc.array());
}

/*
 * 请求超时处理
 *
 * 定时器时间到但响应还没到达，说明网络可能不稳定或服务器无响应。
 * 取消当前请求并通知 Widget 显示错误。
 */
void NetworkLoader::onTimeout()
{
    if (m_reply) {
        // 断开连接，避免已超时的请求仍触发 onReplyFinished
        disconnect(m_reply, &QNetworkReply::finished,
                   this, &NetworkLoader::onReplyFinished);

        // 终止底层 TCP 连接
        m_reply->abort();

        // 延迟销毁
        m_reply->deleteLater();
        m_reply = nullptr;
    }

    emit errorOccurred("请求超时，请检查网络连接");
}
