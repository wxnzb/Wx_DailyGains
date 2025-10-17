kmesh

## Kmesh-ebpf

##### 介绍你做了啥

kmesh是一个基于ebpf技术的服务网格数据面开源项目，目的是为了解决传统服务网格在性能损耗和资源占用上的缺点，他的核心创新点是通过ebpf在内核层实现流量拦截和服务治理，绕过了用户态代理，实现接近原声网络的性能。由于对于ebpf程序缺乏单元测试导致代码质量风险高，迭代效率低，我的任务主要是实现对sendmsg和cgroup两类关键内核态程序的测试用例，sendmsg主要是检测在收发消息时在消息头前加的tlv格式内容是否正确，cgroup分为cgroup_skb和cgroup_sock的实现，cgroup_skb主要是检测数据包并在特定情况下触发发送日志，cgroup_sock主要是在 **新建连接时** 做服务发现 & 负载均衡，主要检测连接时是否正确 **重定向目标地址**，负载均衡策略是否生效，我的测试主要的测试是用c和go编写的，c代码主要负载Mock无法触发的程序，go代码主要通过真实网络行为来验证Ebpf处理结果是否符合预期。

##### ebpf程序

疑问：有了ebpf为什么减少了上下文切换，把没有和有的整个流程详细写出来

传统的服务网格：客户端通过系统调用connect/send,然后到达内核网络栈，通过iptables规则拦截，重定向到envoy sidecar用户态，然后用户态进行envoy处理，比如服务发现呀，负载均衡呀，经过一系列处理后有发送到内核网络栈，然后有内核网络栈将数据进行发送

加入ebpf之后：客户端通过系统调用connect/send达到内核网络栈，在内核网络栈中不同阶段会触发相应的ebpf程序，比如说connect阶段会触发cgroup_sock进行服务发现和负载均衡，sendmsg阶段会触发sendsg进行数据包修改，数据包流动阶段会触发cgroup_skb进行监控，在触发的ebpf程序里面进行相应的服务发现呀，负载均衡的处理，人处理完成后内核网络栈将数据进行发送，就没有回到用户态的上下文开销

sendmsg.c

主要就是在客户端通过waypoint发送数据是，采用tlv格式编码将原是目标地址信息嵌入到发送的数据包中

测试流程：首先加载ebpf程序到内核，然后从内核中获取关键组件，socket哈希表和sendmsg程序，然后将sendmsg程序挂载到socket哈希值上面，并且挂载的时候将挂载类型设置为socket消息发送时触发。之后就是进行的真实触发流程：通过创建tcp服务器，本地客户端进行dial连接可以获得连接的socket文件描述符，然后将这个文件描述符插入到socket哈希中，然后在向连接写入数据，在写入数据时，内核就会发现这个连接的socket挂载在socket哈希上面，而这个sendmsg程序被挂载到sockhash map上，那么就会触发sendmsg这个ebpf程序的执行，在数据前面加上Tlv，将修改后的数据发送给服务器，然后服务端进行接收并验证tlv格式是否正确

cgroup_skb

主要是实现了细粒度的流量监测，同时监控被kmesh管理的流量的入口和出口，进行周期性监控报告

测时流程：首先加载ebpf程序到内核，直接将这个ebpf程序挂载到cgroup上面，然后将本地ip加入到kmesh 管理的map中，然后建立tcp连接，在建立tcp连接时，tcp的握手就会有数据包的产生，因为进程挂载的是cgroup,所以所有的数据包都会被拦截，但只有被kmesh管理的IP的数据包才会触发监控，程序会先检查数据包是否来自kmesh管理的IP，要是时就触发监控并通过ring将监控数据上传，然后在测时端从ring buf中读取监控数据

cgroup_sock

在新建立连接时实现服务发现和负载均衡，主要实现了流量的拦截和重定向

测试流程：首先是加载ebpf程序到内核，直接将这个ebpf程序挂载到cgroup上，然后将本地ip加入到kmesh管理的map中，然后在测试测试端事先将本地ip加入到frontend_map的key,要是service设置了waypoint,那么就直接重定向目标到waypoint ip.port,要是没有，那么会根据service结构体里面的lb_policy进行选择，比如随机选择后端，本地优先必须取本地，本地优先失败后退回远程

**kmesh目前完取那可以代替sidecar吗？**

kmesh目前还不能完全代替sidecar,他目前确实是在追捕覆盖传统的树局面的核心能力，比如在l4层上的连接拦截，负载均衡，流量转发，监控等能力已经比较成熟，比如sockops,sendmsg,cgroup_skb,cgroup_sock这些ebpf程序就能完成连接路由，请求诸如，指标上报等功能。

但是在l7层上的复杂功能，比如http/grpc的内容级路由，请求重写，熔断，限流，认证全等，目前主要依赖用户太组件，比如说istio,因此我觉得他目前是处于发展阶段，可能后续完善了这些功能就有完全代替sidecar的潜力

**ebpf程序是怎样加载到内核的？**

首先就是使用clang把ebpf源码编译成.o文件，这个文件里包含了多个section,比如"sk_smg/sendmsg","cgroup_skb/ingress"等，然后我是在

你在编写ebpf程序的时候又遇到什么问题吗？

在进行测试的时候，因为他牵扯到ebpf索引不太好测试，我是通过将bpf_trace_printk()封装成一个多参数调用的宏，在ebpf程序中像刚开始找bug一样将他的内容打印出来进行检查是否正确

cgroup_sock：首先会进行判断他是不是kmesh管理的，然后会将本地ip和端口注册到frontend_k里面，通过lookup函数找frontend_k对应的frontend_v,这里面有目标ip他属于那个上游；找到之后他会注册service_k,然后也是通过lookup函数找到相应的service_v,service_v主要包含负载均衡策略，端口映射等，他会优先走service，先判断service的waypoint是不是为1,要是为1,说明该服务要求所有流量经由 waypoint，直接重定向到 waypoint。要是不是，就需要根据service的policy选择负载均衡策略，比如随机负载均衡，或者本地优先，必须匹配；或者本地优先，失败了才切远程，最后就直接转发到了目标地址。要是没有service_v,那么就将bankend_k设置成frontend_v,找到相应的backend_v,进行改写目标地址

## 1) 总体评价（面试可直接说）

> 你对 Kmesh 的定位、用 eBPF 替代用户态 sidecar 在 L4 层做拦截/重写/监控的思路是正确的；测试策略（C 端做 mock、Go 端跑真实网络）也很合理。文中个别表述需要用更准确的术语（比如 `sendmsg` hook、sk_msg、sockhash、map attach、verifier 限制、TLV 编码细节等），我下面逐项帮你把点说清楚并给建议。

------

## 2) 逐段校正与建议

### a. 用 eBPF 减少上下文切换 — 更准确的表述

你写得方向对，但面试时把流程用更精确的步骤讲会更好：

- **传统 sidecar 流程（有上下文切换）**：
  1. 应用进程发 `connect/send/sendmsg` → 进入内核网路栈。
  2. iptables/nftables 重定向把流量转到用户态代理（sidecar），内核需要做 NAT / redirect；
  3. 包从内核上升到用户态（context switch 到 sidecar 进程），sidecar 做服务发现/负载均衡/协议解析/修改；
  4. sidecar 再把结果写回内核，最终发出（又一次上下文切换）。
      => 这中间发生了多次内核<->用户态切换和拷贝，增加延迟和 CPU 消耗。
- **使用 eBPF 的流程（减少切换）**：
  1. 应用发 syscall 到内核网络栈；
  2. 在内核对应 hook（如 cgroup/connect、sk_msg/sendmsg、tc/cgroup_skb）上触发 eBPF 程序；
  3. eBPF 在内核上下文内做服务发现/负载均衡/包修改/监控（利用 BPF maps/ringbuf/sockhash 等与用户态协同）；
  4. 内核继续正常发送或重定向。
      => eBPF 在内核直接处理逻辑，少了来回上升到用户态的步骤，减少上下文切换和数据拷贝，从而降低延迟与 CPU 开销。

> 面试强调点：**不是“没有上下文切换”，而是大部分决策/改写都能在内核完成，避免了多次内核-用户态切换带来的开销。**

------

### b. sendmsg.c — 表述与测试流程优化

你写的测试流程思路对，但措辞可更严谨：

- 术语：`sendmsg` hook 在 eBPF 里常是 `sk_msg` 类型（`SEC("sk_msg")`）或 `sk_msg/sendmsg`，不是简单的 userspace `sendmsg`。TLV 是你在 `sk_msg` 把原始目标地址以 TLV 插入 payload 的实现细节，说明清楚你是 **在内核层的 sendmsg hook 上做数据前置**。
- 测试流程（建议用更清楚的步骤）：
  1. 编译并 load sendmsg eBPF 程序（.o）；
  2. 准备并获取 sockhash map 与 program FD；
  3. 把目标 socket fd 插入 sockhash（或通过 sockops 插入）；
  4. 触发真实连接并写数据，内核在 sk_msg hook 上运行 eBPF 程序并插入 TLV；
  5. 服务端读取并断言收到的数据头部 TLV 格式（type/len/ip/port/end tag）。
- 测试提示：在 unit test 时难点常是 `sk_msg_md` 构造以及 BPF helper 的 mock（bpf_msg_push_data 等）。你可以强调你用的 mock helper 列表和验证 map 写入/ ringbuf event 的方法（已有上下文提到）。

------

### c. cgroup_skb — 校正与关键信息

你描述的“拦截入口/出口、只对被 kmesh 管理 IP 触发、通过 ringbuf 上报”都正确，补几点：

- 挂载方式：`SEC("cgroup_skb/ingress")`、`SEC("cgroup_skb/egress")`。
- 典型判定逻辑：先检查 skb 的 family / skb->sk 是否有效，再通过 map lookup 确认该源/目的 IP 是被管理的 frontend，再写入 ringbuf 或更新统计 map。
- 测试要注意：cgroup 程序在不同 kernel 版本的行为可能不同（helper 可用性、skb 修修改限制等），并且 verifier 对栈/循环/内存访问有严格限制——在测试里要控制 helper 的使用和栈大小。

------

### d. cgroup_sock（连接重写/服务发现/LB）

你的逻辑链大致正确，建议更精确表述：

- **数据结构**：`frontend_map` (key: frontend_k -> frontend_v)，`service_map` (key: service_k -> service_v)，以及 `backend_map` 等；用 lookup 找到 service_v，读取 lb_policy、waypoint 配置。
- **重定向逻辑**：
  - 若 service_v.waypoint == 1 → rewrite dst to waypoint ip:port；
  - else → 根据 lb_policy（random / local-first / must-local / fallback）选择后端 IP 并 rewrite；
  - 若没有 service_v，则按默认 backend 逻辑处理。
- **实现细节**：在 connect 阶段，cgroup_sock 可以修改 `sk` 的目标（如通过 `bpf_setsockopt` / helper 或返回错误触发重连到新地址，实际实现细节因 helper 限制不同），面试时可说明你是如何实现“重写目标”的（比如用 `bpf_sock_addr` / `cgroup_skb` 改写或让 userspace 重定向）。

------

### e. 能否完全替代 Sidecar？

你结论很好，但可以把论据更有层次：

- **Kmesh 可以替代 Sidecar 的 L4 能力**：连接拦截、重定向、流量采集、LB、简单监控等都能在内核完成，且延迟低、成本低。你已经实现了许多 L4 能力，这点是事实。
- **目前不能完全替代 Sidecar 的原因**：
  - L7 特性（HTTP/GPRC 内容级路由、header 级路由、复杂协议解析、熔断/限流/认证/策略执行）实现难度高；
  - TLS 终止/证书管理在内核做起来不方便；
  - eBPF 的编程受限（stack/loops/available helpers），复杂业务逻辑更适合在用户态实现；
  - 运维/可观测/策略配置生态（如 Envoy 的丰富 filter 生态）是已成熟的优势。
- **结论**：Kmesh 在 L4 场景能显著替代 sidecar，若未来能在 L7 兼容更多功能或与轻量 sidecar 协同，会有完全替代的潜力。

------

### f. eBPF 程序如何加载到内核

你说得对但可补充流程和工具：

- 编译：`clang/llvm` + `-target bpf -O2 -g` 生成 `.o`（ELF）包含多个 `section`（`SEC("...")`）。
- 加载/attach：典型方式是用 `libbpf`（或 `bpftool`）通过 `BPF_PROG_LOAD` syscall 加载程序；再用 `bpf_prog_attach` / `bpf_map_update_elem` 等把 program/map attach 到 cgroup/tracepoint/sk_msg/sockhash 等。Kit：`bpftool prog load`、`bpftool map update`、Go 侧用 `cilium/ebpf` 或 `libbpfgo`。
- maps：通过 BPF map（sockhash、hash, array, ringbuf）与 userspace 通信。
- 注意 verifier：需要满足 verifier 的限制（最大栈、无动态内存、循环受限等）。

------

### g. 你遇到的测试与调试问题（建议）

你提到用 `bpf_trace_printk()` 打桩。提醒与建议：

- `bpf_trace_printk` 在新内核上被弱化（性能/字数限制），推荐用 **ringbuf / perf events** 或 `bpf_trace_printk` 仅做短期 debug。
- 单元测试 mock 的关键在于：**mock bpf helper**（例如 `bpf_msg_push_data`、`bpf_sk_storage_get`），以及构造正确的 `sk_msg_md`/`sk_buff`/`sock` 上下文。你提到的做法对，另外可以利用 `bpftool` 的 `prog run` + fakedev 或用用户空间的 BPF UT 框架（如 bpftrace/unit tests frameworks）模拟。
- 常见调试点：verifier 报错、栈溢出、helper 不可用、内存对齐、map key/value size mismatch。

------

## 3) 面试加分要点（你可以直接说）

- 明确术语：`sk_msg`、`sockhash`、`cgroup_skb/ingress`、`cgroup_sock/connect`、`BPF maps`、`ringbuf`、`libbpf`/`bpftool`。
- 在讲“减少上下文切换”时，强调：**减少内核<->用户态切换和数据拷贝**，并给出具体节省点（少一次 sendmsg 上升/下降、少一次 NAT、少一次用户态解析）。
- 提及**限制 & 兼容性**：eBPF 依赖 kernel 版本和 helper 集合；复杂逻辑会受 verifier 限制；调度与可观测需要 userspace 协同。
- 给出量化结论（如果有）：比如“压测结果单机 QPS 提升 X%，99p 延迟下降 Y ms”（若有，面试官会非常买账）。

------

## 4) 给你一段可直接替换到 README / 面试稿的**精炼版**（中文）

> Kmesh 是一个基于 eBPF 的 L4 服务网格数据面组件，目标是用内核态拦截和处理流量以降低 sidecar 带来的性能开销。我在项目中负责为关键内核态程序（`sendmsg`、`cgroup_skb`、`cgroup_sock`）搭建单元测试与集成测试框架：
>
> - `sendmsg`：在 `sk_msg` hook 上以 TLV 格式注入原始目标地址，测试流程包括加载 eBPF、把 socket 插入 sockhash、触发真实写入并校验 TLV。
> - `cgroup_skb`：挂载到 cgroup ingress/egress，针对被管理的 IP 做数据包监控并通过 ringbuf 上报，测试包括注入管理 map、建立连接并验证 ringbuf 事件。
> - `cgroup_sock`：在连接建立时做服务发现与负载均衡，支持 waypoint 转发与多种 LB 策略（随机、本地优先、必须本地/回退等），测试包括配置 frontend/service/backend map 并验证重写目标地址。
>    Kmesh 在 L4 场景能大幅降低上下文切换与数据拷贝，从而提升吞吐与延迟，但在 L7（复杂路由、协议解析、TLS 终止、认证等）依然需要用户态组件配合，因此目前更适合与轻量 sidecar 协同演进而不是完全替代。

基于zookeeper怎样实现分布式锁？









