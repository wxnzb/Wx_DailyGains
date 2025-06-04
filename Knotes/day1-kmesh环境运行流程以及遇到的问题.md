##  kmesh环境运行流程以及遇到的问题
### 首先是在根目录下有build.sh
- sudo ./build.sh 构建项目
- sudo ./build.sh -i 安装项目
### 在/test/bpf_ut/bpftest下面有makefile这里是单元测试特别关注
- 不能直接 make  run
- 1.可能会出现bpftest/trf.pb.go:26:2: google.golang.org/protobuf@v1.36.3: Get "https://proxy.golang.org/google.golang.org/protobuf/@v/v1.36.3.zip": dial tcp 142.251.33.81:443: i/o timeout这个网络问题
- 解决方法(使用国内代理)
```
sudo env \
   GOPROXY=https://goproxy.cn,direct \
   GOSUMDB=off \
```
- 2.可能会出现找不到go在sudo中:
-  解决方法：
```
sudo PATH=$PATH:/usr/local/go/bin make run
```
-  现在将1.2总结在一起的形式
```
  sudo env \
  GOPROXY=https://goproxy.cn,direct \
  GOSUMDB=off \
  PATH=$PATH:/usr/local/go/bin \
   make run
```
-  3.上面哪个运行可能会出现:No package 'api-v2-c' found
- 解决方法
```
 #在kmesh根目录下
 #首先找他在哪
 运行find . -type f -name "api-v2-c.pc"
 ./mk/api-v2-c.pc
 # 设置 pkg-config 搜索路径
 运行export PKG_CONFIG_PATH=/home/sweet/git/kmesh/mk:$PKG_CONFIG_PATH
 # 验证
 运行pkg-config --cflags api-v2-c
```
- 最终结果
```
  sudo env \
  PKG_CONFIG_PATH=/home/sweet/git/kmesh/mk:$PKG_CONFIG_PATH \
  GOPROXY=https://goproxy.cn,direct \
  GOSUMDB=off \
  PATH=$PATH:/usr/local/go/bin \
  make run
```
-  4.可能出现: libkmesh_api_v2_c.so: cannot open shared object file: No such file or directory(运行时找不到动态库 .so)
- 解决方法：
```
.so 文件在 /usr/lib64/ 下，但 sudo 环境中找不到。
解决办法：传入 LD_LIBRARY_PATH
```

```
  sudo env \
  PKG_CONFIG_PATH=/home/sweet/git/kmesh/mk:$PKG_CONFIG_PATH \
  GOPROXY=https://goproxy.cn,direct \
  GOSUMDB=off \
  PATH=$PATH:/usr/local/go/bin \
  LD_LIBRARY_PATH=/usr/lib64:$LD_LIBRARY_PATH \
  make run
  ...................................
  #运行结果
  go test ./bpftest -bpf-ut-path /home/sweet/git/kmesh/test/bpf_ut 
  ok      kmesh.net/kmesh/test/bpf_ut/bpftest     15.233s
```
- 下面这个应该是上面那个的详细输出细节
```
  sudo env \
  PKG_CONFIG_PATH=/home/sweet/git/kmesh/mk:$PKG_CONFIG_PATH \
  GOPROXY=https://goproxy.cn,direct \
  GOSUMDB=off \
  PATH=$PATH:/usr/local/go/bin \
  LD_LIBRARY_PATH=/usr/lib64:$LD_LIBRARY_PATH \
  go test ./bpftest -bpf-ut-path /home/sweet/git/kmesh/test/bpf_ut -       test.v
```
