### kmesh的sockops例子也要详细明白
##### 写这个的目的，首先，看test.c到底是些什么内容，怎样写，明确他们是怎样进行触发的，然后对应test_unit看；
- 整体流程
```
 客户端调用 connect()
                 │
     ┌───────────▼────────────┐
     │ BPF_SOCK_OPS_TCP_CONNECT_CB │ ← 发起连接时触发
     └───────────┬────────────┘
                 │
     判断是否连接到了 AGENT + 控制端口
                 │
      ┌──────────▼────────────┐
      │加入或删除本地 IP 到 map │ ← local_ip 成为 Kmesh 管理节点
      └──────────┬────────────┘
                 │
        三次握手成功（连接建立）
                 │
     ┌───────────▼────────────┐
     │BPF_SOCK_OPS_ACTIVE_ESTABLISHED_CB│ ← 客户端连接成功触发
     └───────────┬────────────┘
                 │
   is_managed_by_kmesh(local_ip)?
                 │
              Yes│
                 ▼
    - 记录连接建立（OUTBOUND）
    - 启用 TCP 状态跟踪
    - 获取 sock_storage
    - 若 via_waypoint，则启用 TLV metadata 注入
```
    - -------------------------------------------------------------------------------------
```
int sockops_prog(struct bpf_sock_ops *skops)
{
    if (skops->family != AF_INET && skops->family != AF_INET6)
        return 0;

    switch (skops->op) {
    //首先这个是用户调用connect(),客户端发起SYN前就会调用
    case BPF_SOCK_OPS_TCP_CONNECT_CB:
        skops_handle_kmesh_managed_process(skops);
        break;
        //三次握手完成后，客户端进入ESATBLISHED
    case BPF_SOCK_OPS_ACTIVE_ESTABLISHED_CB:
        //判断是否被 Kmesh 管理
        if (!is_managed_by_kmesh(skops))
            break;
        //记录连接建立事件
        observe_on_connect_established(skops->sk, OUTBOUND);
        //允许后续 TCP 状态变化通知
        if (bpf_sock_ops_cb_flags_set(skops, BPF_SOCK_OPS_STATE_CB_FLAG) != 0)
            BPF_LOG(ERR, SOCKOPS, "set sockops cb failed!\n");
        //获取 bpf_sock 指针
        struct bpf_sock *sk = (struct bpf_sock *)skops->sk;
        if (!sk) {
            break;
        }
        //获取 sock storage 数据
        struct sock_storage_data *storage = NULL;
        storage = bpf_sk_storage_get(&map_of_sock_storage, sk, 0, 0);
        if (!storage) {
            break;
        }
        //启用 metadata 编码（TLV）
        if (storage->via_waypoint) {
            enable_encoding_metadata(skops);
        }
        break;
        //三次握手完成后，accept()调用成功，触发下面这个
    case BPF_SOCK_OPS_PASSIVE_ESTABLISHED_CB:
        if (!is_managed_by_kmesh(skops) || skip_specific_probe(skops))
            break;
        observe_on_connect_established(skops->sk, INBOUND);
        if (bpf_sock_ops_cb_flags_set(skops, BPF_SOCK_OPS_STATE_CB_FLAG) != 0)
            BPF_LOG(ERR, SOCKOPS, "set sockops cb failed!\n");
        auth_ip_tuple(skops);
        break;
        //状态变化的时候就会触发，比如说：SYN_RECV → ESTABLISHED，但是它放在最后，一般把前面的一触发就结束了呀
    case BPF_SOCK_OPS_STATE_CB:
        if (skops->args[1] == BPF_TCP_CLOSE) {
            observe_on_close(skops->sk);
            clean_auth_map(skops);
        }
        break;
    default:
        break;
    }
    return 0;
}
```
- 这个代码咱就是整体知道干啥就行了
- 第一个：首先触发条件：
```
客户端发起 connect()
↓
内核触发 sockops 回调：BPF_SOCK_OPS_TCP_CONNECT_CB
↓
程序进入 switch 分支：
    case BPF_SOCK_OPS_TCP_CONNECT_CB:
        skops_handle_kmesh_managed_process(skops);  ← 🔥 在这里触发！
```
- 作用：用于识别模拟连接（来自 CNI），进而执行 Kmesh IP 注册或删除逻辑
- connect之后触发了，然后获取目的ip:port,根据目标的ip+port来判断是加入/删除本地客户端的ip到kemsh逻辑
- 第二个：触发条件：
```
三次握手完成时（SYN → SYN-ACK → ACK）

只有客户端会触发这个回调
```
- ++++++++++++++++++++++++++++++++++++++++
- 整体流程：
```
TCP三次握手完成（客户端）──┐
                            ▼
                  BPF_SOCK_OPS_ACTIVE_ESTABLISHED_CB
                            │
              ┌────────────┴─────────────┐
              ▼                          ▼
    判断 local_ip 是否在 map_of_manager 中？（是否被 Kmesh 管理）  
              │
            是│
              ▼
      1. 上报连接建立事件（记录方向、状态等）
      2. 启用 TCP 状态跟踪（STATE_CB）
      3. 获取 sk storage
              │
              ▼
      4. 如果设置了 via_waypoint
         → 启用 metadata TLV 注入功能
```
- 这里面就用到了test.c里面的mock，但是他的作用就是
- 3
```
 case BPF_SOCK_OPS_PASSIVE_ESTABLISHED_CB:
        //过滤非管理连接或特定跳过连接
        if (!is_managed_by_kmesh(skops) || skip_specific_probe(skops))
            break;
        //记录服务端新连接的入站建立事件和时间
        observe_on_connect_established(skops->sk, INBOUND);
        //开启连接状态回调，跟踪连接生命周期
        if (bpf_sock_ops_cb_flags_set(skops, BPF_SOCK_OPS_STATE_CB_FLAG) != 0)
            BPF_LOG(ERR, SOCKOPS, "set sockops cb failed!\n");
        //对连接的 IP/端口信息做认证处理
        auth_ip_tuple(skops);
        break;
```
- 4
```
   case BPF_SOCK_OPS_STATE_CB:
        //检测 TCP 连接何时关闭（BPF_TCP_CLOSE）
        if (skops->args[1] == BPF_TCP_CLOSE) {
        //连接关闭时做资源清理和状态统计
            observe_on_close(skops->sk);
        //清理与连接认证相关的映射数据
            clean_auth_map(skops);
        }
        break;
```
- 无论是客户端发起的连接，还是服务端接受的连接，只要成功建立连接，就注册“我想知道这个连接什么时候关闭”
- 客户端和服务端：bpf_sock_ops_cb_flags_set(skops, BPF_SOCK_OPS_STATE_CB_FLAG)
- 你上面设置了 BPF_SOCK_OPS_STATE_CB_FLAG，那么当这个连接关闭时，内核就会进入这个分支
✅ 1. 测试：BPF_SOCK_OPS_TCP_CONNECT_CB 添加 Kmesh 管理 IP
test_sockops_add_kmesh_ip()
{
    // 构造 skops：skops->op = BPF_SOCK_OPS_TCP_CONNECT_CB
    // remote_ip/port 命中 conn_from_sim → 添加 local_ip 到 map_of_manager
    // 验证 map_of_manager 中确实写入了 local_ip
}
✅ 2. 测试：BPF_SOCK_OPS_TCP_CONNECT_CB 删除 Kmesh 管理 IP
test_sockops_del_kmesh_ip()
{
    // 构造 skops：skops->op = BPF_SOCK_OPS_TCP_CONNECT_CB
    // remote_ip/port 命中 conn_from_sim_delete → 删除 local_ip
    // 验证 map_of_manager 被删除对应 key
}
✅ 3. 测试：BPF_SOCK_OPS_ACTIVE_ESTABLISHED_CB 触发 metadata 注入逻辑
test_sockops_active_establish_waypoint()
{
    // mock map_of_manager 使 is_managed_by_kmesh() 命中
    // mock bpf_sk_storage_get() 返回 via_waypoint = 1
    // 构造 skops->op = BPF_SOCK_OPS_ACTIVE_ESTABLISHED_CB
    // 验证 enable_encoding_metadata() 被调用（设置了 cb_flags）
}
✅ 4. 测试：BPF_SOCK_OPS_PASSIVE_ESTABLISHED_CB 服务端建立连接逻辑
test_sockops_passive_establish()
{
    // mock map_of_manager 命中；skip_specific_probe() 返回 false
    // 构造 skops->op = BPF_SOCK_OPS_PASSIVE_ESTABLISHED_CB
    // 验证 observe_on_connect_established 被调用，方向是 INBOUND
    // 验证 enable STATE_CB_FLAG
}
✅ 5. 测试：BPF_SOCK_OPS_STATE_CB 关闭连接触发清理逻辑
test_sockops_state_cb_close()
{
    // 构造 skops->op = BPF_SOCK_OPS_STATE_CB
    // 设置 skops->args[1] = BPF_TCP_CLOSE
    // 验证 observe_on_close() 被调用，clean_auth_map() 被调用
}