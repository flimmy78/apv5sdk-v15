#!/bin/sh

echo 1 > /sys/module/lc_ether/parameters/rt_debug
dmesg >  /tmp/kernellog1
dmesg -c
date
killall udhcpc                     
echo 4 4 -1 | ndis_test > /dev/null 
echo 3 4  -1 | ndis_test > /dev/null ------��һ��disconnect�������ظ�����
sleep 1                            
echo 4 4 -1 | ndis_test > /dev/null -------ȷ��disconnect response
echo 2 4  -1 | ndis_test > /dev/null ------����connect qmi request
sleep 20                             
echo 4 4 4 -1 | ndis_test > /dev/null  -----ȷ��connect response
sleep 20                           
echo 4 4 -1 | ndis_test > /dev/null   -----ȷ��connect response
sleep 20                           
echo 4 4 -1 | ndis_test > /dev/null    -----ȷ��connect response���������⣬Ӧ���Ѿ����ųɹ���
udhcpc -i wan0 &                  ----dhclient ����ip/dns/·��
