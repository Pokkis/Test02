# Test02
测试组播

目前想实现一个，组播客户端重复读取一个h264文件，并将之发送，服务端不定时接入组播网络，并把接收到的数据保存下来。

使用vlc拉取rtp流关键：
需要创建一个sdp文件，添加以下内容：
m=video 5454 RTP/AVP 96
a=rtpmap:96 H264
a=framerate:25
c=IN IP4 127.0.0.1


###################################################################################################
分割线以下主要用于记录任务目标以及完成率

2021/7/25 目标：写一篇有关队列的总结

