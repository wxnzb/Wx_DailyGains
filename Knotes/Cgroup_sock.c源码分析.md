### Cgroup_sock.c源码分析

触发:

 应用层   ─────────────►  connect() 系统调用 ──►【cgroup/connect4】触发（Socket Hook）

```
nkj额为确保int cgroup_connect4_prog(struct bpf_sock_addr *ctx)
{
//将 bpf_sock_addr 的信息封装到 kmesh_context 中备用。dnat_ip/port 默认就是目标地址，后续可能被重写
    struct kmesh_context kmesh_ctx = {0};
    kmesh_ctx.ctx = ctx;
    kmesh_ctx.orig_dst_addr.ip4 = ctx->user_ip4;
    kmesh_ctx.dnat_ip.ip4 = ctx->user_ip4;
    kmesh_ctx.dnat_port = ctx->user_port;
//判断是否跳过：要是连接来自 控制面；或者当前 netns 不在 Kmesh 管理范围内
    if (handle_kmesh_manage_process(&kmesh_ctx) || !is_kmesh_enabled(ctx)) {
        return CGROUP_SOCK_OK;
    }
//只处理 TCP 流量
    if (ctx->protocol != IPPROTO_TCP)
        return CGROUP_SOCK_OK;
//在连接开始前记录当前时间戳到 socket 的 local storage 中
    observe_on_pre_connect(ctx->sk);
    //讲最终目标地址ip:port填写进kmesh_ctx
    int ret = sock_traffic_control(&kmesh_ctx);
    if (ret) {
        return CGROUP_SOCK_OK;
    }
    //将原始的目标地保存在当前 socket 对应的 local storage 里
    ret = set_original_dst_info(&kmesh_ctx);
    if (ret) {
        BPF_LOG(ERR, KMESH, "[IPv4]failed to set original destination info, ret is %d\n", ret);
        return CGROUP_SOCK_OK;
    }
// 将kmesh_ctx（中间态）中的 dnat_ip / dnat_port写回到ctx 中
    SET_CTX_ADDRESS4(ctx, &kmesh_ctx.dnat_ip, kmesh_ctx.dnat_port);
    if (kmesh_ctx.via_waypoint) {
    //map_of_cgr_tail_call
    // //找到map_of_cgr_tail_call中这个index索引的函数跳转过去，ctx是传递给跳转函数的参数，所以他到底调到那去了？？？？
        kmesh_workload_tail_call(ctx, TAIL_CALL_CONNECT4_INDEX)；
        // if tail call failed will run this code
        BPF_LOG(ERR, KMESH, "workload tail call failed, err is %d\n", ret);
    }

    return CGROUP_SOCK_OK;
}
```
- +++++++++++++++++++++++++++++++++++++
```
static inline int sock_traffic_control(struct kmesh_context *kmesh_ctx)
{
    observe_on_operation_start(SOCK_TRAFFIC_CONTROL, kmesh_ctx);
    int ret;
    frontend_value *frontend_v = NULL;
    frontend_key frontend_k = {0};
    struct bpf_sock_addr *ctx = kmesh_ctx->ctx;
    struct ip_addr orig_dst_addr = {0};

    if (ctx->family == AF_INET) {
        orig_dst_addr.ip4 = kmesh_ctx->orig_dst_addr.ip4;
        frontend_k.addr.ip4 = orig_dst_addr.ip4;
    } else if (ctx->family == AF_INET6) {
        IP6_COPY(orig_dst_addr.ip6, kmesh_ctx->orig_dst_addr.ip6);
        // Java applications use IPv6 for communication. In the IPv4 network environment, the control plane delivers the
        // IPv4 address to the bpf map but obtains the IPv4 mapped address from the bpf prog context. Therefore, address
        // translation is required before and after traffic manager.
        if (is_ipv4_mapped_addr(orig_dst_addr.ip6))
            V4_MAPPED_REVERSE(orig_dst_addr.ip6);
        bpf_memcpy(frontend_k.addr.ip6, orig_dst_addr.ip6, IPV6_ADDR_LEN);
    }

    BPF_LOG(
        DEBUG,
        KMESH,
        "origin dst addr=[%u:%s:%u]\n",
        ctx->family,
        ip2str((__u32 *)&kmesh_ctx->orig_dst_addr, (ctx->family == AF_INET)),
        bpf_ntohs(ctx->user_port));
    //我现在很好奇为什么这个特定的map里面会有这对kv,他是什么时候传进去的
    frontend_v = map_lookup_frontend(&frontend_k);
    if (!frontend_v) {
        return -ENOENT;
    }

    ret = frontend_manager(kmesh_ctx, frontend_v);
    if (ret != 0) {
        if (ret != -ENOENT)
            BPF_LOG(ERR, KMESH, "frontend_manager failed, ret:%d\n", ret);
        return ret;
    }
    observe_on_operation_end(SOCK_TRAFFIC_CONTROL, kmesh_ctx);
    return 0;
}
```
```
static inline int frontend_manager(struct kmesh_context *kmesh_ctx, frontend_value *frontend_v)
{
    int ret = 0;
    service_key service_k = {0};
    service_value *service_v = NULL;
    backend_key backend_k = {0};
    backend_value *backend_v = NULL;
    bool direct_backend = false;
    // 1. 用 frontend_value 的 upstream_id 填充 service_key 以查 service_map
    //service_value 中包含负载均衡策略、端口映射、waypoint 等
    service_k.service_id = frontend_v->upstream_id;
    service_v = map_lookup_service(&service_k);
  // 2. service_map 没找到时，认为 upstream_id 代表 backend UID
    if (!service_v) {
        backend_k.backend_uid = frontend_v->upstream_id;
        backend_v = map_lookup_backend(&backend_k);
        if (!backend_v) {
            BPF_LOG(WARN, FRONTEND, "find backend by backend_uid %d failed\n", backend_k.backend_uid);
            return -ENOENT;
        }
        // direct_backend 标记后续流程走后端直接访问逻辑
        direct_backend = true;
    }

    if (direct_backend) { // in this case, we donot check the healthy status of the backend, just let it go
        // For pod direct access, if a pod has waypoint captured, we will redirect to waypoint, otherwise we do nothing.
        //如果有 waypoint，调用 waypoint_manager 进行重定向处理
        if (backend_v->waypoint_port != 0) {
            BPF_LOG(
                DEBUG,
                FRONTEND,
                "find waypoint addr=[%s:%u]\n",
                ip2str((__u32 *)&backend_v->wp_addr, kmesh_ctx->ctx->family == AF_INET),
                bpf_ntohs(backend_v->waypoint_port));
                //这个函数调用就将后面那两个参数天填写进kmesh_ctx的最终ip和port
            ret = waypoint_manager(kmesh_ctx, &backend_v->wp_addr, backend_v->waypoint_port);
            if (ret != 0) {
                BPF_LOG(ERR, BACKEND, "waypoint_manager failed, ret:%d\n", ret);
            }
            return ret;
        }
    } else {
        //否则，走 service_manager 处理服务级别的负载均衡和策略
        ret = service_manager(kmesh_ctx, frontend_v->upstream_id, service_v);
        if (ret != 0) {
            if (ret != -ENOENT)
                BPF_LOG(ERR, FRONTEND, "service_manager failed, ret:%d\n", ret);
            return ret;
        }
    }

    return 0;
}
```
- ++++++++++++++++++++++++++++++++++++
```
static inline int service_manager(struct kmesh_context *kmesh_ctx, __u32 service_id, service_value *service_v)
{
    int ret = 0;
   //如果这两个都不为 0，就说明 服务要求所有流量都必须先经过 waypoint
    if (service_v->wp_addr.ip4 != 0 && service_v->waypoint_port != 0) {
        BPF_LOG(
            DEBUG,
            SERVICE,
            "find waypoint addr=[%s:%u]\n",
            ip2str((__u32 *)&service_v->wp_addr, kmesh_ctx->ctx->family == AF_INET),
            bpf_ntohs(service_v->waypoint_port));
        ret = waypoint_manager(kmesh_ctx, &service_v->wp_addr, service_v->waypoint_port);
        if (ret != 0) {
            BPF_LOG(ERR, BACKEND, "waypoint_manager failed, ret:%d\n", ret);
        }
        return ret;
    }
    //要是没走代理，输出当前服务的负载均衡策略类型
    BPF_LOG(DEBUG, SERVICE, "service [%u] lb policy [%u]", service_id, service_v->lb_policy);

    switch (service_v->lb_policy) {
        //随机负载均衡
    case LB_POLICY_RANDOM:
        ret = lb_random_handle(kmesh_ctx, service_id, service_v);
        break;
        //本地优先，必须匹配
    case LB_POLICY_STRICT:
        ret = lb_locality_strict_handle(kmesh_ctx, service_id, service_v);
        break;
        //本地优先，失败才切远程
    case LB_POLICY_FAILOVER:
        ret = lb_locality_failover_handle(kmesh_ctx, service_id, service_v);
        break;
    default:
        BPF_LOG(ERR, SERVICE, "unsupported load balance type:%u\n", service_v->lb_policy);
        ret = -EINVAL;
        break;
    }

    return ret;
}
```
- 整体流程：
```
service_manager():
    ├─ 如果 service 设置了 waypoint：
    │     └─ 直接重定向连接目标到 waypoint IP/port
    └─ 否则，根据服务的 lb_policy：
          ├─ 随机选择后端
          ├─ 本地优先，必须本地
          └─ 本地优先，失败回退远程
```
- 接着继续
```
static inline int set_original_dst_info(struct kmesh_context *kmesh_ctx)
{
    struct bpf_sock *sk = (struct bpf_sock *)kmesh_ctx->ctx->sk;
    ctx_buff_t *ctx = (ctx_buff_t *)kmesh_ctx->ctx;

    struct sock_storage_data *storage = NULL;
    storage = bpf_sk_storage_get(&map_of_sock_storage, sk, 0, BPF_LOCAL_STORAGE_GET_F_CREATE);
    if (!storage) {
        BPF_LOG(ERR, KMESH, "failed to get storage from map_of_sock_storage");
        return 0;
    }

    if (kmesh_ctx->via_waypoint) {
        storage->via_waypoint = true;
    }

    if (ctx->family == AF_INET && !storage->has_set_ip) {
        storage->sk_tuple.ipv4.daddr = kmesh_ctx->orig_dst_addr.ip4;
        storage->sk_tuple.ipv4.dport = ctx->user_port;
        storage->has_set_ip = true;
    } else if (ctx->family == AF_INET6 && !storage->has_set_ip) {
        bpf_memcpy(storage->sk_tuple.ipv6.daddr, kmesh_ctx->orig_dst_addr.ip6, IPV6_ADDR_LEN);
        storage->sk_tuple.ipv6.dport = ctx->user_port;
        storage->has_set_ip = true;
    }

    return 0;
}
//整体流程---------------------
set_original_dst_info()
 ├─ 获取 socket sk
 ├─ 获取或创建 bpf_sk_storage
 ├─ 若连接走 waypoint → 标记 via_waypoint=true
 └─ 若尚未设置原始目的地：
     ├─ IPv4: 保存 ip4 + port
     └─ IPv6: 保存 ip6 + port
     为什么需要这一步？？
在 连接发起阶段（connect4），原始目标地址还在 ctx 中；
但在 发送数据阶段（sendmsg_prog），sk_msg_md 已经没有这些原始信息了，只能通过 bpf_sk_storage 找回来；
```
### 目前根据代码考虑测试分支情况：
- 目前：SEC("cgroup/connect6")和SEC("cgroup/connect4")只是处理的类型不同，方法可通用
- 1：
```
传入 IPv4 协议非 TCP（比如 UDP），确认程序直接返回，不做处理。
传入 IPv6 协议非 TCP，确认程序直接返回。
传入的 ctx 指针为空（模拟异常场景），程序是否安全返回。
```
- ++++++++++++++++++++++++++++++++++++++++++
- 2
```
handle_kmesh_manage_process() 判断分支测试
模拟 handle_kmesh_manage_process() 返回 true（或非零），验证程序直接返回，连接不被处理。
模拟 is_kmesh_enabled() 返回 false，验证程序直接返回，不执行重定向。
```
- ++++++++++++++++++++++++++++++++++++++++++++
- 3
```
observe_on_pre_connect() 行为测试
传入有效的 ctx->sk，确认调用 bpf_sk_storage_get() 并成功写入时间戳。
模拟 bpf_sk_storage_get() 失败，确认日志打印并安全返回
```
- +++++++++++++++++++++++++++++++++++++
- 4
```
sock_traffic_control() 逻辑测试
```
- 5
```
set_original_dst_info() 测试
验证原始目标地址是否正确写入 bpf_sk_storage 结构（IPv4 和 IPv6 分支都测试）
```
- 6
```
DNAT 重定向的效果
确认调用 SET_CTX_ADDRESS4 或 SET_CTX_ADDRESS6 后，ctx->user_ip4 或 ctx->user_ip6 和端口被正确修改
```
-7 
```
 尾调用 kmesh_workload_tail_call()
```

这是第一个问题：这里的尾调是在干什么？

》sudo bpftool map dump name km_cgr_tailcall

key: 00 00 00 00  value: af 00 00 00
key: 01 00 00 00  value: b0 00 00 00
Found 2 elements
》 sudo bpftool prog show id 175
sudo bpftool prog show id 176

175: cgroup_sock_addr  name cgroup_connect4_prog  tag 1d9ae55e8bfc0a34  gpl
        loaded_at 2025-07-18T09:32:17+0800  uid 0
        xlated 14400B  jited 8113B  memlock 16384B  map_ids 81,82,84,85,86,87,88,89,90,91,92
        btf_id 268
176: cgroup_sock_addr  name cgroup_connect6_prog  tag 369ed384a24dd733  gpl
        loaded_at 2025-07-18T09:32:17+0800  uid 0
        xlated 15024B  jited 8423B  memlock 16384B  map_ids 81,84,85,86,82,87,88,89,90,91,92
        btf_id 269

他好像又调回到自己了，好奇怪，还不清楚怎么了

第二个问题：frontend_v = map_lookup_frontend(&frontend_k);

他在这里将key填充找value,这里的key就是本来要传入的目的ip

关键：首先将原目的ip存入frontend_key结构体，然后调用bpf_map_lookup_elem(&map_of_frontend, frontend_key);来找到相应的frontend_value,这个里面存了__u32 upstream_id上游id,把这个id先存在 service_k.service_id里面通过bpf_map_lookup_elem(&map_of_service,  service_k)去找相应的service_value，这个结构体里面存了：
typedef struct {
    __u32 prio_endpoint_count[PRIO_COUNT]; // endpoint count of current service with prio
    __u32 lb_policy; // load balancing algorithm, currently supports random algorithm, locality loadbalance
                     // Failover/strict mode
    __u32 service_port[MAX_PORT_COUNT]; // service_port[i] and target_port[i] are a pair, i starts from 0 and max value
                                        // is MAX_PORT_COUNT-1
    __u32 target_port[MAX_PORT_COUNT];
    struct ip_addr wp_addr;
    __u32 waypoint_port;
} service_value;要是找到了，
但是要是找不到，就将这个id存到backend_key结构体，kmesh_map_lookup_elem(&map_of_backend, backend_key);，找到相应的backend_value，他长这样：

typedef struct {
    struct ip_addr addr;//后端 Pod 的实际 IP 地址
    __u32 service_count;//当前这个 backend（Pod）属于多少个服务（Service）
    __u32 service[MAX_SERVICE_COUNT];//该 backend（Pod）属于的服务 ID 列表
    struct ip_addr wp_addr;//如果该 Pod 需要通过 Waypoint 中转，这里记录它的 waypoint IP
    __u32 waypoint_port;//Waypoint 节点监听的端口
} backend_value;，然后调用waypoint_manager(kmesh_ctx, &backend_v->wp_addr, backend_v->waypoint_port);这里第二个参数最终ip，第三个参数最终port;
### 进行验证
conn, err := net.Dial("tcp4", "10.96.0.1:8080") // 原始目标
if err != nil {
    t.Fatalf("Dial failed: %v", err)
}
defer conn.Close()

remoteAddr := conn.RemoteAddr().String()
t.Logf("Actual connected to: %s", remoteAddr)

// 解析 IP
host, _, _ := net.SplitHostPort(remoteAddr)
if host != "10.244.1.5" { // Waypoint 或 Pod IP
    t.Fatalf("Expected redirected to 10.244.1.5, but got %s", host)
}

