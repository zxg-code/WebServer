### http
#### http_request
- Issues
    - ParstPost函数中将最外层的if移动到了它的调用者：ParseBody()中，后续使用时需要注意先判断一下
    - 预先定义html页面和tag是干嘛的？

- 对原版的分析
    ```cpp
    bool HttpRequest::UserVerify(const string &name, const string &pwd, bool isLogin) {
        if(name == "" || pwd == "") { return false; }
        LOG_INFO("Verify name:%s pwd:%s", name.c_str(), pwd.c_str());
        MYSQL* sql;
        SqlConnRAII(&sql,  SqlConnPool::Instance());
        assert(sql);
        
        bool flag = false;  // 返回值
        unsigned int j = 0;
        char order[256] = { 0 };
        MYSQL_FIELD *fields = nullptr;
        MYSQL_RES *res = nullptr;
        
        if(!isLogin) { flag = true; }  // 可以看到，返回值和isLogin标志正好相反
        /* 查询用户及密码 */
        snprintf(order, 256, "SELECT username, password FROM user WHERE username='%s' LIMIT 1", name.c_str());
        LOG_DEBUG("%s", order);

        if(mysql_query(sql, order)) { 
            mysql_free_result(res);
            return false; 
        }
        res = mysql_store_result(sql);
        j = mysql_num_fields(res);
        fields = mysql_fetch_fields(res);

        while(MYSQL_ROW row = mysql_fetch_row(res)) {
            LOG_DEBUG("MYSQL ROW: %s %s", row[0], row[1]);
            string password(row[1]);
            /* 注册行为 且 用户名未被使用*/  // 这里注释有误，应该是登录行为
            if(isLogin) {
                if(pwd == password) { flag = true; }
                else {
                    // 不需要设置，因为如果islogin为真，flag一开始已经为假了
                    // 如果islogin为假，无法进入这一段代码
                    flag = false;  
                    LOG_DEBUG("pwd error!");  
                }
            } 
            else {   // islogin为假，这不就是下面注册的一段？
                flag = false;  // 这样做相当于根据islogin判断用户名是否被使用
                LOG_DEBUG("user used!");
            }
        }
        mysql_free_result(res);

        /* 注册行为 且 用户名未被使用*/
        // 经过上段，如果islogin为真，并且密码正确，那么flag为真
        // islogin为真则无法进入这段代码
        // 如果islogin为假，那么不会进入上一段代码，flag保持其初值为真
        // 则可以进入这段代码，因此上一段代码判断用户名被占用的代码逻辑有问题
        if(!isLogin && flag == true) {
            LOG_DEBUG("regirster!");
            bzero(order, 256);
            snprintf(order, 256,"INSERT INTO user(username, password) VALUES('%s','%s')", name.c_str(), pwd.c_str());
            LOG_DEBUG( "%s", order);
            if(mysql_query(sql, order)) { 
                LOG_DEBUG( "Insert error!");
                flag = false; 
            }
            flag = true;
        }
        SqlConnPool::Instance()->FreeConn(sql);
        LOG_DEBUG( "UserVerify success!!");
        return flag;
    }
    ```


- TODO
    - log功能