

- 查找有利因素
- bpf_sock_hash_update
- bpf_msg_redirect_hash
- sock_ops_map----/home/sweet/git/kmesh/oncn-mda/docs/design.md可以认真读一下
- `bpf_map_update_elem` 确实是 eBPF 程序和用户态或内核态交互的关键函数之一。
- 第一个区别
- sec(sockops)他是挂载到cgroup上面的
- 但是sec(sk_msg)他必须是挂载到一个sockfd上面的，两者挂载方式不同
### sendmsg尝试
问题一：
```
=== RUN   TestBPF
=== RUN   TestBPF/Workload
=== RUN   TestBPF/Workload/SendMsg
=== RUN   TestBPF/Workload/SendMsg/workload_sendmsg_test.o
=== RUN   TestBPF/Workload/SendMsg/workload_sendmsg_test.o/hahhahahhahahhhahhahhahahahhahhahaha
    bpf_test.go:216: Skipping program 'sendmsg_prog' of type 'SkMsg': BPF_PROG_RUN not supported
2025-07-11T03:12:34.700779Z     info    program 'sendmsg_prog' not found
FAIL    kmesh.net/kmesh/test/bpf_ut/bpftest     0.045s
FAIL
```
```
原因：delete了
func loadAndPrepSpec(t *testing.T, elfPath string) *ebpf.CollectionSpec {
	spec, err := ebpf.LoadCollectionSpec(elfPath)
	if err != nil {
		t.Fatalf("load spec %s: %v", elfPath, err)
	}
	// Unpin all maps, as we don't want to interfere with other tests
	for _, m := range spec.Maps {
		m.Pinning = ebpf.PinNone
	}

	for n, p := range spec.Programs {
		t.Log(p.Type)
		switch p.Type {
		// https://docs.ebpf.io/linux/syscall/BPF_PROG_TEST_RUN/
		case ebpf.XDP, ebpf.SchedACT, ebpf.SchedCLS, ebpf.SocketFilter, ebpf.CGroupSKB, ebpf.SockOps, ebpf.SkMsg:
			continue
		}

		t.Logf("Skipping program '%s' of type '%s': BPF_PROG_RUN not supported", p.Name, p.Type)
		delete(spec.Programs, n)
	}

	return spec
}

```

- 想法：
```
struct {
    __uint(type, BPF_MAP_TYPE_SOCKHASH);
    __type(key, struct sock_key);
    __type(value, int);
    __uint(max_entries, SKOPS_MAP_SIZE);
    __uint(map_flags, 0);
} SOCK_OPS_MAP_NAME SEC(".maps");
```

