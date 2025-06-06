# bpftrace一行教程

该教程通过12个简单小节帮助你了解bpftrace的使用。每一小节都是一行的命令，你可以尝试运行并立刻看到运行效果。该教程系列用来介绍bpftrace的概念。关于bpftrace的完整参考，见[bpftrace手册](../man/adoc/bpftrace.adoc)。

该教程贡献者是Brendan Gregg, Netflix (2018), 基于他的FreeBSD DTrace教程系列[DTrace Tutorial](https://wiki.freebsd.org/DTrace/Tutorial)。

# 1. 列出所有探针

```
bpftrace -l 'tracepoint:syscalls:sys_enter_*'
```

"bpftrace -l" 列出所有探针，并且可以添加搜索项。

- 探针是用于捕获事件数据的检测点。
- 搜索词支持通配符，如`*`和`?`。
- "bpftrace -l" 也可以通过管道传递给grep，进行完整的正则表达式搜索。

# 2. Hello World

```
# bpftrace -e 'BEGIN { printf("hello world\n"); }'
Attached 1 probe
hello world
^C
```

打印欢迎消息。运行后, 按Ctrl-C结束。

- `BEGIN`是一个特殊的探针，在程序开始时触发探针执行(类似awk的BEGIN)。你可以使用它设置变量和打印消息头。
- 探针可以关联动作，把动作放到{}中。这个例子中，探针被触发时会调用printf()。

# 3. 文件打开

```
# bpftrace -e 'tracepoint:syscalls:sys_enter_openat { printf("%s %s\n", comm, str(args.filename)); }'
Attached 1 probe
snmp-pass /proc/cpuinfo
snmp-pass /proc/stat
snmpd /proc/net/dev
snmpd /proc/net/if_inet6
^C
```

这里我们在文件打开的时候打印进程名和文件名。

- 该命令以`tracepoint:syscalls:sys_enter_openat`开始: 这是tracepoint探针类型(内核静态跟踪)，当进入`openat()`系统调用时执行该探针。相比kprobes探针(内核动态跟踪，在第6节介绍)，我们更加喜欢用tracepoints探针，因为tracepoints有稳定的应用程序编程接口。注意：现代linux系统(glibc >= 2.26)，`open`总是调用`openat`系统调用。
- `comm`是内建变量，代表当前进程的名字。其它类似的变量还有pid和tid，分别表示进程标识和线程标识。
- `args`是一个包含所有tracepoint参数的结构。这个结构是由bpftrace根据tracepoint信息自动生成的。这个结构的成员可以通过命令`bpftrace -vl tracepoint:syscalls:sys_enter_openat`找到。
- `args.filename`用来获取args的成员变量`filename`的值。
- `str()`用来把字符串指针转换成字符串。

# 4. 进程级系统调用计数

```
bpftrace -e 'tracepoint:raw_syscalls:sys_enter { @[comm] = count(); }'
Attached 1 probe
^C

@[bpftrace]: 6
@[systemd]: 24
@[snmp-pass]: 96
@[sshd]: 125
```

按Ctrl-C后打印进程的系统调用计数。

- @: 表示一种特殊的变量类型，称为map，可以以不同的方式来存储和描述数据。你可以在@后添加可选的变量名(如@num)，用来增加可读性或者区分不同的map。
- []: 可选的中括号允许设置map的关键字，比较像关联数组。
- count(): 这是一个map函数 - 记录被调用次数。因为调用次数根据comm保存在map里，输出结果是进程执行系统调用的次数统计。

Maps会在bpftrace结束(如按Ctrl-C)时自动打印出来。

# 5. read()返回值分布统计

```
# bpftrace -e 'tracepoint:syscalls:sys_exit_read /pid == 18644/ { @bytes = hist(args.ret); }'
Attached 1 probe
^C

@bytes:
[0, 1]                12 |@@@@@@@@@@@@@@@@@@@@                                |
[2, 4)                18 |@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@                     |
[4, 8)                 0 |                                                    |
[8, 16)                0 |                                                    |
[16, 32)               0 |                                                    |
[32, 64)              30 |@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@|
[64, 128)             19 |@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@                    |
[128, 256)             1 |@
```

这里统计进程号为18644的进程执行内核函数sys_read()的返回值，并打印出直方图。
- /.../: 这里设置一个过滤条件(条件判断)，满足该过滤条件时才执行{}里面的动作。在这个例子中意思是只追踪进程号为18644的进程。过滤条件表达式也支持布尔运算，如("&&", "||")。
- ret: 表示函数的返回值。对于sys_read()，它可能是-1(错误)或者成功读取的字节数。
- @: 类似于上节的map，但是这里没有key，即[]。该map的名称"bytes"会出现在输出中。
- hist(): 一个map函数，用来描述直方图的参数。输出行以2次方的间隔开始，如`[128, 256)`表示值大于等于128且小于256。后面跟着位于该区间的参数个数统计，最后是ascii码表示的直方图。该图可以用来研究它的模式分布。
- 其它的map函数还有lhist(线性直方图)，count()，sum()，avg()，min()和max()。

# 6. 内核动态跟踪read()返回的字节数

```
# bpftrace -e 'kretprobe:vfs_read { @bytes = lhist(retval, 0, 2000, 200); }'
Attached 1 probe
^C

@bytes:
(...,0]                0 |                                                    |
[0, 200)              66 |@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@|
[200, 400)             2 |@                                                   |
[400, 600)             3 |@@                                                  |
[600, 800)             0 |                                                    |
[800, 1000)            5 |@@@                                                 |
[1000, 1200)           0 |                                                    |
[1200, 1400)           0 |                                                    |
[1400, 1600)           0 |                                                    |
[1600, 1800)           0 |                                                    |
[1800, 2000)           0 |                                                    |
[2000,...)            39 |@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@                      |
```

使用内核动态跟踪技术显示read()返回字节数的直方图。

- `kretprobe:vfs_read`: 这是kretprobe类型(动态跟踪内核函数返回值)的探针，跟踪`vfs_read`内核函数。此外还有kprobe类型的探针(在下一节介绍)用于跟踪内核函数的调用。它们是功能强大的探针类型，让我们可以跟踪成千上万的内核函数。然而它们是"不稳定"的探针类型:由于它们可以跟踪任意内核函数，对于不同的内核版本，kprobe和kretprobe不一定能够正常工作。因为内核函数名，参数，返回值和作用等可能会变化。此外，由于它们用来跟踪底层内核的，你需要浏览内核源代码，理解这些探针的参数和返回值的意义。
- lhist(): 线性直方图函数:参数分别是value，最小值，最大值，步进值。第一个参数(`retval`)表示系统调用sys_read()返回值:即成功读取的字节数。

# 7. read()调用的时间

```
# bpftrace -e 'kprobe:vfs_read { @start[tid] = nsecs; } kretprobe:vfs_read /@start[tid]/ { @ns[comm] = hist(nsecs - @start[tid]); delete(@start, tid); }'
Attached 2 probes

[...]
@ns[snmp-pass]:
[0, 1]                 0 |                                                    |
[2, 4)                 0 |                                                    |
[4, 8)                 0 |                                                    |
[8, 16)                0 |                                                    |
[16, 32)               0 |                                                    |
[32, 64)               0 |                                                    |
[64, 128)              0 |                                                    |
[128, 256)             0 |                                                    |
[256, 512)            27 |@@@@@@@@@                                           |
[512, 1k)            125 |@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@       |
[1k, 2k)              22 |@@@@@@@                                             |
[2k, 4k)               1 |                                                    |
[4k, 8k)              10 |@@@                                                 |
[8k, 16k)              1 |                                                    |
[16k, 32k)             3 |@                                                   |
[32k, 64k)           144 |@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@|
[64k, 128k)            7 |@@                                                  |
[128k, 256k)          28 |@@@@@@@@@@                                          |
[256k, 512k)           2 |                                                    |
[512k, 1M)             3 |@                                                   |
[1M, 2M)               1 |                                                    |
```

根据进程名，以直方图的形式显示read()调用花费的时间，时间单位为纳秒。

- @start[tid]: 使用线程ID作为key。某一时刻，可能有许许多多的read调用正在进行，我们希望为每个调用记录一个起始时间戳。这要如何做到呢？我们可以为每个read调用建立一个唯一的标识符，并用它作为key进行统计。由于内核线程一次只能执行一个系统调用，我们可以使用线程ID作为上述标识符。
- nsecs: 自系统启动到现在的纳秒数。这是一个高精度时间戳，可以用来对事件计时。
- /@start[tid]/: 该过滤条件检查起始时间戳是否被记录。程序可能在某次read调用中途被启动，如果没有这个过滤条件，这个调用的时间会被统计为now-zero，而不是now-start。
- delete(@start, tid): 释放变量。

# 8. 统计进程级别的事件

```
# bpftrace -e 'tracepoint:sched:sched* { @[probe] = count(); } interval:s:5 { exit(); }'
Attached 25 probes
@[tracepoint:sched:sched_wakeup_new]: 1
@[tracepoint:sched:sched_process_fork]: 1
@[tracepoint:sched:sched_process_exec]: 1
@[tracepoint:sched:sched_process_exit]: 1
@[tracepoint:sched:sched_process_free]: 2
@[tracepoint:sched:sched_process_wait]: 7
@[tracepoint:sched:sched_wake_idle_without_ipi]: 53
@[tracepoint:sched:sched_stat_runtime]: 212
@[tracepoint:sched:sched_wakeup]: 253
@[tracepoint:sched:sched_waking]: 253
@[tracepoint:sched:sched_switch]: 510
```

这里统计5秒内进程级的事件并打印。

- sched: `sched`探针可以探测调度器的高级事件和进程事件如fork, exec和上下文切换。
- probe: 探针的完整名称。
- interval:s:5: 这是一个每5秒在每个CPU上触发一次的探针，它用来创建脚本级别的间隔或超时时间。
- exit(): 退出bpftrace。

# 9. 分析内核实时函数栈

```
# bpftrace -e 'profile:hz:99 { @[kstack] = count(); }'
Attached 1 probe
^C

[...]
@[
filemap_map_pages+181
__handle_mm_fault+2905
handle_mm_fault+250
__do_page_fault+599
async_page_fault+69
]: 12
[...]
@[
cpuidle_enter_state+164
do_idle+390
cpu_startup_entry+111
start_secondary+423
secondary_startup_64+165
]: 22122
```

以99赫兹的频率分析内核调用栈并打印次数统计。

- profile:hz:99: 这里所有cpu都以99赫兹的频率采样分析内核栈。为什么是99而不是100或者1000？我们想要抓取足够详细的内核执行时内核栈信息，但是频率太大影响性能。100赫兹足够了，但是我们不想用正好100赫兹，这样采样频率可能与其他定时事件步调一致，所以99赫兹是一个理想的选择。
- kstack: 返回内核调用栈。这里作为map的关键字，可以跟踪次数。这些输出信息可以使用火焰图可视化。此外`ustack`用来分析用户级堆栈。

# 10. 调度器跟踪

```
# bpftrace -e 'tracepoint:sched:sched_switch { @[kstack] = count(); }'
^C
[...]

@[
__schedule+697
__schedule+697
schedule+50
schedule_timeout+365
xfsaild+274
kthread+248
ret_from_fork+53
]: 73
@[
__schedule+697
__schedule+697
schedule_idle+40
do_idle+356
cpu_startup_entry+111
start_secondary+423
secondary_startup_64+165
]: 305
```

这里统计进程上下文切换次数。以上输出被截断，只输出了最后两个结果。

- sched: 跟踪调度类别的调度器事件:sched_switch, sched_wakeup, sched_migrate_task等。
- sched_switch: 当线程释放cpu资源，当前不运行时触发。这里可能的阻塞事件:如等待I/O，定时器，分页/交换，锁等。
- kstack: 内核堆栈跟踪，打印调用栈。
- sched_switch在线程切换的时候触发，打印的调用栈是被切换出cpu的那个线程。像你使用其他探针一样，注意这里的上下文，例如comm, pid, kstack等等，并不一定反映了探针的目标的状态。

# 11. 块级I/O跟踪

```
# bpftrace -e 'tracepoint:block:block_rq_issue { @ = hist(args.bytes); }'
Attached 1 probe
^C

@:
[0, 1]                 1 |@@                                                  |
[2, 4)                 0 |                                                    |
[4, 8)                 0 |                                                    |
[8, 16)                0 |                                                    |
[16, 32)               0 |                                                    |
[32, 64)               0 |                                                    |
[64, 128)              0 |                                                    |
[128, 256)             0 |                                                    |
[256, 512)             0 |                                                    |
[512, 1K)              0 |                                                    |
[1K, 2K)               0 |                                                    |
[2K, 4K)               0 |                                                    |
[4K, 8K)              24 |@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@|
[8K, 16K)              2 |@@@@                                                |
[16K, 32K)             6 |@@@@@@@@@@@@@                                       |
[32K, 64K)             5 |@@@@@@@@@@                                          |
[64K, 128K)            0 |                                                    |
[128K, 256K)           1 |@@                                                  |

```

以上是块I/O请求字节数的直方图。

- tracepoint:block: 块类别的跟踪点跟踪块级I/O事件。
- block_rq_issue: 当I/O提交到块设备时触发。
- args.bytes: 跟踪点block_rq_issue的参数成员bytes，表示提交I/O请求的字节数。

该探针的上下文是非常重要的: 它在I/O请求被提交给块设备时触发。这通常发生在进程上下文，此时通过内核的comm可以得到进程名；也可能发生在内核上下文，(如readahead)，此时不能显示预期的进程号和进程名信息。

# 12. 内核结构跟踪

```
# cat path.bt
#ifndef BPFTRACE_HAVE_BTF
#include <linux/path.h>
#include <linux/dcache.h>
#endif

kprobe:vfs_open
{
	printf("open path: %s\n", str(((struct path *)arg0)->dentry->d_name.name));
}

# bpftrace path.bt
Attached 1 probe
open path: dev
open path: if_inet6
open path: retrans_time_ms
[...]
```


这里使用内核动态跟踪技术跟踪vfs_read()函数，该函数的(struct path *)作为第一个参数。

- kprobe: 如前面所述，这是内核动态跟踪kprobe探针类型，跟踪内核函数的调用(kretprobe探针类型跟踪内核函数返回值)。
- `arg0` 是一个内建变量，表示探针的第一个参数，其含义由探针类型决定。对于`kprobe`类型探针，它表示函数的第一个参数。其它参数使用arg1,...,argN访问。
- `((struct path *)arg0)->dentry->d_name.name`: 这里`arg0`作为`struct path *`并引用dentry。
- #include: 在没有BTF (BPF Type Format) 的情况下,包含必要的path和dentry类型声明的头文件。

bpftrace对内核结构跟踪的支持和bcc是一样的，允许使用内核头文件。这意味着大多数结构是可用的，但是并不是所有的，有时需要手动增加某些结构的声明。例如这个例子，见[dcsnoop tool](../tools/dcsnoop.bt)，包含struct nameidata的声明。倘若内核有提供BTF数据，则所有结构都可用。

现在，你已经理解了bpftrace的大部分功能，你可以开始使用和编写强大的一行命令。查阅[使用说明书](../man/adoc/bpftrace.adoc)更多的功能。
