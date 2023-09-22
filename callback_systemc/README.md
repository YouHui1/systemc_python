## 方法
1. 在cpp端提供python回调函数的指针，在python端初始化时使用该指针指向python的回调函数
2. systemc的非阻塞传输过程分多个状态，在相应状态使用回调函数指针即可

## 运行结果
* 说明：标黄色字体的为对应python的回调函数运行空间

![pic](image.png)
