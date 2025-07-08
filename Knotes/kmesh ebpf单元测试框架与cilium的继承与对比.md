###  kmesh ebpf单元测试框架
#### 下面这是cilium的设计思路
- 参考cilium
- 该框架允许开发者编写测试用例，构建网络数据包，验证ebpf在不同情况下的行为
- 以xdp_nodeport_lib4_test.c为例
```
#include"common.h"
#include"bpf/ctx/xpd.h"
#define fib_lookup mock_fib_lookup
static const char fib_smac[6]={0xDE,0xAD,0xBE,0xEF,0x01,0x02}
static const char fib_dmac[6]={0x13,0x37,0x13,0x37,0x13,0x37}
long mock_fib_lookup(_maybe_unused void *ctx,struct bpf_fib_lookup *params,_maybe_unused int plen,_maybe_unused_u32 flags){
    _bpf_memcpy_builtin(params->smac,fib_smac,ETH_ALEN);
    _bpf_memcpy_builtin(params->dmac,fib_dmac,ETH_ALEN);
    return 0;
}
#include "bpf_xdp.c"
// 在测试代码中使用尾调用执行被测试BPF程序Add commentMore actions
struct {
	__uint(type, BPF_MAP_TYPE_PROG_ARRAY);
	__uint(key_size, sizeof(__u32));
	__uint(max_entries, 2);
	__array(values, int());
} entry_call_map __section(".maps") = {
	.values = {
		[0] = &cil_xdp_entry,
	},
};
// 构建测试xdp包，可以使用PKTGEN宏达到相同的效果
static __always_inline int build_packet(struct __ctx_buff *ctx){}
SETUP("xdp", "xdp_lb4_forward_to_other_node")
int test1_setup(struct __ctx_buff *ctx)
{
	int ret;

	ret = build_packet(ctx);
	if (ret)
		return ret;

	lb_v4_add_service(FRONTEND_IP, FRONTEND_PORT, IPPROTO_TCP, 1, 1);
	lb_v4_add_backend(FRONTEND_IP, FRONTEND_PORT, 1, 124,
			  BACKEND_IP, BACKEND_PORT, IPPROTO_TCP, 0);

	/* Jump into the entrypoint */
	tail_call_static(ctx, entry_call_map, 0);
	/* Fail if we didn't jump */
	return TEST_ERROR;
}
CHECK("xdp", "xdp_lb4_forward_to_other_node")
int test1_check(__maybe_unused const struct __ctx_buff *ctx)
{
	test_init();

	void *data = (void *)(long)ctx->data;
	void *data_end = (void *)(long)ctx->data_end;

	if (data + sizeof(__u32) > data_end)
		test_fatal("status code out of bounds");
     
     //...

	test_finish();
}
```
### common.h中包含的宏
- TEST:定义单个测试用例
- PKTGEN:用于生成网络数据包
- SETUP:初始化环境和前置条件
- CHECK:验证测试结果
#### 测试流程
- test_init:初始化测试环境
- test_finish:完成测试并返回结果
- test_fail(fmt,...)：将当前测试标记为失败
- test_skip(fmt,...):跳过当前测试
#### 断言与日志机制
- assert(cond):判断条件是否为真
- test_log(fmt,...):类似printf
- test_error(fmt,...):记录错误并标记测试失败
- test_fatal(fmt,...):记录严重错误并立即终止测试
- assert_metrics_count(key,count):验证特定指标技术是否符合预期
#### 测试框架使用以下状态码标记测试结果
- TEST_ERROR(0):测试执行遇到错误
- TEST_PASS(1):测试通过
- TEST_FAIL(2):测试失败
- TEST_SKIP(3):测试被跳过
#### 测试执行流程
- 测试启动：make run_bpf_tests
- 容器构建：构建docker测试容器
- 测试编译：用clang编译ebpf测试代码
- 测试协调：go测试框架负责整个测试的生命周期，包含：1：加载编译好的ebpf程序；2：初始化测试环境；3：执行测试用例；4：收集测试结果
#### go与ebpf通信机制
- Protocol Buffer:定义了结构化消息格式，这是他们通信的方式
- 测试结果存储：ebpf测试程序将测试结果编码后存在suit_result_map
- 结果体取与解析：go测试读取map并进行解码，进行检验和报告
#### 单测覆盖率-重点：之前没了解过
- 工作原理：对ebpf字节码进行插庄，为每行代码分配为一序号，添加计数器逻辑：cover_map[line_id]++
- 当程序执行时，访问的每行代码对应的计数器会递增
- 覆盖率分析流程：1：插庄后ebpf执行是收集执行次数数据；2：用户态读取cover_map;3:将手机的数据与源代码行号进行关联；4：生成标准格式的覆盖率报告
#### 数据交换流程
[eBPF Test Program] → [Encode Results] → [suite_result_map] → [Go Test Runner] → [Decode & Report]
go测试框架负责汇报最终结果，测试通过率；覆盖率统计；失败用例分析；
#### 现在是ebpf test
#### 他与cilium有一些差异需要考滤
1. **构建系统差异**：Add commentMore actions
     - Cilium 使用 Clang 直接将 BPF 代码编译为字节码
     - Kmesh 使用 cilium/ebpf 提供的 bpf2go 工具，将 BPF C 代码编译并转换为 Go 代码调用

2. **代码维护挑战**：
     - 当前 Kmesh 中使用 libbpf 维护被测试 BPF 代码，造成需要同时维护 bpf2go 和 unittest-makefile 两套编译命令
     - 核心 eBPF 代码变更后，测试代码需要同步更新，维护成本较高
#### 目标
- 1：设计一个与主代码紧密继承的测试框架
- 减少维护开小
- 使用go测试框架
#### kmesh ebpf test设计框架
#####  整体架构：
- 1：ebpf测试程序层：编写c的测试程序，包含测试用例，测试数据和逻辑验证
- 2：go测试驱动层：加载ebpf程序，在用户态执行测试，收集结果
- 结果通信层：使用Protocol Buffer结构进行测试结果传递
#### ebpf test核心组件
- PKTGEN：生成测试数据包，模拟网络输入
- JUMP：为调用BPF程序
- CHECK：验证测试结果
- 内存数据交换：通过map在BPF和go用户传递测试结果
#### go测试
- unittest类：unittest结构体表示一个测试单元，包含测试名称，ebpf文件名，用户空间测试函数
- 程序加载器：使用 cilium/ebpf库加载变异后的ebpf文件
- 测试执行器：调用BPF程序，传递·测试数据和上下文
- 结果解析器：从ebpf map中读取并解析
#### 测试执行加载
- 用户态unittest中要时有setupInuserSpace,首先运行他设置测试环境
- 要是存在pktgen,运行他生成测试数据
- 运行jump,执行北侧使得bpf程序
- 运行check,验证结果，他是从suit_result_map中读取数据，解析并用go测试框架生成报告

