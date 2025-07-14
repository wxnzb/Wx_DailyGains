###  ebpf户用户态进行交互的关键
bpf_common.h 在整个 Kmesh 的 eBPF 架构中 确实是“用户态验证逻辑的核心桥梁”。
- int bpf_map_update_elem(void *map, const void *key, const void *value, __u64 flags);
```
参数详解
void *map
指向要操作的 BPF map 的指针，通常是声明的 BPF_MAP 变量。

const void *key
指向要插入/更新元素的键（key）的指针。键类型和大小由 map 声明时确定。

const void *value
指向对应键的值（value）指针，数据格式和大小由 map 声明时确定。

__u64 flags
更新行为的标志。常用值：

BPF_ANY（0）: 如果键不存在则插入，存在则更新。

BPF_NOEXIST（1）: 只插入键不存在的新元素。

BPF_EXIST（2）: 只更新已存在的元素。
```
- kmManageMap := coll.Maps["map_of_manager"]这样会报错，但是这样不会
- kmManageMap := coll.Maps["km_manage"]
- #define map_of_manager km_manage，虽然在ebpf文件中宏定义是这样
- 宏 map_of_manager 会被预处理器替换成 km_manage，所以实际定义的是一个叫 km_manage 的 map。意味着最终 ELF 文件和加载后的 eBPF 程序中的 map 名字是 km_manage，而不是 map_of_manager
### _test.c中那些需要进行mock,哪些不需要
- 这点需要注意！！！！
### 有的测试他根本就没有在map里面进行任何输入，那还怎么检查？？？
- 我看sockops的测试他都是通过获取map进行观察测试结果的
- 目前想法：startLogReader(coll)，go端这样，然后在.c端调用这个宏应该就可以，先测试一下
```
//l: 日志级别（如 INFO, ERR, WARN 等）
//块类型名（如 SOCKOPS, CGROUP, XDP）
// 格式字符串（如 "some value = %d"）
#define BPF_LOG(l, t, f, ...)                                                                                          \
    do {                                                                                                               \
        if ((int)(BPF_LOG_##l) <= bpf_log_level) {                                                                     \
        //BPF_LOG(INFO, CGROUP, "monitoring disabled")
        //就会变成：
            static const char fmt[] = "[" #t "] " #l ": " f "";                                                        \
            if (!KERNEL_VERSION_HIGHER_5_13_0)                                                                         \
                bpf_trace_printk(fmt, sizeof(fmt), ##__VA_ARGS__);                                                     \
            else                                                                                                       \
                BPF_LOG_U(fmt, ##__VA_ARGS__);                                                                         \
        }                                                                                                              \
    } while (0)

```
-  现在内核大于5.13,因此最终ebpf端调用的就是他
```
#define BPF_LOG_U(fmt, args...)                                                                                        \
    ({                                                                                                                 \
        struct log_event *e;                                                                                           \
        __u32 ret = 0;                                                                                                 \
        e = bpf_ringbuf_reserve(&kmesh_log_events, sizeof(struct log_event), 0);                                       \
        if (!e)                                                                                                        \
            break;                                                                                                     \
        ret = Kmesh_BPF_SNPRINTF(e->msg, sizeof(e->msg), fmt, args);                                                   \
        e->ret = ret;                                                                                                  \
        if (ret < 0)                                                                                                   \
            break;                                                                                                     \
        bpf_ringbuf_submit(e, 0);                                                                                      \
    })
```
- 在go端startLogReader(coll)这个函数内部长这样
```
func startLogReader(coll *ebpf.Collection) {
    //先判断当前系统内核版本是否 不 低于 5.13
	if !utils.KernelVersionLowerThan5_13() {
		// TODO: use t.Context() instead of context.Background() when go 1.24 is required
		logger.StartLogReader(context.Background(), coll.Maps["km_log_event"])
	}
}
---------------------------------------------------
func StartLogReader(ctx context.Context, logMapFd *ebpf.Map) {
//开启一个异步携程，异步读取 ringbuf 中的日志记录，传入的参数是传入的是你从 coll.Maps["km_log_event"] 拿到的 map
	go handleLogEvents(ctx, logMapFd)
}
---------------------------------------------------
//这个就是打印log的关键
struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, 256 * 1024 /* 256 KB */);
} kmesh_log_events SEC(".maps");

----------------------------------------------------
func handleLogEvents(ctx context.Context, rbMap *ebpf.Map) {
   //第一步：准备日志记录器
	log := NewLoggerScope("ebpf")
	//打开 ringbuf reader
	//ringbuf.NewReader(...) 会准备一个 reader 来接收 BPF 侧用 bpf_ringbuf_submit() 写入的数据
	events, err := ringbuf.NewReader(rbMap)
	if err != nil {
		log.Errorf("ringbuf new reader from rb map failed:%v", err)
		return
	}
    //循环读取
	for {
		select {
		case <-ctx.Done():
			return
		default:
		//每次有新消息，就会调用 events.Read() 读取一个 record.RawSample
			record, err := events.Read()
			if err != nil {
				return
			}
			//解析并打印
			le, err := decodeRecord(record.RawSample)
			if err != nil {
				log.Errorf("ringbuf decode data failed:%v", err)
			}
			log.Infof("%v", le.Msg)
		}
	}
}
```
- 大致流程：
```
eBPF 程序中使用 BPF_LOG()
   ↓
translate 为 bpf_ringbuf_submit(msg, 0)
   ↓
km_log_event ringbuf map（内核空间）
   ↓
coll.Maps["km_log_event"] 映射到用户态
   ↓
StartLogReader() 启动 handleLogEvents() 循环读取
   ↓
decodeRecord + log.Infof 输出
```