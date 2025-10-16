# Reactor
基于C++11实现的**多reactor多线程**TCP网络框架

# 运行步骤:
```
sh build.sh

cd bin

./echoserver yourIP yourPort
./client yourIP
```

# 压力测试
```
./echoserver yourIP yourPort

// test.sh默认使用8080端口，根据自身情况更改
sh test.sh
```
