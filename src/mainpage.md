# 介绍
Salt是一个基于asio的网络库。<br/>
目前已经完成tcp协议的server和client开发。<br/>
项目主页: [github](https://github.com/Tyzual/salt)

# 示例
example目录有Salt库的示例。<br/>
echo目录实现了echo_server和echo_client，你可以从这个示例中了解Salt的使用方法。<br/>
message目录演示了如何使用Salt内置的拆包器来实现基于消息头和消息体的拆包功能，你可以使用message client/message unify client配合echo server使用。<br/>