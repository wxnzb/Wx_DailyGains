### 阅读cilium的bpf单元测试总结【https://docs.cilium.io/en/v1.17/contributing/testing/bpf/#bpf-testing】
- 使用BPF_RPOG_RUN功能在内核运行ebpf程序，而无需将他们附加到实际的钩子上
- 测试本身完全用c语言，测试的结果以Go测试框架输出-【为了更好的集成和其他工具】
#### 运行测试
- 要运行所有测试：make run_bpf_tests
- 要运行单个测试，需指定名称：make run_bpf_tests BPF_TEST_FILE="xdp_nodeport_lb4_nat_lb"
#### 编写测试
- 宏定义在bpf/tests/common.h
- 所有BPF测试都位于bpf/tests目录下
- .c文件包含
- bpf测试程序
- 至少一个CHECK程序，他取代了bpf程序常用的SEC,需要两个参数，因为CHECK实际上就是一个宏，那么所有的.c测试都要包含common.h
##### CHECK程序
- 需要两个参数，一个是程序类型【xdp/tc】，一个是测试的名称；
- 以test_init()开始，test_finish()结束
- 会隐式返回结果，无需加return
- 到达test_finish()时就是pass
- 每个CHECK程序可能包含子测试，使用TEST宏创建子测试
- 例如：
```
#include "common.h"
CHECK("xdp", "nodeport-lb4")
int nodeportLB4(struct __ctx_buff *ctx)
{
        test_init();

    /* ensure preconditions are met */
    /* call the functions you would like to test */
    /* check that everything works as expected */
    //子测试
    TEST("Non-zero", {
        unsigned int hash = jhash_3words(123, 234, 345, 456);

        if (hash != 2698615579)
            test_fatal("expected '2698615579' got '%lu'", hash);
    });
    test_finish();
}
```
-  看来CHECK程序还是很有用的，但是跨尾调用测试需要额外的步走，因为程序在尾调时不会返回到CHECK，因此无法检查他是否成功
-  那么怎样解决这个问题
-  在CHECK程序之前需要
-  先调用PKTGEN-构建bpf上下文(例如：为TC填充struct_sk_buff)-但是这个也可以不是尾调，要是想要模拟复杂的上下文可能需要他
-  然后调用SETUP-进一步设置步骤(例如：填充bpf映射)
-  执行这两个步骤可以使用实际的数据包内容
-  把bpf上下文传递给CHECK，通过执行测试设置和SETUP的尾部调用，可以执行完整个程序；
-  SETUP返回u32的形式床给CHECK数据包的开头，CHECK程序将在(void*)data+4处知道到世纪的数据包数据
-  什么时跨尾调用？
-  程序在执行中使用了bpf_tail_call()跳转到了另一个程序执行不再回来，那么CHECK无法知道执行结果，SETUP会提前模拟他跳转到子程序的执行流程，并将执行结果存在data[0-3] 中，给CHECK到时候检查结果是否一致

#### 函数参考
- test_log(fmt,args)-写入一条日志消息
- test_fail()-将当前测试或者子测试标记为失败但是继续运行
- test_fail_now()-将当前测试或者子测试标记为失败并停止运行
- test_fatal()-写入日志然后调用test_fail_now()
- assert(stmt)-断言，要是不对，调用test_fail_now()
- test_skip()-将当前测试或子测时标记为跳过但是继续执行
- test_skip_now()-将当前测试或子测时标记为跳过但是停止执行
- test_init()-初始化内部测试，调用最早
- test_finish()-提交结果并返回
###### 这些有_now基本都是就停止测试了，在子测试TEST的前提下，要时出现了for,while,switch应该用break,_now不应该出现
#### 模拟函数
- 做法
```
//1
创建一个有惟一名字并与要替换的函数有相同标签
//2
创建一个宏来达到替换的效果
eg:#define A A_mock
//3
包含我们要替换的文件
```
- 相同标签是什么意思？
#### eg:
```
#include "common.h"

#include "bpf/ctx/xdp.h"

#define fib_lookup mock_fib_lookup

static const char fib_smac[6] = {0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x02};
static const char fib_dmac[6] = {0x13, 0x37, 0x13, 0x37, 0x13, 0x37};

long mock_fib_lookup(__maybe_unused void *ctx, struct bpf_fib_lookup *params,
            __maybe_unused int plen, __maybe_unused __u32 flags)
{
    memcpy(params->smac, fib_smac, sizeof(fib_smac));
    memcpy(params->dmac, fib_dmac, sizeof(fib_dmac));
    return 0;
}

#include "bpf_xdp.c"
#include "lib/nodeport.h"
```
### 局限性
- 诸如 test_log() 、 test_fail() 、 test_skip() 等测试函数只能在主程序或 TEST 的范围内执行。这些函数依赖于 test_init() 设置的局部变量。 并且在其他功能中使用时会产生错误。