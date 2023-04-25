### 缓冲池
#### Issues
- std::string RetrieveAllToStr()的返回值是cstring头文件中的，能否使用string头文件呢？
- WriteFd函数并未用到
- 注意缓冲池读的地址和写地址，跟偏移量的区别，大部分函数中操作的都是偏移量
- 保证可写函数有点问题，已改