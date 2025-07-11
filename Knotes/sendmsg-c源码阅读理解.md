 sudo make ebpf_unit_test V=1



项目开发：

首先你应该写三个ebpf文件的测试，首先应该思考的问题

1.这三个ebpf文件写测试的先后顺序：sendmsg.c

2.这三个ebpf文件的ut测试用哪个框架写

3.sendmsg.c他需要真实挂载测试吗？？

#### kmesh技巧
package bpftests可以找到关键文件

### 7.8-熟悉sendMsg源码
#### 主要内容
在sendmsg/send调用过程中，向消息体前部插入一些元数据，后续在waypoint网关中解码处理
- 数据类型|后面的总长度大小|实际数据（一般就是地址）|结束（一般组合是数据类型+为0的长度）
-     1        |      4                   |               6                          |               1+4
- eg:[01][00 00 00 06][C0 A8 01 01][1F 90][FE][00 00 00 00]
- 表示的是：原始目标是192.168.1.1:8080
```
//这个对应的就是上面的数据类型
enum TLV_TYPE{
    TLV_ORG_DST_ADDR=0x01,// 原始目的地址
    TLV_PAYLOAD=0xfe,// 标志 payload 起始或 TLV 结束
}
```
疑问
- 将元信息编码进 payload，payload是什么
- 解答：指的是应用层通过socket发送出去的数据内容
- eg:conn.Write([]byte("hello world")),这里的"hello world"就是payload,在ebpf中，这些数据被存在struct sk_msg_md这个结构体中，你将他进行拦截，然后前面加上TLV,那么他就会变成[TLV][TLV_END][hello world]
- 后续在waypoint网关中解码处理，waypoint是什么
- 解答：
- 他是服务网格中的一个关键流量中转站，像一个网关一样拦截科幻端发送的请求流量，做认证，观测，转发等处理，者也就是kmesh的特殊之处，不依赖sidecar，哈哈哈，说偏了
- [Client Pod] →→→ [Waypoint] →→→ [Target Pod]
- 流程一般如下：数据先送到waypoint,它可以解释TLV，就知道原始目标ip和端口，然后他决定是否进行转发，重定向或者阻止，要是通过，最后就发到target pod了
#### ok,现在开始看函数
- 1
```
//检查写入操作是否会超出消息缓冲区边界，防止内存越界。
static inline int check_overflow(struct sk_msg_md *msg, __u8 *begin, __u32 length)
{
    if (msg->data_end < (void *)(begin + length)) {
        BPF_LOG(ERR, SENDMSG, "msg over flow\n");
        return 1;
    }
    return 0;
}
```

```struct sk_msg_md {
    void *data;      // 指向 payload 开始
    void *data_end;  // 指向 payload 结束
};

```
- begin：你打算写入的起始地址,length：你打算写多少字节,检查 begin(开始写 TLV 数据的起始地址) + length(数据总长度) 是否还在 msg->data_end 之内
- 那我就好起了，他怎么不检查他是不是从大于data开始的呢？
- 默认：在 sk_msg 程序中，eBPF verifier（验证器）已经确保了你访问的数据是从合法区域开始的。只要你用合法指针偏移出来的地址，begin 不会小于 msg->data。

- 2
```
//从 socket 的关联存储中获取“原始目标地址信息”（IP + 端口），以便后续编码成 TLV 插入 payload 中。
static inline int get_origin_dst(struct sk_msg_md *msg, struct ip_addr *dst_ip, __u16 *dst_port)
{
    //msg->sk → 当前这条消息属于哪个 socket → 把这个 socket 转成结构体来看细节
    struct bpf_sock *sk = (struct bpf_sock *)msg->sk;
    //定义一个指针，指向我们之后要查找的 “socket 附加数据”。
    struct sock_storage_data *storage = NULL;
    //map_of_sock_storage 中是否存在 msg->sk 对应的条目，取决于你是否在连接建立时创建并写入过 sock_storage_data，要是存在就返回storage
    storage = bpf_sk_storage_get(&map_of_sock_storage, sk, 0, 0);
    //上面这些：获取当前 socket 之前记录的原始目标地址、是否需要经过 waypoint、是否已经做过 TLV 编码等状态
    //进行判断：没有storage说明这个socket没有任何数据；没有storage->via_waypoint说明这条消息不需要经过Waypoint，也就不需要构造TLV了，storage->has_encoded说明之前已经插过TLV了，不需要在重复插入了
    if (!storage || !storage->via_waypoint || storage->has_encoded) {
        return -ENOENT;
    }
    //当前 socket 是使用 IPv4 协议族的
    if (msg->family == AF_INET) {
        dst_ip->ip4 = storage->sk_tuple.ipv4.daddr;
        *dst_port = storage->sk_tuple.ipv4.dport;
    } else {
        bpf_memcpy(dst_ip->ip6, storage->sk_tuple.ipv6.daddr, IPV6_ADDR_LEN);
        *dst_port = storage->sk_tuple.ipv6.dport;
    }

    storage->has_encoded = true;

    return 0;
}
```
```
struct ip_addr {
    union {
        __u32 ip4;
        __u32 ip6[4];
    };
};
#define IPV6_ADDR_LEN 16
```
- 疑问：
- 1：为啥获取ip地址的方式不一样？ dst_ip->ip4 = storage->sk_tuple.ipv4.daddr;bpf_memcpy(dst_ip->ip6, storage->sk_tuple.ipv6.daddr, IPV6_ADDR_LEN);
- 解答：
- IPv4 是 4 字节整数，可以直接赋值；
- dst_ip->ip6 是一个数组,需要用 bpf_memcpy() 拷贝。
- 3
```
//为消息分配额外空间以容纳 TLV 数据。
static inline int alloc_dst_length(struct sk_msg_md *msg, __u32 length)
{
    int ret;
    ret = bpf_msg_push_data(msg, 0, length, 0);
    if (ret) {
        BPF_LOG(ERR, SENDMSG, "failed to alloc memory for msg, length is %d\n", length);
        return 1;
    }
    return 0;
}
```
- int bpf_msg_push_data(struct sk_msg_md *msg, __u32 start, __u32 len, __u64 flags)
- 这是ebpf中的一个help函数
- 目标：在当前 payload 的最前面（offset=0），插入 len 字节的空间，把原始数据整体向后移动。
- eg:本来是payload，现在变成len字节的长度+payload,空出来的地方，就可以安全地写 TLV 数据
- 4
```
//sk_msg当前消息，当前写入偏移量，payload要写入的数据，payloadlen要写入的数据长度
#define SK_MSG_WRITE_BUF(sk_msg, offset, payload, payloadlen)                                                          \
    do {                                                                                                               \
        __u8 *begin = (__u8 *)((sk_msg)->data) + *(offset);                                                            \
        if (check_overflow((sk_msg), begin, payloadlen)) {                                                             \
            BPF_LOG(ERR, SENDMSG, "sk msg write buf overflow, off: %u, len: %u\n", *(offset), payloadlen);             \
            break;                                                                                                     \
        }                                                                                                              \
        bpf_memcpy(begin, payload, payloadlen);                                                                        \
        *(offset) += (payloadlen);                                                                                     \
    } while (0)
//写入 TLV 结束标记 (TLV_PAYLOAD 类型，长度为 0)。
static inline void encode_metadata_end(struct sk_msg_md *msg, __u32 *off)
{
    __u8 type = TLV_PAYLOAD;
    __u32 size = 0;

    SK_MSG_WRITE_BUF(msg, off, &type, TLV_TYPE_SIZE);
    SK_MSG_WRITE_BUF(msg, off, &size, TLV_LENGTH_SIZE);
    return;
}
```
- 这个函数没啥难的，最后就是在msgoffset后面写上了结束标志：[FE][00 00 00 00]
- 5
```
//编码原始目标地址信息到消息中：获取原始目标地值  分配空间  写入TLV头(类型和长度)  写入IP地址和端口   写入结束标记
static inline void encode_metadata_org_dst_addr(struct sk_msg_md *msg, __u32 *off, bool v4)
{
    struct ip_addr dst_ip = {0};
    __u16 dst_port;
    __u8 type = TLV_ORG_DST_ADDR;
    __u32 tlv_size = (v4 ? TLV_ORG_DST_ADDR4_SIZE : TLV_ORG_DST_ADDR6_SIZE);
    __u32 addr_size = (v4 ? TLV_ORG_DST_ADDR4_LENGTH : TLV_ORG_DST_ADDR6_LENGTH);

    if (get_origin_dst(msg, &dst_ip, &dst_port))
        return;

    if (alloc_dst_length(msg, tlv_size + TLV_END_SIZE))
        return;

    BPF_LOG(DEBUG, SENDMSG, "get valid dst, do encoding...\n");

    // write T
    SK_MSG_WRITE_BUF(msg, off, &type, TLV_TYPE_SIZE);

    // write L
    addr_size = bpf_htonl(addr_size);
    SK_MSG_WRITE_BUF(msg, off, &addr_size, TLV_LENGTH_SIZE);

    // write V
    if (v4)
        SK_MSG_WRITE_BUF(msg, off, (__u8 *)&dst_ip.ip4, TLV_IP4_LENGTH);
    else
        SK_MSG_WRITE_BUF(msg, off, (__u8 *)dst_ip.ip6, TLV_IP6_LENGTH);
    SK_MSG_WRITE_BUF(msg, off, &dst_port, TLV_PORT_LENGTH);

    // write END
    encode_metadata_end(msg, off);
    return;
}
```
- 这个也挺简单的，前提是你要对TLV结构有熟悉的了解
- TLV
```
TLV = Type + Length + Value，其中：

Type：1 字节（例如 0x01 代表原始目标）

Length：4 字节（表示 Value 长度）

Value：

IPv4 = 4 字节 IP + 2 字节端口 = 6 字节

IPv6 = 16 字节 IP + 2 字节端口 = 18 字节

End TLV：再加上一个 [0xFE][0x00000000]
```
- 这里面就有一个不太懂__u32 bpf_htonl(__u32 host_32bits);这也是ebpf提供的helper函数,目的：将一个 32 位的整数从主机字节序转换为网络字节序
- 那为啥就这个要强调，其他的我也没见说呀
- 因为 addr_size 是个 __u32 类型的数值，在 TLV 中属于“协议字段”，必须转成网络字节序；而其他字段（如 IP、端口）已经是网络字节序了，不需要再转
- 6
```
//这是 BPF 程序的入口点，附加到 sk_msg 钩子点
SEC("sk_msg")
int sendmsg_prog(struct sk_msg_md *msg)
{
    __u32 off = 0;
    if (msg->family != AF_INET && msg->family != AF_INET6)
        return SK_PASS;

    // encode org dst addr
    encode_metadata_org_dst_addr(msg, &off, (msg->family == AF_INET));
    return SK_PASS;
}
```
- 疑问：ebpf的help函数是什么意思
- 解答：helper函数就是可以受限制的访问一些内核东西
### 总结：
- 调用过程如下
- 在sendmsg_prog()函数中调用encode_metadata_org_dst_addr()开始进行处理
- 在encode_metadata_org_dst_addr()中的步骤才是重要
- 1:get_origin_dst(msg, &dst_ip, &dst_port)先获得原始地址的ip和port
- 2:alloc_dst_length(msg, tlv_size + TLV_END_SIZE)在msg前面给TLV留出空间
- 3:调用这个宏SK_MSG_WRITE_BUF将TLV的内容插进去
- 4: encode_metadata_end(msg, off);调用这个其实内部也是调用宏SK_MSG_WRITE_BUF将TLV结束标志进行填充
```
              +--------------------------+
User Space -> |      sendmsg() 调用     |
              +--------------------------+
                           ↓
              +--------------------------+
              |     BPF sk_msg 程序触发  |
              +--------------------------+
                           ↓
        ┌─────────────编码目标地址 TLV──────────────┐
        ↓                                           ↓
 [原始 dst ip, port] → 插入消息体开头 → 加上结束 TLV → payload 被修改
        ↓
        └─────────────继续放行(SK_PASS)─────────────┘
                           ↓
              +--------------------------+
              |     数据发往 socket      |
              +--------------------------+
```
### 测试点大概有什么
```
编号	测试点	说明
1	IPv4 TLV 编码正确	编码的 TLV 内容是否正确：type/len/ip/port/end
2	IPv6 TLV 编码正确	IPv6 情况下 TLV 长度为 18，IP 为 16 字节
3	非 AF_INET/AF_INET6 不处理	family=AF_UNIX 时程序应直接返回，不写入
4	没有 via_waypoint 时不写入	storage 里不是经过 waypoint 的连接，跳过
5	has_encoded = true 时不重复编码	保证程序只写一次 TLV，不会多次写入
6	has_set_ip = false 时正常获取 ip/port	保证从 storage 中获取原始目标地址成功
7	TLV 越界保护有效	模拟 TLV 长度太长或 offset 接近 data_end，测试 check_overflow 生效
8	TLV 结束标记正确写入	[0xFE][0 0 0 0] 必须存在，验证 payload 尾部内容
9	bpf_msg_push_data 被正确调用	alloc_dst_length() 是否触发 push_data，空间是否充足
10	bpf_sk_storage_get 返回 NULL 的处理	模拟 storage 没有挂载时应返回 -ENOENT，并跳过编码


```
- 整体分析
- hook 点：sk_msg表示程序挂载到 sockmap 的 sk_msg hook，用于拦截 socket 层面 write/sendmsg 方向的数据流（发包时调用）
- 条件是：FD 已被放入 sockmap 且 attach 成功（AttachSkMsg）。
- 第一个思路
```
✅ 编译 sendmsg.c，生成 sk_msg 类型的 sendmsg_prog	
✅ 定义一个 sockmap，如 struct bpf_map_def km_sockmap	
✅ 加载 BPF 对象（ebpf.NewCollection()）	
✅ 获取 km_sockmap map 和 sendmsg_prog program	
✅ 启动 TCP server + Dial 连接	
✅ 将连接的 socket FD update 到 sockmap	
✅ 此时，sendmsg_prog 会自动 attach 并在 conn.Write() 时被触发	
✅ （可选）使用 ringbuf / log / map 输出编码结果
```
