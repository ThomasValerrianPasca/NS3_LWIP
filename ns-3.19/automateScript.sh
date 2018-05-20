#!/bin/bash

dir="/home/ssrluser/Downloads/ns3_old/ns-allinone-3.19_1/ns-3.19/finalresults/log__1_ue_tcp/"

mkdir $dir

	logfile1=`echo -n "${dir}/log_1ue_lte_position.log"`
        logfile2=`echo -n "${dir}/log_1ue_wifi_position.log"`
        logfile3=`echo -n "${dir}/log_1ue_ltewifi_pktspilt_position.log"`
        logfile4=`echo -n "${dir}/log_1ue_ltewifi_flowsplit_position.log"` 	
        logfile5=`echo -n "${dir}/log_1ue_lteulwifidl_position.log"`
	logfile6=`echo -n "${dir}/log_lte1ue_ltectrldata_wifidata_position.log"`
        (./waf --run "scratch/lena-simple-epc  --interPacketInterval=8192 --lte=1" > $logfile1 2> /dev/null; echo "finished 1")&
        (./waf --run "scratch/lena-simple-epc  --interPacketInterval=1365 --lte=2" > $logfile2 2> /dev/null; echo "finished 2")&
        (./waf --run "scratch/lena-simple-epc  --interPacketInterval=682  --lte=3" > $logfile3 2> /dev/null; echo "finished 3")&
        (./waf --run "scratch/lena-simple-epc  --interPacketInterval=341  --lte=5" > $logfile4 2> /dev/null; echo "finished 4")&
        (./waf --run "scratch/lena-simple-epc  --interPacketInterval=200  --lte=4" > $logfile5 2> /dev/null; echo "finished 5")& 
        (./waf --run "scratch/lena-simple-epc  --interPacketInterval=133  --lte=6" > $logfile6 2> /dev/null; echo "finished 6")& 
        echo "waiting"
        wait
        
	echo "finished waiting"
    
echo "Experiment is completed !!!!!!!!!!!!!"
cd $dir
grep -r "Total Throughput" . > ../throughput_log__1_ue_tcp.txt
#grep -r "Total  Delay" . > ../Delay_log__1_ue_tcp.txt
#grep -r "Percentage of Lost packets" . > ../Ploss_log__1_ue_tcp.txt

chmod -R 777 .
