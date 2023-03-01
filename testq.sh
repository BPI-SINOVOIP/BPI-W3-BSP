#!/bin/bash

TEST_RESULT=0
WIFI_RESULT=0
BT_RESULT=0
ETH_RESULT=0
M2_RESULT=0
SATA1_RESULT=0
SATA2_RESULT=0
USB_RESULT=0

sleep 10

function test_wifi() {
        ifconfig wlan0 down > /dev/null && ifconfig wlan0 up > /dev/null && ifconfig wlan0 | grep UP
        if [ $? -ne 0 ];then
		TEST_RESULT=1
		WIFI_RESULT=1
				echo "
        #################################################
		#################################################
        ############ The wifi test fail !!! #############
        #################################################
		#################################################
                "
        fi
}

function test_bt() {
	bt_pcba_test > /dev/null
	sleep 5
	hciconfig hci0 up > /dev/null && hciconfig -a | grep UP
	if [ $? -ne 0 ];then
		TEST_RESULT=1
		BT_RESULT=1
		echo "The bt test fail !!!"
		echo "
        #################################################
		#################################################
        ############ The BT test fail !!! ###############
        #################################################
		#################################################
                "
	fi
}

function test_eth(){
		udhcpc -t 5 -n -i eth0
		if [ $? -ne 0 ];then
		TEST_RESULT=1
		ETH_RESULT=1
                echo "
        #################################################
		#################################################
        ############ The eth0 test fail !!! #############
        #################################################
		#################################################
                "
        fi
}

function test_M2(){
		lspci | grep Class
		if [ $? -ne 0 ];then
		TEST_RESULT=1
		M2_RESULT=1
                echo "
        #################################################
		#################################################
        ############ The M.2 test fail !!! ##############
        #################################################
		#################################################
                "
        fi
}

function test_SATA(){
		time dd if=/dev/sda2 of=/dev/null bs=1M count=10
		if [ $? -ne 0 ];then
		TEST_RESULT=1
		SATA1_RESULT=1
                echo "
        #################################################
		#################################################
        ############ The SATA1 test fail !!! ############
        #################################################
		#################################################
                "
        fi
		
		time dd if=/dev/sdb2 of=/dev/null bs=1M count=10
		if [ $? -ne 0 ];then
		TEST_RESULT=1
		SATA2_RESULT=1
                echo "
        #################################################
		#################################################
        ############ The SATA2 test fail !!! ############
        #################################################
		#################################################
                "
        fi
}

function test_USB(){
		result=`dmesg | grep " new SuperSpeed Gen 1 USB device" | wc -l`
		if [ ${result} -ne 2 ];then
		TEST_RESULT=1
		USB_RESULT=1
				echo "
		#################################################
        #################################################
        ############ The USB 3.0 test fail !!! ##########
        #################################################
		#################################################
                "
		fi
}


# function main() {
#         TEST_RESULT=0
# 		WIFI_RESULT=0
# 		BT_RESULT=0
# 		ETH_RESULT=0
# 		M2_RESULT=0
# 		SATA_RESULT=0
# 		USB_RESULT=0
        
# 		echo " " 
# 		echo " "
# 		echo " "
# 		echo "######################################################################################"

# 		echo "################################### Testing Start  ###################################"

# 		test_wifi
# 		if [ $? -ne 0 ];then
# 		TEST_RESULT=0
# 		WIFI_RESULT=0
# 		fi
# 		sleep 1

# 		test_bt
# 		if [ $? -ne 0 ];then
# 		TEST_RESULT=0
# 		WIFI_RESULT=0
# 		fi
# 		sleep 1

# 		test_eth
# 		if [ $? -ne 0 ];then
# 		TEST_RESULT=0
# 		WIFI_RESULT=0
# 		fi
# 		sleep 1

# 		test_M2
# 		if [ $? -ne 0 ];then
# 		TEST_RESULT=0
# 		WIFI_RESULT=0
# 		fi
# 		sleep 1

# 		test_SATA
# 		if [ $? -ne 0 ];then
# 		TEST_RESULT=0
# 		WIFI_RESULT=0
# 		fi
# 		sleep 1

# 		test_USB
# 		if [ $? -ne 0 ];then
# 		TEST_RESULT=0
# 		WIFI_RESULT=0
# 		fi
		
# 		echo "
#         ######################################################################################
#         ################################## Test completed !!! ################################
#         ######################################################################################
#                 "
# }
function main() {
		echo " " 
 		echo " "
 		echo " "
 		echo "######################################################################################"

 		echo "################################### Testing Start  ###################################"

		test_wifi
		sleep 1
		test_bt
		sleep 1
		test_eth
		sleep 1
		test_M2
		sleep 1
		test_SATA
		sleep 1
		test_USB
		echo "
        ######################################################################################
        ######################################### Result #####################################
        ######################################################################################
                "
		if [ $WIFI_RESULT -ne 0 ];then
 			echo "WIFI:            PASS"
		else
		    echo "WIFI:            fail"
 		fi

		if [ $BT_RESULT -ne 0 ];then
 			echo "BT:            PASS"
		BT
		    echo "WIFI:            fail"
 		fi
		if [ $ETH_RESULT -ne 0 ];then
 			echo "ETH:            PASS"
		else
		    echo "ETH:            fail"
 		fi

		if [ $M2_RESULT -ne 0 ];then
 			echo "M2:            PASS"
		else
		    echo "M2:            fail"
 		fi

		if [ $SATA1_RESULT -ne 0 ];then
 			echo "SATA1:            PASS"
		else
		    echo "SATA1:            fail"
 		fi

		if [ $SATA2_RESULT -ne 0 ];then
 			echo "SATA2:            PASS"
		else
		    echo "SATA2:            fail"
 		fi

		if [ $USB_RESULT -ne 0 ];then
 			echo "USB:            PASS"
		else
		    echo "USB:            fail"
 		fi

		if [ $TEST_RESULT -ne 0 ];then
			######################################################################################
        	#################################### TEST PASS!!! ####################################
        	######################################################################################
		else
		    ######################################################################################
        	#################################### TEST FAIL!!! ####################################
        	######################################################################################
 		fi
}

main