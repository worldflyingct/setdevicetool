### 注意
通过udp数据包与之通信，发送端口为33234，别用熟悉的mqtt或是restful，简单点。  

### 擦除全部命令
`{"act":"erasechip"}`  
发送后会立刻收到一个wait，然后等待。  
收到success说明成功。  
收到fail说明失败。  
如果连wait都没收到，说明丢包了。  
如果收到了busy...，说明正在做其他事情。  
如果收到了syncfail，说明串口同步失败了。  
如果收到了syncok，说明串口同步成功了。  
如果收到了comerror，说明串口发生了错误。  
如果收到了timeout，说明设备响应串口命令失败了。  

### 擦除数据命令
`{"act":"erasedata"}`  
发送后会立刻收到一个wait，然后等待。  
收到success说明成功。  
收到fail说明失败。  
如果连wait都没收到，说明丢包了。  
如果收到了busy...，说明正在做其他事情。  
如果收到了syncfail，说明串口同步失败了。  
如果收到了syncok，说明串口同步成功了。  
如果收到了comerror，说明串口发生了错误。  
如果收到了timeout，说明设备响应串口命令失败了。  

### 写入命令
`{"act":"writechip","msu0path":"/home/jevian/Desktop/tk8610/tk8610_msu0-V1.4.135.hex","msu1path":"tk8610_app.hex","msu0load":true,"msu1load":true}`  
msu0path、msu1path、msu0load、msu1load为可选参数，不是一定要填，如果不填，默认使用页面上的设置。  
msu0path为0核img的路径，如果不设置，默认使用页面上的路径。  
msu1path为1核img的路径，如果不设置，默认使用页面上的路径。  
msu0load为0核img是否烧录，如果不设置，但msu0path设置了，会自动设置为true，否则使用页面上的设置。  
msu1load为1核img是否烧录，如果不设置，但msu1path设置了，会自动设置为true，否则使用页面上的设置。  
发送后会立刻收到一个wait，然后等待。  
收到success说明成功。  
收到fail说明失败。  
如果连wait都没收到，说明丢包了。  
如果收到了busy...，说明正在做其他事情。  
如果收到了syncfail，说明串口同步失败了。  
如果收到了syncok，说明串口同步成功了。  
如果收到了comerror，说明串口发生了错误。  
如果收到了timeout，说明设备响应串口命令失败了。  

### 设置msu0path
`{"act":"setmsu0path","msu0path":"/home/jevian/Desktop/tk8610/tk8610_msu0-V1.4.135.hex"}`  
修改页面上msu0path的设置。  
收到success说明成功。  
收到fail说明失败。  
如果收到了busy...，说明正在做其他事情。  

### 设置msu1path
`{"act":"setmsu1path","msu1path":"/home/jevian/Desktop/tk8610/tk8610_msu0-V1.4.135.hex"}`  
修改页面上msu1path的设置。  
收到success说明成功。  
收到fail说明失败。  
如果收到了busy...，说明正在做其他事情。  

### 设置msu0load
`{"act":"setmsu0load","msu0load":true}`  
修改页面上msu0load的设置。  
收到success说明成功。  
收到fail说明失败。  
如果收到了busy...，说明正在做其他事情。  

### 设置msu1load
`{"act":"setmsu1load","msu1load":true}`  
修改页面上msu1load的设置。  
收到success说明成功。  
收到fail说明失败。  
如果收到了busy...，说明正在做其他事情。  

### 停止正在运行的工作
`{"act":"stop"}`  
停止正在运行的任务，本功能为强行停止任何命令。  
收到success说明成功。  

### 清空页面上的log记录
`{"act":"clearlog"}`  
将页面的log记录清空，以免导致页面内容太多，内存不足。  
收到success说明成功。  
