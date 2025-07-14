###  cgroup_sck源码理解

触发:

 ──────► TCP连接建立 ──► conn.Write("hello") ──► skb进入cgroup ──►【cgroup_skb/ingress】触发

作用:拦截网络数据包，对符合条件的连接进行监控或统计

```
SEC("cgroup_skb/(e)ingress")
int cgroup_skb_ingress_prog(struct __sk_buff *skb)
{
//如果没有启用“监控”开关，或者没有启用“周期性上报”开关，直接放行数据包，不做任何处理
    if (!is_monitoring_enable() || !is__periodic_report_enable()) {
        return SK_PASS;
    }
    //只处理 IPv4 和 IPv6 数据包
    if (skb->family != AF_INET && skb->family != AF_INET6)
        return SK_PASS;
    //从 skb 中获取 socket 描述符
    struct bpf_sock *sk = skb->sk;
    if (!sk)
        return SK_PASS;
    //跳过模拟连接流量
    if (sock_conn_from_sim(skb)) {
        return SK_PASS;
    }
    //检查当前 socket 是否是被 Kmesh 纳管的连接
    if (!is_managed_by_kmesh_skb(skb))
        return SK_PASS;
//条件全部通过，说明这个 socket 代表一个真实且重要的连接，于是调用 observe_on_data() 执行统计或上报逻辑
    observe_on_data(sk);
    return SK_PASS;
}
```
- +++++++++++++++++++++++++++++++
```
static inline void observe_on_data(struct bpf_sock *sk)
{
    struct bpf_tcp_sock *tcp_sock = NULL;
    struct sock_storage_data *storage = NULL;
    if (!sk)
        return;
    //用于从通用 bpf_sock 获取 TCP 专用信息（例如 RTT、窗口大小、字节数等）。
    tcp_sock = bpf_tcp_sock(sk);
    if (!tcp_sock)
        return;
   //这个 map 绑定了每个 socket 的状态，比如上报时间戳、是否经过 waypoint、是否已编码等。
    storage = bpf_sk_storage_get(&map_of_sock_storage, sk, 0, 0);
    if (!storage) {
        // maybe the connection is established before kmesh start
        return;
    }
    //获取当前时间（ns）
    __u64 now = bpf_ktime_get_ns();
    //如果上一次上报距离当前时间超过了这个间隔，就调用 tcp_report() 上报该连接的状态
    if ((storage->last_report_ns != 0) && (now - storage->last_report_ns > LONG_CONN_THRESHOLD_TIME)) {
        tcp_report(sk, tcp_sock, storage, BPF_TCP_ESTABLISHED);
    }
}
```
- ++++++++++++++++++++++++++++++++++++
```
          cgroup_skb/ingress 触发
                   ↓
        提取 skb->sk 得到 socket
                   ↓
          ↓ sk 不为空 → 调用 observe_on_data(sk)
                             ↓
             - sk 是 TCP？        → 是
             - 有 storage？       → 是
             - last_report_ns 差值超阈值？ → 是
                             ↓
                    → 调用 tcp_report 上报信息
```
将当前 socket 连接的关键信息（连接元数据、TCP 状态、起始时间、持续时间、连接成功与否等）封装为一个 tcp_probe_info 结构体，通过 ringbuf 发送到用户空间，用于调试、观测、可视化、监控等用途
```
tcp_report(struct bpf_sock *sk, struct bpf_tcp_sock *tcp_sock, struct sock_storage_data *storage, __u32 state)
{
    struct tcp_probe_info *info = NULL;

    // store tuple
    //map_of_tcp_probe（类型为 BPF_MAP_TYPE_RINGBUF）中申请一个 tcp_probe_info 的 buffer
    info = bpf_ringbuf_reserve(&map_of_tcp_probe, sizeof(struct tcp_probe_info), 0);
    if (!info) {
        BPF_LOG(ERR, PROBE, "bpf_ringbuf_reserve tcp_report failed\n");
        return;
    }
    //填写连接的 5 元组（源地址、目标地址、端口等）到 info->tuple 中。
    //storage->direction 表示连接方向：主动 / 被动连接（client/server）。
    construct_tuple(sk, &info->tuple, storage->direction);
    //补充连接时间和状态
    info->start_ns = storage->connect_ns;
    info->state = state;
    info->direction = storage->direction;
    info->conn_success = storage->connect_success;
    // 获取 TCP 性能指标
    get_tcp_probe_info(tcp_sock, info);
    //判断是 IPv4 还是 IPv6
    (*info).type = (sk->family == AF_INET) ? IPV4 : IPV6;
    if (is_ipv4_mapped_addr(sk->dst_ip6)) {
        (*info).type = IPV4;
    }
    //填写原始目的地址信息
    construct_orig_dst_info(sk, storage, info);
    //上报时间与连接时长
    info->last_report_ns = bpf_ktime_get_ns();
    info->duration = info->last_report_ns - storage->connect_ns;
    storage->last_report_ns = info->last_report_ns;
    bpf_ringbuf_submit(info, 0);
}
```
- 测试方向
```
编号	测试点	目标	是否应进入 tcp_report()
✅ 1	is_monitoring_enable() == false	提前跳过	❌ 否
✅ 2	is__periodic_report_enable() == false	提前跳过	❌ 否
✅ 3	skb->family != AF_INET && != AF_INET6	提前跳过	❌ 否
✅ 4	sock_conn_from_sim(skb) == true	控制连接	❌ 否
✅ 5	is_managed_by_kmesh_skb(skb) == false	非托管连接	❌ 否
✅ 6	全部满足条件 -> last_report_ns = 0	正常执行报告	✅ 是
✅ 7	last_report_ns 已有值 < 1s	不触发报告	❌ 否
✅ 8	last_report_ns 已有值 > 1s	应触发报告	✅ 是
-------------------------------------------------------------------------
✅ 第一阶段：验证跳过路径（构建稳定）
❌【测试 1】monitoring 关闭时应跳过

❌【测试 2】periodic_report 关闭时应跳过

❌【测试 3】family 不为 IPv4 / IPv6

❌【测试 4】是控制连接

❌【测试 5】非 kmesh 管理连接

✅ 第二阶段：验证正常路径（功能）
✅【测试 6】全部条件满足 + last_report_ns = 0，应执行报告

❌【测试 7】全部条件满足 + last_report_ns = now - 0.5s，不应执行报告

✅【测试 8】全部条件满足 + last_report_ns = now - 2s，应执行报告
```

对于:ingress和egress触发条件:

