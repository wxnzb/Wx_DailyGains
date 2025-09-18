package main

import "fmt"

func quickSort(arr []int, l, r int) {
	if l >= r {
		return
	}
	//选择基准
	pivot := arr[r]
	i := l
	//小于基准的移动到左边
	for j := l; j < r; j++ {
		if arr[j] < pivot {
			arr[i], arr[j] = arr[j], arr[i]
			i++
		}
	}
	arr[i], arr[r] = arr[r], arr[i]
	quickSort(arr, l, i-1)
	quickSort(arr, i+1, r)
}
func main() {
	arr := []int{3, 6, 8, 3, 2, 7, 3}
	fmt.Println("排序前：", arr)
	quickSort(arr, 0, len(arr)-1)
	fmt.Println("排序后：", arr)
}
