### http
#### http_request
- Issues
    - request中，ParstPost函数中将最外层的if移动到了它的调用者：ParseBody()中，后续使用时需要注意先判断一下
    - 原作者的代码的很多函数中，对哈希表查找了两次，一次count，一次find，第一次count找到之后就可以利用at取值了吧
    - request中的Parse函数，search无法重载到BeginWrite的const形式，不知道原因
    - 修改了ConverHex函数
    - get_path的const重载似乎并无必要，成员变量不是指针，返回的也不是指针或引用

- Fix
    - Parse函数中第50行，break前需要先移动读偏移量，不然会有一条RequestLine Error记录

- TODO
    - 在response实现中，有函数例如AddHeader，多次调用了缓冲池的写入函数，我认为应该组建好字符串之后一次性写入
    - response中，析构函数只调用了UnmapFile()，应该不需要这个函数，有析构函数去做

#### http_response
- Issues
    - 根据code判断status的逻辑精简了一下
    - mm_file_stat的命名有些不贴切，它实际表示意义在MakeResponse函数中可以看出，

- TODO


#### http_connect
- Issues
    - 静态成员变量在何时初始化：在webserver.cpp文件中
    - 缓冲池成员用指针就出错了

- TODO
    - 