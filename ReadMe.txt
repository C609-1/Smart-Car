应用程序编程：

对于读取esp8266，单独使用一个子进程来接受和处理，read为阻塞性，可用轮询方式

对于发程序，按8个字节一个数据包来发送程序，每一个包要有一定的时间间隔


注：
缺陷：（这些都是在应用程序中来完成的事情）
对于读数据，我们只能8个字节，8个字节的来读
对于写数据，如果接受端也使用esp8266，那么在应用程序中也需要8个字节，8个字节的来写


编程示例见：
esp_send.c
esp_recv.c