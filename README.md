ThreadPool
==========

A simple C++20 Thread Pool implementation.

基本用法:
```c++
// 创建一个4个工作线程的线程池
ThreadPool pool(4);

// 保存返回结果，std::future<int>
auto result = pool.enqueue([](int answer) { return answer; }, 42);

// 等待结果并输出
std::cout << result.get() << std::endl;

```
```c++
// 创建一个4个工作线程的线程池
ThreadPool pool(4);
// 创建一个future的vector批量保存结构
std::vector<std::future<int>> results;

for(int i = 0; i < 10; i++){
    // 保存返回结果，std::future<int>
    results.emplace_back(pool.enqueue([](int answer) { return answer*answer; }, i));
}

//输出结果
for(auto& res : results){
    std::cout << res.get() << std::endl;
}
results.clear();

```

