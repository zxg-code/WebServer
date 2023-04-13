### http
#### http_request
- Issues
    - request中，ParstPost函数中将最外层的if移动到了它的调用者：ParseBody()中，后续使用时需要注意先判断一下
    - 原作者的代码的很多函数中，对哈希表查找了两次，一次count，一次find，第一次count找到之后就可以利用at取值了吧
    - request中的Parse函数，search无法重载到BeginWrite的const形式，不知道原因
    - response中，UnmapFile()函数修改为：使用delete释放内存

- TODO
    - log功能
    - 在response实现中，有函数例如AddHeader，多次调用了缓冲池的写入函数，我认为应该组建好字符串之后一次性写入
    - response中，析构函数只调用了UnmapFile()，应该不需要这个函数，有析构函数去做

#### http_response
- Issues

- TODO


#### http_connect
- Issues
    - iov_cnt没有初始化