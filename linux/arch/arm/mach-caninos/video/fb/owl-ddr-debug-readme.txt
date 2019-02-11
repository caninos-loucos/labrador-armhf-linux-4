\D4\DAdebug fs \B4򿪵\C4\C7\E9\BF\F6\CF£\AC\D4\DAϵͳ\CF\C2\C3\E6\D3и\F6Ŀ¼ dmm
/sys/kernel/debug/dmm/
  	enable
	master0
	   id 
	   mode
	master1
	   id 
	   mode
	max_statistic_cnt
	result
	sampling_rate
	statistic_cnt
	
enable: \BF\AAʼ\BB\F2\D5\DF\CA\C7ֹͣͳ\BC\C6 
      \BF\AAʼͳ\BC\C6 echo 1 > enable 
      ֹͣͳ\BC\C6 echo 0 > enable 
      
master0 \BA\CD master1 \B7ֱ\F0\CA\C7\D0\E8Ҫͳ\BCƵ\C4\C1\BD\B8\F6master \D0\C5Ϣ

id \A3\BA\D4\DAѡ\D4\F1ID \B5\C4ʱ\BA\F2 \D0\E8Ҫ\B8\F9\BEݴ\CBֵ\BD\F8\D0\D0\C9\E8\D6\C3
	MASTER_ID_CPU = 0,
	MASTER_ID_USB3,
	MASTER_ID_VCE,
	MASTER_ID_ENTHERNET,
	MASTER_ID_USB2,
	MASTER_ID_DE,
	MASTER_ID_GPU3D,
	MASTER_ID_SI,
	MASTER_ID_DMA,
	MASTER_ID_DAP,
	MASTER_ID_ALL,
	MASTER_ID_IDLE,
	
mode \A3\BA\D4\DAѡ\D4\F1MODE  \B5\C4ʱ\BA\F2 \D0\E8Ҫ\B8\F9\BEݴ\CBֵ\BD\F8\D0\D0\C9\E8\D6\C3
	MASTER_MODE_READ = 0,   // ͳ\BCƶ\C1
	MASTER_MODE_WRITE, //ͳ\BC\C6д
	MASTER_MODE_ALL, //ͳ\BCƶ\C1д

\BF\C9\D2\D4ͨ\B9\FD\C9\E8\D6\C3\C0\B4\D0޸\C4\D0\E8Ҫͳ\BCƵ\C4master \BA\CDͳ\BCƵ\C4ģʽ

max_statistic_cnt\A3\BA \D7\EE\B4\F3\C4ܹ\BBͳ\BCƵĴ\CE\CA\FD\A3\ACֻҪ\CA\C7\C4ڴ\E6\CF\DE\D6ơ\A3Ĭ\C8\CF\CA\C71000\B4Σ\AC\BF\C9\D2\D4ͨ\B9\FDecho 2000 > max_statistic_cnt \C0\B4\D0޸\C4

sampling_rate\A3\BA\B2\C9\D1\F9\B5ļ\E4\B8\F4ʱ\BC䣬 Ĭ\C8\CF\CA\C71s ͳ\BC\C6һ\B4Σ\ACҲ\BF\C9\D2\D4ͨ\B9\FDecho 2000 > sampling_rate \D0޸ġ\A3

statistic_cnt\A3\BA\B5\B1ǰͳ\BC\C6\C1˶\E0\C9ٴΣ\ACֻ\B6\C1\A1\A3ͨ\B9\FDcat statistic_cnt \B6\C1ȡ

result:ͳ\BCƵĽ\E1\B9\FB\A3\ACֻ\B6\C1 ͨ\B9\FDcat result \BB\F1ȡ
\BD\E1\B9\FB˵\C3\F7\A3\BA
       master id:   mode   bandwidth(M byte)     percent of total(%)        master id:   mode   bandwidth(M byte)    percent of total(%)
             ALL:    RW               400                         13            IDLE:    RW               433                         85 
             ALL:    RW               612                         21            IDLE:    RW              1178                         60 
             ALL:    RW               714                         24            IDLE:    RW               989                         66 
             ALL:    RW               616                         21            IDLE:    RW              1165                         60 
             ALL:    RW               635                         22            IDLE:    RW              1128                         61 
             ALL:    RW               667                         23            IDLE:    RW              1067                         63 
             ALL:    RW               625                         21            IDLE:    RW              1145                         61 
             ALL:    RW               655                         22            IDLE:    RW              1096                         62 
             ALL:    RW               649                         22            IDLE:    RW              1110                         62 
master id : ָͳ\BCƵ\C4\CA\C7\C4Ǹ\F6master \C8磺 all \B1\ED\C3\F7\CB\F9\D3е\C4master \A3\ACIDLE \B1\EAʾͳ\BC\C6DDR IDEL \B5\C4ʱ\BC\E4
MODE \A3\BA ָ\CA\C7ͳ\BCƵ\C4ģʽ\A3\AC R \A3\ACW \A3\AC RW \B5\C83\D6\D0ģʽ
bandwidth\A3\BAͳ\BCƵ\C4master\D4\DAָ\B6\A8ʱ\BC\E4\C4ڵĴ\F8\BF\ED\A3\ACM Ϊ\B5\A5λ 400M byte
percent of total\A3\BA ͳ\BCƵİٷֱȣ\AC\CAǵ\B1ǰmaster \B5Ĵ\F8\BF\ED\BA\CDϵͳ\C0\ED\C2\DB\D7ܴ\F8\BF\ED\B5ıȣ\AC IDLE \CAǸ\F6\C0\FD\CD⣬\B5\B1ͳ\BC\C6IDLE\B5\C4ʱ\BA򣬴\CBʱ\B5İٷֱȻ\BB\CB\E3Ϊ\C1˷\C7IDLE\B5\C4cycle \BA\CDϵͳDDR\D7ܵ\C4cycle ֮\BC\E4\B5ı\C8\C0\FD

ʹ\D3\C3\C1\F7\B3̣\BA
1.\C9\E8\D6\C3\CF\E0\B9ز\CE\CA\FD
2.\BF\AAʼͳ\BC\C6 
3.ֹͣͳ\BC\C6
4.\B2鿴\BD\E1\B9\FB
  
  
\C8磺\D0\E8Ҫͳ\BC\C6DE \B5Ĵ\F8\BF\ED\BA\CDGPU\B5Ĵ\F8\BF\ED\A3\AC\B2\C9\D1\F9\C2\CA\CA\C72s\B2\C9\D1\F9һ\B4Σ\AC\D7\EE\B4\F3ͳ\BCƴ\CE\CA\FDΪ 500\B4\CE
echo 5 > /sys/kernel/debug/dmm/master0/id 
echo 0 > /sys/kernel/debug/dmm/master0/mode 
echo 6 > /sys/kernel/debug/dmm/master1/id
echo 2 > /sys/kernel/debug/dmm/master1/mode

echo 2000 > /sys/kernel/debug/dmm/sampling_rate
echo 500 > /sys/kernel/debug/dmm/max_statistic_cnt

echo 1 > /sys/kernel/debug/dmm/enable
\B2\D9\D7\F7UI һ\B6\CEʱ\BC\E4\BA\F3
\A1\A3\A1\A3\A1\A3\A1\A3\A1\A3\A1\A3
echo 0 > /sys/kernel/debug/dmm/enable

cat /sys/kernel/debug/dmm/result > /data/result.txt
\B2鿴\BD\E1\B9\FB



Ĭ\C8\CF\C7\E9\BF\F6\CF\C2ͳ\BCƵ\C4\CA\C7 \CB\F9\D3\D0master \B5Ĵ\F8\BF\ED\BA\CD IDLE \B5\C4\C7\E9\BF\F6\A3\AC\B2\C9\D1\F9\C2\CA\CA\C71s \A3\AC\D7\EE\B4\F3ͳ\BCƴ\CE\CA\FD\CA\C71000\B4\CE

\C8磺 
echo 1 > /sys/kernel/debug/dmm/enable
\B2\D9\D7\F7UI һ\B6\CEʱ\BC\E4\BA\F3
\A1\A3\A1\A3\A1\A3\A1\A3\A1\A3\A1\A3
echo 0 > /sys/kernel/debug/dmm/enable

cat /sys/kernel/debug/dmm/result > /data/result.txt  \BE\CD\CAǰ\B4\D5\D5\C9\CF\C3\E6\B5\C4Ĭ\C8\CF\C9\E8\D6ý\F8\D0\D0ͳ\BCơ\A3
