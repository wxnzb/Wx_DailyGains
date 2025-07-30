package main

import (
	"fmt"
)

// go并发携程+Channel同步通信
func main() {
	numCh := make(chan struct{})
	letterCh := make(chan struct{})
	//done := make(chan struct{})
	numbers := []rune{'1', '2', '3'}
	letters := []rune{'a', 'b', 'c'}
	go func() {
		i := 0
		for {
			<-numCh
			fmt.Printf("%c", numbers[i])
			i = (i + 1) % len(numbers)
			letterCh <- struct{}{}
		}
	}()
	go func() {
		i := 0
		for {
			<-letterCh
			fmt.Printf("%c", letters[i])
			i = (i + 1) % len(numbers)
			numCh <- struct{}{}
		}
	}()
	numCh <- struct{}{}
	// 阻塞主线程，避免退出，非常关键
	select {}
}

//而当你写成 select {}（没有 case 分支）时，它就永远在等待，没有任何 channel 可用，也不会执行任何 case —— 于是：它就永远阻塞在这儿，不会退出
//当主协程（main 函数）结束时，程序就退出了，所以即使你开启了多个 goroutine（子协程），如果 main() 很快返回，那些子协程也会被直接杀死，不会继续执行
